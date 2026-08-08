#ifndef DISPLAY_COMMAND_PROCESSOR_HPP
#define DISPLAY_COMMAND_PROCESSOR_HPP

#include <stdint.h>

#include "serial.h"
#include "DisplayParameters.hpp"
#include "DisplayDriver_FD0604.hpp"
#include "DisplaySettingsManager.hpp"
#include "DisplayModeManager.hpp"


/**
 * @class DisplayCommandProcessor
 * @brief Processes user commands from serial input
 * 
 * Handles:
 * - Command parsing and dispatch
 * - Help system
 * - System information display
 * - Memory info
 * - Reboot functionality
 */
class DisplayCommandProcessor {
public:
    /**
     * @brief Constructor
     * @param display Display Driver object reference
     * @param settingsManager Settings Manager object reference
     * @param modeManager Mode Manager object reference
     * @param params Display Parameters object reference
     */
    DisplayCommandProcessor(DisplayDriver_FD0604& display, DisplaySettingsManager& settingsManager, DisplayModeManager& modeManager, const DisplayParameters& params);

    /**
     * @brief Process a command string
     * @param input Command input from serial
     * @param isConfigurationCommand Output flag indicating if the command was a configuration command
     * @return true if command was processed, false if it was a number
     */
    bool processCommand(const char* input, bool& isConfigurationCommand);
    
    /**
     * @brief Show available commands
     */
    void showHelp();
    
    /**
     * @brief Show system information
     */
    void showInfo();
    
    /**
     * @brief Show free memory
     */
    void showMemory();
    
    /**
     * @brief Reboot the system
     */
    void reboot();

private:
    DisplayDriver_FD0604& _display;
    DisplaySettingsManager& _settingsManager;
    DisplayModeManager& _modeManager;
    const DisplayParameters& _params;

    // Calculate total RAM (AVR) or use manual define
    #if !defined(TOTAL_RAM) && defined(__AVR__)
    static constexpr uint16_t TOTAL_RAM = RAMEND - RAMSTART + 1;
    #elif !defined(TOTAL_RAM)
    static constexpr unsigned uint16_t TOTAL_RAM = 0; // Fallback
    #endif

    // pin aliases
    const char* _temperaturePinAlias;
    const char* _rawInputPinAlias;

    static const char _commandList[][MAX_INPUT_SIZE] PROGMEM;
    static const uint8_t _commandListSize;

    using CommandHandler = void(DisplayCommandProcessor::*)();
    static const CommandHandler _commandHandlers[] PROGMEM;
    static const uint8_t _maxCommandOptions;
    static const uint8_t _configurationCommandEndIndex;

    void _handleHelp();
    void _handleInfo();
    void _handleMem();
    void _handleSettings();
    void _handleErase();
    void _handleReset();
    void _handleHistory();
    void _handleOff();
    void _handleCycle();
    void _handleNull();
    void _handleTemp();
    void _handleRawInput();
    void _handleReboot();

    // helper functions
    int8_t _findCommandIndex(const char* input);
    void _getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize);
    uint16_t _freeMemory();

    // disable copy constructor and assignment
    DisplayCommandProcessor(const DisplayCommandProcessor&) = delete;
    DisplayCommandProcessor& operator=(const DisplayCommandProcessor&) = delete;


};

#endif // DISPLAY_COMMAND_PROCESSOR_HPP