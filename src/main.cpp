#include <Arduino.h>
#include <SoftwareSerial.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "DisplayController_FD0604.hpp"
#include "configs.hpp"

String input;

SoftwareSerial softSerial(softRX, softTX);
DisplayController_FD0604 displayController(minimalDisplayParams, controllerParams);


// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();

  Serial.begin(hardwareSerialBaud);
  softSerial.begin(softwareSerialBaud);
  analogReference(EXTERNAL);

  displayController.showInfo();
  displayController.showAvailableCommands();
  Serial.print(F("A secondary serial interface is enabled and attached on: RX:"));
  Serial.print(softRX);
  Serial.print(F(" TX:"));
  Serial.print(softTX);
  Serial.println(F(" (self)."));
  Serial.println();

  softSerial.println(F("Secondary Serial Interface - INPUT (NUMBERS) ONLY. "));
  softSerial.println(F("Refer to Main Serial Interface for Verbose Output. "));

  for (;;) {
    if (Serial.available() > 0) {
      displayController.clear();

      input = Serial.readStringUntil('\n');
      input.trim();
      displayController.processInput(input);
    }

    if (softSerial.available() > 0) {
      input = softSerial.readStringUntil('\n');
      input.trim();
      displayController.processSecondaryInput(input);
    }

    displayController.updateDisplay();
  }

  return 0;
}


