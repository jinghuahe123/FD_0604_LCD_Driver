#include "DisplayController_FD0604.hpp"

/*
    EEPROM Memory Map:
    0x0000-0x001F   -   Firmware Version
    0x0020-0x003F   -   Display Paramaters
    0x0040-0x03FF   -   Displayed Number History
*/

const uint8_t softRX = 4;
const uint8_t softTX = 5;

const uint32_t hardwareSerialBaud = 1000000;
const uint32_t softwareSerialBaud = 19200;

DisplayDriver_FD0604::DriverParams_MinimalWiring minimalDisplayParams = {
    1,          // transistor enabled

    6,          // arduino latchpin
    7,          // arduino clockpin
    8           // arduino datapin
};

DisplayDriver_FD0604::DriverParams_DIRECTPORT_MinimalWiring minimalPortDisplayParams = {
    1,          // transistor enabled
	
	&DDRD,      // latchpin DDRx register
	&PORTD,     // latchpin PORTx register
	6,          // latchpin physical pin (on register)
	
	&DDRD,      // clockpin DDRx register
	&PORTD,     // clockpin PORTx register
	7,          // clockpin physical pin (on register)
	
	&DDRB,      // datapin DDRx register
	&PORTB,     // datapin PORTx register
	0,          // datapin physical pin (on register)
};


DisplayController_FD0604::DisplayController_FD0604_Parameters controllerParams = {
    0x0040,     // EEPROM address to start writing writing from
    6,          // uint32_t for sequence number (for wear levelling) + uint16_t for number
    160,        // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

    0x0020,     // EEPROM Address that stores the delay between counting intervals 

    A7,         // temperature sensor  pin
    10000.0,    // temperature sensor accompanying resistor
    0x0022,     // EEPROM address that stores the delay between the temperature reading updating
    0x0024,     // EEPROM address for enable serial output for temperature sensor

    A6,         // raw input pin
    0x0026,     // EEPROM address that stores the delay between the raw input reading updating 
    0x0028,     // EEPROM address for enable serial output for raw input

    0x002A,      // EEPROM address for storing display orientation data
    128         // How many numbers to search for in history. Don't set too large otherwise MCU will run out of RAM. 
};
