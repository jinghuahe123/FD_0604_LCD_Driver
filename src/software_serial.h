/*
    DeepSeek provided Software Serial Library (Print Functions)
    Mimics Arduino SoftwareSerial.h library, but written in C without any heap allocation or Arduino String
    This is the header file to include for using this library. 
*/

#ifndef SOFTWARE_SERIAL_H
#define SOFTWARE_SERIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "software_serial_core.h"
#include "avr_pins.h"
#include <avr/pgmspace.h>

//=============================================================================
// RX Operations - Extended
//=============================================================================

uint8_t software_serial_read_string_until(SoftwareSerial_t *ss, 
                                         char delimiter, 
                                         char* buffer, 
                                         uint8_t max_len);

//=============================================================================
// TX Operations - Strings
//=============================================================================

void software_serial_print(SoftwareSerial_t *ss, const char *s);
void software_serial_println(SoftwareSerial_t *ss, const char *s);
void software_serial_print_P(SoftwareSerial_t *ss, const char *p);
void software_serial_println_P(SoftwareSerial_t *ss, const char *p);
void software_serial_ln(SoftwareSerial_t *ss);

//=============================================================================
// TX Operations - Integers
//=============================================================================

void software_serial_print_i8(SoftwareSerial_t *ss, int8_t v);
void software_serial_print_u8(SoftwareSerial_t *ss, uint8_t v);
void software_serial_print_i16(SoftwareSerial_t *ss, int16_t v);
void software_serial_print_u16(SoftwareSerial_t *ss, uint16_t v);
void software_serial_print_i32(SoftwareSerial_t *ss, int32_t v);
void software_serial_print_u32(SoftwareSerial_t *ss, uint32_t v);
void software_serial_print_i64(SoftwareSerial_t *ss, int64_t v);
void software_serial_print_u64(SoftwareSerial_t *ss, uint64_t v);

//=============================================================================
// TX Operations - Formats
//=============================================================================

void software_serial_print_hex8(SoftwareSerial_t *ss, uint8_t v);
void software_serial_print_hex16(SoftwareSerial_t *ss, uint16_t v);
void software_serial_print_hex32(SoftwareSerial_t *ss, uint32_t v);
void software_serial_print_bin8(SoftwareSerial_t *ss, uint8_t v);
void software_serial_print_oct(SoftwareSerial_t *ss, uint32_t v);

//=============================================================================
// TX Operations - Float
//=============================================================================

void software_serial_print_float(SoftwareSerial_t *ss, float f, uint8_t decimals);

//=============================================================================
// TX Operations - Buffer
//=============================================================================

void software_serial_write_buffer(SoftwareSerial_t *ss, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // SOFTWARE_SERIAL_H