#include "DisplayController_FD0604.hpp"

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
    0,          // EEPROM address to start writing writing from
    6,          // uint32_t for sequence number (for wear levelling) + uint16_t for number
    170,        // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

    100,        // number of ms to delay between counting intervals

    A7,         // temperature sensor  pin
    10000.0,    // temperature sensor accompanying resistor
    256,        // temperature sensor update interval
    0,          // enable serial output for temperature sensor

    A6,         // raw input pin
    196,        // raw input update interval

    E2END,      // last EEPROM address for storing display orientation data
    128         // How many numbers to search for in history. Don't set too large otherwise MCU will run out of RAM. 
};
