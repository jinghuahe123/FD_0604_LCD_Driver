#include <Arduino.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "DisplayController_FD0604.hpp"
#include "configs.hpp"

String input;

#ifdef USE_MINIMAL_WIRING
DisplayDriver_FD0604 display(pins, true);
#else
DisplayDriver_FD0604 display(gnd, pins, true);
#endif
PersistentStorageManager storageManager(BASE_ADDR, SLOT_SIZE, NUM_SLOTS);

DisplayController_FD0604 displayController(display, storageManager, displayParameters);


// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();

  //Serial.begin(115200);
  Serial.begin(1000000);
  analogReference(EXTERNAL);

  displayController._showAvailableCommands();

  for (;;) {
    if (Serial.available() > 0) {
      display.clear();

      input = Serial.readStringUntil('\n');
      input.trim();
      displayController.processInput(input);

    }
    
    displayController.updateDisplay();
    
  }

  return 0;
}


