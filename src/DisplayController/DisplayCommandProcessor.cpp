#include "DisplayCommandProcessor.hpp"

#include <avr/wdt.h>
#include "drivers/core/char_helper.h"
#include "PersistentStorageManager/PersistentStorageManager.hpp"

// command list in PROGMEM
const char DisplayCommandProcessor::_commandList[][MAX_INPUT_SIZE] PROGMEM = {
    "HELP", 
    "INFO",
    "MEM",
    "SETTINGS", 
    "ERASE",
    "RESET",
    "HISTORY",
    "OFF", 
    "CYCLE", 
    "NULL", 
    "TEMP",
    "RAW", 
    "REBOOT",
};

const uint8_t DisplayCommandProcessor::_commandListSize = 
    sizeof(_commandList) / sizeof(_commandList[0]);

const DisplayCommandProcessor::CommandHandler DisplayCommandProcessor::_commandHandlers[] PROGMEM = {
    &DisplayCommandProcessor::_handleHelp,
    &DisplayCommandProcessor::_handleInfo,
    &DisplayCommandProcessor::_handleMem,
    &DisplayCommandProcessor::_handleSettings,
    &DisplayCommandProcessor::_handleErase,
    &DisplayCommandProcessor::_handleReset,
    &DisplayCommandProcessor::_handleHistory,
    &DisplayCommandProcessor::_handleOff,
    &DisplayCommandProcessor::_handleCycle,
    &DisplayCommandProcessor::_handleNull,
    &DisplayCommandProcessor::_handleTemp,
    &DisplayCommandProcessor::_handleRawInput,
    &DisplayCommandProcessor::_handleReboot,
};

const uint8_t DisplayCommandProcessor::_maxCommandOptions = 
    sizeof(_commandHandlers) / sizeof(_commandHandlers[0]) - 1;

const uint8_t DisplayCommandProcessor::_configurationCommandEndIndex = 6; // index of last configuration command in _commandList

DisplayCommandProcessor::DisplayCommandProcessor(DisplayDriver& display, DisplaySettingsManager& settingsManager, DisplayModeManager& modeManager, const DisplayParameters& params)
    : _display(display), _settingsManager(settingsManager), _modeManager(modeManager), _params(params)
{
    _temperaturePinAlias = _settingsManager.getTemperaturePinAlias();
    _rawInputPinAlias = _settingsManager.getRawInputPinAlias();
}

bool DisplayCommandProcessor::processCommand(const char* input, bool& isConfigurationCommand) {
    if (input == nullptr || input[0] == '\0') return false;

    // no need to trim, already trimmed by controller when passed to function

    // find command index
    int8_t cmdIndex = _findCommandIndex(input);
    if (cmdIndex == -1 || cmdIndex > _maxCommandOptions) return false;

    if (cmdIndex <= _configurationCommandEndIndex) {
        isConfigurationCommand = true;
    } else {
        isConfigurationCommand = false;
    }

    // call corresponding handler
    CommandHandler handler;
    memcpy_P(&handler, &_commandHandlers[cmdIndex], sizeof(handler));
    (this->*handler)();
    return true;
}

void DisplayCommandProcessor::showHelp() {
    uint16_t numHistory = _settingsManager.readHistoryDepth();

    Serial.println(F("============================== FD-0604 LED Display Commands ============================="));
    Serial.println(F("Enter any number to display on the screen:"));
    Serial.println(F("- 0000~3999 with normal orientation."));
    Serial.println(F("- 000~999 with inverted orientation."));
    Serial.println();

    Serial.println(F("Alternative available commands:"));
    Serial.print(F("TEMP       -  Turns the display into a thermometer using thermosistor attached on pin ")); Serial.print(_temperaturePinAlias); Serial.println(F("."));
    Serial.print(F("RAW        -  Shows RAW input value on pin ")); Serial.print(_rawInputPinAlias); Serial.println(F(". CAUTION: analogReference may be set!"));
    Serial.println(F("CYCLE      -  Cycles continuously 0~3999 / 0~999 with 100ms delay between numbers."));
    Serial.println(F("NULL       -  Shows --:-- on the display."));
    Serial.println(F("OFF        -  Turns off the display."));
    Serial.println();

    Serial.println(F("Configuration commands:"));
    Serial.println(F("HELP       -  Shows this help page."));
    Serial.println(F("INFO       -  Shows the hardware information of the board."));
    Serial.println(F("SETTINGS   -  Shows settings page and changes hardware configurations."));
    Serial.println(F("MEM        -  Prints to Serial the available free memory on the MCU."));
    Serial.println(F("ERASE      -  Erases previously displayed number history."));
    Serial.println(F("RESET      -  Resets to factory defaults. CAUTION - WILL ERASE ALL USER DATA!"));
    Serial.print(F("HISTORY    -  Prints to Serial the last ")); Serial.print(numHistory); Serial.println(F(" numbers displayed."));
    Serial.println(F("REBOOT     -  Reboots system."));

    Serial.println(F("========================================================================================="));
    _delay_ms(3);
    Serial.println();
}

void DisplayCommandProcessor::showInfo() {
    // TODO: add check to ensure settingsmanager object is valid
    _settingsManager.displaySettingsInfo();
}

void DisplayCommandProcessor::showMemory() {
    float percentFree;
    uint16_t freeMem = _freeMemory();
    percentFree = 100.0f * static_cast<float>(freeMem) / TOTAL_RAM;

    Serial.print(F("MEMORY: "));
    Serial.print(freeMem);
    Serial.print(F(" of "));
    Serial.print(TOTAL_RAM);
    Serial.print(F(" bytes free. ("));
    Serial.print(percentFree, 2);
    Serial.println(F("%)"));

    extern int __heap_start, *__brkval;
    uint16_t heapSize = (__brkval == 0 ? 0 : reinterpret_cast<uint16_t>(__brkval) - reinterpret_cast<uint16_t>(&__heap_start));
    if (heapSize > 0) {
        Serial.println(F("CAUTION: Heap allocations detected. Resolve in release builds."));

        Serial.print(F("Heap Size: "));
        Serial.print(heapSize);
        Serial.print(F(" bytes. ("));
        Serial.print(100.0f * static_cast<float>(heapSize) / TOTAL_RAM, 2);
        Serial.println(F("%)"));
    }
}

void DisplayCommandProcessor::reboot() {
    Serial.println(F("Rebooting..."));
    wdt_enable(WDTO_15MS);
    while (1);
}

void DisplayCommandProcessor::_handleHelp() {
    showHelp();
}

void DisplayCommandProcessor::_handleInfo() {
    showInfo();
}

void DisplayCommandProcessor::_handleMem() {
    showMemory();
}

void DisplayCommandProcessor::_handleSettings() {
    // TODO: add check to ensure settingsmanager object is valid
    _settingsManager.showSettingsMenu();
}

void DisplayCommandProcessor::_handleErase() {
    Serial.print(F("Erasing... "));
    PersistentStorageManager<int16_t> _storageManager(_params.persistentStorage.BASE_ADDR, _params.persistentStorage.NUM_SLOTS);
    _storageManager.erase();
    Serial.println(F("Successfully erased previous history."));
}

void DisplayCommandProcessor::_handleReset() {
    // TODO: add check to ensure settingsmanager object is valid
    Serial.print(F("Resetting to factory defaults... "));
    _settingsManager.factoryReset();
}

void DisplayCommandProcessor::_handleHistory() {
    auto parse_and_print_value = [](int16_t val) {
        switch (val) {
            case MODE_OFF:           Serial.print(F("OFF"));           break;
            case MODE_CYCLE:         Serial.print(F("CYCLE"));         break;
            case MODE_NULL:          Serial.print(F("NULL_DISP"));     break;
            case MODE_TEMP:          Serial.print(F("TEMP"));          break;
            case MODE_RAWINPUT:      Serial.print(F("RAW"));           break;
            default:                 Serial.print(val);              break;
        }
    };

    uint16_t numHistory = _settingsManager.readHistoryDepth();
    PersistentStorageManager<int16_t> _storageManager(_params.persistentStorage.BASE_ADDR, _params.persistentStorage.NUM_SLOTS);
    _storageManager.printHistory(numHistory, parse_and_print_value);
}

void DisplayCommandProcessor::_handleOff() {
    _modeManager.setMode(MODE_OFF);
}

void DisplayCommandProcessor::_handleCycle() {
    _modeManager.resetCycleCounter();
    _modeManager.setMode(MODE_CYCLE);
}

void DisplayCommandProcessor::_handleNull() {
    _modeManager.setMode(MODE_NULL);
}

void DisplayCommandProcessor::_handleTemp() {
    _modeManager.setMode(MODE_TEMP);
}

void DisplayCommandProcessor::_handleRawInput() {
    _modeManager.setMode(MODE_RAWINPUT);
    if (_display.getDisplayOrientation() == DisplayDriver::DisplayOrientation::FLIPPED) {
        Serial.println(F("CAUTION: Inverted display does not support last digit output."));
        Serial.println(F("Output will be one order of magnitude smaller than real value, and truncated."));
    }
}

void DisplayCommandProcessor::_handleReboot() {
    reboot();
}

int8_t DisplayCommandProcessor::_findCommandIndex(const char* input) {
    char buffer[sizeof(_commandList[0])];
    for (int8_t i = 0; i < _commandListSize; i++) {
        _getCommandFromFlash(i, buffer, sizeof(buffer));
        if (strcasecmp(input, buffer) == 0) { // returns true if matching
            return i; // Return the index of the matching command
        }
    }
    return -1; // Not found
}

void DisplayCommandProcessor::_getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize) {
    if (index >= _commandListSize) {
        if (bufSize > 0) buffer[0] = '\0';
        return;
    }
    strlcpy_P(buffer, _commandList[index], bufSize);
}

uint16_t DisplayCommandProcessor::_freeMemory() {
    extern int __heap_start, *__brkval;
    uint8_t v;
    return (uint16_t)(&v - (__brkval == 0 ? (uint16_t)&__heap_start : (uint16_t)__brkval));
}