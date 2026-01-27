#include <Arduino.h>
#include <DisplayDriver.h>

#define USE_MINIMAL_WIRING

const uint8_t gnd[2] = {2, 3}; // first two pins of display in order of connection
const uint8_t pins[3] = {6, 7, 8}; // order of latchpin, clockpin, datapin
uint16_t number;

#ifdef USE_MINIMAL_WIRING
DisplayDriver myDisplay(pins, true);
#else
DisplayDriver myDisplay(gnd, pins, true);
#endif

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
  //myDisplay.clear();

  for (;;) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      
      int16_t tempNumber;
      if (!checkIfNumeric(input, tempNumber)  || tempNumber >= 4000 || tempNumber < 0) {
        Serial.print(F("Error parsing \'"));
        Serial.print(input);
        Serial.println(F("\'. Please make sure you have entered the correct format and is between 0 and 3999."));
      } else {
        number = tempNumber;
        Serial.print(F("Now showing: "));
        Serial.println(number);
      }
      
    }
    myDisplay.writeArray(number, 1);
  }
  
  return 0;
}
