#include "serial.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef F_CPU
#error F_CPU not defined. 
#endif

#define RX_BUFFER_SIZE 16 // increase as needed, right now max input command does not pass 16 characters inc null terminator

volatile uint8_t rx_buf[RX_BUFFER_SIZE];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

void serial_init(uint32_t baud) {
    UCSR0A = (1 << U2X0);  // enable double-speed mode

    uint16_t ubrr = (F_CPU / (8UL * baud)) - 1;

    UBRR0H = ubrr >> 8;
    UBRR0L = ubrr;

    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    sei();
}

// ===== TX =====
static void uart_tx(uint8_t c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void serial_write(uint8_t c) {
    uart_tx(c);
}


// ===== RX ISR =====
ISR(USART_RX_vect) {
    uint8_t d = UDR0;
    uint8_t next = (rx_head + 1) % RX_BUFFER_SIZE;
    if (next != rx_tail) {
        rx_buf[rx_head] = d;
        rx_head = next;
    }
}

// ===== RX API =====
int serial_available(void) {
    return (RX_BUFFER_SIZE + rx_head - rx_tail) % RX_BUFFER_SIZE;
}

uint8_t serial_read(void) {
    if (rx_head == rx_tail) return 0;
    uint8_t d = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return d;
}

uint8_t serial_read_string_until(char delimiter, char* buffer, uint8_t max_len) {
    uint8_t count = 0;
    char c;
    
    // ensure space for null terminator
    if (max_len == 0) return 0;
    
    while (count < (max_len - 1)) {
        if (serial_available() > 0) {
            c = serial_read();
            
            if (c == delimiter) break;
            
            buffer[count++] = c;
        }
    }
    buffer[count] = '\0'; // ensure null termination
    
    return count;
}

// ===== STRINGS =====
void serial_print(const char *s) {
    while (*s) uart_tx(*s++);
}

void serial_println(const char *s) {
    serial_print(s);
    serial_ln();
}

void serial_print_P(const char *p) {
    char c;
    while ((c = pgm_read_byte(p++))) uart_tx(c);
}

void serial_println_P(const char *p) {
    serial_print_P(p);
    serial_ln();
}

void serial_ln(void) {
    uart_tx('\r');
    uart_tx('\n');
}

// ===== INTEGER CORE =====

// generic recursive print for any unsigned integer type
#define DEFINE_PRINT_UNSIGNED(type, name) \
    static void print_##name##_recursive(type v) { \
        if (v >= 10) { \
            print_##name##_recursive(v / 10); \
            uart_tx('0' + (v % 10)); \
        } else { \
            uart_tx('0' + v); \
        } \
    } \
    void serial_print_u##name(type v) { \
        if (v == 0) uart_tx('0'); \
        else print_##name##_recursive(v); \
    }

// Generate functions for each type
DEFINE_PRINT_UNSIGNED(uint8_t, 8)
DEFINE_PRINT_UNSIGNED(uint16_t, 16)
DEFINE_PRINT_UNSIGNED(uint32_t, 32)
DEFINE_PRINT_UNSIGNED(uint64_t, 64)

// Signed versions
void serial_print_i8(int8_t v) {
    if (v < 0) { uart_tx('-'); v = -v; }
    serial_print_u8(v);
}

void serial_print_i16(int16_t v) {
    if (v < 0) { uart_tx('-'); v = -v; }
    serial_print_u16(v);
}

void serial_print_i32(int32_t v) {
    if (v < 0) { uart_tx('-'); v = -v; }
    serial_print_u32(v);
}

void serial_print_i64(int64_t v) {
    if (v < 0) { uart_tx('-'); v = -v; }
    serial_print_u64(v);
}

// ===== FORMATS =====
void serial_print_hex8(uint8_t v) {
    const char hex[] = "0123456789ABCDEF";
    //uart_tx('0'); uart_tx('x');
    uart_tx(hex[v >> 4]);
    uart_tx(hex[v & 0x0F]);
}

void serial_print_hex16(uint16_t v) {
    serial_print_hex8(v >> 8);
    serial_print_hex8(v & 0xFF);
}

void serial_print_hex32(uint32_t v) {
    serial_print_hex16(v >> 16);
    serial_print_hex16(v & 0xFFFF);
}

void serial_print_bin8(uint8_t v) {
    for (int i = 7; i >= 0; i--)
        uart_tx((v & (1 << i)) ? '1' : '0');
}

void serial_print_oct(uint32_t v) {
    char buf[16];
    uint8_t i = 0;

    if (!v) {
        uart_tx('0');
        return;
    }

    while (v) {
        buf[i++] = '0' + (v & 7);
        v >>= 3;
    }
    while (i--) uart_tx(buf[i]);
}

// ===== FLOAT =====
void serial_print_float(float f, uint8_t d) {
    if (f < 0) {
        uart_tx('-');
        f = -f;
    }

    uint32_t i = (uint32_t)f;
    serial_print_u32(i);

    uart_tx('.');

    float frac = f - i;

    while (d--) {
        frac *= 10;
        uint8_t digit = (uint8_t)frac;
        uart_tx('0' + digit);
        frac -= digit;
    }
}