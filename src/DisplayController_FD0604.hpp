#ifndef DISPLAYCONTROLLER_FD0604
#define DISPLAYCONTROLLER_FD0604

// prevent serial conflicts with custom serial driver
#define HardwareSerial_h
#define HardwareSerial_h_ 
#define DISABLE_HARDWARE_SERIAL
#undef Serial

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "serial.h"
#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"

#define MAX_INPUT_SIZE  RX_BUFFER_SIZE

// special display states
#define OFF         -1 
#define CYCLE       -2 
#define NULL_DISP   -3
#define TEMP        -4 
#define RAWINPUT    -5

// predefine classes used
class DisplayDriver_FD0604;
class PersistentStorageManager;

class DisplayController_FD0604 {
public:
    struct DisplayController_FD0604_Parameters {
        const uint16_t BASE_ADDR;
        const uint8_t SLOT_SIZE;
        const uint16_t NUM_SLOTS;

        const uint16_t countingIntervalAddress; // EEPROM address for storing counting interval data 

        const uint8_t temperaturePin; // pin that the temperature probe is connected to
        const float resistorValue; // accompanying resistor value for temperature probe
        const uint16_t temperatureUpdateIntervalAddress; // EEPROM address for storing temperature update interval data 
        const uint16_t temperatureSerialEnabledAddress; // EEPROM address for storing serial enabled data for temperature probe

        const uint8_t rawInputPin; // pin that the raw input is connected to 
        const uint16_t rawInputUpdateIntervalAddress; // EEPROM address for storing raw input update interval data  
        const uint16_t rawInputSerialEnabledAddress; // EEPROM address for storing serial enabled data for raw input

        const uint16_t displayOrientationAddress; // EEPROM address for storing display orientation data
        const uint16_t numHistoryAddress; // EEPROM address for storing how number history count to recall
        
    };

    DisplayController_FD0604(DisplayDriver_FD0604::DriverParams& driverParams, DisplayController_FD0604_Parameters& params);
    DisplayDriver_FD0604* getDisplayDriverObject();
    
    void updateDisplay();
    void processInput(const char* input);
    void processSecondaryInput(const char* input);
    void showAvailableCommands();
    void showInfo();
    void clear();

private:
    static const char processor[] PROGMEM;

    DisplayController_FD0604_Parameters _params;
    DisplayDriver_FD0604 _display;
    PersistentStorageManager _storageManager;
    const bool transistor_enabled_flag;
    
    // Calculate total RAM (AVR) or use manual define
    #if !defined(TOTAL_RAM) && defined(__AVR__)
    const uint16_t TOTAL_RAM = RAMEND - RAMSTART + 1;
    #elif !defined(TOTAL_RAM)
    const unsigned int TOTAL_RAM = 0; // Fallback
    #endif

    int16_t _number = 0;
    uint16_t _cycle_number = 0;
    char _input[MAX_INPUT_SIZE] = {0};
    unsigned long previousMillis = 0;
    bool staticDisplayShown = false;

    const char* temperaturePinAlias = "  ";
    const char* rawInputPinAlias = "  ";
    
    void _init();

    static const uint8_t _commandListSize;
    static const char _commandList[][MAX_INPUT_SIZE] PROGMEM;
    void _getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize);
    int8_t _findCommandIndex(const char* input);

    void _updateDisplay();
    void _displayInit(int8_t initTime = 60);
    static int _freeMemory();
    static bool _checkIfNumeric(const char* str, int16_t& number);
    bool _parseAndSetNumber(const char* input);

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

    using SettingsHandler = void(DisplayController_FD0604::*)();
    static const SettingsHandler settingsHandlers[]; // static const array of function pointers
    static const uint8_t maxSettingsOptions;

    void _exitSettings();
    void _updateCycleInterval();
    void _updateTemperatureInterval();
    void _updateTemperatureSerialOutput();
    void _updateRawInputInterval();
    void _updateRawInputSerialOutput();
    void _updateDisplayOrientation();
    void _updateHistoryRecallDepth();
    
};

#endif // DISPLAY_CONTROLLER_FD0604_HPP