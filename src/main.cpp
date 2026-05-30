#include <Arduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#include "serial.h"
#include "char_helper.h"
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
      if (print) serial_write(c);
    } else {
      EEPROM.update(i, ' '); // fill rest with blanks
    }
  }
  if (print) serial_ln();
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

static uint8_t soft_serial_read_string_until(char delimiter, char* buffer, uint8_t max_len) {
    uint8_t count = 0;
    char c;

    if (max_len == 0) return 0;
    
    while (count < (max_len - 1)) {
        if (softSerial.available() > 0) {
            c = softSerial.read();
            
            if (c == delimiter) break;
            buffer[count++] = c;
        }
    }
    buffer[count] = '\0';
    
    return count;
}


// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();
  
  init_timer2_for_1000hz();

  serial_init(HARDWARE_SERIAL_BAUD);
  softSerial.begin(SOFTWARE_SERIAL_BAUD);
  analogReference(EXTERNAL);
  updateVersion();

  displayController.showInfo();
  displayController.showAvailableCommands();
  serial_print_P(F("A secondary serial interface is enabled and attached on: RX:"));
  serial_print_u8(SOFT_RX);
  serial_print_P(F(" TX:"));
  serial_print_u8(SOFT_TX);
  serial_println_P(F(" (self)."));
  serial_print_P(F("Baud rate: "));
  serial_print_u32(SOFTWARE_SERIAL_BAUD);
  serial_ln();

  #ifdef SECONDARY_INPUT_INTRO_TEXT
  softSerial.println(F("Secondary Serial Interface - INPUT (NUMBERS) ONLY. "));
  softSerial.println(F("Refer to Main Serial Interface for Verbose Output. "));
  #endif

  
  for (;;) {
    if (serial_available() > 0) {
      char input[RX_BUFFER_SIZE] = {0};
      serial_read_string_until('\n', input, sizeof(input));
      trim(input);
      displayController.processInput(input);
    }

    
    if (softSerial.available() > 0) {
      // technically, the max that is implemented by softwareserial can be larger than this
      // but no command will go past rx_buffer_size anyway as enforced by main serial
      char input[RX_BUFFER_SIZE] = {0}; 
      soft_serial_read_string_until('\n', input, sizeof(input));
      trim(input);
      displayController.processSecondaryInput(input);
    }

    displayController.updateDisplay();
  }

  return 0;
}


