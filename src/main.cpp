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

void writeArray(int number, int interval, bool clock) {
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
    digitalWrite(gnd[0], LOW);
    digitalWrite(gnd[1], HIGH);
    for (int i=0; i<15; i++) {
      digitalWrite(vcc[i], out[0][i]);
    }
    delay(1);
    digitalWrite(gnd[0], HIGH);
    digitalWrite(gnd[1], LOW);
    for (int i=0; i<15; i++) {
      digitalWrite(vcc[i], out[1][i]);
    }
    delay(1);
  }
}

void setup() {
  // put your setup code here, to run once:
  clear();

  Serial.begin(9600);
}

void loop() {
  for (int i=0; i<4000; i++) {
    writeArray(i, interval, 0);
  }

}