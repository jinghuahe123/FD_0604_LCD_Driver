#ifndef DISPLAY_PARAMETERS_HPP
#define DISPLAY_PARAMETERS_HPP

#include <stdint.h>
#include "DisplayDriver_FD0604.hpp"

struct DisplayParameters {
    DisplayDriver_FD0604::DriverParameters driverParams;

    const uint16_t BASE_ADDR;
    const uint16_t NUM_SLOTS;

    const uint16_t countingIntervalAddress; // EEPROM address for storing counting interval data 

    const uint16_t displayOrientationAddress; // EEPROM address for storing display orientation data
    const uint16_t numHistoryAddress; // EEPROM address for storing how number history count to recall

    struct {
        volatile uint8_t* const DDRx_temperaturePin;        // data direction register for temperature pin
        volatile uint8_t* const PORTx_temperaturePin;       // port register for temperature pin
        const uint8_t PIN_temperaturePin;                   // pin on port for temperature pin

        const float resistorValue;                          // accompanying resistor value for temperature probe
        const uint16_t temperatureUpdateIntervalAddress;    // EEPROM address for storing temperature update interval data 
        const uint16_t temperatureSerialEnabledAddress;     // EEPROM address for storing serial enabled data for temperature probe
    } tempSensor;
    
    struct {
        volatile uint8_t* const DDRx_rawInputPin;           // data direction register for raw input pin
        volatile uint8_t* const PORTx_rawInputPin;          // port register for raw input pin
        const uint8_t PIN_rawInputPin;                      // pin on port for raw input pin

        const uint16_t rawInputUpdateIntervalAddress;       // EEPROM address for storing raw input update interval data  
        const uint16_t rawInputSerialEnabledAddress;        // EEPROM address for storing serial enabled data for raw input
    } rawInput;
};

#endif
