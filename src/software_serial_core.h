/*
    DeepSeek provided Software Serial Library (Core Bitbanging)
    Mimics Arduino SoftwareSerial.h library, but written in C without any heap allocation or Arduino String
*/

#ifndef SOFTWARE_SERIAL_CORE_H
#define SOFTWARE_SERIAL_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay_basic.h>
#include <stdint.h>
#include <stdbool.h>

// Configuration
#ifndef SS_RX_BUFFER_SIZE
#define SS_RX_BUFFER_SIZE 16
#endif

// Pin Change Interrupt vectors
#if defined(PCINT0_vect)
#define HAS_PCINT0 1
#endif
#if defined(PCINT1_vect)
#define HAS_PCINT1 1
#endif
#if defined(PCINT2_vect)
#define HAS_PCINT2 1
#endif

// Port definitions for ATmega328P
// Port B: pins 8-13 (Arduino D8-D13)
// Port C: pins 14-19 (Arduino A0-A5)
// Port D: pins 0-7 (Arduino D0-D7)

typedef enum {
    SS_PORT_B = 0,
    SS_PORT_C = 1,
    SS_PORT_D = 2
} SS_Port_t;

typedef struct {
    // Pin configuration
    uint8_t rx_pin;              // Pin number within port (0-7)
    SS_Port_t rx_port;           // Port identifier
    uint8_t tx_pin;              // Pin number within port (0-7)
    SS_Port_t tx_port;           // Port identifier
    
    // Pin masks
    uint8_t rx_bit_mask;         // 1 << rx_pin
    uint8_t tx_bit_mask;         // 1 << tx_pin
    
    // Register pointers
    volatile uint8_t *rx_port_in;    // PINx register
    volatile uint8_t *tx_port_out;   // PORTx register
    volatile uint8_t *tx_ddr;        // DDRx register
    
    // PCINT configuration
    volatile uint8_t *pcint_mask_reg;  // PCMSKx register
    uint8_t pcint_mask_value;          // Bit value in PCMSK
    uint8_t pcint_vector;              // PCINT0, PCINT1, PCINT2
    
    // Timing delays (in 4-cycle units for _delay_loop_2)
    uint16_t tx_delay;
    uint16_t rx_delay_centering;
    uint16_t rx_delay_intrabit;
    uint16_t rx_delay_stopbit;
    
    // Status flags
    bool inverse_logic:1;
    bool buffer_overflow:1;
    bool is_listening:1;
    bool initialized:1;
    
    // Ring buffer
    volatile uint8_t rx_buffer[SS_RX_BUFFER_SIZE];
    volatile uint8_t rx_buffer_head;
    volatile uint8_t rx_buffer_tail;
} SoftwareSerial_t;

// Global active instance
extern SoftwareSerial_t *active_serial_instance;

//=============================================================================
// Core Initialization & Control
//=============================================================================

void software_serial_init(SoftwareSerial_t *ss, 
                         uint8_t rx_port, uint8_t rx_pin,
                         uint8_t tx_port, uint8_t tx_pin,
                         bool inverse_logic);

void software_serial_begin(SoftwareSerial_t *ss, uint32_t baud_rate);
void software_serial_end(SoftwareSerial_t *ss);

bool software_serial_listen(SoftwareSerial_t *ss);
bool software_serial_stop_listening(SoftwareSerial_t *ss);
bool software_serial_is_listening(SoftwareSerial_t *ss);

//=============================================================================
// Core RX Operations
//=============================================================================

int software_serial_available(SoftwareSerial_t *ss);
uint8_t software_serial_read(SoftwareSerial_t *ss);
int16_t software_serial_peek(SoftwareSerial_t *ss);
bool software_serial_overflow(SoftwareSerial_t *ss);
void software_serial_flush(SoftwareSerial_t *ss);

//=============================================================================
// Core TX Operations
//=============================================================================

void software_serial_write(SoftwareSerial_t *ss, uint8_t data);

//=============================================================================
// Internal functions (exposed for interrupt handling)
//=============================================================================

void software_serial_handle_interrupt(void);
uint8_t software_serial_rx_pin_read(SoftwareSerial_t *ss);

#ifdef __cplusplus
}
#endif

#endif // SOFTWARE_SERIAL_CORE_H