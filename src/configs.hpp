#include "DisplayController_FD0604.hpp"

#define USE_MINIMAL_WIRING

DisplayController_FD0604::DisplayController_FD0604_Parameters displayParameters = {
    100,        // number of ms to delay between counting intervals

    A7,         // temperature sensor  pin
    10000.0,    // temperature sensor accompanying resistor
    250,        // temperature sensor update interval
    0,          // enable serial output for temperature sensor

    E2END       // last EEPROM address for storing display orientation data
};


const uint16_t BASE_ADDR = 0;            // EEPROM address to start writing writing from
const uint8_t SLOT_SIZE = 6;            // uint32_t for sequence number (for wear levelling) + uint16_t for number

const uint8_t gnd[2] = {2, 3};      // first two pins of display in order of connection
const uint8_t pins[3] = {6, 7, 8};  // order of latchpin, clockpin, datapin

const uint16_t NUM_SLOTS = 170;          // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())
