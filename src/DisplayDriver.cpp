#include <DisplayDriver.h>
#include <Digit_Patterns.h>

DisplayDriver::DisplayDriver(const int* gnds, const int* vccs) {
    gnd = gnds;
    vcc = vccs;
}

void DisplayDriver::getDisplayDigit(int digit, int (&output)[2][15]) {
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

void DisplayDriver::writeArray(int number, unsigned long interval, bool npn, bool clock) {
    /**
     * @param number      The desired display number.
     * @param interval    The time the number should be displayed for.
     * @param npn         Toggle if the NPN-transistor is connected.
     * @param clock       Toggle the clock LEDs. 
     */
      
    int out[2][15];

    int arr[4] = {};
    // Parse the number into the array
    for (int i = 3; i >= 0; i--) {
    arr[i] = number % 10; // Extract the last digit
    number /= 10;         // Remove the last digit from the number
    }

    int arr0[2][15];
    int arr1[2][15];
    int arr2[2][15];
    int arr3[2][15];

    int arr4[2][15] = {0};

    getDisplayDigit(arr[0]+30, arr0);
    getDisplayDigit(arr[1]+20, arr1);
    getDisplayDigit(arr[2]+10, arr2);
    getDisplayDigit(arr[3], arr3);

    if (clock) getDisplayDigit(34, arr4);

    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 15; j++) {
        // Use bitwise OR to combine the values from all four arrays
        out[i][j] = arr0[i][j] | arr1[i][j] | arr2[i][j] | arr3[i][j] | arr4[i][j];
      }
    }

    unsigned long currentMillis = millis();
    previousMillis = currentMillis;
    while (currentMillis - previousMillis <= interval) {
    if (interval == 0) currentMillis = 0;
    currentMillis = millis();

    clear();
    digitalWrite(gnd[0], npn ? HIGH : LOW); // Invert GND output if NPN is connected.
    digitalWrite(gnd[1], npn ? LOW : HIGH);
    for (int i=0; i<15; i++) digitalWrite(vcc[i], out[0][i]);
    delay(1);
    digitalWrite(gnd[0], npn ? LOW : HIGH);
    digitalWrite(gnd[1], npn ? HIGH : LOW);
    for (int i=0; i<15; i++) digitalWrite(vcc[i], out[1][i]);
    delay(1);
    }
}