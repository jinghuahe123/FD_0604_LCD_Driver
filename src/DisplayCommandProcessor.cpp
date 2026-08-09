#include "DisplayCommandProcessor.hpp"

#include <avr/wdt.h>
#include "char_helper.h"
#include "PersistentStorageManager.hpp"

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

DisplayCommandProcessor::DisplayCommandProcessor(DisplayDriver_FD0604& display, DisplaySettingsManager& settingsManager, DisplayModeManager& modeManager, const DisplayParameters& params)
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

    serial_println_P(F("============================== FD-0604 LED Display Commands ============================="));
    serial_println_P(F("Enter any number to display on the screen:"));
    serial_println_P(F("- 0000~3999 with normal orientation."));
    serial_println_P(F("- 000~999 with inverted orientation."));
    serial_ln();

    serial_println_P(F("Alternative available commands:"));
    serial_print_P(F("TEMP       -  Turns the display into a thermometer using thermosistor attached on pin ")); serial_print(_temperaturePinAlias); serial_println_P(F("."));
    serial_print_P(F("RAW        -  Shows RAW input value on pin ")); serial_print(_rawInputPinAlias); serial_println_P(F(". CAUTION: analogReference may be set!"));
    serial_println_P(F("CYCLE      -  Cycles continuously 0~3999 / 0~999 with 100ms delay between numbers."));
    serial_println_P(F("NULL       -  Shows --:-- on the display."));
    serial_println_P(F("OFF        -  Turns off the display."));
    serial_ln();

    serial_println_P(F("Configuration commands:"));
    serial_println_P(F("HELP       -  Shows this help page."));
    serial_println_P(F("INFO       -  Shows the hardware information of the board."));
    serial_println_P(F("SETTINGS   -  Shows settings page and changes hardware configurations."));
    serial_println_P(F("MEM        -  Prints to Serial the available free memory on the MCU."));
    serial_println_P(F("ERASE      -  Erases previously displayed number history."));
    serial_println_P(F("RESET      -  Resets to factory defaults. CAUTION - WILL ERASE ALL USER DATA!"));
    serial_print_P(F("HISTORY    -  Prints to Serial the last ")); serial_print_u16(numHistory); serial_println_P(F(" numbers displayed."));
    serial_println_P(F("REBOOT     -  Reboots system."));

    serial_println_P(F("========================================================================================="));
    _delay_ms(3);
    serial_ln();
}

void DisplayCommandProcessor::showInfo() {
    // TODO: add check to ensure settingsmanager object is valid
    _settingsManager.displaySettingsInfo();
}

void DisplayCommandProcessor::showMemory() {
    float percentFree;
    uint16_t freeMem = _freeMemory();
    percentFree = 100.0f * static_cast<float>(freeMem) / TOTAL_RAM;

    serial_print_P(F("MEMORY: "));
    serial_print_u16(freeMem);
    serial_print_P(F(" of "));
    serial_print_u16(TOTAL_RAM);
    serial_print_P(F(" bytes free. ("));
    serial_print_float(percentFree, 2);
    serial_println_P(F("%)"));

    extern int __heap_start, *__brkval;
    uint16_t heapSize = (__brkval == 0 ? 0 : (uint16_t)__brkval - (uint16_t)&__heap_start);
    if (heapSize > 0) {
        serial_println_P(F("CAUTION: Heap allocations detected. Resolve in release builds."));

        serial_print_P(F("Heap Size: "));
        serial_print_u16(heapSize);
        serial_print_P(F(" bytes. ("));
        serial_print_float(100.0f * static_cast<float>(heapSize) / TOTAL_RAM, 2);
        serial_println_P(F("%)"));
    }
}

void DisplayCommandProcessor::reboot() {
    serial_println_P(F("Rebooting..."));
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
    serial_print_P(F("Erasing... "));
    PersistentStorageManager<int16_t> _storageManager(_params.BASE_ADDR, _params.NUM_SLOTS);
    _storageManager.erase();
    serial_println_P(F("Successfully erased previous history."));
}

void DisplayCommandProcessor::_handleReset() {
    // TODO: add check to ensure settingsmanager object is valid
    serial_print_P(F("Resetting to factory defaults... "));
    _settingsManager.factoryReset();
}

void DisplayCommandProcessor::_handleHistory() {
    uint16_t numHistory = _settingsManager.readHistoryDepth();

    // check if free memory is enough to create the array for storing number history
    const uint16_t total_ram = RAMEND - RAMSTART + 1;
    const uint16_t min_allowed_ram = total_ram * 0.1; // leave 10% buffer room
    const uint16_t mem_required = numHistory * sizeof(PersistentStorageManager<int16_t>::StorageEntry);
    int free_memory = _freeMemory();
    
    if (free_memory < 0) free_memory = 0;

    bool enough_memory_available = (min_allowed_ram + mem_required >= (unsigned int)free_memory) ? true : false;

    if (enough_memory_available) {
        serial_print_P(F("MCU does not have enough free memory to display "));
        serial_print_u16(numHistory);
        serial_println_P(F(" number histories."));
    } else {
        PersistentStorageManager<int16_t>::StorageEntry entries[numHistory] = {0};

        auto print_padded_u32 = [](uint32_t value, uint8_t digits) {
            char buffer[digits + 1]; // max 10 digits + null terminator
            char* ptr = buffer + digits; // point to the end of the buffer
            *ptr = '\0';

            for (uint8_t i=0; i<digits; i++) {
                ptr--;
                *ptr = '0' + (value % 10);
                value /= 10;
            }

            serial_print(ptr);
        };

        auto parse_and_print_value = [](int16_t val) {
            switch (val) {
                case MODE_OFF:           serial_print_P(F("OFF"));           break;
                case MODE_CYCLE:         serial_print_P(F("CYCLE"));         break;
                case MODE_NULL:          serial_print_P(F("NULL_DISP"));     break;
                case MODE_TEMP:          serial_print_P(F("TEMP"));          break;
                case MODE_RAWINPUT:      serial_print_P(F("RAW"));           break;
                default:                 serial_print_i16(val);              break;
            }
        };

        serial_println_P(F("=============================================================="));
        serial_println_P(F("                     EEPROM STORAGE HISTORY"));
        serial_println_P(F("=============================================================="));
        serial_print_P(F("Base Address: 0x")); serial_print_hex16(_params.BASE_ADDR); serial_ln();
        serial_print_P(F("Total Slots: "));
        serial_print_u16(_params.NUM_SLOTS); serial_ln();
        serial_print_P(F("Display Orientation: "));
        serial_println_P(_settingsManager.readDisplayOrientation() == DisplayDriver_FD0604::DisplayOrientation::FLIPPED ? F("Inverted Display") : F("Normal Display"));
        _handleMem();
        serial_println_P(F("--------------------------------------------------------------"));
        PersistentStorageManager<int16_t> _storageManager(_params.BASE_ADDR, _params.NUM_SLOTS);
        uint16_t uninitialised = _storageManager.readHistory(numHistory, entries);

        if (uninitialised != 0xFFFF) {
            for (uint16_t i = 0; i < numHistory - uninitialised; i++) {
                serial_print_P(F("[")); print_padded_u32(i, 4);
                serial_print_P(F("] Address: 0x")); serial_print_hex16(entries[i].address);
                serial_print_P(F(" | Sequence: ")); print_padded_u32(entries[i].sequence, 10);
                serial_print_P(F(" | Value: ")); parse_and_print_value(entries[i].value);
                serial_ln();
            }
            serial_println_P(F("--------------------------------------------------------------"));
            serial_print_P(F("Total entries searched: ")); serial_print_u16(numHistory - uninitialised);
            serial_ln();
            serial_print_P(F("Empty entries searched: ")); serial_print_u16(uninitialised);
            serial_ln();
        } else {
            serial_println_P(F("No data found in storage."));
        }

        serial_println_P(F("=============================================================="));
    }
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
    if (_display.getDisplayOrientation() == DisplayDriver_FD0604::DisplayOrientation::FLIPPED) {
        serial_println_P(F("CAUTION: Inverted display does not support last digit output."));
        serial_println_P(F("Output will be one order of magnitude smaller than real value, and truncated."));
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