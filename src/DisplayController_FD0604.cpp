#include "DisplayController_FD0604.hpp"
#include "timer.h"
#include "io_helper.h"
#include "char_helper.h"
#include <avr/wdt.h>
#include <stdlib.h>

#include <EEPROM.h>

#if defined(__AVR__)
    #define STR_HELPER(x) #x
    #define STR(x) STR_HELPER(x)

    const char DisplayController_FD0604::processor[] PROGMEM = STR(__AVR_DEVICE_NAME__);
#else
    const char DisplayController_FD0604::processor[] PROGMEM = "Non-AVR / Unknown CPU";
#endif

const char DisplayController_FD0604::_commandList[][MAX_INPUT_SIZE] PROGMEM = { 
    "HELP", 
    "INFO",
    "MEM", 
    "INIT", 
    "SETTINGS", 
    "ERASE",
    "RESET",
    "HISTORY",
    "OFF", 
    "CYCLE", 
    "NULL", 
    "TEMP",
    "RAW", 
};

const char DisplayController_FD0604::pinAlias[][3] PROGMEM = {
    "A0",
    "A1",
    "A2",
    "A3",
    "A4",
    "A5",
    "A6",
    "A7",
};

const uint8_t DisplayController_FD0604::_commandListSize = 
    sizeof(DisplayController_FD0604::_commandList) / sizeof(DisplayController_FD0604::_commandList[0]);


/**
 * @details                 Create a DisplayController_FD0604 object.
 * @param driverParams      DisplayDriver_FD0604 params to pass.
 * @param params            Controller-specific parameters struct to pass.
 */
DisplayController_FD0604::DisplayController_FD0604(const DisplayDriver_FD0604::DriverParameters& driverParams, const DisplayController_FD0604_Parameters& params) : 
        _params(params), _display(driverParams), _storageManager(_params.BASE_ADDR, _params.SLOT_SIZE, _params.NUM_SLOTS), 
        transistor_enabled_flag(driverParams.npn_transistor_enable) {

    _init();
}

DisplayDriver_FD0604* DisplayController_FD0604::getDisplayDriverObject() {
    return &_display;
}

/**
 * @details         Initialise the display paramaters & get last display configuration from EEPROM. 
 */
void DisplayController_FD0604::_init() {
    // WARNING - this may not work for chip other than ATMEGA328P
    if (_params.tempSensor.PIN_temperaturePin >= 0 && _params.tempSensor.PIN_temperaturePin <= 5) {
        *(_params.tempSensor.DDRx_temperaturePin) &= ~(1 << _params.tempSensor.PIN_temperaturePin); // set to input
        *(_params.tempSensor.PORTx_temperaturePin) &= ~(1 << _params.tempSensor.PIN_temperaturePin); // disable pullup
    }

    if (_params.rawInput.PIN_rawInputPin >= 0 && _params.rawInput.PIN_rawInputPin <= 5) {
        *(_params.rawInput.DDRx_rawInputPin) &= ~(1 << _params.rawInput.PIN_rawInputPin); // set to input
        *(_params.rawInput.PORTx_rawInputPin) &= ~(1 << _params.rawInput.PIN_rawInputPin); // disable pullup
    }

    strlcpy_P(temperaturePinAlias, pinAlias[_params.tempSensor.PIN_temperaturePin], sizeof(temperaturePinAlias));
    strlcpy_P(rawInputPinAlias, pinAlias[_params.rawInput.PIN_rawInputPin], sizeof(rawInputPinAlias));

    _number = _storageManager.readData_uint16();
    _display.setDisplayOrientation(EEPROM.read(_params.displayOrientationAddress));
}

/**
 * @details         Process a C-style string as display confiugration parameters.
 * @param input     Array to pass as configuration parameters. 
 */
void DisplayController_FD0604::processInput(const char* input) {
    if (input == nullptr) return;

    // copy string
    strncpy(_input, input, MAX_INPUT_SIZE-1);
    _input[MAX_INPUT_SIZE-1] = '\0'; // ensure null termination
    trim(_input);

    if (_input[0] == '\0') return;

    int8_t cmdIndex = _findCommandIndex(_input);

    // process as a switch statement and then pass to individual functions rather than as a continuous if statement
    if (cmdIndex != -1) {
        switch (cmdIndex) {
            case 0:     _handleHelp();      break;
            case 1:     _handleInfo();      break;
            case 2:     _handleMem();       break;
            case 3:     _handleInit();      break;
            case 4:     _handleSettings();  break;
            case 5:     _handleErase();     break;
            case 6:     _handleReset();     break;
            case 7:     _handleHistory();   break;
            case 8:     _handleOff();       break;
            case 9:     _handleCycle();     break;
            case 10:    _handleNull();      break;
            case 11:    _handleTemp();      break;
            case 12:    _handleRAWInput();  break;
            default:                        break;
        }
    } else {
        // Not a command, try to parse as a number
        if (!_parseAndSetNumber(_input)) { // also sets the number in the class variable
            serial_print_P(F("Error parsing '"));
            serial_print(input);
            serial_println_P(F("'. Please enter a valid command or number."));
        }
    }

    staticDisplayShown = false;
}

void DisplayController_FD0604::processSecondaryInput(const char* input) {
    if (input == nullptr) return;

    strncpy(_input, input, MAX_INPUT_SIZE-1);
    _input[MAX_INPUT_SIZE-1] = '\0'; // ensure null termination
    trim(_input);

    if (_input[0] == '\0') return;

    if (!_parseAndSetNumber(_input)) {
        serial_print_P(F("Error parsing '"));
        serial_print(input);
        serial_println_P(F("'. Secondary input accepts numbers only."));
    }
}

/**
 * @details         Show the processed input on the display. 
 * @note            Requires continuous polling otherwise will not work, except for the static number display. Probably should fix. 
 */
void DisplayController_FD0604::updateDisplay() {
    if (_number < 0) {
        switch (_number) {
            case -1: _displayOff();      break;
            case -2: _displayCycle();    break;
            case -3: _displayNull();     break;
            case -4: _displayTemp();     break;
            case -5: _displayRAWInput(); break;
            default: break;
        }
    } else if (!staticDisplayShown) {
        _display.showNumber(_display.getDisplayOrientation() ? _number * 10 : _number);
        staticDisplayShown = true;
    }
}

/**
 * @details         Gets an individual command into memory from a PROGMEM store. 
 */
void DisplayController_FD0604::_getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize) {
    if (index >= _commandListSize) {
        if (bufSize > 0) buffer[0] = '\0';
        return;
    }
    strlcpy_P(buffer, _commandList[index], bufSize);
}

/**
 * @details         Find the corresponding command from C-style string input.
 * @param input     C-style string input to pass for parsing. 
 * @return          Returns the index of the matching command. 
 */
int8_t DisplayController_FD0604::_findCommandIndex(const char* input) {
    char buffer[sizeof(_commandList[0])];
    for (int8_t i = 0; i < _commandListSize; i++) {
        _getCommandFromFlash(i, buffer, sizeof(buffer));
        if (strcasecmp(input, buffer) == 0) { // returns true if matching
            return i; // Return the index of the matching command
        }
    }
    return -1; // Not found
}

/**
 * @details         Displays the available display commands to serial. 
 */
void DisplayController_FD0604::showAvailableCommands() {
    uint16_t numHistory = 0;
    EEPROM.get(_params.numHistoryAddress, numHistory);

    serial_println_P(F("============================== FD-0604 LED Display Commands ============================="));
    serial_println_P(F("Enter any number to display on the screen:"));
    serial_println_P(F("- 0000~3999 with normal orientation."));
    serial_println_P(F("- 000~999 with inverted orientation."));
    serial_ln();

    serial_println_P(F("Alternative available commands:"));
    serial_print_P(F("TEMP       -  Turns the display into a thermometer using thermosistor attached on pin ")); serial_print(temperaturePinAlias); serial_println_P(F("."));
    serial_print_P(F("RAW        -  Shows RAW input value on pin ")); serial_print(rawInputPinAlias); serial_println_P(F(". CAUTION: analogReference may be set!"));
    serial_println_P(F("CYCLE      -  Cycles continuously 0~3999 / 0~999 with 100ms delay between numbers."));
    serial_println_P(F("INIT       -  Flashes all possible digits and letters once."));
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

    serial_println_P(F("========================================================================================="));
    _delay_ms(3);
    serial_ln();
}

/**
 * @details         Displays a hardware report of configs to serial. 
 */
void DisplayController_FD0604::showInfo() {
    uint16_t countingInterval, temperatureUpdateInterval, rawInputUpdateInterval, numHistory;
    bool serial_enabled_temperature, serial_enabled_raw_input;

    EEPROM.get(_params.countingIntervalAddress, countingInterval);
    EEPROM.get(_params.tempSensor.temperatureUpdateIntervalAddress, temperatureUpdateInterval);
    EEPROM.get(_params.tempSensor.temperatureSerialEnabledAddress, serial_enabled_temperature);
    EEPROM.get(_params.rawInput.rawInputUpdateIntervalAddress, rawInputUpdateInterval);
    EEPROM.get(_params.rawInput.rawInputSerialEnabledAddress, serial_enabled_raw_input);
    EEPROM.get(_params.numHistoryAddress, numHistory);


    serial_println_P(F("=========================== FD-0604 LED Display HARDWARE INFO ==========================="));
    
    // == Basic Configs ==
    serial_print_P(F("CPU Model:                                      "));
        for (uint8_t i=0; ;i++) { char c = pgm_read_byte(&processor[i]); if(c==0)break; serial_write(c); } serial_ln();
    serial_print_P(F("CPU Clock Frequency:                            ")); serial_print_u8(F_CPU / 1000000); serial_println_P(F("MHz"));
    serial_print_P(F("Transistor Driver Circuit:                      ")); serial_println_P((transistor_enabled_flag) ? F("Enabled") : F("Disabled"));
    serial_print_P(F("Display Orientation:                            ")); serial_println_P((EEPROM.read(_params.displayOrientationAddress)) ? F("Inverted Display") : F("Normal Display"));
    serial_print_P(F("History recall depth:                           ")); serial_print_u16(numHistory); serial_ln();
    serial_print_P(F("Cycle Function Interval Time:                   ")); serial_print_u16(countingInterval); serial_println_P(F("ms"));
    serial_ln();

    // == Temp sensor ==
    serial_print_P(F("Temperature Pin:                                ")); serial_println(temperaturePinAlias);
    serial_print_P(F("Temperature Refresh Interval:                   ")); serial_print_u16(temperatureUpdateInterval); serial_println_P(F("ms"));
    serial_print_P(F("Temperature Sensor Auxiliary Resistor Value:    ")); serial_print_float(_params.tempSensor.resistorValue, 2); serial_println_P(F("ohm"));
    serial_print_P(F("Temperature Serial Output:                      ")); serial_println_P((serial_enabled_temperature) ? F("Enabled") : F("Disabled"));
    serial_ln();

    // == RAW Input ==
    serial_print_P(F("RAW Input Pin:                                  ")); serial_println(rawInputPinAlias);
    serial_print_P(F("RAW Input Refresh Interval:                     ")); serial_print_u16(rawInputUpdateInterval); serial_println_P(F("ms"));
    serial_print_P(F("RAW Input Serial Output:                        ")); serial_println_P((serial_enabled_raw_input) ? F("Enabled") : F("Disabled"));
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

/**
 * @details         Clears the display. 
 */
void DisplayController_FD0604::clear() {
    _display.clear();
}

/**
 * @details         Update EEPROM with new display data and print update information to serial.
 */
void DisplayController_FD0604::_updateDisplay() {
    auto data = _storageManager.writeData_uint16(_number);
    serial_println_P(F("====================="));
    serial_print_P(F("Wrote Data: ")); if (_number > 0) {serial_print_i16(_number); serial_ln();} else {serial_println(_input);}
    serial_print_P(F("Written Slot: ")); serial_print_u16(data.writeSlot); serial_ln();
    serial_print_P(F("EEPROM Address: 0x")); serial_print_hex16(data.writeAddress); serial_ln();
    if (_number == RAWINPUT) serial_println_P(F("CAUTION: analogReference may be set!"));
    serial_println_P(F("====================="));
}

/**
 * @details         Show all the numbers and letters in sequence.
 * @param initTime  Time delay between each displayed value. 
 * @note            Uses blocking delays, could be implemented better. 
 */
void DisplayController_FD0604::_displayInit(int8_t initTime) {
    _display.showNull();                        busy_delay(initTime + 35);
    _display.showNumber(1111, true, true);      busy_delay(initTime);
    _display.showNumber(2222, true, true);      busy_delay(initTime);
    _display.showNumber(3333, true, true);      busy_delay(initTime);
    _display.showNumber(4444, true, true);      busy_delay(initTime);
    _display.showNumber(5555, true, true);      busy_delay(initTime);
    _display.showNumber(6666, true, true);      busy_delay(initTime);
    _display.showNumber(7777, true, true);      busy_delay(initTime);
    _display.showNumber(8888, true, true);      busy_delay(initTime);
    _display.showNumber(9999, true, true);      busy_delay(initTime);
    _display.showNumber(0000, true, true);      busy_delay(initTime);
    _display.showLetter("AAAA", true);          busy_delay(initTime);
    _display.showLetter("BBBB", true);          busy_delay(initTime);
    _display.showLetter("CCCC", true);          busy_delay(initTime);
    _display.showLetter("DDDD", true);          busy_delay(initTime);
    _display.showLetter("EEEE", true);          busy_delay(initTime);
    _display.showLetter("FFFF", true);          busy_delay(initTime);
    _display.showNull();                        busy_delay(initTime + 35);
    _display.clear();
}


/**
 * @details         Gets the number of bytes from the stack to the heap.
 * @return          Returns the number of bytes of memory from the stack to the heap. 
 */
int DisplayController_FD0604::_freeMemory() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

/**
 * @details         Checks if a C-style string is a numberic integer.
 * @param string    C-style string to check.
 * @param number    Variable to write if is a valid number.
 * @return          Returns if the input string is a valid number. 
 */
bool DisplayController_FD0604::_checkIfNumericSigned(const char* string, int16_t &number) {
    if (string == nullptr || string[0] == '\0') return false;

    for (uint8_t i=0; string[i] != '\0'; i++) {
        if (string[i] < '0' || string[i] > '9') return false;
    }
    number = atoi(string);
    return true;
}

/**
 * @details         Checks if a C-style string is a numberic integer.
 * @param string    C-style string to check.
 * @param number    Variable to write if is a valid number.
 * @return          Returns if the input string is a valid number. 
 */
bool DisplayController_FD0604::_checkIfNumericUnsigned(const char* string, uint16_t &number) {
    if (string == nullptr || string[0] == '\0') return false;

    for (uint8_t i=0; string[i] != '\0'; i++) {
        if (string[i] < '0' || string[i] > '9') return false;
    }

    unsigned long val = strtoul(string, nullptr, 10);
    if (val > 65535) return false;  // Check uint16_t bounds

    number = (uint16_t)val;
    return true;
}

/**
 * @details         Check if input conforms to the display output limitations.
 * @param input     C-style string input to check if is an integer and whether it conforms.
 * @return          Returns if the input conforms. 
 */
bool DisplayController_FD0604::_parseAndSetNumber(const char* input) {
    uint16_t number = 0;
    if (!_checkIfNumericUnsigned(input, number) || number < 0) {
        return false;
    }
    if (!_display.getDisplayOrientation() && number > 3999) {
        return false;
    }
    if (_display.getDisplayOrientation() && number > 999) {
        return false;
    }

    _number = number;
    _updateDisplay();
    return true;
}




/*
    ================================================================================================================
    Setting Dispayed Number code below. 
    ================================================================================================================
*/

/**
 * @details         Handles action if the HELP command is inputted. 
 */
void DisplayController_FD0604::_handleHelp() {
    showAvailableCommands();
}

void DisplayController_FD0604::_handleInfo() {
    showInfo();
}

/**
 * @details         Handles action if MEM command is inputted. 
 */
void DisplayController_FD0604::_handleMem() {
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
}

/**
 * @details         Handles action if INIT command is inputted.
 */
void DisplayController_FD0604::_handleInit() {
    _displayInit();
}


const DisplayController_FD0604::SettingsHandler DisplayController_FD0604::settingsHandlers[] = {
    nullptr,  // index 0 unused (menu starts at 1)
    &DisplayController_FD0604::_exitSettings,
    &DisplayController_FD0604::_updateCycleInterval,
    &DisplayController_FD0604::_updateTemperatureInterval,
    &DisplayController_FD0604::_updateTemperatureSerialOutput,
    &DisplayController_FD0604::_updateRawInputInterval,
    &DisplayController_FD0604::_updateRawInputSerialOutput,
    &DisplayController_FD0604::_updateDisplayOrientation,
    &DisplayController_FD0604::_updateHistoryRecallDepth
};

const uint8_t DisplayController_FD0604::maxSettingsOptions = 
        sizeof(DisplayController_FD0604::settingsHandlers) / 
        sizeof(DisplayController_FD0604::settingsHandlers[0]) - 1;

/**
 * @details         Handles action if SETTINGS command is inputted. 
 */
void DisplayController_FD0604::_handleSettings() {
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
            } else if (!_checkIfNumericUnsigned(input, option)) {
                serial_println_P(F("Invalid Option. Please enter a number."));
                continue;
            } else if (option < 1 || option > maxSettingsOptions) {
                serial_println_P(F("Invalid Number selected."));
                continue;
            }

            optionSelected = true;
        }
        wdt_reset();
    }

    if (option >= 1 && option <= maxSettingsOptions) {
        (this->*settingsHandlers[option])();
    }
}

/**
 * @details         Erases the EEPROM addresses containing previous number data.
 */
void DisplayController_FD0604::_handleErase() {
    serial_print_P(F("Erasing... "));
    _storageManager.clearData();
    serial_println_P(F("Successfully erased previous history."));
}

void DisplayController_FD0604::_handleReset() {
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


/**
 * @details         Handles displaying EEPROM history. 
 */
void DisplayController_FD0604::_handleHistory() {
    uint16_t numHistory = 0;
    EEPROM.get(_params.numHistoryAddress, numHistory);

    // check if free memory is enough to create the array for storing number history
    int freeMemory = _freeMemory();
    if (freeMemory < 0) freeMemory = 0;
    freeMemory = freeMemory * 0.8; // leave 20% buffer room 
    if ((unsigned int)freeMemory < numHistory * sizeof(PersistentStorageManager::StorageEntry) || numHistory > _params.NUM_SLOTS) {
        serial_print_P(F("MCU does not have enough free memory to display "));
        serial_print_u16(numHistory);
        serial_println_P(F(" number histories."));
    } else {
        PersistentStorageManager::StorageEntry entries[numHistory] = {0};

        auto print_padded_u32 = [](uint32_t value, uint8_t digits) {
            char buffer[11]; // max 10 digits + null terminator
            char* ptr = buffer + 10;
            *ptr = '\0';

            for (uint8_t i=0; i<digits; i++) {
                ptr--;
                *ptr = '0' + (value % 10);
                value /= 10;
            }

            serial_print(ptr);
        };

        serial_println_P(F("=============================================================="));
        serial_println_P(F("                     EEPROM STORAGE HISTORY"));
        serial_println_P(F("=============================================================="));
        serial_print_P(F("Base Address: 0x")); serial_print_hex16(_params.BASE_ADDR); serial_ln();
        serial_print_P(F("Total Slots: "));
        serial_print_u16(_params.NUM_SLOTS); serial_ln();
        serial_print_P(F("Display Orientation: "));
        serial_println_P((EEPROM.read(_params.displayOrientationAddress)) ? F("Inverted Display") : F("Normal Display"));
        _handleMem();
        serial_println_P(F("--------------------------------------------------------------"));
        uint16_t uninitialised = _storageManager.getLastEntries(numHistory, entries);

        if (uninitialised != 0xFFFF) {
            for(size_t i = 0; i < numHistory - uninitialised; i++) {
                serial_print_P(F("["));
                //Serial.printf("%04u", (unsigned)i);
                print_padded_u32(i, 4);
                serial_print_P(F("] Address: 0x"));
                serial_print_hex16(entries[i].address);
                serial_print_P(F(" | Sequence: "));
                print_padded_u32(entries[i].sequence, 10);
                //Serial.printf("%010lu", (unsigned long)entries[i].sequence);
                serial_print_P(F(" | Value: "));
                switch (entries[i].value) {
                    case OFF:           serial_print_P(F("OFF"));           break;
                    case CYCLE:         serial_print_P(F("CYCLE"));         break;
                    case NULL_DISP:     serial_print_P(F("NULL_DISP"));     break;
                    case TEMP:          serial_print_P(F("TEMP"));          break;      
                    case RAWINPUT:      serial_print_P(F("RAW"));           break;
                    default:            serial_print_i16(entries[i].value);   break;
                }
                serial_ln();
            }
            serial_println_P(F("--------------------------------------------------------------"));
            serial_print_P(F("Total entries searched: "));
            serial_print_u16(numHistory - uninitialised);
            serial_ln();
            serial_print_P(F("Empty entries searched: "));
            serial_print_u16(uninitialised);
            serial_ln();
        } else {
            serial_println_P(F("No data found in storage."));
        }

        serial_println_P(F("=============================================================="));
    }
}

/**
 * @details         Handles action if OFF command is inputted. 
 */
void DisplayController_FD0604::_handleOff() {  
    _number = OFF;
    _updateDisplay();

}

/**
 * @details         Handles action if CYCLE command is inputted. 
 */
void DisplayController_FD0604::_handleCycle() {
    _number = CYCLE;
    _cycle_number = 0;
    _updateDisplay();
}

/**
 * @details         Handles action if NULL command is inputted. 
 */
void DisplayController_FD0604::_handleNull() {
    _number = NULL_DISP;
    _updateDisplay();
}

/**
 * @details         Handles action if TEMP command is inputted. 
 */
void DisplayController_FD0604::_handleTemp() {
    _number = TEMP;
    _updateDisplay();
}

void DisplayController_FD0604::_handleRAWInput() {
    _number = RAWINPUT;
    _updateDisplay();
    if (_display.getDisplayOrientation() == INVERTED_DISPLAY) {
        serial_println_P(F("CAUTION: Inverted display does not support last digit output."));
        serial_println_P(F("Output will be one order of magnitude smaller than real value, and truncated."));
    }
}





/*
    ================================================================================================================
    Showing Display Number code below. 
    Should be polled often, fix for this should be implemented. 
    ================================================================================================================
*/

/**
 * @details         Shows the OFF display.
 */
void DisplayController_FD0604::_displayOff() {
    // do nothing as display is off
    _display.clear();
}

/**
 * @details         Shows the CYCLE display. 
 */
void DisplayController_FD0604::_displayCycle() {
    unsigned long currentMillis = millis();
    uint16_t countingInterval;
    EEPROM.get(_params.countingIntervalAddress, countingInterval);

    if (currentMillis - previousMillis > static_cast<unsigned long>(countingInterval)) {
        previousMillis = currentMillis;
        _cycle_number = (_display.getDisplayOrientation() ? ((_cycle_number + 1) % 1000) : (_cycle_number + 1) % 4000);

        if (_display.getDisplayOrientation() == FLIPPED_DISPLAY) {
            _display.showNumber(_cycle_number*10);
        } else {
            _display.showNumber(_cycle_number);
        } 
    }
}

/**
 * @details         Shows the NULL display.
 */
void DisplayController_FD0604::_displayNull() {
    _display.showNull();
}

/** 
 * @details         Shows the TEMP display. 
 */
void DisplayController_FD0604::_displayTemp() {
    uint16_t displayTemp, temperatureUpdateInterval;
    char output[5] = {0};
    bool serial_enabled;
    EEPROM.get(_params.tempSensor.temperatureUpdateIntervalAddress, temperatureUpdateInterval);
    EEPROM.get(_params.tempSensor.temperatureSerialEnabledAddress, serial_enabled);

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis > static_cast<unsigned long>(temperatureUpdateInterval)) {
        previousMillis = currentMillis;
        
        uint16_t temperatureReading = analog_read(_params.tempSensor.PIN_temperaturePin);

        double tempK = log(_params.tempSensor.resistorValue * (1024.0 / temperatureReading - 1));
        tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );
        float tempC = tempK - 273.15;

        if (serial_enabled) {
            serial_print_P(F("Temperature: ")); serial_print_float(tempC, 2); serial_println_P(F("*C"));
        }

        displayTemp = tempC * 100; // 2 virtual decimal places
        displayTemp /= 10; // for a 3sf display output

        // format into the display buffer
        for (int8_t i=2; i>=0; i--) {
            output[i] = (displayTemp % 10) + '0'; // convert to ascii
            displayTemp /= 10;
        }
        //snprintf(output, sizeof(output), "%u", displayTemp);
        output[3] = 'o'; // rewrites 4th digit as 'o'

        if ((displayTemp < 4000 && _display.getDisplayOrientation() == NORMAL_DISPLAY)) {
            _display.showDisplay(output);
        } else if (_display.getDisplayOrientation() == FLIPPED_DISPLAY) {
            _display.showDisplay(output);
        } 
    }
}

/**
 * @details         Shows the RAW Analog Input on Display. 
 */
void DisplayController_FD0604::_displayRAWInput() {
    uint16_t value, rawInputUpdateInterval;
    bool serial_enabled;
    EEPROM.get(_params.rawInput.rawInputUpdateIntervalAddress, rawInputUpdateInterval);
    EEPROM.get(_params.rawInput.rawInputSerialEnabledAddress, serial_enabled);

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis > static_cast<unsigned long>(rawInputUpdateInterval)) {
        previousMillis = currentMillis;

        value = analog_read(_params.rawInput.PIN_rawInputPin);

        if (serial_enabled) {
            serial_print_P(F("RAW Input Value: ")); serial_print_u16(value); //Serial.printf("%04d\n", value);
        }

        _display.showNumber(value, 1);
    }   
}






/*
    ================================================================================================================
    Settings menu code below. 
    ================================================================================================================
*/

uint16_t DisplayController_FD0604::_getSerial() { 
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
            } else if (!_checkIfNumericUnsigned(input, value)) {
                serial_println_P(F("Invalid Option. Please enter a number."));
                continue;
            }

            intervalSet = true;
        }
        wdt_reset();
    }

    return value;
}

void DisplayController_FD0604::_exitSettings() {
    serial_println_P(F("Thank you. Exiting Settings..."));
    _delay_ms(100);
    showInfo();
    //showAvailableCommands();
}

void DisplayController_FD0604::_updateCycleInterval() {
    uint16_t oldInterval = 0;
    EEPROM.get(_params.countingIntervalAddress, oldInterval);

    serial_print_P(F("Old Cycle Interval Time: ")); serial_print_u16(oldInterval); serial_ln();
    serial_print_P(F("Enter New Cycle Interval Time: ")); 

    uint16_t value = _getSerial();

    EEPROM.put(_params.countingIntervalAddress, value);
    serial_print_P(F("New Cycle Interval Time Set To: ")); serial_print_u16(value); serial_ln();

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateTemperatureInterval() {
    uint16_t oldInterval = 0;
    EEPROM.get(_params.tempSensor.temperatureUpdateIntervalAddress, oldInterval);

    serial_print_P(F("Old Temperature Interval Time: ")); serial_print_u16(oldInterval); serial_ln();
    serial_print_P(F("Enter New Temperature Interval Time: ")); 

    uint16_t value = _getSerial();

    EEPROM.put(_params.tempSensor.temperatureUpdateIntervalAddress, value);
    serial_print_P(F("New Temperature Interval Time Set To: ")); serial_print_u16(value); serial_ln();

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateTemperatureSerialOutput() {
    bool tempOutput = !EEPROM.read(_params.tempSensor.temperatureSerialEnabledAddress);
    EEPROM.update(_params.tempSensor.temperatureSerialEnabledAddress, tempOutput);

    serial_print_P(F("Temperature Serial Output set to: "));
    serial_println_P((tempOutput) ? F("Enabled.") : F("Disabled."));

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateRawInputInterval() {
    uint16_t oldInterval = 0;
    EEPROM.get(_params.rawInput.rawInputUpdateIntervalAddress, oldInterval);

    serial_print_P(F("Old Raw Input Interval Time: ")); serial_print_u16(oldInterval); serial_ln();
    serial_print_P(F("Enter New Raw Input Interval Time: ")); 

    uint16_t value = _getSerial();

    EEPROM.put(_params.rawInput.rawInputUpdateIntervalAddress, value);
    serial_print_P(F("New Raw Input Interval Time Set To: ")); serial_print_u16(value); serial_ln();

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateRawInputSerialOutput() {
    bool rawSerialOutput = !EEPROM.read(_params.rawInput.rawInputSerialEnabledAddress);
    EEPROM.update(_params.rawInput.rawInputSerialEnabledAddress, rawSerialOutput);

    serial_print_P(F("RAW Input Serial Output set to: "));
    serial_println_P((rawSerialOutput) ? F("Enabled.") : F("Disabled."));

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateDisplayOrientation() {
    _display.flipDisplayOrientation();

    bool orientation = _display.getDisplayOrientation(); 
    EEPROM.update(_params.displayOrientationAddress, orientation);

    serial_print_P(F("Display Orientation set to: "));
    serial_println_P((orientation) ? F("INVERTED.") : F("NORMAL."));

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateHistoryRecallDepth() {
    uint16_t numHistory;
    EEPROM.get(_params.numHistoryAddress, numHistory);

    serial_print_P(F("Old History Recall Depth: ")); serial_print_u16(numHistory); serial_ln();
    serial_print_P(F("Enter New History Recall Depth: ")); 

    numHistory = _getSerial();

    EEPROM.put(_params.numHistoryAddress, numHistory);
    serial_print_P(F("New History Recal Depth set to: "));
    serial_print_u16(numHistory); serial_ln();

    _delay_ms(20);
    _exitSettings();
}

