/*
MIT License
Copyright (c) 2026 Jinghua He
See LICENSE file in the project root for full license text.
*/

#include <Arduino.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || \
    defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || \
    defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__) || \
    defined(__AVR_ATtiny26__) || defined(__AVR_ATtiny261__) || defined(__AVR_ATtiny461__) || defined(__AVR_ATtiny861__) || \
    defined(__AVR_ATtiny43__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__) || \
    defined(__AVR_ATtiny48__) || defined(__AVR_ATtiny88__) || \
    defined(__AVR_ATtiny13__) || defined(__AVR_ATtiny13A__) || \
    defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__) || \
    defined(__AVR_ATtiny1634__) || \
    defined(__AVR_ATtiny828__) || \
    defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny841__) || \
    defined(__AVR_ATtiny204__) || defined(__AVR_ATtiny404__) || defined(__AVR_ATtiny804__) || defined(__AVR_ATtiny1604__) || \
    defined(__AVR_ATtiny212__) || defined(__AVR_ATtiny412__) || defined(__AVR_ATtiny806__) || defined(__AVR_ATtiny1606__) || \
    defined(__AVR_ATtiny416__) || defined(__AVR_ATtiny417__) || defined(__AVR_ATtiny816__) || defined(__AVR_ATtiny817__) || defined(__AVR_ATtiny1616__) || defined(__AVR_ATtiny3216__) || \
    defined(__AVR_ATtiny1624__) || defined(__AVR_ATtiny1626__) || defined(__AVR_ATtiny1627__)

  #define IS_ATTINY
#endif
#define OFF 4000
#define CYCLE 4001
#define NULL_DISP 4002

#define USE_MINIMAL_WIRING

const int BASE_ADDR = 0; // EEPROM address to start writing writing from
const int SLOT_SIZE = 6; // uint32_t for sequence number (for wear levelling) + uint16_t for number
String input;
uint16_t number;

unsigned long previousMillis = 0;
const long interval = 100;
int cycle_number = 0;

#ifdef IS_ATTINY
  #include <SoftwareSerial.h>

  const uint8_t pins[3] = {0, 1, 2}; // order of latchpin, clockpin, datapin
  const byte rxPin = 3;
  const byte txPin = 4;

  const int NUM_SLOTS = 85; // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

  DisplayDriver_FD0604 display(pins, true);
  SoftwareSerial mySerial(rxPin, txPin);

  #define Serial mySerial
#else
  const uint8_t gnd[2] = {2, 3}; // first two pins of display in order of connection
  const uint8_t pins[3] = {6, 7, 8}; // order of latchpin, clockpin, datapin

  const int NUM_SLOTS = 170; // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

  #ifdef USE_MINIMAL_WIRING
  DisplayDriver_FD0604 display(pins, true);
  #else
  DisplayDriver_FD0604 display(gnd, pins, true);
  #endif
#endif

PersistentStorageManager storageManager(BASE_ADDR, SLOT_SIZE, NUM_SLOTS);


void displayInit(DisplayDriver_FD0604& display, const int8_t initTime = 60) {
  display.writeNull(initTime + 35);
  display.writeNumber(1111, initTime, true, true);
  display.writeNumber(2222, initTime, true, true);
  display.writeNumber(3333, initTime, true, true);
  display.writeNumber(444, initTime, true, true);
  display.writeNumber(555, initTime, true, true);
  display.writeNumber(666, initTime, true, true);
  display.writeNumber(777, initTime, true, true);
  display.writeNumber(888, initTime, true, true);
  display.writeNumber(999, initTime, true, true);
  display.writeNumber(000, initTime, true, true);
  display.writeLetter("AAA", initTime, true);
  display.writeLetter("BBB", initTime, true);
  display.writeLetter("CCC", initTime, true);
  display.writeLetter("DDD", initTime, true);
  display.writeLetter("EEE", initTime, true);
  display.writeLetter("FFF", initTime, true);
  display.writeNull(initTime + 35);
  display.clear();
}

// Calculate total RAM (AVR) or use manual define
#if !defined(TOTAL_RAM) && defined(__AVR__)
  const unsigned int TOTAL_RAM = RAMEND - RAMSTART + 1;
#elif !defined(TOTAL_RAM)
  const unsigned int TOTAL_RAM = 0; // Fallback
#endif

extern int __heap_start, *__brkval;
int freeMemory(void) {
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

bool checkIfNumeric(String string, int16_t &number) {
  if (string.length() == 0) return false;

  for (uint16_t i=0; i<string.length(); i++) {
    char c = string.charAt(i);
    if (c < '0' || c > '9') return false;
  }
  number = string.toInt();
  return true;
}

void updateDisplay() {
  PersistentStorageManager::writtenData data = storageManager.writeData_uint16(number);
  Serial.println(F("====================="));
  Serial.print(F("Wrote Data: ")); Serial.println((number <= 3999 && number >=0) ? String(number) : input); // Serial.println((number == 4000) ? F("CYCLE") : String(number));
  Serial.print(F("Written Slot: ")); Serial.println(data.writeSlot);
  Serial.print(F("EEPROM Address: 0x")); Serial.println(data.writeAddress, HEX);
  Serial.println(F("====================="));
}

// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();

  #ifdef IS_ATTINY
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    Serial.begin(9600);
  #else
    Serial.begin(115200);
  #endif

  //displayInit(display);

  number = storageManager.readData_uint16();

  for (;;) {
    if (Serial.available() > 0) {
      display.clear();

      #ifdef IS_ATTINY
        String input;
        unsigned long startTime = millis();

        while (millis() - startTime < 1000) {
          if (mySerial.available()) {
            char c = mySerial.read();
            if (c == '\n' || c == '\r') break;
            input += c;
          }
        }
      #else
        input = Serial.readStringUntil('\n');
        input.trim();
      #endif

      int16_t tempNumber;
      if (input == "MEM") {
        float percentFree;
        uint16_t freeMem = freeMemory();
        percentFree = 100.0f * static_cast<float>(freeMem) / TOTAL_RAM;

        Serial.print(F("RAM: "));
        Serial.print(freeMem);
        Serial.print(F(" of "));
        Serial.print(TOTAL_RAM);
        Serial.print(F(" bytes free. ("));
        Serial.print(percentFree);
        Serial.println(F("%)"));

      } else if (input == "INIT") {
        displayInit(display);
      }
      else if (input == "OFF") {
        display.clear();
        number = OFF;
        updateDisplay();

      } else if (input == "CYCLE") {
        number = CYCLE;
        cycle_number = 0;
        updateDisplay();

      } else if (input == "NULL") {
        number = NULL_DISP;
        updateDisplay();

      } else if (!checkIfNumeric(input, tempNumber) || tempNumber > 3999 || tempNumber < 0) {
        Serial.print(F("Error parsing \'"));
        Serial.print(input);
        Serial.println(F("\'. Please make sure you have entered the correct format and is between 0 and 3999."));

      } else {
        number = tempNumber;
        updateDisplay();
      }      
    }

    if (number >= 0 && number <= 3999) {
      display.writeNumber(number, 1);

    } else if (number == CYCLE) {
      unsigned long currentMillis = millis();

      if (currentMillis - previousMillis > interval) {
        previousMillis = currentMillis;
        cycle_number = (cycle_number + 1) % 4000;
      }

      display.writeNumber(cycle_number, 1);
    } else if (number == NULL_DISP) {
      display.writeNull(1);
    }
  }
  
  return 0;
}


// definitions:
// 4000 - off
// 4001 - infinte cycle
// 4002 - null

