#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <avr/pgmspace.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef F
#ifndef F
#define F(s)    PSTR(s)
#endif

#if !defined(USE_HW_UART) 
    #if defined(UCSR0A) || defined(UCSRA)   /* ATmega multi-UART or ATtiny2313/4313 */
        #define USE_HW_UART
    #endif
#endif

#if defined(USE_HW_UART)

#define RX_BUFFER_SIZE 16

void serial_init(uint32_t baud);

// TX/RX
void serial_write(uint8_t c);
int serial_available(void);
uint8_t serial_read(void);
uint8_t serial_read_string_until(char delimiter, char* buffer, uint8_t max_len);

// Strings
void serial_print(const char *s);
void serial_println(const char *s);
void serial_print_P(const char *p);
void serial_println_P(const char *p);

// Integers
void serial_print_i8(int8_t v);
void serial_print_u8(uint8_t v);
void serial_print_i16(int16_t v);
void serial_print_u16(uint16_t v);
void serial_print_i32(int32_t v);
void serial_print_u32(uint32_t v);
void serial_print_i64(int64_t v);
void serial_print_u64(uint64_t v);

// Formats
void serial_print_hex8(uint8_t v);
void serial_print_hex16(uint16_t v);
void serial_print_hex32(uint32_t v);
void serial_print_bin8(uint8_t v);
void serial_print_oct(uint32_t v);

// Float
void serial_print_float(float f, uint8_t decimals);

// New Line
void serial_ln(void);

#endif // USE_HW_UART

#ifdef __cplusplus
}
#endif

#endif // SERIAL_H