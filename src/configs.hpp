#include "DisplayController_FD0604.hpp"

/*
    EEPROM Memory Map:
    0x0000-0x001F   -   Firmware Version
    0x0020-0x003F   -   Display Paramaters
                            0x0020-0x002F   -   General Display Parameters
                            0x0030-0x0037   -   Temperature Parameters
                            0x0038-0x003F   -   Raw Input Parameters
    0x0040-0x03FF   -   Displayed Number History
*/


#define HARDWARE_SERIAL_BAUD    1000000

#define FIRMWARE_VER_SIZE       32
constexpr char version[] PROGMEM =  "FD_0604 LED Display v0.1.34";

const DisplayDriver_FD0604::DriverParameters displayParams = {
    .npn_transistor_enable = 1,
	
	.DDRx_latchPin = &DDRD,             // latchpin data direction register
	.PORTx_latchPin = &PORTD,           // latchpin port register
	.PIN_latchPin = 6,                  // latchpin physical pin (on register)
	
	.DDRx_clockPin = &DDRD,             // clockpin data direction register   
	.PORTx_clockPin = &PORTD,           // clockpin port register
	.PIN_clockPin = 7,                  // clockpin physical pin (on register)
	
	.DDRx_dataPin = &DDRB,              // datapin data direction register
	.PORTx_dataPin = &PORTB,            // datapin port register
	.PIN_dataPin = 0,                   // datapin physical pin (on register)
};


const DisplayController_FD0604::DisplayController_FD0604_Parameters controllerParams = {
    .BASE_ADDR = 0x0040,     // EEPROM address to start writing writing from
    .SLOT_SIZE = 6,          // uint32_t for sequence number (for wear levelling) + uint16_t for number
    .NUM_SLOTS = 160,        // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

    .countingIntervalAddress = 0x0020,     // EEPROM Address that stores the delay between counting intervals 

    .displayOrientationAddress = 0x0022,     // EEPROM address for storing display orientation data
    .numHistoryAddress = 0x0024,             // EEPROM address for history recall depth

    .tempSensor = {
        // analog channel 7 cannot be output, does not have corresponding port
        // otherwise for atmega328p would be &DDRC and &PORTC
        .DDRx_temperaturePin = nullptr,       // data direction register for temp sensor
        .PORTx_temperaturePin = nullptr,     // port register for temp sensor
        .PIN_temperaturePin = 7,            // pin on port of temp sensor

        .resistorValue = 10000.0,                       // temperature sensor accompanying resistor
        .temperatureUpdateIntervalAddress = 0x0030,     // EEPROM address that stores the delay between the temperature reading updating
        .temperatureSerialEnabledAddress = 0x0032,      // EEPROM address for enable serial output for temperature sensor
    },

    .rawInput = {
        // analog channel 6 cannot be output, does not have corresponding port
        .DDRx_rawInputPin = nullptr,        // data direction register for raw input
        .PORTx_rawInputPin = nullptr,       // port register for raw input
        .PIN_rawInputPin = 6,               // pin on port of raw input

        .rawInputUpdateIntervalAddress = 0x0038,        // EEPROM address that stores the delay between the raw input reading updating 
        .rawInputSerialEnabledAddress = 0x003A,         // EEPROM address for enable serial output for raw input
    },
};
