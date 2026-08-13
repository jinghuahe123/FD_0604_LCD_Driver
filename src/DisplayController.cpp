#include "DisplayController.hpp"

#include <stdlib.h>
#include "char_helper.h"
#include "software_serial.h"
#include "DisplayUtils.hpp"
#include "StackHighWatermarkUsage.hpp"


DisplayController::DisplayController(const DisplayParameters& params)
    : _params(params),
      _display(params.driverParams),
      _storageManager(params.BASE_ADDR, params.NUM_SLOTS),
      _settingsManager(_display, params),
      _modeManager(_display, _settingsManager, params),
      _commandProcessor(_display, _settingsManager, _modeManager, params),
      _transistorEnabled(params.driverParams.npn_transistor_enable)
{
    xDisplayControllerSemaphore = xSemaphoreCreateMutexStatic(&xDisplayControllerSemaphoreBuffer);

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

void DisplayController::startDisplayUpdateTask() {
    if (_displayUpdateTask == nullptr) {
        _displayUpdateTask = xTaskCreateStatic(
            startTaskCallback,
            "DisplayUpdate",
            taskStackSize,
            this,
            taskPriority,
            _displayUpdateTaskStack,
            &_displayUpdateTaskBuffer
        );
    }
    StackHighWatermarkUsage& stackUsage = StackHighWatermarkUsage::getInstance();
    stackUsage.addTaskHandle(_displayUpdateTask);
}

void DisplayController::stopDisplayUpdateTask() {
    if (_displayUpdateTask != nullptr) {
        vTaskDelete(_displayUpdateTask);
        vTaskDelay(pdMS_TO_TICKS(100)); // give time for the task to clean up
        _displayUpdateTask = nullptr;

    }
}

void DisplayController::startTaskCallback(void* pvParameters) {
    DisplayController* controller = static_cast<DisplayController*>(pvParameters);
    if (controller) {
        controller->updateDisplay();
    }
}

void DisplayController::updateDisplay() {
    for (;;) {
        if (xSemaphoreTake(xDisplayControllerSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (_number < 0) {
                // let mode manager special mode handle
                _modeManager.updateDisplay();
            } else if (!_staticDisplayShown) {
                // show static number
                _display.showNumber(_display.getDisplayOrientation() == DisplayDriver_FD0604::DisplayOrientation::FLIPPED ? _number * 10 : _number);
                _staticDisplayShown = true;
            }
            xSemaphoreGive(xDisplayControllerSemaphore);    
        }
        vTaskDelay(pdMS_TO_TICKS(5)); // small delay to prevent task hogging CPU
    }
}

void DisplayController::processInput(const char* input) {
    if (input == nullptr || strlen(input) == 0) {
        return;
    }

    // Take mutex for the entire operation
    if (xSemaphoreTake(xDisplayControllerSemaphore, pdMS_TO_TICKS(100)) != pdTRUE) {
        serial_println_P(F("ERROR: Display mutex timeout in processInput"));
        return;
    }

    strncpy(_input, input, MAX_INPUT_SIZE - 1);
    _input[MAX_INPUT_SIZE - 1] = '\0';
    trim(_input);

    if (_input[0] == '\0') {
        xSemaphoreGive(xDisplayControllerSemaphore);
        return;
    }

    bool isConfigurationCommand = false;
    if (_commandProcessor.processCommand(_input, isConfigurationCommand)) {
        int16_t number = _modeManager.getMode();
        if (!isConfigurationCommand) {
            if (number != _number) {
                _number = number;
            }
            _updateDisplayNumber(); // safe - mutex is held
        }
        _staticDisplayShown = false;
        xSemaphoreGive(xDisplayControllerSemaphore);
        return;
    }

    if (_parseAndSetNumber(_input)) {
        _staticDisplayShown = false;
        xSemaphoreGive(xDisplayControllerSemaphore);
        return;
    }

    xSemaphoreGive(xDisplayControllerSemaphore);
    
    // error message outside mutex
    serial_print_P(F("Error parsing '"));
    serial_print(input);
    serial_println_P(F("'. Please enter a valid command or number."));
}

void DisplayController::processSecondaryInput(const char* input) {
    if (input == nullptr || strlen(input) == 0) {
        return;
    }

    if (xSemaphoreTake(xDisplayControllerSemaphore, pdMS_TO_TICKS(100)) != pdTRUE) {
        serial_println_P(F("ERROR: Display mutex timeout in processSecondaryInput"));
        return;
    }

    strncpy(_input, input, MAX_INPUT_SIZE - 1);
    _input[MAX_INPUT_SIZE - 1] = '\0';
    trim(_input);

    if (_input[0] == '\0') {
        xSemaphoreGive(xDisplayControllerSemaphore);
        return;
    }

    bool isConfigurationCommand = false;
    if (_commandProcessor.processCommand(_input, isConfigurationCommand)) {
        _staticDisplayShown = false;
        xSemaphoreGive(xDisplayControllerSemaphore);
        return;
    }

    if (!_parseAndSetNumber(_input)) {
        xSemaphoreGive(xDisplayControllerSemaphore);
        serial_print_P(F("Error parsing '"));
        serial_print(input);
        serial_println_P(F("'. Secondary input accepts numbers only."));
        return;
    }

    _staticDisplayShown = false;
    xSemaphoreGive(xDisplayControllerSemaphore);
}

void DisplayController::clear() {
    if (xSemaphoreTake(xDisplayControllerSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        _display.clear();
        xSemaphoreGive(xDisplayControllerSemaphore);
    }
}

void DisplayController::showAvailableCommands() {
    _commandProcessor.showHelp();
}

void DisplayController::showInfo() {
    _commandProcessor.showInfo();
}

void DisplayController::setNumber(int16_t number) {
    if (xSemaphoreTake(xDisplayControllerSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        _number = number;
        _updateDisplayNumber();
        xSemaphoreGive(xDisplayControllerSemaphore);
    }
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
    if (_display.getDisplayOrientation() == DisplayDriver_FD0604::DisplayOrientation::FLIPPED && number > 3999) {
        return false;
    }
    if (_display.getDisplayOrientation() == DisplayDriver_FD0604::DisplayOrientation::NORMAL && number > 999) {
        return false;
    }

    _number = number;
    _updateDisplayNumber();
    return true;
}
