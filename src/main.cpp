#include <Arduino.h>
#include <DisplayDriver.h>

/*
  --ARDUINO PIN-- | --DISPLAY PIN--
        2                 1
        3                 2
        4                 6
        5                 7
        6                 8
        7                 9
        8                 10
        9                 12
        10                13
        11                15
        12                16
        13                17
        A0                18
        A1                19 
        A2                20
        A3                21
        A4                30
*/

const int gnd[2] = {2, 3}; // first two pins of display in order of connection
const int vcc[15] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A0, A1, A2, A3, A4}; // the rest of the display pins in order
const unsigned long interval = 100; // delay time

DisplayDriver myDisplay(gnd, vcc, true);

int main(void) {
  init();
  //initVariant();

  Serial.begin(115200);
  myDisplay.clear();

  for(;;) {
    for (int i=0; i<4000; i++) {
      myDisplay.writeArray(i, interval, i%2);
    }
  }
}