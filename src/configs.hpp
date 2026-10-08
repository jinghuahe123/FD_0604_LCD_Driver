#include "DisplayController/DisplayParameters.hpp"
#include <avr/pgmspace.h>

/*
    ============================= EEPROM Memory Map =============================

    0x0000-0x001F   -   Firmware Version
    0x0020-0x003F   -   Display Paramaters
                            0x0020-0x002F   -   General Display Parameters
                            0x0030-0x0037   -   Temperature Parameters
                            0x0038-0x003F   -   Raw Input Parameters
    0x0040-0x03FF   -   Displayed Number History



    ============================= NANO Board Pinout =============================

            Status Heartbeat LED    D13 |  | D12    Secondary Input TX
                                    3V3 |  | D11    Secondary Input RX
                      AREF (3V3)    REF |  | D10
                                    A0  |  |  D9
                                    A1  |  |  D8
                                    A2  |  |  D7
                                    A3  |  |  D6    Shift Register Data Pin
                                    A4  |  |  D5    Shift Register Clock Pin
                                    A5  |  |  D4    Shift Register Latch Pin
            Raw Input (AREF) Pin    A6  |  |  D3    
          Temp Sensor (AREF) Pin    A7  |  |  D2    
                                    5V  |  | GND
                                    RST |  | RST
                                    GND |  | RX0    Main Input RX
                                    VIN |  | TX1    Main Input TX

*/


constexpr uint32_t HARDWARE_SERIAL_BAUD = 1000000;

constexpr uint32_t SOFTWARE_SERIAL_BAUD = 4800;
constexpr uint8_t SOFT_RX_DIGITAL_PIN = 11;
constexpr uint8_t SOFT_TX_DIGITAL_PIN = 12;

// whether secondary serial output should print initialisation text
#define SOFT_SERIAL_OUTPUT     1

constexpr uint8_t FIRMWARE_VER_SIZE = 32;
constexpr char version[] PROGMEM =  "FD_0604 LED Display v0.1.41";

constexpr uint16_t statusLEDBlinkInterval = 32; // ms
constexpr uint8_t statusLEDPin = 13; // D13 pin for status LED


DisplayParameters displayParams = {
    .driverParams = {
        .latchPin = 4,
        .clockPin = 5,
        .dataPin = 6,

        .npn_transistor_enable = 1
    },

    .persistentStorage = {
        .BASE_ADDR = 0x0040,     // EEPROM address to start writing writing from
        .NUM_SLOTS = 160,        // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())
    },

    .countingIntervalAddress = 0x0020,     // EEPROM Address that stores the delay between counting intervals 

    .displayOrientationAddress = 0x0022,     // EEPROM address for storing display orientation data
    .numHistoryAddress = 0x0024,             // EEPROM address for history recall depth

    .tempSensor = {
        .temperaturePin = HardwarePin(20),              // A6 pin for temperature sensor

        .resistorValue = 10000.0,                       // temperature sensor accompanying resistor
        .temperatureUpdateIntervalAddress = 0x0030,     // EEPROM address that stores the delay between the temperature reading updating
        .temperatureSerialEnabledAddress = 0x0032,      // EEPROM address for enable serial output for temperature sensor
    },

    .rawInput = {
        .rawInputPin = HardwarePin(21),                    // A7 pin for raw input

        .rawInputUpdateIntervalAddress = 0x0038,        // EEPROM address that stores the delay between the raw input reading updating 
        .rawInputSerialEnabledAddress = 0x003A,         // EEPROM address for enable serial output for raw input
    },
};
