#include <DisplayDriver.h>
#include <Digit_Patterns.h>

DisplayDriver::DisplayDriver(const int* gnds, const int* vccs, bool npn_toggle) {
  /**
   * @param gnds        The GND pins to address.
   * @param vccs        The VCC pins to address.
   * @param npn         Toggle output polarity if the NPN-transistor is connected.
   */

  gnd = gnds;
  vcc = vccs;
  npn = npn_toggle;
}

void DisplayDriver::getDisplayDigit(int digit, int16_t (&output)[2]) {
  memcpy_P(output, &display[digit], sizeof(display[digit]));
}

void DisplayDriver::clear() {
  for (int i=0; i<2; i++) {
    pinMode(gnd[i], OUTPUT);
    digitalWrite(gnd[i], HIGH);
  }

  for (int i=0; i<15; i++) {
    pinMode(vcc[i], OUTPUT);
    digitalWrite(vcc[i], LOW);
  }
}

void DisplayDriver::writePins(unsigned long interval, int16_t* displayPins) {
  /**
   * @param interval      The time the number should be displayed for.
   * @param displayPins   The pointer array of to-display LED's on/off pins. 
   */

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while (currentMillis - previousMillis <= interval) {
    if (interval == 0) currentMillis = 0;
    currentMillis = millis();

    int16_t pins0 = displayPins[0];
    int16_t pins1 = displayPins[1];

    //clear();

    digitalWrite(gnd[0], npn ? HIGH : LOW); // Invert GND output if NPN is connected.
    digitalWrite(gnd[1], npn ? LOW : HIGH);
    for (int i=14; i>=0; i--) {
      digitalWrite(vcc[i], pins0&1);
      pins0 = pins0 >> 1;
    }
    _delay_ms(1);
    
    digitalWrite(gnd[0], npn ? LOW : HIGH);
    digitalWrite(gnd[1], npn ? HIGH : LOW);
    for (int i=14; i>=0; i--) {
      digitalWrite(vcc[i], pins1&1);
      pins1 = pins1 >> 1;
    }
    _delay_ms(1);
  }
}

void DisplayDriver::writeArray(int number, unsigned long interval, bool clock) {
  /**
   * @param number      The desired display number.
   * @param interval    The time the number should be displayed for.
   * @param clock       Toggle the clock LEDs. 
   */
  
  int arr[4] = {0};
  int16_t arr0[2], arr1[2], arr2[2], arr3[2], arr4[2], out[2] = {0};
  
  // Parse the number into the array
  for (int i = 3; i >= 0; i--) {
    arr[i] = number % 10; // Extract the last digit
    number /= 10;         // Remove the last digit from the number
  }  

  // Addition for confining to digit patterns array layout
  getDisplayDigit(arr[0]+30, arr0);
  getDisplayDigit(arr[1]+20, arr1);
  getDisplayDigit(arr[2]+10, arr2);
  getDisplayDigit(arr[3], arr3);

  if (clock) getDisplayDigit(34, arr4);

  for (int i = 0; i < 2; i++) {
    // Use bitwise OR to combine the values from all four arrays
    out[i] = arr0[i] | arr1[i] | arr2[i] | arr3[i] | arr4[i];
  }

  DisplayDriver::writePins(interval, out);
}

void DisplayDriver::writeNull(unsigned long interval, bool clock) {
  /**
   * @param interval    The time the number should be displayed for.
   * @param clock       Toggle the clock LEDs. 
   */

  int16_t null_digits[2], clock_digits[2], out[2];
  
  getDisplayDigit(35, null_digits);
  if (clock) getDisplayDigit(34, clock_digits);

  for (int i=0; i<2; i++) {
    out[i] = null_digits[i] | clock_digits[i];
  }

  DisplayDriver::writePins(interval, out);
}