#ifndef DISPLAY_MODE_MANAGER_HPP
#define DISPLAY_MODE_MANAGER_HPP

#include <stdint.h>

#include "DisplayParameters.hpp"
#include "DisplayDriver_FD0604.hpp"
#include "DisplaySettingsManager.hpp"

// special display states
#define MODE_OFF        -1 
#define MODE_CYCLE      -2 
#define MODE_NULL       -3
#define MODE_TEMP       -4 
#define MODE_RAWINPUT   -5

/**
 * @class DisplayModeManager
 * @brief Manages special display modes (OFF, CYCLE, NULL, TEMP, RAW INPUT)
 * 
 * This class handles all non-static display modes including:
 * - Temperature display with thermistor
 * - RAW analog input display
 * - Cycling through numbers
 * - OFF and NULL display states
 * 
 * Assumes: 
 * - DisplayDriver_FD0604 is already initialized and configured
 * - Temperature and Raw Innput pin are already set as analog inputs
 */
class DisplayModeManager {
public:
    DisplayModeManager(DisplayDriver_FD0604& display, DisplaySettingsManager& settingsManager,const DisplayParameters& params);

    /**
     * @brief Set the current display mode
     * @param mode Mode to set (MODE_OFF, MODE_CYCLE, etc.)
     */
    void setMode(int16_t mode);

    /**
     * @brief Get current display mode
     * @return Current mode value
     */
    int16_t getMode() const { return _currentMode; }

    /**
     * @brief Update display based on current mode
     * @note Should be called frequently in the main loop
     */
    void updateDisplay();

    /**
     * @brief Reset cycle counter to start from beginning
     */
    void resetCycleCounter() { _cycleNumber = 0; }

private:
    DisplayDriver_FD0604& _display;
    DisplaySettingsManager& _settingsManager;
    const DisplayParameters& _params;

    int16_t _currentMode = 0;
    uint16_t _cycleNumber = 0;
    unsigned long _previousMillis = 0;

    // display mode handler pointer function array
    using DisplayHandler = void(DisplayModeManager::*)();
    static const DisplayHandler displayHandlers[] PROGMEM; // static const array of function pointers to display handler functions
    static const uint8_t maxDisplayHandlers;

    // display mode handlers
    void _displayOff();
    void _displayCycle();
    void _displayNull();
    void _displayTemp();
    void _displayRAWInput();

    // disable copy constructor and assignment
    DisplayModeManager(const DisplayModeManager&) = delete;
    DisplayModeManager& operator=(const DisplayModeManager&) = delete;


};

#endif // DISPLAY_MODE_MANAGER_HPP 