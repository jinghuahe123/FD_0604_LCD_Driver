#include <Arduino.h>

#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "DisplayController_FD0604.hpp"
#include "configs.hpp"

//#define OFF 4000
//#define CYCLE 4001
//#define NULL_DISP 4002
//#define TEMP 4003

//unsigned long previousMillis = 0;
String input;
//uint16_t number;
//int cycle_number = 0;

#ifdef USE_MINIMAL_WIRING
DisplayDriver_FD0604 display(pins, true);
#else
DisplayDriver_FD0604 display(gnd, pins, true);
#endif
PersistentStorageManager storageManager(BASE_ADDR, SLOT_SIZE, NUM_SLOTS);

DisplayController_FD0604 displayController(display, storageManager, displayParameters);

/*
// Calculate total RAM (AVR) or use manual define
#if !defined(TOTAL_RAM) && defined(__AVR__)
  const unsigned int TOTAL_RAM = RAMEND - RAMSTART + 1;
#elif !defined(TOTAL_RAM)
  const unsigned int TOTAL_RAM = 0; // Fallback
#endif
*/

/*
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

void showAvailableCommands() {
  Serial.println(F("================================= FD-0604 LED Display ================================="));
  Serial.println(F("Enter any number to display on the screen:                                             "));
  Serial.println(F("- 0000~3999, 7000~7999 with normal orientation                                         "));
  Serial.println(F("- 0002~9992 with inverted orientation (Ones digit only '2' is supported.)              "));
  //Serial.println(F("Letters are supported as a 4-digit sequence of A-F.                                    "));
  Serial.println();

  Serial.println(F("Alternative available commands:                                                        "));
  Serial.println(F("TEMP     -  Turns the display into a thermometer using thermosistor attached.          "));
  Serial.println(F("CYCLE    -  Cycles continuously 0~3999 / 0~999 with 100ms delay between numbers.       "));
  Serial.println(F("INIT     -  Flashes all possible digits and letters once.                              "));
  Serial.println(F("NULL     -  Shows --:-- on the display.                                                "));
  Serial.println(F("OFF      -  Turns off the display.                                                     "));
  Serial.println();

  Serial.println(F("Configuration commands:                                                                "));
  Serial.println(F("HELP     -  Shows this help page.                                                      "));
  Serial.println(F("INVERT   -  Flips the screen orientation.                                              "));
  Serial.println(F("MEM      -  Prints to Serial the available free memory on the MCU.                     "));

  Serial.println(F("======================================================================================="));
  Serial.println();
}

void updateDisplay() {
  PersistentStorageManager::writtenData data = storageManager.writeData_uint16(number);
  Serial.println(F("====================="));
  //Serial.print(F("Wrote Data: ")); Serial.println(display.getDisplayOrientation() ? 
  //                                    ((number <=999 && number >=0) ? String(number) : input) : 
  //                                    ((number <= 3999 && number >=0) ? String(number) : input)
  //                                  ); // Serial.println((number == 4000) ? F("CYCLE") : String(number));
  Serial.print(F("Wrote Data: ")); Serial.println((number <=3999) ? String(number) : input);
  Serial.print(F("Written Slot: ")); Serial.println(data.writeSlot);
  Serial.print(F("EEPROM Address: 0x")); Serial.println(data.writeAddress, HEX);
  Serial.println(F("====================="));
}

void displayInit(DisplayDriver_FD0604& display, const int8_t initTime = 60) {
  display.showNull(initTime + 35);
  display.showNumber(1111, initTime, true, true);
  display.showNumber(2222, initTime, true, true);
  display.showNumber(3333, initTime, true, true);
  display.showNumber(4444, initTime, true, true);
  display.showNumber(5555, initTime, true, true);
  display.showNumber(6666, initTime, true, true);
  display.showNumber(7777, initTime, true, true);
  display.showNumber(8888, initTime, true, true);
  display.showNumber(9999, initTime, true, true);
  display.showNumber(0000, initTime, true, true);
  display.showLetter("AAAA", initTime, true);
  display.showLetter("BBBB", initTime, true);
  display.showLetter("CCCC", initTime, true);
  display.showLetter("DDDD", initTime, true);
  display.showLetter("EEEE", initTime, true);
  display.showLetter("FFFF", initTime, true);
  display.showNull(initTime + 35);
  display.clear();
}
*/

/*
// === Typedef for Command Handlers ===
typedef void (*CommandHandler)(void);

// === Enum for Command Types ===
enum CommandType {
  CMD_HELP,
  CMD_MEM,
  CMD_INIT,
  CMD_INVERT,
  CMD_OFF,
  CMD_CYCLE,
  CMD_NULL,
  CMD_TEMP,
  CMD_INVALID
};

// === Command List (Text Commands) ===
const char* const commandList[] = {
  "HELP",
  "MEM",
  "INIT",
  "INVERT",
  "OFF",
  "CYCLE",
  "NULL",
  "TEMP"
};

// === Forward Declarations of Handler Functions ===
void handleHelp();
void handleMem();
void handleInit();
void handleInvert();
void handleOff();
void handleCycle();
void handleNull();
void handleTemp();
bool parseAndSetNumber(const String& input);

// === Command Handler Function Array ===
const CommandHandler commandHandlers[] = {
  handleHelp,
  handleMem,
  handleInit,
  handleInvert,
  handleOff,
  handleCycle,
  handleNull,
  handleTemp
};

void handleHelp() {
  showAvailableCommands();
}

void handleMem() {
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
}

void handleInit() {
  displayInit(display);
}

void handleInvert() {
  display.flipDisplayOrientation();

  bool orientation = display.getDisplayOrientation(); 
  EEPROM.update(displayOrientationAddress, orientation);

  Serial.print(F("Display Orientation set to: "));
  Serial.println((orientation) ? F("INVERTED. ") : F("NORMAL. "));
}

void handleOff() {
  display.clear();
  number = OFF;
  updateDisplay();
}

void handleCycle() {
  number = CYCLE;
  cycle_number = 0;
  updateDisplay();
}

void handleNull() {
  number = NULL_DISP;
  updateDisplay();
}

void handleTemp() {
  number = TEMP;
  updateDisplay();
}

int16_t tempNumber;
bool parseAndSetNumber(const String& input) {
  if (!checkIfNumeric(input, tempNumber) || tempNumber < 0) {
    return false;
  }

  if (display.getDisplayOrientation() == false && tempNumber > 3999) {
    return false;
  }
  if (display.getDisplayOrientation() == true && tempNumber > 999) {
    return false;
  }

  number = tempNumber;
  updateDisplay();
  return true;
}

CommandType findCommandIndex(const String& input) {
  for (size_t i = 0; i < sizeof(commandList) / sizeof(commandList[0]); ++i) {
    if (input.equalsIgnoreCase(commandList[i])) { // Case-insensitive
      return static_cast<CommandType>(i);
    }
  }
  return CMD_INVALID;
}

void processInput(const String& input) {
  String trimmed_input = input;
  trimmed_input.trim();

  if (trimmed_input.length() == 0) return;

  CommandType cmd = findCommandIndex(trimmed_input);

  if (cmd != CMD_INVALID) {
    commandHandlers[cmd]();
  } else {
    if (!parseAndSetNumber(trimmed_input)) {
      Serial.print(F("Error parsing \'"));
      Serial.print(input);
      Serial.println(F("\'. Please make sure you have entered the correct format."));
    }
  }
}
*/

// int main(void) __attribute__((weak));
int main(void) {
  init();
  //initVariant();

  Serial.begin(115200);
  analogReference(EXTERNAL);

  displayController._showAvailableCommands();

  //displayController.init();

  //display.setDisplayOrientation(EEPROM.read(displayOrientationAddress));

  //number = storageManager.readData_uint16();

  for (;;) {
    if (Serial.available() > 0) {
      display.clear();

      input = Serial.readStringUntil('\n');
      input.trim();
      displayController.processInput(input);
      //number = displayController._number;
      

      // check for which input was given
      /*
      if (input == "HELP") {
        showAvailableCommands();
      } else if (input == "MEM") {
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
      } else if (input == "INVERT") {
        display.flipDisplayOrientation();

        bool orientation = display.getDisplayOrientation(); 
        EEPROM.update(displayOrientationAddress, orientation);

        Serial.print(F("Display Orientation set to: "));
        Serial.println((orientation) ? F("INVERTED. ") : F("NORMAL. "));
      } else if (input == "OFF") {
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

      } else if (input == "TEMP") {
        number = TEMP;
        updateDisplay();

      } else if (!checkIfNumeric(input, tempNumber) || tempNumber < 0 || (display.getDisplayOrientation() == NORMAL_DISPLAY && tempNumber > 3999) || (display.getDisplayOrientation() == FLIPPED_DISPLAY && tempNumber > 999)) {
        Serial.print(F("Error parsing \'"));
        Serial.print(input);
        Serial.println(F("\'. Please make sure you have entered the correct format."));

      } else {
        //number = display.getDisplayOrientation() ? tempNumber * 10 : tempNumber;
        number = tempNumber;
        updateDisplay();
      }      */
    }
    
    displayController.updateDisplay();
    /*
    // do the action that was inputted
    if ((display.getDisplayOrientation() == NORMAL_DISPLAY && number >= 0 && number <= 3999) || (display.getDisplayOrientation() == FLIPPED_DISPLAY && number >= 0 && number <= 999)) {
      display.showNumber((display.getDisplayOrientation() ? number * 10 : number), 1);

    } else if (number == CYCLE) {
      unsigned long currentMillis = millis();

      if (currentMillis - previousMillis > countingInterval) {
        previousMillis = currentMillis;
        cycle_number = (display.getDisplayOrientation() ? ((cycle_number + 1) % 1000) : (cycle_number + 1) % 4000);
      }

      if (display.getDisplayOrientation() == FLIPPED_DISPLAY) {
        display.showNumber(cycle_number*10, 1);
      } else {
        display.showNumber(cycle_number, 1);
      } 
    } else if (number == NULL_DISP) {
      display.showNull(1);
    } else if (number == TEMP) {
        uint16_t displayTemp;
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis > temperatureUpdateInterval) {
          previousMillis = currentMillis;
          
          uint16_t temperatureReading = analogRead(temperaturePin);

          double tempK = log(resistorValue * (1024.0 / temperatureReading - 1));
          tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );
          float tempC = tempK - 273.15;

          #if !defined(MODE_NO_SERIAL)
            Serial.print(F("Temperature: ")); Serial.print(tempC); Serial.println(F("*C"));
          #endif

          displayTemp = tempC * 100; // 2 virtual decimal places
          
          //if (display.getDisplayOrientation() == FLIPPED_DISPLAY) {
          //  displayTemp = displayTemp / 10; // truncates to only 1 virtual decimal place
          //  displayTemp = displayTemp * 10;
          //}
        }

        if ((displayTemp < 4000 && display.getDisplayOrientation() == NORMAL_DISPLAY)) {
          display.showNumber(displayTemp, 1);
        } else if (display.getDisplayOrientation() == FLIPPED_DISPLAY) {
          String output = String(displayTemp / 10) + 'o'; // use 'o' to activate temperature symbol

          display.showDisplay(output, 1);
        } //else {
          //display.showNull(1);
        //} 
        
      }
        */
  }

  return 0;
}


