#include "DisplayController_FD0604.hpp"

/*
    EEPROM Memory Map:
    0x0000-0x001F   -   Firmware Version
    0x0020-0x003F   -   Display Paramaters
    0x0040-0x03FF   -   Displayed Number History
*/

#define SECONDARY_INPUT_INTRO_TEXT

#define SOFT_RX                 4
#define SOFT_TX                 5

#define HARDWARE_SERIAL_BAUD    1000000
#define SOFTWARE_SERIAL_BAUD    19200

#define FIRMWARE_VER_SIZE       32
constexpr char version[] PROGMEM =  "FD_0604 LED Display v0.1.28";

DisplayDriver_FD0604::DriverParams displayParams = {
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


DisplayController_FD0604::DisplayController_FD0604_Parameters controllerParams = {
    .BASE_ADDR = 0x0040,     // EEPROM address to start writing writing from
    .SLOT_SIZE = 6,          // uint32_t for sequence number (for wear levelling) + uint16_t for number
    .NUM_SLOTS = 160,        // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

    .countingIntervalAddress = 0x0020,     // EEPROM Address that stores the delay between counting intervals 

    .temperaturePin = A7,                           // temperature sensor  pin
    .resistorValue = 10000.0,                       // temperature sensor accompanying resistor
    .temperatureUpdateIntervalAddress = 0x0022,     // EEPROM address that stores the delay between the temperature reading updating
    .temperatureSerialEnabledAddress = 0x0024,      // EEPROM address for enable serial output for temperature sensor

    .rawInputPin = A6,                          // raw input pin
    .rawInputUpdateIntervalAddress = 0x0026,    // EEPROM address that stores the delay between the raw input reading updating 
    .rawInputSerialEnabledAddress = 0x0028,     // EEPROM address for enable serial output for raw input

    .displayOrientationAddress = 0x002A,     // EEPROM address for storing display orientation data
    .numHistoryAddress = 0x002C              // EEPROM address for history recall depth
};
