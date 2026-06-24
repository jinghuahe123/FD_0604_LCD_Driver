#include "software_serial.h"
#include <avr/wdt.h>
#include <stddef.h>

//=============================================================================
// RX Operations - Extended
//=============================================================================

uint8_t software_serial_read_string_until(SoftwareSerial_t *ss, 
                                         char delimiter, 
                                         char* buffer, 
                                         uint8_t max_len)
{
    uint8_t count = 0;
    char c;
    
    // ensure space for null terminator
    if (max_len == 0 || ss == NULL) return 0;

    // Give the ISR a chance to fill the buffer
    _delay_loop_2(50);  // ~25 µs at 8 MHz
    
    while (count < (max_len - 1)) {
        if (software_serial_available(ss) > 0) {
            c = software_serial_read(ss);
            
            if (c == delimiter) break;
            
            buffer[count++] = c;
        }
        
        wdt_reset(); // ensures watchdog does not reset board
    }
    buffer[count] = '\0'; // ensure null termination
    
    return count;
}

//=============================================================================
// TX Operations - Strings
//=============================================================================

void software_serial_print(SoftwareSerial_t *ss, const char *s)
{
    while (*s) software_serial_write(ss, (uint8_t)*s++);
}

void software_serial_println(SoftwareSerial_t *ss, const char *s)
{
    software_serial_print(ss, s);
    software_serial_ln(ss);
}

void software_serial_print_P(SoftwareSerial_t *ss, const char *p)
{
    char c;
    while ((c = pgm_read_byte(p++))) software_serial_write(ss, (uint8_t)c);
}

void software_serial_println_P(SoftwareSerial_t *ss, const char *p)
{
    software_serial_print_P(ss, p);
    software_serial_ln(ss);
}

void software_serial_ln(SoftwareSerial_t *ss)
{
    software_serial_write(ss, '\r');
    software_serial_write(ss, '\n');
}

//=============================================================================
// TX Operations - Integer Core
//=============================================================================

// Generic recursive print for any unsigned integer type
#define DEFINE_SS_PRINT_UNSIGNED(type, name) \
    static void print_ss_##name##_recursive(SoftwareSerial_t *ss, type v) { \
        if (v >= 10) { \
            print_ss_##name##_recursive(ss, v / 10); \
            software_serial_write(ss, (uint8_t)('0' + (v % 10))); \
        } else { \
            software_serial_write(ss, (uint8_t)('0' + v)); \
        } \
    } \
    void software_serial_print_u##name(SoftwareSerial_t *ss, type v) { \
        if (v == 0) software_serial_write(ss, '0'); \
        else print_ss_##name##_recursive(ss, v); \
    }

// Generate functions for each type
DEFINE_SS_PRINT_UNSIGNED(uint8_t, 8)
DEFINE_SS_PRINT_UNSIGNED(uint16_t, 16)
DEFINE_SS_PRINT_UNSIGNED(uint32_t, 32)
DEFINE_SS_PRINT_UNSIGNED(uint64_t, 64)

// Signed versions
void software_serial_print_i8(SoftwareSerial_t *ss, int8_t v)
{
    if (v < 0) { software_serial_write(ss, '-'); v = -v; }
    software_serial_print_u8(ss, v);
}

void software_serial_print_i16(SoftwareSerial_t *ss, int16_t v)
{
    if (v < 0) { software_serial_write(ss, '-'); v = -v; }
    software_serial_print_u16(ss, v);
}

void software_serial_print_i32(SoftwareSerial_t *ss, int32_t v)
{
    if (v < 0) { software_serial_write(ss, '-'); v = -v; }
    software_serial_print_u32(ss, v);
}

void software_serial_print_i64(SoftwareSerial_t *ss, int64_t v)
{
    if (v < 0) { software_serial_write(ss, '-'); v = -v; }
    software_serial_print_u64(ss, v);
}

//=============================================================================
// TX Operations - Formats
//=============================================================================

/*
void software_serial_print_hex8(SoftwareSerial_t *ss, uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    software_serial_write(ss, (uint8_t)hex[v >> 4]);
    software_serial_write(ss, (uint8_t)hex[v & 0x0F]);
}*/

void software_serial_print_hex8(SoftwareSerial_t *ss, uint8_t v)
{
    static const char hex[] PROGMEM = "0123456789ABCDEF";
    software_serial_write(ss, (uint8_t)pgm_read_byte(&hex[v >> 4]));
    software_serial_write(ss, (uint8_t)pgm_read_byte(&hex[v & 0x0F]));
}

void software_serial_print_hex16(SoftwareSerial_t *ss, uint16_t v)
{
    software_serial_print_hex8(ss, v >> 8);
    software_serial_print_hex8(ss, v & 0xFF);
}

void software_serial_print_hex32(SoftwareSerial_t *ss, uint32_t v)
{
    software_serial_print_hex16(ss, v >> 16);
    software_serial_print_hex16(ss, v & 0xFFFF);
}

void software_serial_print_bin8(SoftwareSerial_t *ss, uint8_t v)
{
    for (int i = 7; i >= 0; i--)
        software_serial_write(ss, (v & (1 << i)) ? '1' : '0');
}

void software_serial_print_oct(SoftwareSerial_t *ss, uint32_t v)
{
    char buf[16];
    uint8_t i = 0;

    if (!v) {
        software_serial_write(ss, '0');
        return;
    }

    while (v) {
        buf[i++] = '0' + (v & 7);
        v >>= 3;
    }
    while (i--) software_serial_write(ss, (uint8_t)buf[i]);
}

//=============================================================================
// TX Operations - Float
//=============================================================================

void software_serial_print_float(SoftwareSerial_t *ss, float f, uint8_t decimals)
{
    if (f < 0) {
        software_serial_write(ss, '-');
        f = -f;
    }

    uint32_t i = (uint32_t)f;
    software_serial_print_u32(ss, i);

    software_serial_write(ss, '.');

    float frac = f - i;

    while (decimals--) {
        frac *= 10;
        uint8_t digit = (uint8_t)frac;
        software_serial_write(ss, (uint8_t)('0' + digit));
        frac -= digit;
    }
}

//=============================================================================
// TX Operations - Buffer
//=============================================================================

void software_serial_write_buffer(SoftwareSerial_t *ss, const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        software_serial_write(ss, data[i]);
    }
}