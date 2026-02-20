/*
MIT License
Copyright (c) 2026 Jinghua He
See LICENSE file in the project root for full license text.
*/

#include "DisplayDriver_FD0604.hpp"
#include "Digit_Patterns.hpp"

/**
 * @param gnds        The GND pins to address.
 * @param vccs        The VCC pins to address.
 * @param npn         Toggle output polarity if the NPN-transistor is connected.
 */
DisplayDriver_FD0604::DisplayDriver_FD0604(const uint8_t* gnds, const uint8_t* pins, bool npn_toggle) : 
    gnd(gnds), latchPin(pins[0]), clockPin(pins[1]), dataPin(pins[2]), npn(npn_toggle), wiring_style(NORMAL_WIRING) {

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(gnd[0], OUTPUT);
  pinMode(gnd[1], OUTPUT);

  clear();
}

/**
 * @param pins        The array of pins to address.
 * @param npn         Toggle output polarity if the NPN-transistor is connected.
 */
DisplayDriver_FD0604::DisplayDriver_FD0604(const uint8_t* pins, bool npn_toggle) : 
    latchPin(pins[0]), clockPin(pins[1]), dataPin(pins[2]), npn(npn_toggle), wiring_style(MINIMAL_WIRING) {

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  clear();
}

/**
 * @details   Clears the display.
 */
void DisplayDriver_FD0604::clear() {
  switch (wiring_style) {
    case NORMAL_WIRING :
      for (int i=0; i<2; i++) {
        digitalWrite(gnd[i], HIGH);
      }
      break;
    
    case MINIMAL_WIRING :
      uint16_t pattern = (1 << 0) & (1 << 15);
      writeShiftRegister(pattern);
      break;
  }
}

void DisplayDriver_FD0604::getDisplayDigit(uint8_t digit, uint16_t (&output)[2]) {
  memcpy_P(output, &display[digit], sizeof(display[digit]));
}

/**
 * @details                 Parses each individual display number together.
 * @param number            The desired display number.
 * @param interval          The time the number should be displayed for.
 * @param leading_zeroes    Toggles whether the display will show leading zeroes.
 * @param clock             Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::writeArray(uint16_t number, unsigned long interval, bool leading_zeroes, bool clock) {
  uint16_t each_digit[4] = {0};
  uint16_t arr[5][2] = {0};
  uint16_t out[2] = {0};
  bool leading_digit = true;
  
  // Parse the number into the array
  for (int8_t i = 3; i >= 0; i--) {
    each_digit[i] = number % 10; // Extract the last digit
    number /= 10;         // Remove the last digit from the number
  }  

  // Addition for confining to digit patterns array layout
  //getDisplayDigit(each_digit[0]+30, arr[0]);
  //getDisplayDigit(each_digit[1]+20, arr[1]);
  //getDisplayDigit(each_digit[2]+10, arr[2]);
  //getDisplayDigit(each_digit[3], arr[3]);
  for (int8_t i=0; i<4; i++) {
    if (leading_digit && each_digit[i] != 0) {
      leading_digit = false;
    } 
    if (!leading_digit || leading_zeroes) {
      getDisplayDigit(each_digit[i] + 10*(3-i), arr[i]); // substitues the previous 4 commands into a loop
    }
  }
  if (clock) getDisplayDigit(34, arr[4]);

  for (int8_t i = 0; i < 2; i++) {
    // Use bitwise OR to combine the values from all four arrays
    out[i] = arr[0][i] | arr[1][i] | arr[2][i] | arr[3][i] | arr[4][i];
  }

  DisplayDriver_FD0604::writePins(interval, out);
}

/**
 * @details           Parses the null display.
 * @param interval    The time the number should be displayed for.
 * @param clock       Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::writeNull(unsigned long interval, bool clock) {
  

  uint16_t null_digits[2], clock_digits[2], out[2];
  
  getDisplayDigit(35, null_digits);
  if (clock) getDisplayDigit(34, clock_digits);

  for (int8_t i=0; i<2; i++) {
    out[i] = null_digits[i] | clock_digits[i];
  }

  DisplayDriver_FD0604::writePins(interval, out);
}

/**
 * @details             Multiplexes the display.
 * @param interval      The time the number should be displayed for.
 * @param displayPins   The pointer array of to-display LED's on/off pins. 
 */
void DisplayDriver_FD0604::writePins(unsigned long interval, uint16_t* displayPins) {
  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while (currentMillis - previousMillis <= interval) {
    if (interval == 0) currentMillis = 0;
    currentMillis = millis();

    //clear();

    switch (wiring_style) {
      case NORMAL_WIRING: {
        digitalWrite(gnd[0], npn ? HIGH : LOW); // Invert GND output if NPN is connected.
        digitalWrite(gnd[1], npn ? LOW : HIGH);
        writeShiftRegister(displayPins[0]);
        _delay_ms(MULTIPLEX_SPEED);
        
        digitalWrite(gnd[0], npn ? LOW : HIGH);
        digitalWrite(gnd[1], npn ? HIGH : LOW);
        writeShiftRegister(displayPins[1]);
        _delay_ms(MULTIPLEX_SPEED);

        break;
      }
      case MINIMAL_WIRING: {
        uint16_t mask = (1 << 0) | (1 << 15);
        uint16_t pattern = npn ? (1 << 0) : (1 << 15);

        displayPins[0] = (displayPins[0] & ~mask) | pattern;
        displayPins[1] = (displayPins[1] & ~mask) | (pattern ^ mask); // Invert pattern (gnd layout)

        writeShiftRegister(displayPins[0]);
        _delay_ms(MULTIPLEX_SPEED);
        writeShiftRegister(displayPins[1]);
        _delay_ms(MULTIPLEX_SPEED);

        break;
      }
      default:
        break;
    }
    
  }
}

/**
 * @details       Writes dual shift registers with display data
 * @param data    Entire two bytes of data for each ground pin. 
 */
void DisplayDriver_FD0604::writeShiftRegister(uint16_t data) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, (uint8_t)data);
  shiftOut(dataPin, clockPin, LSBFIRST, (uint8_t)(data >> 8));
  digitalWrite(latchPin, HIGH);
}
