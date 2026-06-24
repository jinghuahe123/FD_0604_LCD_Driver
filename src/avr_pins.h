/*
    DeepSeek provided AVR Pin to Arduino Digital Pin library.
    Gets information about a digital pin based on its port and raw pin number. 
*/

#ifndef AVR_PINS_H
#define AVR_PINS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//=============================================================================
// ATmega328P Digital Pin Mapping (Arduino-style)
//=============================================================================

// Port B pins (digital pins 8-13)
#define PIN_D8   8   // PB0
#define PIN_D9   9   // PB1
#define PIN_D10  10  // PB2
#define PIN_D11  11  // PB3
#define PIN_D12  12  // PB4
#define PIN_D13  13  // PB5

// Port C pins (analog pins A0-A5, can be used as digital 14-19)
#define PIN_D14  14  // PC0 (A0)
#define PIN_D15  15  // PC1 (A1)
#define PIN_D16  16  // PC2 (A2)
#define PIN_D17  17  // PC3 (A3)
#define PIN_D18  18  // PC4 (A4)
#define PIN_D19  19  // PC5 (A5)

// Port D pins (digital pins 0-7)
#define PIN_D0   0   // PD0 (RX)
#define PIN_D1   1   // PD1 (TX)
#define PIN_D2   2   // PD2
#define PIN_D3   3   // PD3
#define PIN_D4   4   // PD4
#define PIN_D5   5   // PD5
#define PIN_D6   6   // PD6
#define PIN_D7   7   // PD7

//=============================================================================
// Generic Port Definitions 
//=============================================================================

// Use these with ANY code that needs port/pin representation
typedef enum {
    PORT_B = 0,
    PORT_C = 1,
    PORT_D = 2
} AVR_Port_t;

//=============================================================================
// Core Conversion Functions (Generic - No Dependencies)
//=============================================================================

/**
 * Convert port/pin to Arduino-style digital pin number
 * 
 * @param port  PORT_B, PORT_C, or PORT_D
 * @param pin   Pin number within port (0-7)
 * @return      Digital pin number (0-19), or 0xFF if invalid
 * 
 * Examples:
 *   avr_pin_to_digital(PORT_B, 3)  -> 11  (D11)
 *   avr_pin_to_digital(PORT_D, 2)  -> 2   (D2)
 *   avr_pin_to_digital(PORT_C, 0)  -> 14  (D14/A0)
 */
static inline uint8_t avr_pin_to_digital(AVR_Port_t port, uint8_t pin)
{
    switch(port) {
        case PORT_B: return 8 + pin;   // PB0-PB5 = D8-D13
        case PORT_C: return 14 + pin;  // PC0-PC5 = D14-D19 (A0-A5)
        case PORT_D: return pin;       // PD0-PD7 = D0-D7
        default: return 0xFF;          // Invalid
    }
}

/**
 * Convert Arduino-style digital pin number to port/pin
 * 
 * @param digital_pin  Digital pin number (0-19)
 * @param port         Output: PORT_B, PORT_C, or PORT_D
 * @param pin          Output: Pin number within port (0-7)
 * @return             true if valid, false if invalid
 * 
 * Examples:
 *   avr_digital_to_pin(11, &port, &pin)  -> port=PORT_B, pin=3
 *   avr_digital_to_pin(2, &port, &pin)   -> port=PORT_D, pin=2
 *   avr_digital_to_pin(14, &port, &pin)  -> port=PORT_C, pin=0
 */
static inline bool avr_digital_to_pin(uint8_t digital_pin, AVR_Port_t *port, uint8_t *pin)
{
    if (digital_pin >= 0 && digital_pin <= 7) {
        // D0-D7 = Port D
        *port = PORT_D;
        *pin = digital_pin;
        return true;
    }
    else if (digital_pin >= 8 && digital_pin <= 13) {
        // D8-D13 = Port B
        *port = PORT_B;
        *pin = digital_pin - 8;
        return true;
    }
    else if (digital_pin >= 14 && digital_pin <= 19) {
        // D14-D19 = Port C
        *port = PORT_C;
        *pin = digital_pin - 14;
        return true;
    }
    
    return false;  // Invalid pin number
}

/**
 * Check if a port/pin combination is valid
 */
static inline bool avr_pin_valid(AVR_Port_t port, uint8_t pin)
{
    switch(port) {
        case PORT_B: return (pin <= 5);  // PB0-PB5
        case PORT_C: return (pin <= 5);  // PC0-PC5
        case PORT_D: return (pin <= 7);  // PD0-PD7
        default: return false;
    }
}

/**
 * Check if a digital pin number is valid
 */
static inline bool avr_digital_pin_valid(uint8_t digital_pin)
{
    return (digital_pin <= 19);  // D0-D19
}

//=============================================================================
// Register Access Functions (Generic)
//=============================================================================

/**
 * Get the PORTx register for a given port
 */
static inline volatile uint8_t* avr_get_port_reg(AVR_Port_t port)
{
    switch(port) {
        case PORT_B: return &PORTB;
        case PORT_C: return &PORTC;
        case PORT_D: return &PORTD;
        default: return NULL;
    }
}

/**
 * Get the DDRx register for a given port
 */
static inline volatile uint8_t* avr_get_ddr_reg(AVR_Port_t port)
{
    switch(port) {
        case PORT_B: return &DDRB;
        case PORT_C: return &DDRC;
        case PORT_D: return &DDRD;
        default: return NULL;
    }
}

/**
 * Get the PINx register for a given port
 */
static inline volatile uint8_t* avr_get_pin_reg(AVR_Port_t port)
{
    switch(port) {
        case PORT_B: return &PINB;
        case PORT_C: return &PINC;
        case PORT_D: return &PIND;
        default: return NULL;
    }
}

/**
 * Get the bit mask for a pin within a port
 */
static inline uint8_t avr_get_pin_mask(uint8_t pin)
{
    return (1 << pin);
}

//=============================================================================
// Pin Operation Functions (Convenience)
//=============================================================================

/**
 * Set a pin as output
 */
static inline void avr_pin_output(AVR_Port_t port, uint8_t pin)
{
    volatile uint8_t *ddr = avr_get_ddr_reg(port);
    if (ddr) *ddr |= (1 << pin);
}

/**
 * Set a pin as input
 */
static inline void avr_pin_input(AVR_Port_t port, uint8_t pin)
{
    volatile uint8_t *ddr = avr_get_ddr_reg(port);
    if (ddr) *ddr &= ~(1 << pin);
}

/**
 * Enable pull-up on a pin
 */
static inline void avr_pin_pullup(AVR_Port_t port, uint8_t pin, bool enable)
{
    volatile uint8_t *port_reg = avr_get_port_reg(port);
    if (port_reg) {
        if (enable) {
            *port_reg |= (1 << pin);
        } else {
            *port_reg &= ~(1 << pin);
        }
    }
}

/**
 * Write a value to a pin
 */
static inline void avr_pin_write(AVR_Port_t port, uint8_t pin, bool value)
{
    volatile uint8_t *port_reg = avr_get_port_reg(port);
    if (port_reg) {
        if (value) {
            *port_reg |= (1 << pin);
        } else {
            *port_reg &= ~(1 << pin);
        }
    }
}

/**
 * Read a pin value
 */
static inline bool avr_pin_read(AVR_Port_t port, uint8_t pin)
{
    volatile uint8_t *pin_reg = avr_get_pin_reg(port);
    if (pin_reg) {
        return (*pin_reg & (1 << pin)) ? true : false;
    }
    return false;
}

/**
 * Toggle a pin
 */
static inline void avr_pin_toggle(AVR_Port_t port, uint8_t pin)
{
    volatile uint8_t *pin_reg = avr_get_pin_reg(port);
    if (pin_reg) {
        *pin_reg |= (1 << pin);  // Writing to PINx toggles PORTx
    }
}

//=============================================================================
// String Names for Debugging (Generic)
//=============================================================================

/**
 * Get port name string - stores in provided buffer
 */
static inline void avr_port_name(AVR_Port_t port, char* buffer, size_t buflen)
{
    const char* name;
    switch(port) {
        case PORT_B: name = "PORTB"; break;
        case PORT_C: name = "PORTC"; break;
        case PORT_D: name = "PORTD"; break;
        default:     name = "INVALID"; break;
    }
    strlcpy(buffer, name, buflen);
}

/**
 * Get pin name string (e.g., "D11", "A3") - stores in provided buffer
 */
static inline void avr_pin_name(AVR_Port_t port, uint8_t pin, char* buffer, size_t buflen)
{
    uint8_t digital = avr_pin_to_digital(port, pin);
    
    if (digital == 0xFF) {
        strlcpy(buffer, "INVALID", buflen);
        return;
    }
    
    // Check if it's an analog pin
    if (digital >= 14 && digital <= 19) {
        snprintf(buffer, buflen, "D%d/A%d", digital, digital - 14);
    } else {
        snprintf(buffer, buflen, "D%d", digital);
    }
}

//=============================================================================
// Pin Information Structure (Generic)
//=============================================================================

typedef struct {
    uint8_t digital_pin;      // Arduino digital pin number
    AVR_Port_t port;          // Port (B, C, D)
    uint8_t pin;              // Pin number within port (0-7)
    char port_name[8];        // "PORTB", "PORTC", "PORTD"
    char pin_name[8];         // "D11", "A3", etc.
    volatile uint8_t *port_reg;  // PORTx register pointer
    volatile uint8_t *ddr_reg;   // DDRx register pointer
    volatile uint8_t *pin_reg;   // PINx register pointer
} AVR_PinInfo_t;

/**
 * Get complete pin information structure
 */
static inline AVR_PinInfo_t avr_get_pin_info(AVR_Port_t port, uint8_t pin)
{
    AVR_PinInfo_t info;
    info.port = port;
    info.pin = pin;
    info.digital_pin = avr_pin_to_digital(port, pin);
    avr_port_name(port, info.port_name, sizeof(info.port_name));
    avr_pin_name(port, pin, info.pin_name, sizeof(info.pin_name));
    info.port_reg = avr_get_port_reg(port);
    info.ddr_reg = avr_get_ddr_reg(port);
    info.pin_reg = avr_get_pin_reg(port);
    return info;
}

//=============================================================================
// Helper Macros for Common Configurations
//=============================================================================

// Define pins with their port (for easy initialization)
#define AVR_PIN_D0  { PORT_D, 0 }
#define AVR_PIN_D1  { PORT_D, 1 }
#define AVR_PIN_D2  { PORT_D, 2 }
#define AVR_PIN_D3  { PORT_D, 3 }
#define AVR_PIN_D4  { PORT_D, 4 }
#define AVR_PIN_D5  { PORT_D, 5 }
#define AVR_PIN_D6  { PORT_D, 6 }
#define AVR_PIN_D7  { PORT_D, 7 }
#define AVR_PIN_D8  { PORT_B, 0 }
#define AVR_PIN_D9  { PORT_B, 1 }
#define AVR_PIN_D10 { PORT_B, 2 }
#define AVR_PIN_D11 { PORT_B, 3 }
#define AVR_PIN_D12 { PORT_B, 4 }
#define AVR_PIN_D13 { PORT_B, 5 }
#define AVR_PIN_A0  { PORT_C, 0 }
#define AVR_PIN_A1  { PORT_C, 1 }
#define AVR_PIN_A2  { PORT_C, 2 }
#define AVR_PIN_A3  { PORT_C, 3 }
#define AVR_PIN_A4  { PORT_C, 4 }
#define AVR_PIN_A5  { PORT_C, 5 }

#ifdef __cplusplus
}
#endif

#endif // AVR_PINS_H