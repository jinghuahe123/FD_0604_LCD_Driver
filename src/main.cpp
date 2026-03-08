#include <Arduino.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "configs.h"

// ATTINY board definitions used to live here

#define OFF 4000
#define CYCLE 4001
#define NULL_DISP 4002
#ifndef IS_ATTINY
  #define TEMP 4003
#endif

// Global Configs used to live here

unsigned long previousMillis = 0;
String input;
uint16_t number;
int cycle_number = 0;

#ifdef IS_ATTINY
  #include <SoftwareSerial.h>

  // ATTINY configurations used to live here

  DisplayDriver_FD0604 display(pins, true);
  SoftwareSerial mySerial(rxPin, txPin);

  #define Serial mySerial
#else

  // Generic configurations used to live here

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
    analogReference(EXTERNAL);

    #ifndef MODE_NO_SERIAL
      Serial.begin(115200);
      Serial.println(F("== FD-0604 LED Display Temperature Sensor =="));
    #endif

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

      } 
      #ifndef IS_ATTINY
        else if (input == "TEMP") {
          number = TEMP;
          updateDisplay();

        }
      #endif 
      else if (!checkIfNumeric(input, tempNumber) || tempNumber > 3999 || tempNumber < 0) {
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

      if (currentMillis - previousMillis > countingInterval) {
        previousMillis = currentMillis;
        cycle_number = (cycle_number + 1) % 4000;
      }

      display.writeNumber(cycle_number, 1);
    } else if (number == NULL_DISP) {
      display.writeNull(1);
    } 
    #ifndef IS_ATTINY 
      else if (number == TEMP) {
        uint16_t displayTemp;
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis > temperatureUpdateInterval) {
          previousMillis = currentMillis;
          
          uint16_t temperatureReading = analogRead(temperaturePin);

          double tempK = log(resistorValue * (1024.0 / temperatureReading - 1));
          tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );
          float tempC = tempK - 273.15;

          #if !defined(IS_ATTINY) && !defined(MODE_NO_SERIAL)
            Serial.print(F("Temperature: ")); Serial.print(tempC); Serial.println(F("*C"));
          #endif

          displayTemp = tempC * 100;
        }

        display.writeNumber(displayTemp, 1);
      }
    #endif
  }
  
  return 0;
}


// definitions:
// 4000 - off
// 4001 - infinte cycle
// 4002 - null
// 4003 - temperature
