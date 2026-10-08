#include "DisplaySettingsManager.hpp"

#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include "EEPROM.h"
#include "drivers/HardwareSerial/HardwareSerial.hpp"
#include "drivers/core/char_helper.h"
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


DisplaySettingsManager::DisplaySettingsManager(DisplayDriver& display, const DisplayParameters& params)
    : _display(display), _params(params) 
{
    // TODO: this can be fragile if the pin numbers are not in the range of 0-7. 
    strlcpy_P(_temperaturePinAlias, _pinAlias[_params.tempSensor.temperaturePin.getPin() - 14], sizeof(_temperaturePinAlias));
    strlcpy_P(_rawInputPinAlias, _pinAlias[_params.rawInput.rawInputPin.getPin() - 14], sizeof(_rawInputPinAlias));
}

void DisplaySettingsManager::showSettingsMenu() {
    // implementation of the settings menu display and handling
    Serial.println(F("============================== FD-0604 LED Display Settings ============================="));
    Serial.println(F("Select one of the options below by typing a number."));
    Serial.println();
    Serial.println(F("[1] Exit this menu."));
    Serial.println(F("[2] Set Cycle Interval Time."));
    Serial.println(F("[3] Set Temperature Refresh Interval Time."));
    Serial.println(F("[4] Enable / Disable Temperature Serial Output."));
    Serial.println(F("[5] Set RAW Input Refresh Interval Time."));
    Serial.println(F("[6] Enable / Disable RAW Input Serial Output."));
    Serial.println(F("[7] Flip Display Orientation."));
    Serial.println(F("[8] Set History Recall Depth."));
    Serial.println(F("========================================================================================="));

    bool optionSelected = false;
    uint16_t option = 0; 

    while (!optionSelected) {
        if (Serial.available() > 0) {
            char input[MAX_INPUT_SIZE] = {0};
            Serial.readStringUntil('\n', input, MAX_INPUT_SIZE);
            trim(input);

            if (input[0] == '\0') {
                Serial.println(F("Please select an option."));
                continue;
            } else if (!checkIfNumericUnsigned(input, option)) {
                Serial.println(F("Invalid Option. Please enter a number."));
                continue;
            } else if (option < 1 || option > _maxSettingsOptions) {
                Serial.println(F("Invalid Number selected."));
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


    Serial.println(F("=========================== FD-0604 LED Display SETTINGS INFO ==========================="));
    
    // == Basic Configs ==
    Serial.print(F("Display Orientation:                            ")); Serial.println(readDisplayOrientation() == DisplayDriver::DisplayOrientation::FLIPPED ? F("Inverted Display") : F("Normal Display"));
    Serial.print(F("History recall depth:                           ")); Serial.print(numHistory); Serial.println();
    Serial.print(F("Cycle Function Interval Time:                   ")); Serial.print(countingInterval); Serial.println(F("ms"));
    Serial.println();

    // == Temp sensor ==
    Serial.print(F("Temperature Pin:                                ")); Serial.println(_temperaturePinAlias);
    Serial.print(F("Temperature Refresh Interval:                   ")); Serial.print(temperatureUpdateInterval); Serial.println(F("ms"));
    Serial.print(F("Temperature Sensor Auxiliary Resistor Value:    ")); Serial.print(_params.tempSensor.resistorValue, 2); Serial.println(F("ohm"));
    Serial.print(F("Temperature Serial Output:                      ")); Serial.println(readTemperatureSerialEnabled() ? F("Enabled") : F("Disabled"));
    Serial.println();

    // == RAW Input ==
    Serial.print(F("RAW Input Pin:                                  ")); Serial.println(_rawInputPinAlias);
    Serial.print(F("RAW Input Refresh Interval:                     ")); Serial.print(rawInputUpdateInterval); Serial.println(F("ms"));
    Serial.print(F("RAW Input Serial Output:                        ")); Serial.println(readRawInputSerialEnabled() ? F("Enabled") : F("Disabled"));
    Serial.println();

    // == EEPROM ==
    Serial.print(F("EEPROM Base Address:                            0x")); Serial.printHex((unsigned)_params.persistentStorage.BASE_ADDR); Serial.println(); 
    Serial.print(F("EEPROM Wear Levelling Slots:                    ")); Serial.print(_params.persistentStorage.NUM_SLOTS); Serial.println();

    Serial.println(F("========================================================================================="));
    _delay_ms(3);
    Serial.println();

    if (numHistory == 0 || countingInterval == 0 || temperatureUpdateInterval == 0 || rawInputUpdateInterval == 0
        || numHistory == UINT16_MAX || countingInterval == UINT16_MAX || temperatureUpdateInterval == UINT16_MAX || rawInputUpdateInterval == UINT16_MAX) {
        Serial.println(F("CAUTION: Board may have been reset. Multiple settings are incorrect."));
        Serial.println(F("Please run SETTINGS command to set the parameters. Thank you."));
        Serial.println();
    }
}

void DisplaySettingsManager::factoryReset() {
    Serial.println(F("You have selected RESET. This will wipe all program storage data!"));
    Serial.println(F("CAUTION: This action is irreversable!"));
    Serial.println(F("Please Type 'RESET ALL' to confirm this action."));

    char input[MAX_INPUT_SIZE] = {0};
    bool hasInput = false;

    while (!hasInput) {
        if (Serial.available() > 0) {
            Serial.readStringUntil('\n', input, MAX_INPUT_SIZE);
            trim(input);

            hasInput = true;
        }
        wdt_reset();
    }

    if (strcasecmp(input, "RESET ALL") == 0) {
        wdt_disable(); // disable watchdog timer to prevent reset during EEPROM write
        
        Serial.println(F("RESET Command recieved. Resetting..."));
        for (uint16_t i=0; i<EEPROM.length(); i++) {
            EEPROM.write(i, 0x00);
        }
        for (uint16_t i=0; i<EEPROM.length(); i++) {
            EEPROM.write(i, 0xFF);
        }
        
        Serial.println(F("RESET Complete. Rebooting..."));

        wdt_enable(WDTO_15MS);
        while (true) {}
    } else {
        Serial.println(F("Input is incorrect. No data has been changed."));
    }
}

// ================================ EEPROM READ/WRITE API (HELPERS) ==================================

DisplayDriver::DisplayOrientation DisplaySettingsManager::readDisplayOrientation() {
    return static_cast<DisplayDriver::DisplayOrientation>(EEPROM.read(_params.displayOrientationAddress));
}

void DisplaySettingsManager::writeDisplayOrientation(DisplayDriver::DisplayOrientation orientation) {
    EEPROM.update(_params.displayOrientationAddress, static_cast<uint8_t>(orientation));
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
        if (Serial.available() > 0) {
            Serial.readStringUntil('\n', input, MAX_INPUT_SIZE);
            trim(input);
            Serial.println();

            if (input[0] == '\0') {
                Serial.println(F("Please enter a number."));
                continue;
            } else if (!checkIfNumericUnsigned(input, value)) {
                Serial.println(F("Invalid Option. Please enter a number."));
                continue;
            }

            intervalSet = true;
        }
        wdt_reset();
    }

    return value;
}

void DisplaySettingsManager::_exitSettings() {
    Serial.println(F("Thank you. Exiting Settings..."));
    _delay_ms(100);
    displaySettingsInfo();
    //showAvailableCommands();
}

void DisplaySettingsManager::_updateCycleInterval() {
    uint16_t oldInterval = readCycleInterval();

    Serial.print(F("Old Cycle Interval Time: ")); Serial.print(oldInterval); Serial.println();
    Serial.print(F("Enter New Cycle Interval Time: ")); 

    uint16_t value = _getSerialInput();

    writeCycleInterval(value);
    Serial.print(F("New Cycle Interval Time Set To: ")); Serial.print(value); Serial.println();

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateTemperatureInterval() {
    uint16_t oldInterval = readTemperatureInterval();

    Serial.print(F("Old Temperature Interval Time: ")); Serial.print(oldInterval); Serial.println();
    Serial.print(F("Enter New Temperature Interval Time: ")); 

    uint16_t value = _getSerialInput();

    writeTemperatureInterval(value);
    Serial.print(F("New Temperature Interval Time Set To: ")); Serial.print(value); Serial.println();

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateTemperatureSerialOutput() {
    bool tempOutput = !readTemperatureSerialEnabled();
    writeTemperatureSerialEnabled(tempOutput);

    Serial.print(F("Temperature Serial Output set to: "));
    Serial.println((tempOutput) ? F("Enabled.") : F("Disabled."));

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateRawInputInterval() {
    uint16_t oldInterval = readRawInputInterval();

    Serial.print(F("Old Raw Input Interval Time: ")); Serial.print(oldInterval); Serial.println();
    Serial.print(F("Enter New Raw Input Interval Time: ")); 

    uint16_t value = _getSerialInput();

    writeRawInputInterval(value);
    Serial.print(F("New Raw Input Interval Time Set To: ")); Serial.print(value); Serial.println();

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateRawInputSerialOutput() {
    bool rawSerialOutput = !readRawInputSerialEnabled();
    writeRawInputSerialEnabled(rawSerialOutput);

    Serial.print(F("RAW Input Serial Output set to: "));
    Serial.println((rawSerialOutput) ? F("Enabled.") : F("Disabled."));

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateDisplayOrientation() {
    _display.flipDisplayOrientation();

    DisplayDriver::DisplayOrientation orientation = _display.getDisplayOrientation(); 
    writeDisplayOrientation(orientation);

    Serial.print(F("Display Orientation set to: "));
    Serial.println((orientation == DisplayDriver::DisplayOrientation::FLIPPED) ? F("INVERTED.") : F("NORMAL."));

    _delay_ms(20);
    _exitSettings();
}

void DisplaySettingsManager::_updateHistoryRecallDepth() {
    uint16_t numHistory;
    numHistory = readHistoryDepth();

    Serial.print(F("Old History Recall Depth: ")); Serial.print(numHistory); Serial.println();
    Serial.print(F("Enter New History Recall Depth: ")); 

    numHistory = _getSerialInput();

    if (numHistory > _params.persistentStorage.NUM_SLOTS) {
        Serial.print(F("Cannot set recall to more than max slots of "));
        Serial.print(_params.persistentStorage.NUM_SLOTS);
        Serial.println();

        _delay_ms(20);
        _exitSettings();
        return;
    }

    writeHistoryDepth(numHistory);
    Serial.print(F("New History Recall Depth set to: "));
    Serial.print(numHistory); Serial.println();

    _delay_ms(20);
    _exitSettings();
}

