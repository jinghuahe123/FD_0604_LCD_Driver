#include <Arduino.h>

#include "DisplayDriver.h"

const int gnd[2] = {2, 3};
const int vcc[15] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A0, A1, A2, A3, A4};

unsigned long previousMillis = 0;
const unsigned long interval = 100; // delay time
int digitData[2][15];

void clear() {
  for (int i=0; i<2; i++) {
    pinMode(gnd[i], OUTPUT);
    digitalWrite(gnd[i], HIGH);
  }

  for (int i=0; i<16; i++) {
    pinMode(vcc[i], OUTPUT);
    digitalWrite(vcc[i], LOW);
  }
}

void writeDigit(int number) {

  getDisplayDigit(number, digitData);

  clear();
  digitalWrite(gnd[0], LOW);
  digitalWrite(gnd[1], HIGH);
  for (int i=0; i<15; i++) {
    digitalWrite(vcc[i], digitData[0][i]);
  }
  delay(1);
  digitalWrite(gnd[0], HIGH);
  digitalWrite(gnd[1], LOW);
  for (int i=0; i<15; i++) {
    digitalWrite(vcc[i], digitData[1][i]);
  }
  delay(1);
}

void initDisplay(int a, int b, int c, int d) {

  // display ones digit
  writeDigit(d);

  // display tens digit
  writeDigit(c+10);

  // display hundreds digit
  writeDigit(b+20);
  
}

void writeDisplay(int number, int interval) {
  int arr[4] = {};
  // Parse the number into the array
  for (int i = 3; i >= 0; i--) {
    arr[i] = number % 10; // Extract the last digit
    number /= 10;         // Remove the last digit from the number
  }

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while (currentMillis - previousMillis <= interval) {
    currentMillis = millis();
    initDisplay(arr[0], arr[1], arr[2], arr[3]);
  }
}

void setup() {
  // put your setup code here, to run once:
  clear();

  Serial.begin(9600);
}

void loop() {
  for (int i=0; i<4000; i++) {
    writeDisplay(i, interval);
  }

}
