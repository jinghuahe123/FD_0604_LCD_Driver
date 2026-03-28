#include "DisplayController_FD0604.hpp"

//namespace std {
//    ohserialstream cout(Serial);
//}

const char DisplayController_FD0604::_commandList[][8] PROGMEM = { 
    "HELP", 
    "MEM", 
    "INIT", 
    "INVERT", 
    "ERASE",
    "HISTORY",
    "OFF", 
    "CYCLE", 
    "NULL", 
    "TEMP" 
};

const uint8_t DisplayController_FD0604::_commandListSize = 
    sizeof(DisplayController_FD0604::_commandList) / sizeof(DisplayController_FD0604::_commandList[0]);

/**
 * @details         Create a DisplayController_FD0604 object.
 * @param disp      DisplayDriver_FD0604 object to pass.
 * @param pStore    PersistentStorageManager object to pass.
 * @param params    Object parameters struct to pass.
 */
DisplayController_FD0604::DisplayController_FD0604(DisplayDriver_FD0604& disp, PersistentStorageManager& pStore, DisplayController_FD0604_Parameters& params) : 
        _display(disp),
        _storageManager(pStore),
        _params(params)
{
    init();
}

/**
 * @details         Get last display configuration from EEPROM. 
 */
void DisplayController_FD0604::init() {
    _number = _storageManager.readData_uint16();
    _display.setDisplayOrientation(EEPROM.read(_params.displayOrientationAddress));
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
            case 0: _handleHelp();    break;
            case 1: _handleMem();     break;
            case 2: _handleInit();    break;
            case 3: _handleInvert();  break;
            case 4: _handleErase();   break;
            case 5: _handleHistory(); break;
            case 6: _handleOff();     break;
            case 7: _handleCycle();   break;
            case 8: _handleNull();    break;
            case 9: _handleTemp();    break;
            default: break;
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
            default: break;
        }
    } else {
        _display.showNumber((_display.getDisplayOrientation() ? _number * 10 : _number), 1);
    }
}

/**
 * @details         Gets an individual command into memory from a PROGMEM store. 
 */
void DisplayController_FD0604::getCommandFromFlash(uint8_t index, char* buffer, size_t bufSize) {
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
        getCommandFromFlash(i, buffer, sizeof(buffer));
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
    case OFF:     return F("OFF");
    case CYCLE:   return F("CYCLE");
    case NULL_DISP: return F("NULL_DISP");
    case TEMP:    return F("TEMP");
    default:      return String(value);
  }
}

/**
 * @details         Displays the available display commands to Serial. 
 */
void DisplayController_FD0604::_showAvailableCommands() {
    Serial.println(F("================================= FD-0604 LED Display ================================="));
    Serial.println(F("Enter any number to display on the screen:                                             "));
    Serial.println(F("- 0000~3999 with normal orientation                                                    "));
    Serial.println(F("- 000~999 with inverted orientation                                                    "));
    //Serial.println(F("Letters are supported as a 4-digit sequence of A-F.                                    "));
    Serial.println();

    Serial.println(F("Alternative available commands:                                                        "));
    Serial.println(F("TEMP     -  Turns the display into a thermometer using thermosistor attached.          "));
    Serial.println(F("CYCLE    -  Cycles continuously 0~3999 / 0~999 with 100ms delay between numbers.       "));
    Serial.println(F("INIT     -  Flashes all possible digits and letters once.                              "));
    Serial.println(F("NULL     -  Shows --:-- on the display.                                                "));
    Serial.println(F("OFF      -  Turns off the display.                                                     "));
    Serial.println();

    Serial.println(F("Configuration commands:                                                                "));
    Serial.println(F("HELP     -  Shows this help page.                                                      "));
    Serial.println(F("INVERT   -  Flips the screen orientation.                                              "));
    Serial.println(F("MEM      -  Prints to Serial the available free memory on the MCU.                     "));
    Serial.println(F("ERASE    -  Erases previously displayed number history.                                "));
    Serial.println(F("HISTORY  -  Prints to Serial the last 10 numbers displayed.                            "));

    Serial.println(F("======================================================================================="));
    _delay_ms(3);
    Serial.println();
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
    _showAvailableCommands();
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
 * @details         Handles action if INVERT command is inputted. 
 */
void DisplayController_FD0604::_handleInvert() {
    _display.flipDisplayOrientation();

    bool orientation = _display.getDisplayOrientation(); 
    EEPROM.update(_params.displayOrientationAddress, orientation);

    Serial.print(F("Display Orientation set to: "));
    Serial.println((orientation) ? F("INVERTED. ") : F("NORMAL. "));
}

/**
 * @details         Erases the EEPROM addresses containing previous number data.
 */
void DisplayController_FD0604::_handleErase() {
    Serial.print(F("Erasing... "));
    _storageManager.clearData();
    Serial.println(F("Successfully erased previous history. "));
}

/**
 * @details         Handles displaying EEPROM history. 
 */
void DisplayController_FD0604::_handleHistory() {
    PersistentStorageManager::StorageEntry entries[_params.numHistory];
    //std::vector<PersistentStorageManager::StorageEntry> entries;

    Serial.println(F("=============================================================="));
    Serial.println(F("                     EEPROM STORAGE HISTORY                   "));
    Serial.println(F("=============================================================="));
    Serial.printf("Base Address: 0x%04x\n", (unsigned)_storageManager.getBaseAddr());
    //Serial.print(F("Base Address: 0x"));
    //Serial.println(_storageManager.getBaseAddr(), HEX);
    Serial.print(F("Total Slots: "));
    Serial.println(_storageManager.getNumSlots());
    Serial.print(F("Display Orientation: "));
    Serial.println((EEPROM.read(_params.displayOrientationAddress)) ? F("Inverted Display") : F("Normal Display"));
    _handleMem();
    Serial.println(F("--------------------------------------------------------------"));
    uint16_t uninitialised = _storageManager.getLastEntries(_params.numHistory, entries, _params.numHistory);

    if (uninitialised != 0xFFFFFFFF) {
        for(size_t i = 0; i < _params.numHistory; i++) {

            Serial.printf(
                "[%04u] Address: 0x%04x | Sequence: %010lu | Value: %s\n",
                (unsigned)(i + 1),
                (unsigned)entries[i].address,
                (unsigned long)entries[i].sequence,
                getValueDisplay(entries[i].value).c_str()
            );
        }
        Serial.println(F("--------------------------------------------------------------"));
        Serial.print(F("Total entries searched: "));
        Serial.println(_params.numHistory - uninitialised);
        Serial.print(F("Empty entries searched: "));
        Serial.println(uninitialised);
    } else {
        Serial.println(F("No data found in storage."));
    }

    Serial.println(F("=============================================================="));

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

    if (currentMillis - previousMillis > _params.countingInterval) {
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
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis > _params.temperatureUpdateInterval) {
        previousMillis = currentMillis;
        
        uint16_t temperatureReading = analogRead(_params.temperaturePin);

        double tempK = log(_params.resistorValue * (1024.0 / temperatureReading - 1));
        tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );
        float tempC = tempK - 273.15;

        if (_params.serial_enabled) {
            Serial.print(F("Temperature: ")); Serial.print(tempC); Serial.println(F("*C"));
        }

        displayTemp = tempC * 100; // 2 virtual decimal places
        
        //if (display.getDisplayOrientation() == FLIPPED_DISPLAY) {
        //  displayTemp = displayTemp / 10; // truncates to only 1 virtual decimal place
        //  displayTemp = displayTemp * 10;
        //}
    }

    if ((displayTemp < 4000 && _display.getDisplayOrientation() == NORMAL_DISPLAY)) {
        String output = String(displayTemp / 10) + 'o';

        _display.showDisplay(output, 1);
    } else if (_display.getDisplayOrientation() == FLIPPED_DISPLAY) {
        String output = String(displayTemp / 10) + 'o'; // use 'o' to activate temperature symbol

        _display.showDisplay(output, 1);
    } 
}
