#include <Arduino.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "DisplayController_FD0604.hpp"
#include "configs.hpp"

String input;

DisplayController_FD0604 displayController(minimalDisplayParams, controllerParams);


// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();

  //Serial.begin(115200);
  Serial.begin(1000000);
  analogReference(EXTERNAL);

  displayController.showInfo();
  displayController.showAvailableCommands();

  for (;;) {
    if (Serial.available() > 0) {
      displayController.clear();

      input = Serial.readStringUntil('\n');
      input.trim();
      displayController.processInput(input);

    }
    
    displayController.updateDisplay();
    
  }

  return 0;
}


