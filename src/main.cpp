#include <EEPROM.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "configs.hpp"
#include "drivers/HardwareSerial/HardwareSerial.hpp"
#include "drivers/SoftwareSerial/SoftwareSerial.hpp"
#include "drivers/core/char_helper.h"
#include "drivers/Timer0/timer0.h"
#include "DisplayController/DisplayController.hpp"
#include "drivers/HardwarePin/HardwarePin.hpp"

static DisplayController displayController(displayParams);
static SoftwareSerial secondarySerialInterface(SOFT_RX_DIGITAL_PIN, SOFT_TX_DIGITAL_PIN, false); // non-inverted logic
static HardwarePin statusLED = HardwarePin(statusLEDPin); // D13 pin for status LED

static void updateVersion(const bool print=0) {
    for (uint8_t i=0; i<FIRMWARE_VER_SIZE; i++) {
        if (i<sizeof(version)-1) { // ensure no null terminator is written
            char c = pgm_read_byte(&version[i]);
            EEPROM.update(i, c);
            if (print) Serial.write(c);
        } else {
            EEPROM.update(i, ' '); // fill rest with blanks
        }
    }
    if (print) Serial.println();
}

static void updateStatusLED(const uint16_t overflow = 1000) {
    static uint16_t counter = 0;

    if (counter == 0) {
        statusLED.digitalWrite(!statusLED.digitalRead()); // toggle LED
    }

    counter = (counter + 1) % overflow;
}

// function of timer0.h that runs every ms
void isr_ms_timer(void) {
    displayController.multiplexDisplay();
    updateStatusLED(statusLEDBlinkInterval);
}


int main(void) {
    // ======= initialisation of basic system functions =======
    init_timer0();
    HardwarePin::initADC(HardwarePin::adcRef_t::AREF);
    Stream::tickSource = millis;
    statusLED.setMode(HardwarePin::Mode::OUTPUT);

    // ======= initialisation of main (hardware) serial interface =======
    Serial.begin(HARDWARE_SERIAL_BAUD);
    Serial.setTimeout(SERIAL_TIMEOUT); // 4s timeout for user input
    

    // ======= initialisation of secondary (software) serial interface =======
    secondarySerialInterface.begin(SOFTWARE_SERIAL_BAUD);
    secondarySerialInterface.setTimeout(SERIAL_TIMEOUT); // 4s timeout for user input


    // ======= initialisation of EEPROM storage =======
    updateVersion();


    // ======= initialisation of watchdog =======
    wdt_enable(WDTO_8S);
    wdt_reset();


    // ======= show main startup information =======
    displayController.showInfo();
    displayController.showAvailableCommands();

    // ======= show secondary serial interface startup information =======
    Serial.println(F("A secondary serial interface is attached for input numbers only."));
    Serial.print(F("RX: D")); Serial.print(SOFT_RX_DIGITAL_PIN); 
    Serial.print(F(", TX: D")); Serial.print(SOFT_TX_DIGITAL_PIN);
    Serial.println(F(" (self)."));


    // ======= show secondary serial interface startup information =======
    #if SOFT_SERIAL_OUTPUT
    secondarySerialInterface.println(F("Secondary Serial Interface - INPUT (NUMBERS) ONLY."));
    secondarySerialInterface.println(F("Refer to Main Serial Interface for Verbose Output."));
    #endif

    sei(); // enable global interrupts

    for (;;) {
        wdt_reset();

        // main input handler
        if (Serial.available() > 0) {
            char input[RX_BUFFER_SIZE] = {0};
            Serial.readStringUntil('\n', input, sizeof(input));
            trim(input);
            displayController.processInput(input);
        }

        // secondary input handler
        if (secondarySerialInterface.available() > 0) {
            char input[_SS_MAX_RX_BUFF] = {0};
            secondarySerialInterface.readStringUntil('\n', input, sizeof(input));
            trim(input);
            displayController.processSecondaryInput(input);
        }

        displayController.updateDisplay();
    }

    return 0;
}


