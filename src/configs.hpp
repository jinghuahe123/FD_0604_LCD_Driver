#include "DisplayController_FD0604.hpp"

#define PORTMANIP

DisplayDriver_FD0604::DriverParams_MinimalWiring minimalDisplay = {
    1,          // transistor enabled

    6,          // arduino latchpin
    7,          // arduino clockpin
    8           // arduino datapin
};

DisplayDriver_FD0604::DriverParams_DIRECTPORT_MinimalWiring minimalPortDisplay = {
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


DisplayController_FD0604::DisplayController_FD0604_Parameters displayParameters = {
    100,        // number of ms to delay between counting intervals

    A7,         // temperature sensor  pin
    10000.0,    // temperature sensor accompanying resistor
    250,        // temperature sensor update interval
    0,          // enable serial output for temperature sensor

    E2END,       // last EEPROM address for storing display orientation data
    128          // How many numbers to search for in history. Don't set too large otherwise MCU will run out of RAM. 
};


const uint16_t BASE_ADDR = 0;            // EEPROM address to start writing writing from
const uint8_t SLOT_SIZE = 6;            // uint32_t for sequence number (for wear levelling) + uint16_t for number
const uint16_t NUM_SLOTS = 170;          // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

//const uint8_t gnd[2] = {2, 3};      // first two pins of display in order of connection
//const uint8_t pins[3] = {6, 7, 8};  // order of latchpin, clockpin, datapin
