#include "DisplayController_FD0604.hpp"

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


#define HARDWARE_SERIAL_BAUD    1000000

// @ 8MHz 2400 is the maximum working speed (1200 with heartbeat LED)
// fix? - code issue or just hardware limit
#define SOFTWARE_SERIAL_BAUD    1200
#define SOFT_RX_DIGITAL_PIN    11
#define SOFT_TX_DIGITAL_PIN    12
// whether secondary serial output should print initialisation text
#define SOFT_SERIAL_OUTPUT

#define FIRMWARE_VER_SIZE       32
constexpr char version[] PROGMEM =  "FD_0604 LED Display v0.1.37";

const DisplayDriver_FD0604::DriverParameters displayParams = {
    .npn_transistor_enable = 1,
	
    // pin D4
	.DDRx_latchPin = &DDRD,             // latchpin data direction register
	.PORTx_latchPin = &PORTD,           // latchpin port register
	.PIN_latchPin = 4,                  // latchpin physical pin (on register)
	
    // pin D5
	.DDRx_clockPin = &DDRD,             // clockpin data direction register   
	.PORTx_clockPin = &PORTD,           // clockpin port register
	.PIN_clockPin = 5,                  // clockpin physical pin (on register)
	
    // pin D6
	.DDRx_dataPin = &DDRD,              // datapin data direction register
	.PORTx_dataPin = &PORTD,            // datapin port register
	.PIN_dataPin = 6,                   // datapin physical pin (on register)
};


const DisplayController_FD0604::DisplayController_FD0604_Parameters controllerParams = {
    .BASE_ADDR = 0x0040,     // EEPROM address to start writing writing from
    .NUM_SLOTS = 160,        // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

    .countingIntervalAddress = 0x0020,     // EEPROM Address that stores the delay between counting intervals 

    .displayOrientationAddress = 0x0022,     // EEPROM address for storing display orientation data
    .numHistoryAddress = 0x0024,             // EEPROM address for history recall depth

    .tempSensor = {
        // for analog channel 6/7, as there it is input pin only, MUST SET DDRx and PORTx as nullptr!
        .DDRx_temperaturePin = nullptr,       // data direction register for temp sensor
        .PORTx_temperaturePin = nullptr,     // port register for temp sensor
        .PIN_temperaturePin = 7,            // pin on port of temp sensor

        .resistorValue = 10000.0,                       // temperature sensor accompanying resistor
        .temperatureUpdateIntervalAddress = 0x0030,     // EEPROM address that stores the delay between the temperature reading updating
        .temperatureSerialEnabledAddress = 0x0032,      // EEPROM address for enable serial output for temperature sensor
    },

    .rawInput = {
        // for analog channel 6/7, as there it is input pin only, MUST SET DDRx and PORTx as nullptr!
        .DDRx_rawInputPin = nullptr,        // data direction register for raw input
        .PORTx_rawInputPin = nullptr,       // port register for raw input
        .PIN_rawInputPin = 6,               // pin on port of raw input

        .rawInputUpdateIntervalAddress = 0x0038,        // EEPROM address that stores the delay between the raw input reading updating 
        .rawInputSerialEnabledAddress = 0x003A,         // EEPROM address for enable serial output for raw input
    },
};
