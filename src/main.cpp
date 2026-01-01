#include <Arduino.h>

#include <DisplayDriver.h>

const int gnd[2] = {2, 3}; // first two pins of display in order of connection
const int vcc[15] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A0, A1, A2, A3, A4}; // the rest of the display pins in order
const unsigned long interval = 100; // delay time

DisplayDriver myDisplay(gnd, vcc);

void setup() {
  Serial.begin(115200);
  myDisplay.clear();
}

void loop() {
  for (int i=0; i<4000; i++) {
    myDisplay.writeArray(i, interval, 1, 1);
    //Serial.println(i);
  }
}