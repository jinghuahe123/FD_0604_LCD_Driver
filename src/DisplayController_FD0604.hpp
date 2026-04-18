#ifndef DISPLAYCONTROLLER_FD0604
#define DISPLAYCONTROLLER_FD0604

#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
//#include <StandardCplusplus.h>
//#include <iomanip>
//#include <serstream>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"

// special display states
#define OFF         -1 
#define CYCLE       -2 
#define NULL_DISP   -3
#define TEMP        -4 
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
        const uint16_t BASE_ADDR;
        const uint8_t SLOT_SIZE;
        const uint16_t NUM_SLOTS;

        const uint16_t countingIntervalAddress; // EEPROM address for storing counting interval data 

        const uint8_t temperaturePin; // pin that the temperature probe is connected to
        const double resistorValue; // accompanying resistor value for temperature probe
        const uint16_t temperatureUpdateIntervalAddress; // EEPROM address for storing temperature update interval data 
        const uint16_t temperatureSerialEnabledAddress; // EEPROM address for storing serial enabled data for temperature probe

        const uint8_t rawInputPin; // pin that the raw input is connected to 
        const uint16_t rawInputUpdateIntervalAddress; // EEPROM address for storing raw input update interval data  
        const uint16_t rawInputSerialEnabledAddress; // EEPROM address for storing serial enabled data for raw input

        const uint16_t displayOrientationAddress; // EEPROM address for storing display orientation data
        const uint16_t numHistoryAddress; // EEPROM address for storing how number history count to recall
        
    };

    DisplayController_FD0604(DisplayDriver_FD0604::DriverParams_DIRECTPORT& driverParams, DisplayController_FD0604_Parameters& params);
    DisplayController_FD0604(DisplayDriver_FD0604::DriverParams_DIRECTPORT_MinimalWiring& driverParams, DisplayController_FD0604_Parameters& params);
    DisplayDriver_FD0604* getDisplayDriverObject();
    
    void updateDisplay();
    void processInput(const String& input);
    void processSecondaryInput(const String& input);
    void showAvailableCommands();
    void showInfo();
    void clear();

private:
    static const char processor[] PROGMEM;

    DisplayController_FD0604_Parameters _params;
    DisplayDriver_FD0604 _display;
    PersistentStorageManager _storageManager;
    const bool transistor_enabled_flag;
    const bool minimal_pin_flag;
    
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
    bool staticDisplayShown = false;

    const char* temperaturePinAlias = "  ";
    const char* rawInputPinAlias = "  ";
    
    void _init();

    static const uint8_t _commandListSize;
    static const char _commandList[][10] PROGMEM;
    void _getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize);
    int8_t _findCommandIndex(const String& input);
    String getValueDisplay(uint16_t value);

    void _updateDisplay();
    void _displayInit(int8_t initTime = 60);
    int _freeMemory();
    bool _checkIfNumeric(const String& str, int16_t& number);
    bool _parseAndSetNumber(const String& input);

    void _handleHelp();
    void _handleInfo();
    void _handleMem();
    void _handleInit();
    void _handleSettings();
    void _handleErase();
    void _handleReset();
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

    int16_t _getSerial();

    const uint8_t maxSettingsOptions = 8; // need to fix thsi for a more elegant solution
    void _exitSettings();
    void _updateCycleInterval();
    void _updateTemperatureInterval();
    void _updateTemperatureSerialOutput();
    void _updateRawInputInterval();
    void _updateRawInputSerialOutput();
    void _updateDisplayOrientation();
    void _updateHistoryRecallDepth();
    
};

#endif