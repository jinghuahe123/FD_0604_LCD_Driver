#ifndef DISPLAYCONTROLLER_FD0604_HPP
#define DISPLAYCONTROLLER_FD0604_HPP

#ifndef __AVR_ATmega328P__
#warning Chip is not certified for this code. ADC measurements may be off among other flukes.
#endif

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

class DisplayController_FD0604 {
public:
    // code assumes struct is placed in PROGMEM
    struct DisplayController_FD0604_Parameters {
        const uint16_t BASE_ADDR;
        const uint16_t NUM_SLOTS;

        const uint16_t countingIntervalAddress; // EEPROM address for storing counting interval data 

        const uint16_t displayOrientationAddress; // EEPROM address for storing display orientation data
        const uint16_t numHistoryAddress; // EEPROM address for storing how number history count to recall

        struct {
            volatile uint8_t* const DDRx_temperaturePin;        // data direction register for temperature pin
            volatile uint8_t* const PORTx_temperaturePin;       // port register for temperature pin
            const uint8_t PIN_temperaturePin;                   // pin on port for temperature pin

            const float resistorValue;                          // accompanying resistor value for temperature probe
            const uint16_t temperatureUpdateIntervalAddress;    // EEPROM address for storing temperature update interval data 
            const uint16_t temperatureSerialEnabledAddress;     // EEPROM address for storing serial enabled data for temperature probe
        } tempSensor;
        
        struct {
            volatile uint8_t* const DDRx_rawInputPin;           // data direction register for raw input pin
            volatile uint8_t* const PORTx_rawInputPin;          // port register for raw input pin
            const uint8_t PIN_rawInputPin;                      // pin on port for raw input pin

            const uint16_t rawInputUpdateIntervalAddress;       // EEPROM address for storing raw input update interval data  
            const uint16_t rawInputSerialEnabledAddress;        // EEPROM address for storing serial enabled data for raw input
        } rawInput;
    };


    DisplayController_FD0604(const DisplayDriver_FD0604::DriverParameters& driverParams, const DisplayController_FD0604_Parameters& params);
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
    PersistentStorageManager<int16_t> _storageManager;
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

    static const char pinAlias[][3] PROGMEM;

    char temperaturePinAlias[3] = {0};
    char rawInputPinAlias[3] = {0};
    
    void _init();

    static const uint8_t _commandListSize;
    static const char _commandList[][MAX_INPUT_SIZE] PROGMEM;
    void _getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize);
    int8_t _findCommandIndex(const char* input);

    void _updateDisplay();
    void _displayInit(int8_t initTime = 60);
    static int _freeMemory();
    static bool _checkIfNumericSigned(const char* str, int16_t& number);
    static bool _checkIfNumericUnsigned(const char* str, uint16_t& number);
    bool _parseAndSetNumber(const char* input);

    using CommandHandler = void(DisplayController_FD0604::*)();
    static const CommandHandler commandHandlers[]; // static const array of function pointers to command handler functions
    static const uint8_t maxCommandOptions;
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
    void _handleReboot();

    using DisplayHandler = void(DisplayController_FD0604::*)();
    static const DisplayHandler displayHandlers[];
    static const uint8_t maxDisplayHandlers;
    void _displayOff();
    void _displayCycle();
    void _displayNull();
    void _displayTemp();
    void _displayRAWInput();

    uint16_t _getSerial();
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