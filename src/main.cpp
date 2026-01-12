#include <Arduino.h>
#include <DisplayDriver.h>

#define USE_MINIMAL_WIRING

const uint8_t gnd[2] = {2, 3}; // first two pins of display in order of connection
const uint8_t pins[3] = {4, 5, 6}; // order of latchpin, clockpin, datapin
const unsigned long interval = 100; // delay time

#ifdef USE_MINIMAL_WIRING
DisplayDriver myDisplay(pins, true);
#else
DisplayDriver myDisplay(gnd, pins, true);
#endif


int main(void) {
  init();
  //initVariant();

  Serial.begin(115200);
  //myDisplay.clear();

  for(;;) {
    for (int i=0; i<4000; i++) {
      myDisplay.writeArray(i, interval, i%2);
    }
  }
  
  return 0;
}
