#include <DisplayDriver.h>
#include <Digit_Patterns.h>

/**
 * @param gnds        The GND pins to address.
 * @param vccs        The VCC pins to address.
 * @param npn         Toggle output polarity if the NPN-transistor is connected.
 */
DisplayDriver::DisplayDriver(const uint8_t* gnds, const uint8_t* pins, bool npn_toggle) {
  gnd = gnds;
  latchPin = pins[0];
  clockPin = pins[1];
  dataPin = pins[2];
  npn = npn_toggle;
  wiring_style = NORMAL_WIRING;

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(gnd[0], OUTPUT);
  pinMode(gnd[1], OUTPUT);

  clear();
}

DisplayDriver::DisplayDriver(const uint8_t* pins, bool npn_toggle) {
  latchPin = pins[0];
  clockPin = pins[1];
  dataPin = pins[2];
  npn = npn_toggle;
  wiring_style = MINIMAL_WIRING;

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  clear();
}

void DisplayDriver::getDisplayDigit(uint8_t digit, uint16_t (&output)[2]) {
  memcpy_P(output, &display[digit], sizeof(display[digit]));
}

/**
 * @details   Clears the display.
 */
void DisplayDriver::clear() {
  for (int i=0; i<2; i++) {
    digitalWrite(gnd[i], HIGH);
  }
}

/**
 * @details       Writes dual shift registers with display data
 * @param data    Entire two bytes of data for each ground pin. 
 */
void DisplayDriver::writeShiftRegister(uint16_t data) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, (uint8_t)data);
  shiftOut(dataPin, clockPin, LSBFIRST, (uint8_t)(data >> 8));
  digitalWrite(latchPin, HIGH);
}

/**
 * @details             Multiplexes the display.
 * @param interval      The time the number should be displayed for.
 * @param displayPins   The pointer array of to-display LED's on/off pins. 
 */
void DisplayDriver::writePins(unsigned long interval, uint16_t* displayPins) {
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
 * @details           Parses each individual display number together.
 * @param number      The desired display number.
 * @param interval    The time the number should be displayed for.
 * @param clock       Toggle the clock LEDs. 
 */
void DisplayDriver::writeArray(uint16_t number, unsigned long interval, bool clock) {
  uint16_t arr[4] = {0};
  uint16_t arr0[2], arr1[2], arr2[2], arr3[2], arr4[2], out[2] = {0};
  
  // Parse the number into the array
  for (int8_t i = 3; i >= 0; i--) {
    arr[i] = number % 10; // Extract the last digit
    number /= 10;         // Remove the last digit from the number
  }  

  // Addition for confining to digit patterns array layout
  getDisplayDigit(arr[0]+30, arr0);
  getDisplayDigit(arr[1]+20, arr1);
  getDisplayDigit(arr[2]+10, arr2);
  getDisplayDigit(arr[3], arr3);

  if (clock) getDisplayDigit(34, arr4);

  for (int8_t i = 0; i < 2; i++) {
    // Use bitwise OR to combine the values from all four arrays
    out[i] = arr0[i] | arr1[i] | arr2[i] | arr3[i] | arr4[i];
  }

  DisplayDriver::writePins(interval, out);
}

/**
 * @details           Parses the null display.
 * @param interval    The time the number should be displayed for.
 * @param clock       Toggle the clock LEDs. 
 */
void DisplayDriver::writeNull(unsigned long interval, bool clock) {
  

  uint16_t null_digits[2], clock_digits[2], out[2];
  
  getDisplayDigit(35, null_digits);
  if (clock) getDisplayDigit(34, clock_digits);

  for (int8_t i=0; i<2; i++) {
    out[i] = null_digits[i] | clock_digits[i];
  }

  DisplayDriver::writePins(interval, out);
}