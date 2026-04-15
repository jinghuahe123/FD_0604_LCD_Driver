#include <Arduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "DisplayController_FD0604.hpp"
#include "configs.hpp"

SoftwareSerial softSerial(SOFT_RX, SOFT_TX);
DisplayController_FD0604 displayController(minimalDisplayParams, controllerParams);

static void updateVersion() {
  for (uint8_t i=0; i<FIRMWARE_VER_SIZE; i++) {
    if (i<sizeof(version)-1) { // ensure no null terminator is written
      char c = pgm_read_byte(&version[i]);
      EEPROM.update(i, c);
    } else {
      EEPROM.update(i, ' '); // fill rest with blanks
    }
  }
}

// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();

  Serial.begin(HARDWARE_SERIAL_BAUD);
  softSerial.begin(SOFTWARE_SERIAL_BAUD);
  analogReference(EXTERNAL);
  updateVersion();

  displayController.showInfo();
  displayController.showAvailableCommands();
  Serial.print(F("A secondary serial interface is enabled and attached on: RX:"));
  Serial.print(SOFT_RX);
  Serial.print(F(" TX:"));
  Serial.print(SOFT_TX);
  Serial.println(F(" (self)."));
  Serial.println();

  #ifdef SECONDARY_INPUT_INTRO_TEXT
  softSerial.println(F("Secondary Serial Interface - INPUT (NUMBERS) ONLY. "));
  softSerial.println(F("Refer to Main Serial Interface for Verbose Output. "));
  #endif

  String input;

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


