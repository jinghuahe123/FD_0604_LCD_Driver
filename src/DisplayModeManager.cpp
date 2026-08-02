#include "DisplayModeManager.hpp"

#include "timer.h"
#include "serial.h"
#include "adc.h"


const DisplayModeManager::DisplayHandler DisplayModeManager::displayHandlers[] PROGMEM = {
    nullptr,
    &DisplayModeManager::_displayOff,
    &DisplayModeManager::_displayCycle,
    &DisplayModeManager::_displayNull,
    &DisplayModeManager::_displayTemp,
    &DisplayModeManager::_displayRAWInput,
};

const uint8_t DisplayModeManager::maxDisplayHandlers = 
    sizeof(DisplayModeManager::displayHandlers) / 
    sizeof(DisplayModeManager::displayHandlers[0]) - 1;

DisplayModeManager::DisplayModeManager(DisplayDriver_FD0604& display, DisplaySettingsManager& settingsManager, const DisplayParameters& params)
    : _display(display), _settingsManager(settingsManager), _params(params),
      _currentMode(0), _cycleNumber(0), _previousMillis(0) 
{

}

void DisplayModeManager::setMode(int16_t mode) {
    if (mode >= 0) {
        return; // invalid mode, must be negative for special modes
    }

    int8_t index = mode * -1;
    if (index > maxDisplayHandlers) {
        return; // invalid mode, out of range
    }

    _currentMode = mode;

    if (mode == MODE_CYCLE) {
        _cycleNumber = 0; // reset cycle number when entering cycle mode
    }

    _previousMillis = 0; // reset timer for mode updates
}

void DisplayModeManager::updateDisplay() {
    // already check in setMode for valid mode, however probably best practice to check again here
    if (_currentMode >= 0) {
        return; // invalid mode, must be negative for special modes
    }

    int8_t index = _currentMode * -1;
    if (index > maxDisplayHandlers) {
        return; // invalid mode, out of range
    }

    DisplayHandler handler;
    memcpy_P(&handler, &displayHandlers[index], sizeof(handler));
    (this->*handler)();
}



// ================================== INTERNAL API FOR SPECIAL DISPLAY MODES ==================================

void DisplayModeManager::_displayOff() {
    _display.clear();
}

void DisplayModeManager::_displayCycle() {
    unsigned long currentMillis = millis();
    uint16_t countingInterval = _settingsManager.readCycleInterval();

    if (currentMillis - _previousMillis > static_cast<unsigned long>(countingInterval)) {
        _previousMillis = currentMillis;
        _cycleNumber = (_display.getDisplayOrientation() ? ((_cycleNumber + 1) % 1000) : (_cycleNumber + 1) % 4000);

        if (_display.getDisplayOrientation() == FLIPPED_DISPLAY) {
            _display.showNumber(_cycleNumber*10);
        } else {
            _display.showNumber(_cycleNumber);
        } 
    }
}

void DisplayModeManager::_displayNull() {
    _display.showNull();
}

void DisplayModeManager::_displayTemp() {
    // TODO: handle negative temperatures?

    uint16_t temperatureUpdateInterval = _settingsManager.readTemperatureInterval();
    char output[5] = {0};
    bool serial_enabled = _settingsManager.readTemperatureSerialEnabled();

    unsigned long currentMillis = millis();
    if (currentMillis - _previousMillis > static_cast<unsigned long>(temperatureUpdateInterval)) {
        _previousMillis = currentMillis;
        
        uint16_t temperatureReading = analog_read(_params.tempSensor.PIN_temperaturePin);

        double tempK = log(_params.tempSensor.resistorValue * (1024.0 / temperatureReading - 1));
        tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );
        float tempC = tempK - 273.15;

        if (serial_enabled) {
            serial_print_P(F("Temperature: ")); serial_print_float(tempC, 2); serial_println_P(F("*C"));
        }

        uint16_t rawDisplayTemp = tempC * 100; // 2 virtual decimal places
        uint16_t displayTemp = rawDisplayTemp / 10; // for a 3sf display output

        // format into the display buffer
        for (int8_t i=2; i>=0; i--) {
            output[i] = (displayTemp % 10) + '0'; // convert to ascii
            displayTemp /= 10;
        }
        //snprintf(output, sizeof(output), "%u", displayTemp);
        output[3] = 'o'; // rewrites 4th digit as 'o'

        if ((rawDisplayTemp < 4000 && _display.getDisplayOrientation() == NORMAL_DISPLAY)) {
            _display.showDisplay(output);
        } else if (_display.getDisplayOrientation() == FLIPPED_DISPLAY) {
            _display.showDisplay(output);
        } 
    }
}

void DisplayModeManager::_displayRAWInput() {
    uint16_t value, rawInputUpdateInterval = _settingsManager.readRawInputInterval();
    bool serial_enabled = _settingsManager.readRawInputSerialEnabled();

    unsigned long currentMillis = millis();
    if (currentMillis - _previousMillis > static_cast<unsigned long>(rawInputUpdateInterval)) {
        _previousMillis = currentMillis;

        value = analog_read(_params.rawInput.PIN_rawInputPin);

        if (serial_enabled) {
            serial_print_P(F("RAW Input Value: ")); serial_print_u16(value); //Serial.printf("%04d\n", value);
        }

        _display.showNumber(value, 1);
    }   
}

