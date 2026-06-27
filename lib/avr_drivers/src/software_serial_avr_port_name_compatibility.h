#ifndef SOFTWARE_SERIAL_AVR_PORT_COMPATIBILITY_H
#define SOFTWARE_SERIAL_AVR_PORT_COMPATIBILITY_H

#include "software_serial.h"
#include "core/avr_pins.h"

// SoftwareSerial port to pin compatibility layer to work with avr_pins.h
// these helpers bridge the gap between AVR_Port_t and SoftwareSerial's SS_Port_t

// convert SoftwareSerial port to generic AVR port
static inline AVR_Port_t ss_port_to_avr_port(SS_Port_t ss_port) {
    switch(ss_port) {
        #if defined(PORTB)
        case SS_PORT_B: return PORT_B;
        #endif
        #if defined(PORTC)
        case SS_PORT_C: return PORT_C;
        #endif
        #if defined(PORTD)
        case SS_PORT_D: return PORT_D;
        #endif
        default: return (AVR_Port_t)0xFF; // Invalid
    }
}

// convert generic AVR port to SoftwareSerial port
static inline SS_Port_t avr_port_to_ss_port(AVR_Port_t avr_port) {
    switch(avr_port) {
        #if defined(PORTB)
        case PORT_B: return SS_PORT_B;
        #endif
        #if defined(PORTC)
        case PORT_C: return SS_PORT_C;
        #endif
        #if defined(PORTD)
        case PORT_D: return SS_PORT_D;
        #endif
        default: return (SS_Port_t)0xFF; // Invalid
    }
}

// get digital pin from SoftwareSerial instance
static inline uint8_t ss_get_rx_digital(SoftwareSerial_t *ss) {
    return avr_pin_to_digital(ss_port_to_avr_port(ss->rx_port), ss->rx_pin);
}

static inline uint8_t ss_get_tx_digital(SoftwareSerial_t *ss) {
    return avr_pin_to_digital(ss_port_to_avr_port(ss->tx_port), ss->tx_pin);
}

#endif // SOFTWARE_SERIAL_AVR_PORT_COMPATIBILITY_H