#ifndef DISPLAY_CONTROLLER_HPP
#define DISPLAY_CONTROLLER_HPP

#include "DisplayParameters.hpp"
#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "DisplayModeManager.hpp"
#include "DisplaySettingsManager.hpp"
#include "DisplayCommandProcessor.hpp"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/**
 * @class DisplayController_FD0604
 * @brief Main controller class that orchestrates all display components
 * 
 * This is the main entry point that coordinates:
 * - Display driver
 * - Mode management (special displays)
 * - Settings management
 * - Command processing
 * - Number storage
 */
class DisplayController {
public:
    DisplayController(const DisplayParameters& params);
    ~DisplayController() = default;

    // core display functions
    void multiplexDisplay();
    void clear();
    void startDisplayUpdateTask();
    void stopDisplayUpdateTask();

    // input processing
    void processInput(const char* input);
    void processSecondaryInput(const char* input);
    
    // information display
    void showAvailableCommands();
    void showInfo();

    // getters
    int16_t getNumber() const { return _number; }
    void setNumber(int16_t number);


private:
    const DisplayParameters& _params;
    
    // Component objects
    DisplayDriver_FD0604 _display;
    PersistentStorageManager<int16_t> _storageManager;
    DisplaySettingsManager _settingsManager;
    DisplayModeManager _modeManager;
    DisplayCommandProcessor _commandProcessor;

    // state
    int16_t _number = 0;
    char _input[MAX_INPUT_SIZE] = {0};
    bool _staticDisplayShown = false;
    const bool _transistorEnabled;

    StaticSemaphore_t xDisplayControllerSemaphoreBuffer;
    SemaphoreHandle_t xDisplayControllerSemaphore;

    // display update task
    static constexpr configSTACK_DEPTH_TYPE taskStackSize = 196; // stack size for display update task
    static constexpr UBaseType_t taskPriority = 3; // priority for display update task
    TaskHandle_t _displayUpdateTask;
    StaticTask_t _displayUpdateTaskBuffer;
    StackType_t _displayUpdateTaskStack[taskStackSize];
    static void startTaskCallback(void* pvParameters);
    void updateDisplay();
    

    // initialization
    void _init();
    void _initPins();

    // number handling
    // WARNING: ASSUMES NUTEX IS ALREADY HELD BY CALLER
    void _updateDisplayNumber();
    bool _parseAndSetNumber(const char* input);
    
    // delete copy
    DisplayController(const DisplayController&) = delete;
    DisplayController& operator=(const DisplayController&) = delete;

};


#endif // DISPLAY_CONTROLLER_HPP