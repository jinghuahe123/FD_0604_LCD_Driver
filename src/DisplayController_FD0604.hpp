#ifndef DISPLAYCONTROLLER_FD0604
#define DISPLAYCONTROLLER_FD0604

#include <Arduino.h>
#include <EEPROM.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"

// special display states
#define OFF         -1 //4000
#define CYCLE       -2 //4001
#define NULL_DISP   -3 //4002
#define TEMP        -4 //4003

// predefine classes used
class DisplayDriver_FD0604;
class PersistentStorageManager;

class DisplayController_FD0604 {
public:
    struct DisplayController_FD0604_Parameters {
        //const int BASE_ADDR; // EEPROM address to start writing writing from
        //const int SLOT_SIZE; // uint32_t for sequence number (for wear levelling) + uint16_t for number
        //const uint8_t gnd[2]; // first two pins of display in order of connection
        //const uint8_t pins[3]; // order of latchpin, clockpin, datapin

        // local variables for class
        const unsigned long countingInterval;
        const uint8_t temperaturePin;
        const double resistorValue;
        const unsigned long temperatureUpdateInterval;
        const bool serial_enabled;
        //const int NUM_SLOTS; // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())
        const int displayOrientationAddress; // last EEPROM address for storing display orientation data
        
    };

    DisplayController_FD0604(DisplayDriver_FD0604& disp, PersistentStorageManager& pStore, DisplayController_FD0604_Parameters& params);

    
    void updateDisplay();
    void processInput(const String& input);
    void _showAvailableCommands();

private:
    DisplayDriver_FD0604& _display;
    PersistentStorageManager& _storageManager;
    DisplayController_FD0604_Parameters _params;

    // Calculate total RAM (AVR) or use manual define
    #if !defined(TOTAL_RAM) && defined(__AVR__)
    const unsigned int TOTAL_RAM = RAMEND - RAMSTART + 1;
    #elif !defined(TOTAL_RAM)
    const unsigned int TOTAL_RAM = 0; // Fallback
    #endif

    int16_t _number = 0;
    uint16_t _cycle_number = 0;
    String _input = "";
    unsigned long previousMillis = 0;
    
    void init();

    static const uint8_t _commandListSize;
    static const char _commandList[][8] PROGMEM;
    void getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize);
    int8_t _findCommandIndex(const String& input);

    void _updateDisplay();
    void _displayInit(int8_t initTime = 60);
    int _freeMemory();
    bool _checkIfNumeric(const String& str, int16_t& number);
    bool _parseAndSetNumber(const String& input);

    void _handleHelp();
    void _handleMem();
    void _handleInit();
    void _handleInvert();
    void _handleOff();
    void _handleCycle();
    void _handleNull();
    void _handleTemp();

    void _displayOff();
    void _displayCycle();
    void _displayNull();
    void _displayTemp();

};

#endif