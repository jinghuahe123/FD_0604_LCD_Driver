#include "DisplaySettingsManager.hpp"

#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include "EEPROM.h"
#include "serial.h"
#include "char_helper.h"
#include "DisplayUtils.hpp"


const DisplaySettingsManager::SettingsHandler DisplaySettingsManager::_settingsHandlers[] PROGMEM= {
    nullptr,  // index 0 unused (menu starts at 1)
    &DisplaySettingsManager::_exitSettings,
    &DisplaySettingsManager::_updateCycleInterval,
    &DisplaySettingsManager::_updateTemperatureInterval,
    &DisplaySettingsManager::_updateTemperatureSerialOutput,
    &DisplaySettingsManager::_updateRawInputInterval,
    &DisplaySettingsManager::_updateRawInputSerialOutput,
    &DisplaySettingsManager::_updateDisplayOrientation,
    &DisplaySettingsManager::_updateHistoryRecallDepth
};

const uint8_t DisplaySettingsManager::_maxSettingsOptions = 
    sizeof(_settingsHandlers) / sizeof(_settingsHandlers[0]) - 1;

const char DisplaySettingsManager::_pinAlias[][3] PROGMEM = {
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7"
};


DisplaySettingsManager::DisplaySettingsManager(DisplayDriver_FD0604& display, const DisplayParameters& params)
    : _display(display), _params(params) 
{
    strlcpy_P(_temperaturePinAlias, _pinAlias[_params.tempSensor.PIN_temperaturePin], sizeof(_temperaturePinAlias));
    strlcpy_P(_rawInputPinAlias, _pinAlias[_params.rawInput.PIN_rawInputPin], sizeof(_rawInputPinAlias));
}

void DisplaySettingsManager::showSettingsMenu() {
    // implementation of the settings menu display and handling
    serial_println_P(F("============================== FD-0604 LED Display Settings ============================="));
    serial_println_P(F("Select one of the options below by typing a number."));
    serial_ln();
    serial_println_P(F("[1] Exit this menu."));
    serial_println_P(F("[2] Set Cycle Interval Time."));
    serial_println_P(F("[3] Set Temperature Refresh Interval Time."));
    serial_println_P(F("[4] Enable / Disable Temperature Serial Output."));
    serial_println_P(F("[5] Set RAW Input Refresh Interval Time."));
    serial_println_P(F("[6] Enable / Disable RAW Input Serial Output."));
    serial_println_P(F("[7] Flip Display Orientation."));
    serial_println_P(F("[8] Set History Recall Depth."));
    serial_println_P(F("========================================================================================="));

    bool optionSelected = false;
    uint16_t option = 0; 

    while (!optionSelected) {
        if (serial_available() > 0) {
            char input[MAX_INPUT_SIZE] = {0};
            serial_read_string_until('\n', input, MAX_INPUT_SIZE);
            trim(input);

            if (input[0] == '\0') {
                serial_println_P(F("Please select an option."));
                continue;
            } else if (!checkIfNumericUnsigned(input, option)) {
                serial_println_P(F("Invalid Option. Please enter a number."));
                continue;
            } else if (option < 1 || option > _maxSettingsOptions) {
                serial_println_P(F("Invalid Number selected."));
                continue;
            }

            optionSelected = true;
        }
        wdt_reset();
    }

    if (option >= 1 && option <= _maxSettingsOptions) {
        SettingsHandler handler;
        memcpy_P(&handler, &_settingsHandlers[option], sizeof(handler));
        (this->*handler)();
    }
}

void DisplaySettingsManager::displaySettingsInfo() {
    uint16_t countingInterval = readCycleInterval();
    uint16_t temperatureUpdateInterval = readTemperatureInterval();
    uint16_t rawInputUpdateInterval = readRawInputInterval();
    uint16_t numHistory = readHistoryDepth();


    serial_println_P(F("=========================== FD-0604 LED Display SETTINGS INFO ==========================="));
    
    // == Basic Configs ==
    serial_print_P(F("Display Orientation:                            ")); serial_println_P(readDisplayOrientation() ? F("Inverted Display") : F("Normal Display"));
    serial_print_P(F("History recall depth:                           ")); serial_print_u16(numHistory); serial_ln();
    serial_print_P(F("Cycle Function Interval Time:                   ")); serial_print_u16(countingInterval); serial_println_P(F("ms"));
    serial_ln();

    // == Temp sensor ==
    serial_print_P(F("Temperature Pin:                                ")); serial_println(_temperaturePinAlias);
    serial_print_P(F("Temperature Refresh Interval:                   ")); serial_print_u16(temperatureUpdateInterval); serial_println_P(F("ms"));
    serial_print_P(F("Temperature Sensor Auxiliary Resistor Value:    ")); serial_print_float(_params.tempSensor.resistorValue, 2); serial_println_P(F("ohm"));
    serial_print_P(F("Temperature Serial Output:                      ")); serial_println_P(readTemperatureSerialEnabled() ? F("Enabled") : F("Disabled"));
    serial_ln();

    // == RAW Input ==
    serial_print_P(F("RAW Input Pin:                                  ")); serial_println(_rawInputPinAlias);
    serial_print_P(F("RAW Input Refresh Interval:                     ")); serial_print_u16(rawInputUpdateInterval); serial_println_P(F("ms"));
    serial_print_P(F("RAW Input Serial Output:                        ")); serial_println_P(readRawInputSerialEnabled() ? F("Enabled") : F("Disabled"));
    serial_ln();

    // == EEPROM ==
    serial_print_P(F("EEPROM Base Address:                            0x")); serial_print_hex16((unsigned)_params.BASE_ADDR); serial_ln(); 
    serial_print_P(F("EEPROM Wear Levelling Slots:                    ")); serial_print_u16(_params.NUM_SLOTS); serial_ln();

    serial_println_P(F("========================================================================================="));
    _delay_ms(3);
    serial_ln();

    if (numHistory == 0 || countingInterval == 0 || temperatureUpdateInterval == 0 || rawInputUpdateInterval == 0
        || numHistory == 65535 || countingInterval == 65535 || temperatureUpdateInterval == 65535 || rawInputUpdateInterval == 65535) {
        serial_println_P(F("CAUTION: Board may have been reset. Multiple settings are incorrect."));
        serial_println_P(F("Please run SETTINGS command to set the parameters. Thank you."));
        serial_ln();
    }
}

void DisplaySettingsManager::factoryReset() {
    serial_println_P(F("You have selected RESET. This will wipe all program storage data!"));
    serial_println_P(F("CAUTION: This action is irreversable!"));
    serial_println_P(F("Please Type 'RESET ALL' to confirm this action."));

    char input[MAX_INPUT_SIZE] = {0};
    bool hasInput = false;

    while (!hasInput) {
        if (serial_available() > 0) {
            serial_read_string_until('\n', input, MAX_INPUT_SIZE);
            trim(input);

            hasInput = true;
        }
        wdt_reset();
    }

    if (strcasecmp(input, "RESET ALL") == 0) {
        wdt_disable(); // disable watchdog timer to prevent reset during EEPROM write
        
        serial_println_P(F("RESET Command recieved. Resetting..."));
        for (uint16_t i=0; i<EEPROM.length(); i++) {
            EEPROM.write(i, 0x00);
        }
        for (uint16_t i=0; i<EEPROM.length(); i++) {
            EEPROM.write(i, 0xFF);
        }
        
        serial_println_P(F("RESET Complete. Rebooting..."));

        wdt_enable(WDTO_15MS);
        while (true) {}
    } else {
        serial_println_P(F("Input is incorrect. No data has been changed."));
    }
}

// ================================ EEPROM READ/WRITE API (HELPERS) ==================================

bool DisplaySettingsManager::readDisplayOrientation() {
    return EEPROM.read(_params.displayOrientationAddress);
}

void DisplaySettingsManager::writeDisplayOrientation(bool inverted) {
    EEPROM.update(_params.displayOrientationAddress, inverted);
}

uint16_t DisplaySettingsManager::readHistoryDepth() {
    uint16_t depth;
    EEPROM.get(_params.numHistoryAddress, depth);
    return depth;
}

void DisplaySettingsManager::writeHistoryDepth(uint16_t depth) {
    EEPROM.put(_params.numHistoryAddress, depth);
}

uint16_t DisplaySettingsManager::readCycleInterval() {
    uint16_t interval;
    EEPROM.get(_params.countingIntervalAddress, interval);
    return interval;
}

void DisplaySettingsManager::writeCycleInterval(uint16_t interval) {
    EEPROM.put(_params.countingIntervalAddress, interval);
}

uint16_t DisplaySettingsManager::readTemperatureInterval() {
    uint16_t interval;
    EEPROM.get(_params.tempSensor.temperatureUpdateIntervalAddress, interval);
    return interval;
}

void DisplaySettingsManager::writeTemperatureInterval(uint16_t interval) {
    EEPROM.put(_params.tempSensor.temperatureUpdateIntervalAddress, interval);
}

bool DisplaySettingsManager::readTemperatureSerialEnabled() {
    return EEPROM.read(_params.tempSensor.temperatureSerialEnabledAddress);
}

void DisplaySettingsManager::writeTemperatureSerialEnabled(uint8_t enabled) {
    EEPROM.update(_params.tempSensor.temperatureSerialEnabledAddress, enabled);
}

uint16_t DisplaySettingsManager::readRawInputInterval() {
    uint16_t interval;
    EEPROM.get(_params.rawInput.rawInputUpdateIntervalAddress, interval);
    return interval;
}

void DisplaySettingsManager::writeRawInputInterval(uint16_t interval) {
    EEPROM.put(_params.rawInput.rawInputUpdateIntervalAddress, interval);
}

bool DisplaySettingsManager::readRawInputSerialEnabled() {
    return EEPROM.read(_params.rawInput.rawInputSerialEnabledAddress);
}

void DisplaySettingsManager::writeRawInputSerialEnabled(uint8_t enabled) {
    EEPROM.update(_params.rawInput.rawInputSerialEnabledAddress, enabled);
}



// ================================== INTERNAL API ==================================

uint16_t DisplaySettingsManager::_getSerialInput() {
    bool intervalSet = false;
    char input[MAX_INPUT_SIZE] = {0};
    uint16_t value = 0;

    while (!intervalSet) {
        if (serial_available() > 0) {
            serial_read_string_until('\n', input, MAX_INPUT_SIZE);
            trim(input);
            serial_ln();

            if (input[0] == '\0') {
                serial_println_P(F("Please enter a number."));
                continue;
            } else if (!checkIfNumericUnsigned(input, value)) {
                serial_println_P(F("Invalid Option. Please enter a number."));
                continue;
            }

            intervalSet = true;
        }
        wdt_reset();
    }

    return value;
}

void DisplaySettingsManager::_exitSettings() {
    serial_println_P(F("Thank you. Exiting Settings..."));
    _delay_ms(100);
    displaySettingsInfo();
    //showAvailableCommands();
}

void DisplaySettingsManager::_updateCycleInterval() {
    uint16_t oldInterval = readCycleInterval();

    serial_print_P(F("Old Cycle Interval Time: ")); serial_print_u16(oldInterval); serial_ln();
    serial_print_P(F("Enter New Cycle Interval Time: ")); 

    uint16_t value = _getSerialInput();

    writeCycleInterval(value);
    serial_print_P(F("New Cycle Interval Time Set To: ")); serial_print_u16(value); serial_ln();

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateTemperatureInterval() {
    uint16_t oldInterval = readTemperatureInterval();

    serial_print_P(F("Old Temperature Interval Time: ")); serial_print_u16(oldInterval); serial_ln();
    serial_print_P(F("Enter New Temperature Interval Time: ")); 

    uint16_t value = _getSerialInput();

    writeTemperatureInterval(value);
    serial_print_P(F("New Temperature Interval Time Set To: ")); serial_print_u16(value); serial_ln();

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateTemperatureSerialOutput() {
    bool tempOutput = !readTemperatureSerialEnabled();
    writeTemperatureSerialEnabled(tempOutput);

    serial_print_P(F("Temperature Serial Output set to: "));
    serial_println_P((tempOutput) ? F("Enabled.") : F("Disabled."));

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateRawInputInterval() {
    uint16_t oldInterval = readRawInputInterval();

    serial_print_P(F("Old Raw Input Interval Time: ")); serial_print_u16(oldInterval); serial_ln();
    serial_print_P(F("Enter New Raw Input Interval Time: ")); 

    uint16_t value = _getSerialInput();

    writeRawInputInterval(value);
    serial_print_P(F("New Raw Input Interval Time Set To: ")); serial_print_u16(value); serial_ln();

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateRawInputSerialOutput() {
    bool rawSerialOutput = !readRawInputSerialEnabled();
    writeRawInputSerialEnabled(rawSerialOutput);

    serial_print_P(F("RAW Input Serial Output set to: "));
    serial_println_P((rawSerialOutput) ? F("Enabled.") : F("Disabled."));

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateDisplayOrientation() {
    _display.flipDisplayOrientation();

    bool orientation = _display.getDisplayOrientation(); 
    writeDisplayOrientation(orientation);

    serial_print_P(F("Display Orientation set to: "));
    serial_println_P((orientation) ? F("INVERTED.") : F("NORMAL."));

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateHistoryRecallDepth() {
    uint16_t numHistory;
    numHistory = readHistoryDepth();

    serial_print_P(F("Old History Recall Depth: ")); serial_print_u16(numHistory); serial_ln();
    serial_print_P(F("Enter New History Recall Depth: ")); 

    numHistory = _getSerialInput();

    if (numHistory > _params.NUM_SLOTS) {
        serial_print_P(F("Cannot set recall to more than max slots of "));
        serial_print_u16(_params.NUM_SLOTS);
        serial_ln();

        _delay_ms(20);
        _exitSettings();
        return;
    }

    writeHistoryDepth(numHistory);
    serial_print_P(F("New History Recall Depth set to: "));
    serial_print_u16(numHistory); serial_ln();

    _delay_ms(20);
    _exitSettings();
}

