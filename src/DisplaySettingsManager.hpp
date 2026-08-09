#ifndef DISPLAY_SETTINGS_MANAGER_HPP
#define DISPLAY_SETTINGS_MANAGER_HPP

#include <stdint.h>

#include "DisplayParameters.hpp"
#include "DisplayDriver_FD0604.hpp"

#ifndef MAX_INPUT_SIZE
#define MAX_INPUT_SIZE  RX_BUFFER_SIZE
#endif

/**
 * @class DisplaySettingsManager
 * @brief Manages all settings for the display system
 * 
 * Handles:
 * - Settings menu interface
 * - EEPROM read/write operations
 * - Factory reset
 * - Settings validation
 */
class DisplaySettingsManager {
public:
    DisplaySettingsManager(DisplayDriver_FD0604& display, const DisplayParameters& params);

    /**
     * @brief Show and handle settings menu
     */
    void showSettingsMenu();
    
    /**
     * @brief Display current settings to serial
     */
    void displaySettingsInfo();
    
    /**
     * @brief Reset all settings to factory defaults (EEPROM erase)
     * @note This is irreversible!
     */
    void factoryReset();
    
    /**
     * @brief Read display orientation from EEPROM
     * @return The current display orientation
     */
    DisplayDriver_FD0604::DisplayOrientation readDisplayOrientation();
    
    /**
     * @brief Write display orientation to EEPROM
     * @param orientation The desired display orientation
     */
    void writeDisplayOrientation(DisplayDriver_FD0604::DisplayOrientation orientation);
    
    /**
     * @brief Read history depth from EEPROM
     * @return Number of history entries to recall
     */
    uint16_t readHistoryDepth();
    
    /**
     * @brief Write history depth to EEPROM
     * @param depth Number of history entries to recall
     */
    void writeHistoryDepth(uint16_t depth);
    
    /**
     * @brief Read cycle interval from EEPROM
     * @return Cycle interval in milliseconds
     */
    uint16_t readCycleInterval();
    
    /**
     * @brief Write cycle interval to EEPROM
     * @param interval Cycle interval in milliseconds
     */
    void writeCycleInterval(uint16_t interval);

    /**
     * @brief Read temperature update interval from EEPROM
     * @return Temperature update interval in milliseconds
     */
    uint16_t readTemperatureInterval();

    /**
     * @brief Write temperature update interval to EEPROM
     * @param interval Temperature update interval in milliseconds
     */
    void writeTemperatureInterval(uint16_t interval);

    /**
     * @brief Read temperature serial output enabled state from EEPROM
     * @return True if enabled, false otherwise
     */
    bool readTemperatureSerialEnabled();

    /**
     * @brief Write temperature serial output enabled state to EEPROM
     * @param enabled True to enable, false to disable
     */
    void writeTemperatureSerialEnabled(uint8_t enabled);

    /**
     * @brief Read raw input update interval from EEPROM
     * @return Raw input update interval in milliseconds
     */
    uint16_t readRawInputInterval();
    
    /**
     * @brief Write raw input update interval to EEPROM
     * @param interval Raw input update interval in milliseconds
     */
    void writeRawInputInterval(uint16_t interval);

    /**
     * @brief Read raw input serial output enabled state from EEPROM
     * @return True if enabled, false otherwise
     */
    bool readRawInputSerialEnabled();

    /**
     * @brief Write raw input serial output enabled state to EEPROM
     * @param enabled True to enable, false to disable
     */
    void writeRawInputSerialEnabled(uint8_t enabled);


    /**
     * @brief Get the alias for the temperature pin
     * @return Pointer to the temperature pin alias
     */
    const char* getTemperaturePinAlias() const { return _temperaturePinAlias; }

    /**
     * @brief Get the alias for the raw input pin
     * @return Pointer to the raw input pin alias
     */
    const char* getRawInputPinAlias() const { return _rawInputPinAlias; }


private:
    DisplayDriver_FD0604& _display;
    const DisplayParameters& _params;

    static const char _pinAlias[][3] PROGMEM;
    char _temperaturePinAlias[3] = {0};
    char _rawInputPinAlias[3] = {0};
    
    using SettingsHandler = void(DisplaySettingsManager::*)();
    static const SettingsHandler _settingsHandlers[] PROGMEM;
    static const uint8_t _maxSettingsOptions;
    
    // settings menu handlers
    void _exitSettings();
    void _updateCycleInterval();
    void _updateTemperatureInterval();
    void _updateTemperatureSerialOutput();
    void _updateRawInputInterval();
    void _updateRawInputSerialOutput();
    void _updateDisplayOrientation();
    void _updateHistoryRecallDepth();
    
    // helper functions
    uint16_t _getSerialInput();
    static bool _checkIfNumericUnsigned(const char* str, uint16_t& number);
    
    // disable copy constructor and assignment
    DisplaySettingsManager(const DisplaySettingsManager&) = delete;
    DisplaySettingsManager& operator=(const DisplaySettingsManager&) = delete;

};


#endif // DISPLAY_SETTINGS_MANAGER_HPP