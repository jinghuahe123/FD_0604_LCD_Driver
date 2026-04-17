#include "DisplayController_FD0604.hpp"

//namespace std {
//    ohserialstream cout(Serial);
//}

#if defined(__AVR__)
    #define STR_HELPER(x) #x
    #define STR(x) STR_HELPER(x)

    const char DisplayController_FD0604::processor[] PROGMEM = STR(__AVR_DEVICE_NAME__);
#else
    const char DisplayController_FD0604::processor[] PROGMEM = "Non-AVR / Unknown CPU";
#endif

const char DisplayController_FD0604::_commandList[][10] PROGMEM = { 
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

const uint8_t DisplayController_FD0604::_commandListSize = 
    sizeof(DisplayController_FD0604::_commandList) / sizeof(DisplayController_FD0604::_commandList[0]);

/**
 * @details                 Create a DisplayController_FD0604 object.
 * @param driverParams      DisplayDriver_FD0604 params to pass.
 * @param params            Controller-specific parameters struct to pass.
 */
DisplayController_FD0604::DisplayController_FD0604(DisplayDriver_FD0604::DriverParams& driverParams, DisplayController_FD0604_Parameters& params) : 
        _params(params), _display(driverParams), _storageManager(_params.BASE_ADDR, _params.SLOT_SIZE, _params.NUM_SLOTS),
        transistor_enabled_flag(driverParams.npn_transistor_enable), register_manipulation_flag(0), minimal_pin_flag(0) {

    _init();
}

/**
 * @details                 Create a DisplayController_FD0604 object.
 * @param driverParams      DisplayDriver_FD0604 params to pass.
 * @param params            Controller-specific parameters struct to pass.
 */
DisplayController_FD0604::DisplayController_FD0604(DisplayDriver_FD0604::DriverParams_MinimalWiring& driverParams, DisplayController_FD0604_Parameters& params) : 
        _params(params), _display(driverParams), _storageManager(_params.BASE_ADDR, _params.SLOT_SIZE, _params.NUM_SLOTS), 
        transistor_enabled_flag(driverParams.npn_transistor_enable), register_manipulation_flag(0), minimal_pin_flag(1) {

    _init();
}

/**
 * @details                 Create a DisplayController_FD0604 object.
 * @param driverParams      DisplayDriver_FD0604 params to pass.
 * @param params            Controller-specific parameters struct to pass.
 */
DisplayController_FD0604::DisplayController_FD0604(DisplayDriver_FD0604::DriverParams_DIRECTPORT& driverParams, DisplayController_FD0604_Parameters& params) : 
        _params(params), _display(driverParams), _storageManager(_params.BASE_ADDR, _params.SLOT_SIZE, _params.NUM_SLOTS), 
        transistor_enabled_flag(driverParams.npn_transistor_enable), register_manipulation_flag(1), minimal_pin_flag(0) {

    _init();
}

/**
 * @details                 Create a DisplayController_FD0604 object.
 * @param driverParams      DisplayDriver_FD0604 params to pass.
 * @param params            Controller-specific parameters struct to pass.
 */
DisplayController_FD0604::DisplayController_FD0604(DisplayDriver_FD0604::DriverParams_DIRECTPORT_MinimalWiring& driverParams, DisplayController_FD0604_Parameters& params) : 
        _params(params), _display(driverParams), _storageManager(_params.BASE_ADDR, _params.SLOT_SIZE, _params.NUM_SLOTS), 
        transistor_enabled_flag(driverParams.npn_transistor_enable), register_manipulation_flag(1), minimal_pin_flag(1) {

    _init();
}

/**
 * @details         Initialise the display paramaters & get last display configuration from EEPROM. 
 */
void DisplayController_FD0604::_init() {
    if (_params.temperaturePin != A6 && _params.temperaturePin != A7) pinMode(_params.temperaturePin, INPUT);
    if (_params.rawInputPin != A6 && _params.rawInputPin != A7) pinMode(_params.rawInputPin, INPUT);

    switch (_params.temperaturePin) {
        case A0: temperaturePinAlias = "A0"; break;
        case A1: temperaturePinAlias = "A1"; break;
        case A2: temperaturePinAlias = "A2"; break;
        case A3: temperaturePinAlias = "A3"; break;
        case A4: temperaturePinAlias = "A4"; break;
        case A5: temperaturePinAlias = "A5"; break;
        case A6: temperaturePinAlias = "A6"; break;
        case A7: temperaturePinAlias = "A7"; break;
        default: break;
    }

    switch (_params.rawInputPin) {
        case A0: rawInputPinAlias = "A0"; break;
        case A1: rawInputPinAlias = "A1"; break;
        case A2: rawInputPinAlias = "A2"; break;
        case A3: rawInputPinAlias = "A3"; break;
        case A4: rawInputPinAlias = "A4"; break;
        case A5: rawInputPinAlias = "A5"; break;
        case A6: rawInputPinAlias = "A6"; break;
        case A7: rawInputPinAlias = "A7"; break;
        default: break;
    }

    _number = _storageManager.readData_uint16();
    _display.setDisplayOrientation(EEPROM.read(_params.displayOrientationAddress));
}

/**
 * @details         Prints a line bar consisting of '=' characters. Used for saving flash space on AVR. 
 * @param number    Number of '=' characters to print.
 */
void DisplayController_FD0604::_printLineBar(uint8_t number) {
    while (number-- > 0) {
        Serial.print(F("="));
    }
    Serial.println();
}

/**
 * @details         Process a String input as display confiugration parameters.
 * @param input     String to pass as configuration parameters. 
 */
void DisplayController_FD0604::processInput(const String& input) {
    _input = input;
    String trimmed_input = input;
    trimmed_input.trim();

    if (trimmed_input.length() == 0) return;

    int8_t cmdIndex = _findCommandIndex(trimmed_input);

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
        if (!_parseAndSetNumber(trimmed_input)) { // also sets the number in the class variable
            Serial.print(F("Error parsing '"));
            Serial.print(_input);
            Serial.println(F("'. Please enter a valid command or number."));
        }
    }
}

void DisplayController_FD0604::processSecondaryInput(const String& input) {
    _input = input;
    String trimmed_input = input;
    trimmed_input.trim();

    if (trimmed_input.length() == 0) return;

    if (!_parseAndSetNumber(trimmed_input)) {
        Serial.print(F("Error parsing '"));
        Serial.print(_input);
        Serial.println(F("'. Secondary input accepts numbers only."));
    }
}

/**
 * @details         Show the processed input on the display. NOTE: Requires continuous polling otherwise will not work. 
 */
void DisplayController_FD0604::updateDisplay() {
    //Serial.println(_number);
    if (_number < 0) {
        //uint8_t cmd = _number - 4000;

        switch (_number) {
            case -1: _displayOff();      break;
            case -2: _displayCycle();    break;
            case -3: _displayNull();     break;
            case -4: _displayTemp();     break;
            case -5: _displayRAWInput(); break;
            default: break;
        }
    } else {
        _display.showNumber((_display.getDisplayOrientation() ? _number * 10 : _number), 1);
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
 * @details         Find the corresponding command from String input.
 * @param input     String input to pass for parsing. 
 * @return          Returns the index of the matching command. 
 */
int8_t DisplayController_FD0604::_findCommandIndex(const String& input) {
    char buffer[sizeof(_commandList[0])];
    for (int8_t i = 0; i < _commandListSize; i++) {
        _getCommandFromFlash(i, buffer, sizeof(buffer));
        if (input.equalsIgnoreCase(buffer)) {
            return i; // Return the index of the matching command
        }
    }
    return -1; // Not found
}

/**
 * @details         Helper function for parsing special display states.
 * @return          Return String of special display state. 
 */
String DisplayController_FD0604::getValueDisplay(uint16_t value) {
  switch(value) {
    case OFF:           return F("OFF");
    case CYCLE:         return F("CYCLE");
    case NULL_DISP:     return F("NULL_DISP");
    case TEMP:          return F("TEMP");
    case RAWINPUT:      return F("RAW");
    default:            return String(value);
  }
}

/**
 * @details         Displays the available display commands to Serial. 
 */
void DisplayController_FD0604::showAvailableCommands() {
    Serial.println(F("============================== FD-0604 LED Display Commands ============================="));
    Serial.println(F("Enter any number to display on the screen:                                             "));
    Serial.println(F("- 0000~3999 with normal orientation.                                                   "));
    Serial.println(F("- 000~999 with inverted orientation.                                                   "));
    //Serial.println(F("Letters are supported as a 4-digit sequence of A-F.                                    "));
    Serial.println();

    Serial.println(F("Alternative available commands:                                                        "));
    Serial.print(F("TEMP       -  Turns the display into a thermometer using thermosistor attached on pin ")); Serial.print(temperaturePinAlias); Serial.println(F(". "));
    Serial.print(F("RAW        -  Shows RAW input value on pin ")); Serial.print(rawInputPinAlias); Serial.println(F(". CAUTION: analogReference may be set! "));
    Serial.println(F("CYCLE      -  Cycles continuously 0~3999 / 0~999 with 100ms delay between numbers.       "));
    Serial.println(F("INIT       -  Flashes all possible digits and letters once.                              "));
    Serial.println(F("NULL       -  Shows --:-- on the display.                                                "));
    Serial.println(F("OFF        -  Turns off the display.                                                     "));
    Serial.println();

    Serial.println(F("Configuration commands:                                                                  "));
    Serial.println(F("HELP       -  Shows this help page.                                                      "));
    Serial.println(F("INFO       -  Shows the hardware information of the board.                               "));
    Serial.println(F("SETTINGS   -  Shows settings page and changes hardware configurations.                   "));
    Serial.println(F("MEM        -  Prints to Serial the available free memory on the MCU.                     "));
    Serial.println(F("ERASE      -  Erases previously displayed number history.                                "));
    Serial.println(F("RESET      -  Resets to factory defaults. CAUTION - WILL ERASE ALL USER DATA!            "));
    Serial.print(F("HISTORY    -  Prints to Serial the last ")); Serial.print(_params.numHistory); Serial.println(F(" numbers displayed. "));
    //    10 numbers displayed.                            "));

    //Serial.println(F("========================================================================================="));
    _printLineBar(90); // instead of saving copies of massive string of = chars, saves space on flash
    _delay_ms(3);
    Serial.println();
}

/**
 * @details         Displays a hardware report of configs to Serial. 
 */
void DisplayController_FD0604::showInfo() {
    int16_t countingInterval, temperatureUpdateInterval, rawInputUpdateInterval;
    bool serial_enabled_temperature, serial_enabled_raw_input;

    EEPROM.get(_params.countingIntervalAddress, countingInterval);
    EEPROM.get(_params.temperatureUpdateIntervalAddress, temperatureUpdateInterval);
    EEPROM.get(_params.temperatureSerialEnabledAddress, serial_enabled_temperature);
    EEPROM.get(_params.rawInputUpdateIntervalAddress, rawInputUpdateInterval);
    EEPROM.get(_params.rawInputSerialEnabledAddress, serial_enabled_raw_input);


    Serial.println(F("=========================== FD-0604 LED Display HARDWARE INFO ==========================="));
    
    // == Basic Configs ==
    Serial.print(F("CPU Model:                                      "));
        for (uint8_t i=0; ;i++) { char c = pgm_read_byte(&processor[i]); if(c==0)break; Serial.print(c); } Serial.println();
    Serial.print(F("CPU Clock Frequency:                            ")); Serial.print(F_CPU / 1000000); Serial.println(F("MHz"));
    Serial.print(F("Minimal Pin Configuration:                      ")); Serial.println((minimal_pin_flag) ? F("Enabled") : F("Disabled"));
    Serial.print(F("Fast Mulitplex:                                 ")); Serial.println((register_manipulation_flag) ? F("Enabled") : F("Disabled"));
    Serial.print(F("Transistor Driver Circuit:                      ")); Serial.println((transistor_enabled_flag) ? F("Enabled") : F("Disabled"));
    Serial.print(F("Display Orientation:                            ")); Serial.println((EEPROM.read(_params.displayOrientationAddress)) ? F("Inverted Display") : F("Normal Display"));
    Serial.print(F("Cycle Function Interval Time:                   ")); Serial.print(countingInterval); Serial.println(F("ms"));
    Serial.println();

    // == Temp sensor ==
    Serial.print(F("Temperature Pin:                                ")); Serial.println(temperaturePinAlias);
    Serial.print(F("Temperature Refresh Interval:                   ")); Serial.print(temperatureUpdateInterval); Serial.println(F("ms"));
    Serial.print(F("Temperature Sensor Auxiliary Resistor Value:    ")); Serial.print(_params.resistorValue); Serial.println(F("ohm"));
    Serial.print(F("Temperature Serial Output:                      ")); Serial.println((serial_enabled_temperature) ? F("Enabled") : F("Disabled"));
    Serial.println();

    // == RAW Input ==
    Serial.print(F("RAW Input Pin:                                  ")); Serial.println(rawInputPinAlias);
    Serial.print(F("RAW Input Refresh Interval:                     ")); Serial.print(rawInputUpdateInterval); Serial.println(F("ms"));
    Serial.print(F("RAW Input Serial Output:                        ")); Serial.println((serial_enabled_raw_input) ? F("Enabled") : F("Disabled"));
    Serial.println();

    // == EEPROM ==
    Serial.print(F("EEPROM Base Address:                            0x")); Serial.printf("%04x\n", (unsigned)_params.BASE_ADDR);
    Serial.print(F("EEPROM Wear Levelling Slots:                    ")); Serial.println(_params.NUM_SLOTS);

    //Serial.println(F("========================================================================================="));
    _printLineBar(90); // instead of saving copies of massive string of = chars, saves space on flash
    _delay_ms(3);
    Serial.println();

    if (countingInterval <= 0 || temperatureUpdateInterval <= 0 || rawInputUpdateInterval <= 0 
        || countingInterval == 32767 || temperatureUpdateInterval == 32767 || rawInputUpdateInterval == 32767) {
        Serial.println(F("CAUTION: Board may have been reset. Multiple settings are incorrect. "));
        Serial.println(F("Please run SETTINGS command to set the parameters. Thank you. "));
        Serial.println();
    }
}

/**
 * @details         Clears the display. 
 */
void DisplayController_FD0604::clear() {
    _display.clear();
}

/**
 * @details         Update EEPROM with new display data and print update information to Serial.
 */
void DisplayController_FD0604::_updateDisplay() {
    auto data = _storageManager.writeData_uint16(_number);
    Serial.println(F("====================="));
    //Serial.print(F("Wrote Data: ")); Serial.println(display.getDisplayOrientation() ? 
    //                                    ((number <=999 && number >=0) ? String(number) : input) : 
    //                                    ((number <= 3999 && number >=0) ? String(number) : input)
    //                                  ); // Serial.println((number == 4000) ? F("CYCLE") : String(number));
    Serial.print(F("Wrote Data: ")); Serial.println((_number > 0) ? String(_number) : _input);
    Serial.print(F("Written Slot: ")); Serial.println(data.writeSlot);
    Serial.print(F("EEPROM Address: 0x")); Serial.println(data.writeAddress, HEX);
    if (_number == RAWINPUT) Serial.println(F("CAUTION: analogReference may be set! "));
    Serial.println(F("====================="));
}

/**
 * @details         Show all the numbers and letters in sequence.
 * @param initTime  Time delay between each displayed value. 
 */
void DisplayController_FD0604::_displayInit(int8_t initTime) {
    _display.showNull(initTime + 35);
    _display.showNumber(1111, initTime, true, true);
    _display.showNumber(2222, initTime, true, true);
    _display.showNumber(3333, initTime, true, true);
    _display.showNumber(4444, initTime, true, true);
    _display.showNumber(5555, initTime, true, true);
    _display.showNumber(6666, initTime, true, true);
    _display.showNumber(7777, initTime, true, true);
    _display.showNumber(8888, initTime, true, true);
    _display.showNumber(9999, initTime, true, true);
    _display.showNumber(0000, initTime, true, true);
    _display.showLetter("AAAA", initTime, true);
    _display.showLetter("BBBB", initTime, true);
    _display.showLetter("CCCC", initTime, true);
    _display.showLetter("DDDD", initTime, true);
    _display.showLetter("EEEE", initTime, true);
    _display.showLetter("FFFF", initTime, true);
    _display.showNull(initTime + 35);
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
 * @details         Checks if a String is a numberic integer.
 * @param string    String to check.
 * @param number    Variable to write if is a valid number.
 * @return          Returns if the input string is a valid number. 
 */
bool DisplayController_FD0604::_checkIfNumeric(const String& string, int16_t &number) {
    if (string.length() == 0) return false;

    for (uint16_t i=0; i<string.length(); i++) {
        char c = string.charAt(i);
        if (c < '0' || c > '9') return false;
    }
    number = string.toInt();
    return true;
}

/**
 * @details         Check if input conforms to the display output limitations.
 * @param input     String input to check if is an integer and whether it conforms.
 * @return          Returns if the input conforms. 
 */
bool DisplayController_FD0604::_parseAndSetNumber(const String& input) {
    int16_t temporaryNumber = 0;
    if (!_checkIfNumeric(input, temporaryNumber) || temporaryNumber < 0) {
        return false;
    }
    if (!_display.getDisplayOrientation() && temporaryNumber > 3999) {
        return false;
    }
    if (_display.getDisplayOrientation() && temporaryNumber > 999) {
        return false;
    }

    _number = input.toInt();
    _updateDisplay();
    return true;
}






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

    Serial.print(F("MEMORY: "));
    Serial.print(freeMem);
    Serial.print(F(" of "));
    Serial.print(TOTAL_RAM);
    Serial.print(F(" bytes free. ("));
    Serial.print(percentFree);
    Serial.println(F("%)"));
}

/**
 * @details         Handles action if INIT command is inputted.
 */
void DisplayController_FD0604::_handleInit() {
    _displayInit();
}

/**
 * @details         Handles action if SETTINGS command is inputted. 
 */
void DisplayController_FD0604::_handleSettings() {
    Serial.println(F("============================== FD-0604 LED Display Settings ============================="));
    Serial.println(F("Select one of the options below by typing a number. "));
    Serial.println();
    Serial.println(F("[1] Exit this menu. "));
    Serial.println(F("[2] Set Cycle Interval Time. "));
    Serial.println(F("[3] Set Temperature Refresh Interval Time. "));
    Serial.println(F("[4] Enable / Disable Temperature Serial Output. "));
    Serial.println(F("[5] Set RAW Input Refresh Interval Time. "));
    Serial.println(F("[6] Enable / Disable RAW Input Serial Output. "));
    Serial.println(F("[7] Flip Display Orientation. "));
    //Serial.println(F("========================================================================================="));
    _printLineBar(90); // instead of saving copies of massive string of = chars, saves space on flash

    bool optionSelected = false;
    String input;
    int16_t option = 0; // int16_t required for _checkIfNumeric function

    while (!optionSelected) {
        if (Serial.available() > 0) {
            input = Serial.readStringUntil('\n');
            input.trim();

            if (input.length() == 0) {
                Serial.println(F("Please select an option. "));
            } else if (input.length() != 1) {
                Serial.println(F("Invalid Option. Please enter a single number. "));
                continue;
            } else if (!_checkIfNumeric(input, option)) {
                Serial.println(F("Invalid Option. Please enter a number. "));
                continue;
            } else if (option < 1 || option > maxSettingsOptions) {
                Serial.println(F("Invalid Number selected. "));
                continue;
            }

            optionSelected = true;
        }
    }

    switch (option) {
        case 1: _exitSettings();                    return; break;
        case 2: _updateCycleInterval();             return; break;
        case 3: _updateTemperatureInterval();       return; break;
        case 4: _updateTemperatureSerialOutput();   return; break;
        case 5: _updateRawInputInterval();          return; break;
        case 6: _updateRawInputSerialOutput();      return; break;
        case 7: _updateDisplayOrientation();        return; break;
        default: return; break;
    }
}

/**
 * @details         Erases the EEPROM addresses containing previous number data.
 */
void DisplayController_FD0604::_handleErase() {
    Serial.print(F("Erasing... "));
    _storageManager.clearData();
    Serial.println(F("Successfully erased previous history. "));
}

void DisplayController_FD0604::_handleReset() {
    Serial.println(F("You have selected RESET. This will wipe all program storage data!"));
    Serial.println(F("CAUTION: This action is irreversable! "));
    Serial.println(F("Please Type 'RESET ALL' to confirm this action. "));

    bool hasInput = false;
    String input;

    while (!hasInput) {
        if (Serial.available() > 0) {
            input = Serial.readStringUntil('\n');
            input.trim();

            hasInput = true;
        }
    }

    if (input == "RESET ALL") {
        Serial.println(F("RESET Command recieved. Resetting..."));
        for (uint16_t i=0; i<EEPROM.length(); i++) {
            EEPROM.write(i, 0x00);
        }
        for (uint16_t i=0; i<EEPROM.length(); i++) {
            EEPROM.write(i, 0xFF);
        }
        
        Serial.println(F("RESET Complete. Rebooting..."));
        Serial.flush();

        wdt_enable(WDTO_15MS);
        while (true) {}
    } else {
        Serial.println(F("Input is incorrect. No data has been changed. "));
    }
}


/**
 * @details         Handles displaying EEPROM history. 
 */
void DisplayController_FD0604::_handleHistory() {
    PersistentStorageManager::StorageEntry entries[_params.numHistory] = {0};
    //std::vector<PersistentStorageManager::StorageEntry> entries;

    //Serial.println(F("=============================================================="));
    _printLineBar(63); // instead of saving copies of massive string of = chars, saves space on flash
    Serial.println(F("                     EEPROM STORAGE HISTORY                   "));
    _printLineBar(63);
    //Serial.println(F("=============================================================="));
    Serial.print(F("Base Address: 0x")); Serial.printf("%04x\n", (unsigned)_params.BASE_ADDR);
    //Serial.print(F("Base Address: 0x"));
    //Serial.println(_storageManager.getBaseAddr(), HEX);
    Serial.print(F("Total Slots: "));
    Serial.println(_params.NUM_SLOTS);
    Serial.print(F("Display Orientation: "));
    Serial.println((EEPROM.read(_params.displayOrientationAddress)) ? F("Inverted Display") : F("Normal Display"));
    _handleMem();
    Serial.println(F("--------------------------------------------------------------"));
    uint16_t uninitialised = _storageManager.getLastEntries(_params.numHistory, entries, _params.numHistory);

    if (uninitialised != 0xFFFF) {
        for(size_t i = 0; i < _params.numHistory - uninitialised; i++) {

            Serial.print(F("["));
            Serial.printf("%04u", (unsigned)i);
            Serial.print(F("] Address: 0x"));
            Serial.printf("%04x", (unsigned)entries[i].address);
            Serial.print(F(" | Sequence: "));
            Serial.printf("%010lu", (unsigned long)entries[i].sequence);
            Serial.print(F(" | Value: "));
            Serial.printf("%s\n", getValueDisplay(entries[i].value).c_str());
            /*
            Serial.printf(
                "[%04u] Address: 0x%04x | Sequence: %010lu | Value: %s\n",
                (unsigned)(i),
                (unsigned)entries[i].address,
                (unsigned long)entries[i].sequence,
                getValueDisplay(entries[i].value).c_str()
            );*/
        }
        Serial.println(F("--------------------------------------------------------------"));
        Serial.print(F("Total entries searched: "));
        Serial.println(_params.numHistory - uninitialised);
        Serial.print(F("Empty entries searched: "));
        Serial.println(uninitialised);
    } else {
        Serial.println(F("No data found in storage."));
    }

    //Serial.println(F("=============================================================="));
    _printLineBar(63);

    //entries.clear();
}

/**
 * @details         Handles action if OFF command is inputted. 
 */
void DisplayController_FD0604::_handleOff() {
    _display.clear();
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
}




/**
 * @details         Shows the OFF display.
 */
void DisplayController_FD0604::_displayOff() {
    // do nothing as display is off
}
/**
 * @details         Shows the CYCLE display. 
 */
void DisplayController_FD0604::_displayCycle() {
    unsigned long currentMillis = millis();
    int16_t countingInterval;
    EEPROM.get(_params.countingIntervalAddress, countingInterval);

    if (currentMillis - previousMillis > static_cast<unsigned long>(countingInterval)) {
        previousMillis = currentMillis;
        _cycle_number = (_display.getDisplayOrientation() ? ((_cycle_number + 1) % 1000) : (_cycle_number + 1) % 4000);
    }

    if (_display.getDisplayOrientation() == FLIPPED_DISPLAY) {
        _display.showNumber(_cycle_number*10, 1);
    } else {
        _display.showNumber(_cycle_number, 1);
    } 
}

/**
 * @details         Shows the NULL display.
 */
void DisplayController_FD0604::_displayNull() {
    _display.showNull(1);
}

/** 
 * @details         Shows the TEMP display. 
 */
void DisplayController_FD0604::_displayTemp() {
    uint16_t displayTemp;
    int16_t temperatureUpdateInterval;
    char output[5] = {0};
    bool serial_enabled;
    EEPROM.get(_params.temperatureUpdateIntervalAddress, temperatureUpdateInterval);
    EEPROM.get(_params.temperatureSerialEnabledAddress, serial_enabled);

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis > static_cast<unsigned long>(temperatureUpdateInterval)) {
        previousMillis = currentMillis;
        
        uint16_t temperatureReading = analogRead(_params.temperaturePin);

        double tempK = log(_params.resistorValue * (1024.0 / temperatureReading - 1));
        tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );
        float tempC = tempK - 273.15;

        if (serial_enabled) {
            Serial.print(F("Temperature: ")); Serial.print(tempC); Serial.println(F("*C"));
        }

        displayTemp = tempC * 100; // 2 virtual decimal places
        displayTemp /= 10; // for a 3sf display output
    }

    snprintf(output, sizeof(output), "%u", displayTemp);
    output[3] = 'o'; // rewrites the \n terminator as a o

    if ((displayTemp < 4000 && _display.getDisplayOrientation() == NORMAL_DISPLAY)) {
        // String output = String(displayTemp / 10) + 'o';

        _display.showDisplay(output, 1);
    } else if (_display.getDisplayOrientation() == FLIPPED_DISPLAY) {
        // String output = String(displayTemp / 10) + 'o'; // use 'o' to activate temperature symbol

        _display.showDisplay(output, 1);
    } 
}

/**
 * @details         Shows the RAW Analog Input on Display. 
 */
void DisplayController_FD0604::_displayRAWInput() {
    uint16_t value;
    int16_t rawInputUpdateInterval;
    bool serial_enabled;
    EEPROM.get(_params.rawInputUpdateIntervalAddress, rawInputUpdateInterval);
    EEPROM.get(_params.rawInputSerialEnabledAddress, serial_enabled);

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis > static_cast<unsigned long>(rawInputUpdateInterval)) {
        previousMillis = currentMillis;

        value = analogRead(_params.rawInputPin);

        if (serial_enabled) {
            Serial.print(F("RAW Input Value: ")); Serial.printf("%04d\n", value);
        }
    }

    _display.showNumber(value, 1);
}



int16_t DisplayController_FD0604::_getSerial() {
    bool intervalSet = false;
    String input;
    int16_t value = 0;

    while (!intervalSet) {
        if (Serial.available() > 0) {
            input = Serial.readStringUntil('\n');
            input.trim();
            Serial.println();

            if (!_checkIfNumeric(input, value)) {
                Serial.println(F("Invalid Option. Please enter a number. "));
                continue;
            }

            intervalSet = true;
        }
    }

    return value;
}

void DisplayController_FD0604::_exitSettings() {
    Serial.println(F("Thank you. Exiting Settings..."));
    _delay_ms(100);
    showInfo();
    //showAvailableCommands();
}

void DisplayController_FD0604::_updateCycleInterval() {
    int16_t oldInterval = 0;
    EEPROM.get(_params.countingIntervalAddress, oldInterval);

    Serial.print(F("Old Cycle Interval Time: ")); Serial.println(oldInterval);
    Serial.print(F("Enter New Cycle Interval Time: ")); 

    int16_t value = _getSerial();

    EEPROM.put(_params.countingIntervalAddress, value);
    Serial.print(F("New Cycle Interval Time Set To: ")); Serial.println(value);

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateTemperatureInterval() {
    int16_t oldInterval = 0;
    EEPROM.get(_params.temperatureUpdateIntervalAddress, oldInterval);

    Serial.print(F("Old Temperature Interval Time: ")); Serial.println(oldInterval);
    Serial.print(F("Enter New Temperature Interval Time: ")); 

    int16_t value = _getSerial();

    EEPROM.put(_params.temperatureUpdateIntervalAddress, value);
    Serial.print(F("New Temperature Interval Time Set To: ")); Serial.println(value);

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateTemperatureSerialOutput() {
    bool tempOutput = !EEPROM.read(_params.temperatureSerialEnabledAddress);
    EEPROM.update(_params.temperatureSerialEnabledAddress, tempOutput);

    Serial.print(F("Temperature Serial Output set to: "));
    Serial.println((tempOutput) ? F("Enabled. ") : F("Disabled. "));

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateRawInputInterval() {
    int16_t oldInterval = 0;
    EEPROM.get(_params.rawInputUpdateIntervalAddress, oldInterval);

    Serial.print(F("Old Raw Input Interval Time: ")); Serial.println(oldInterval);
    Serial.print(F("Enter New Raw Input Interval Time: ")); 

    int16_t value = _getSerial();

    EEPROM.put(_params.rawInputUpdateIntervalAddress, value);
    Serial.print(F("New Raw Input Interval Time Set To: ")); Serial.println(value);

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateRawInputSerialOutput() {
    bool rawSerialOutput = !EEPROM.read(_params.rawInputSerialEnabledAddress);
    EEPROM.update(_params.rawInputSerialEnabledAddress, rawSerialOutput);

    Serial.print(F("RAW Input Serial Output set to: "));
    Serial.println((rawSerialOutput) ? F("Enabled. ") : F("Disabled. "));

    _delay_ms(20);
    _exitSettings();
}

void DisplayController_FD0604::_updateDisplayOrientation() {
    _display.flipDisplayOrientation();

    bool orientation = _display.getDisplayOrientation(); 
    EEPROM.update(_params.displayOrientationAddress, orientation);

    Serial.print(F("Display Orientation set to: "));
    Serial.println((orientation) ? F("INVERTED. ") : F("NORMAL. "));

    _delay_ms(20);
    _exitSettings();
}



