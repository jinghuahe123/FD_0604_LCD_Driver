#include "DisplayController.hpp"

#include <stdlib.h>
#include "char_helper.h"
#include "software_serial.h"
#include "DisplayUtils.hpp"


DisplayController::DisplayController(const DisplayParameters& params)
    : _params(params),
      _display(params.driverParams),
      _storageManager(params.BASE_ADDR, params.NUM_SLOTS),
      _settingsManager(_display, params),
      _modeManager(_display, _settingsManager, params),
      _commandProcessor(_display, _settingsManager, _modeManager, params),
      _transistorEnabled(params.driverParams.npn_transistor_enable) 
{
    _init();
}

void DisplayController::_init() {
    _initPins();

    _number = _storageManager.read();
    _display.setDisplayOrientation(_settingsManager.readDisplayOrientation());

    if (_number < 0) _modeManager.setMode(_number);
    _staticDisplayShown = false;
}

void DisplayController::_initPins() {
    // only a basic check that is confirmed working for atmega328p, may not work for all chips
    if (_params.tempSensor.DDRx_temperaturePin != nullptr && _params.tempSensor.PORTx_temperaturePin != nullptr
            && _params.tempSensor.PIN_temperaturePin >= 0 && _params.tempSensor.PIN_temperaturePin <= 5) 
        {
        *(_params.tempSensor.DDRx_temperaturePin) &= ~(1 << _params.tempSensor.PIN_temperaturePin); // set to input
        *(_params.tempSensor.PORTx_temperaturePin) &= ~(1 << _params.tempSensor.PIN_temperaturePin); // disable pullup
    }

    if (_params.rawInput.DDRx_rawInputPin != nullptr && _params.rawInput.PORTx_rawInputPin != nullptr
            && _params.rawInput.PIN_rawInputPin >= 0 && _params.rawInput.PIN_rawInputPin <= 5) 
        {
        *(_params.rawInput.DDRx_rawInputPin) &= ~(1 << _params.rawInput.PIN_rawInputPin); // set to input
        *(_params.rawInput.PORTx_rawInputPin) &= ~(1 << _params.rawInput.PIN_rawInputPin); // disable pullup
    }
}

void DisplayController::multiplexDisplay() {
    _display.multiplexDisplay();
}

void DisplayController::updateDisplay() {
    if (_number < 0) {
        // let mode manager special mode handle
        _modeManager.updateDisplay();
    } else if (!_staticDisplayShown) {
        // show static number
        _display.showNumber(_display.getDisplayOrientation() ? _number * 10 : _number);
        _staticDisplayShown = true;
    }
}

void DisplayController::processInput(const char* input) {
    if (input == nullptr || strlen(input) == 0) {
        return;
    }

    strncpy(_input, input, MAX_INPUT_SIZE - 1);
    _input[MAX_INPUT_SIZE - 1] = '\0'; // Ensure null
    trim(_input); // remove quotes and trim whitespace

    if (_input[0] == '\0') {
        return; // empty input after trimming
    }

    // try to process as command 
    bool isConfigurationCommand = false;
    if (_commandProcessor.processCommand(_input, isConfigurationCommand)) {
        int16_t number = _modeManager.getMode();
        if (!isConfigurationCommand) {
            if (number != _number) {
                _number = number;
            }
            _updateDisplayNumber(); // update EEPROM and show number if not a configuration command
        }

        _staticDisplayShown = false; // reset static display flag to allow for new display update
        return; // command processed successfully
    }

    // try as number
    if (_parseAndSetNumber(_input)) {
        _staticDisplayShown = false; // reset static display flag to allow for new display update
        return; // number processed successfully
    }

    serial_print_P(F("Error parsing '"));
    serial_print(input);
    serial_println_P(F("'. Please enter a valid command or number."));
}

void DisplayController::processSecondaryInput(const char* input) {
    if (input == nullptr || strlen(input) == 0) {
        return;
    }

    strncpy(_input, input, MAX_INPUT_SIZE - 1);
    _input[MAX_INPUT_SIZE - 1] = '\0'; // Ensure null
    trim(_input); // remove quotes and trim whitespace

    if (_input[0] == '\0') {
        return; // empty input after trimming
    }

    // try to process as command 
    bool isConfigurationCommand = false; // this is redundant for secondary input, but we keep it for consistency
    if (_commandProcessor.processCommand(_input, isConfigurationCommand)) {
        _staticDisplayShown = false; // reset static display flag to allow for new display update
        return; // command processed successfully
    }

    if (!_parseAndSetNumber(_input)) {
        serial_print_P(F("Error parsing '"));
        serial_print(input);
        serial_println_P(F("'. Secondary input accepts numbers only."));
    }
}

void DisplayController::clear() {
    _display.clear();
}

void DisplayController::showAvailableCommands() {
    _commandProcessor.showHelp();
}

void DisplayController::showInfo() {
    _commandProcessor.showInfo();
}

void DisplayController::setNumber(int16_t number) {
    _number = number;
    _updateDisplayNumber();
}

void DisplayController::_updateDisplayNumber() {
    auto data = _storageManager.write(_number);
    serial_println_P(F("====================="));
    serial_print_P(F("Wrote Data: ")); if (_number > 0) {serial_print_i16(_number); serial_ln();} else {serial_println(_input);}
    serial_print_P(F("Written Slot: ")); serial_print_u16(data.writeSlot); serial_ln();
    serial_print_P(F("EEPROM Address: 0x")); serial_print_hex16(data.writeAddress); serial_ln();
    if (_number == MODE_RAWINPUT) serial_println_P(F("CAUTION: analogReference may be set!"));
    serial_println_P(F("====================="));
}

bool DisplayController::_parseAndSetNumber(const char* input) {
    uint16_t number = 0;
    if (!checkIfNumericUnsigned(input, number) || number < 0) {
        return false;
    }
    if (!_display.getDisplayOrientation() && number > 3999) {
        return false;
    }
    if (_display.getDisplayOrientation() && number > 999) {
        return false;
    }

    _number = number;
    _updateDisplayNumber();
    return true;
}
