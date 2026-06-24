#ifndef SOFTWARE_SERIAL_AVR_PORT_COMPATIBILITY_H
#define SOFTWARE_SERIAL_AVR_PORT_COMPATIBILITY_H

#include "software_serial.h"
#include "avr_pins.h"

// SoftwareSerial port to pin compatibility layer to work with avr_pins.h
// these helpers bridge the gap between AVR_Port_t and SoftwareSerial's SS_Port_t

// convert SoftwareSerial port to generic AVR port
static inline AVR_Port_t ss_port_to_avr_port(SS_Port_t ss_port) {
    switch(ss_port) {
        case SS_PORT_B: return PORT_B;
        case SS_PORT_C: return PORT_C;
        case SS_PORT_D: return PORT_D;
        default: return PORT_D;
    }
}

// convert generic AVR port to SoftwareSerial port
static inline SS_Port_t avr_port_to_ss_port(AVR_Port_t avr_port) {
    switch(avr_port) {
        case PORT_B: return SS_PORT_B;
        case PORT_C: return SS_PORT_C;
        case PORT_D: return SS_PORT_D;
        default: return SS_PORT_D;
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