#include <Arduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "DisplayController_FD0604.hpp"
#include "configs.hpp"

SoftwareSerial softSerial(SOFT_RX, SOFT_TX);
DisplayController_FD0604 displayController(displayParams, controllerParams);

static void updateVersion(bool print=0) {
  for (uint8_t i=0; i<FIRMWARE_VER_SIZE; i++) {
    if (i<sizeof(version)-1) { // ensure no null terminator is written
      char c = pgm_read_byte(&version[i]);
      EEPROM.update(i, c);
      if (print) Serial.print(c);
    } else {
      EEPROM.update(i, ' '); // fill rest with blanks
    }
  }
  if (print) Serial.println();
}

static void init_timer2_for_1000hz(void) {
  // clear timer2 control registers
  TCCR2A = 0x00; // normal mode, OC2 disconnected
  TCCR2B = 0x00;
  TCCR2A |= (1 << WGM21); // set CTC mode
  
  // Prescaler 128 
  TCCR2B |= (1 << CS22) | (1 << CS20);  // CS22=1, CS21=0, CS20=1 = /128
  OCR2A = 124;  // 16MHz/128 = 125kHz / 125 = 1000Hz
  
  TIMSK2 |= (1 << OCIE2A); // enable interrupts
  sei();
}

ISR(TIMER2_COMPA_vect) {
  DisplayDriver_FD0604::isr_mutliplex_display_callback(displayController.getDisplayDriverObject());
}

// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();

  init_timer2_for_1000hz();

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


