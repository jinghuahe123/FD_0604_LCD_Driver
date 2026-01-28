#include <Arduino.h>

#include <DisplayDriver_FD0604.hpp>
#include <PersistentStorageManager.hpp>

#define USE_MINIMAL_WIRING

const uint8_t gnd[2] = {2, 3}; // first two pins of display in order of connection
const uint8_t pins[3] = {6, 7, 8}; // order of latchpin, clockpin, datapin

const int BASE_ADDR = 0; // EEPROM address to start writing writing from
const int SLOT_SIZE = 6; // uint32_t for sequence number (for wear levelling) + uint16_t for number
const int NUM_SLOTS = 170; // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

uint16_t number;

#ifdef USE_MINIMAL_WIRING
DisplayDriver_FD0604 display(pins, true);
#else
DisplayDriver_FD0604 display(gnd, pins, true);
#endif

PersistentStorageManager storageManager(BASE_ADDR, SLOT_SIZE, NUM_SLOTS);


bool checkIfNumeric(String string, int16_t &number) {
  if (string.length() == 0) return false;

  for (uint16_t i=0; i<string.length(); i++) {
    char c = string.charAt(i);
    if (c < '0' || c > '9') return false;
  }
  number = string.toInt();
  return true;
}


// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();

  Serial.begin(115200);
  //display.clear();

  number = storageManager.readData_uint16();

  for (;;) {
    if (Serial.available() > 0) {
      display.clear();

      String input = Serial.readStringUntil('\n');
      input.trim();
      
      int16_t tempNumber;
      if (!checkIfNumeric(input, tempNumber)  || tempNumber >= 4000 || tempNumber < 0) {
        Serial.print(F("Error parsing \'"));
        Serial.print(input);
        Serial.println(F("\'. Please make sure you have entered the correct format and is between 0 and 3999."));
        Serial.println();
      } else {
        number = tempNumber;
        storageManager.writeData_uint16(number);
        Serial.print(F("Now showing: "));
        Serial.println(number);
        Serial.println();
      }
      
    }
    display.writeArray(number, 1);
  }
  
  return 0;
}
