#ifndef DISPLAYCONTROLLER_FD0604
#define DISPLAYCONTROLLER_FD0604

#include <Arduino.h>
#include <EEPROM.h>
//#include <StandardCplusplus.h>
//#include <iomanip>
//#include <serstream>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"

// special display states
#define OFF         -1 //4000
#define CYCLE       -2 //4001
#define NULL_DISP   -3 //4002
#define TEMP        -4 //4003
#define RAWINPUT    -5

// predefine classes used
class DisplayDriver_FD0604;
class PersistentStorageManager;

//namespace std {
//    extern ohserialstream cout;
//}

class DisplayController_FD0604 {
public:
    struct DisplayController_FD0604_Parameters {
        const unsigned long countingInterval;

        const uint8_t temperaturePin;
        const double resistorValue;
        const unsigned long temperatureUpdateInterval;
        const bool serial_enabled;

        const uint8_t rawInputPin;
        const unsigned long rawInputUpdateInterval; 

        const uint16_t displayOrientationAddress; // last EEPROM address for storing display orientation data
        const uint16_t numHistory;
        
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

    const char* temperaturePinAlias = "  ";
    const char* rawInputPinAlias = "  ";
    
    void init();

    static const uint8_t _commandListSize;
    static const char _commandList[][8] PROGMEM;
    void getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize);
    int8_t _findCommandIndex(const String& input);
    String getValueDisplay(uint16_t value);

    void _updateDisplay();
    void _displayInit(int8_t initTime = 60);
    int _freeMemory();
    bool _checkIfNumeric(const String& str, int16_t& number);
    bool _parseAndSetNumber(const String& input);

    void _handleHelp();
    void _handleMem();
    void _handleInit();
    void _handleInvert();
    void _handleErase();
    void _handleHistory();
    void _handleOff();
    void _handleCycle();
    void _handleNull();
    void _handleTemp();
    void _handleRAWInput();

    void _displayOff();
    void _displayCycle();
    void _displayNull();
    void _displayTemp();
    void _displayRAWInput();

};

#endif