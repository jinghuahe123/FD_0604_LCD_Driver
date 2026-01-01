#include <Arduino.h>

const int gnd[2] = {2, 3};
const int vcc[15] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A0, A1, A2, A3, A4};

unsigned long previousMillis = 0;
const unsigned long interval = 500; // delay time
int currentState;

const int display[10][2][15] { // [digit][gnd pin][vcc pin]
  // for vcc pin:
  // -- ENTRY NO. -- | -- PIN --
  //       0               6
  //       1               7
  //       2               8
  //       3               9
  //       4               10
  //       5               12
  //       6               13
  //       7               15
  //       8               16
  //       9               17
  //       10              18
  //       11              19
  //       12              20
  //       13              21
  //       14              30
  //
  // ref:
  //{0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14}
  //{6, 7, 8, 9,10,12,13,15,16,17,18,19,20,21,30}


  { // number 0 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0}
  },
  { // number 1 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0}
  },
  { // number 2 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0}
  },
  { // number 3 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0}
  },
  { // number 4 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0}
  },
  { // number 5 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0}
  },
  { // number 6 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0}
  },
  { // number 7 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0}
  },
  { // number 8 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0}
  },
  { // number 9 in ones digit
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0}
  }
};

const int displayParams[10] {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

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
  clear();
  digitalWrite(gnd[0], LOW);
  digitalWrite(gnd[1], HIGH);
  for (int i=0; i<15; i++) {
    digitalWrite(vcc[i], display[number][0][i]);
  }
  delay(1);
  digitalWrite(gnd[0], HIGH);
  digitalWrite(gnd[1], LOW);
  for (int i=0; i<15; i++) {
    digitalWrite(vcc[i], display[number][1][i]);
  }
  delay(1);
}

void writeDisplay(int number, int interval) {
  unsigned long currentMillis = millis();

  // Check if the interval has passed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Reset the timer
    currentState = (currentState + 1) % 10; // Cycle through states 
  }

  // Continuously call writeDisplay() based on the current state
  writeDigit(displayParams[currentState]);
}


void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i=0; i<10; i++) {
    writeDisplay(i, interval);
  }
}
