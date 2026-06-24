#include <EEPROM.h>
#include <avr/wdt.h>
#include <avr/io.h>

#include "configs.hpp"
#include "avr_pins.h"
#include "serial.h"
#include "software_serial.h"
#include "software_serial_avr_port_name_compatibility.h"
#include "char_helper.h"
#include "timer.h"
#include "io_helper.h"
#include "DisplayDriver_FD0604.hpp"
#include "DisplayController_FD0604.hpp"

static DisplayController_FD0604 displayController(displayParams, controllerParams);
static DisplayDriver_FD0604* displayDriver = displayController.getDisplayDriverObject();
static SoftwareSerial_t secondarySerialInterface;

static void updateVersion(const bool print=0) {
    for (uint8_t i=0; i<FIRMWARE_VER_SIZE; i++) {
        if (i<sizeof(version)-1) { // ensure no null terminator is written
            char c = pgm_read_byte(&version[i]);
            EEPROM.update(i, c);
            if (print) serial_write(c);
        } else {
            EEPROM.update(i, ' '); // fill rest with blanks
        }
    }
    if (print) serial_ln();
}

static void initStatusLED() {
    DDRB |= (1 << PB5);
    PORTB &= ~(1 << PB5);
}

static void updateStatusLED(const uint32_t overflow = 1000) {
    static uint32_t counter = 0;

    if (counter == 0) {
        PINB |= (1 << PB5);
    }

    counter = (counter + 1) % overflow;
}

// function of timer.h that runs every ms
void isr_ms_timer(void) {
    DisplayDriver_FD0604::isr_multiplex_display_callback(displayDriver);
    updateStatusLED(100);
}


int main(void) {
    // ======= initialisation of basic system functions =======
    initStatusLED();
    init_timer0_millis();
    init_adc(ADC_REF_AREF);
    serial_init(HARDWARE_SERIAL_BAUD);


    // ======= initialisation of secondary (software) serial interface =======
    software_serial_init(&secondarySerialInterface, 
                        SOFT_RX_PORT, SOFT_RX_PIN,
                        SOFT_TX_PORT, SOFT_TX_PIN,
                        false); // non-inverted logic
    software_serial_begin(&secondarySerialInterface, SOFTWARE_SERIAL_BAUD);


    // ======= initialisation of EEPROM storage =======
    updateVersion();


    // ======= initialisation of watchdog =======
    wdt_enable(WDTO_2S);
    wdt_reset();


    // ======= show main startup information =======
    displayController.showInfo();
    displayController.showAvailableCommands();

    uint8_t soft_serial_rx_pin = ss_get_rx_digital(&secondarySerialInterface);
    uint8_t soft_Serial_tx_pin = ss_get_tx_digital(&secondarySerialInterface);
    serial_println_P(F("A secondary serial interface is attached for input numbers only."));
    serial_print_P(F("RX: D")); serial_print_u8(soft_serial_rx_pin); 
    serial_print_P(F(", TX: D")); serial_print_u8(soft_Serial_tx_pin);
    serial_println_P(F(" (self)."));


    // ======= show secondary serial interface startup information =======
    #ifdef SOFT_SERIAL_OUTPUT
    software_serial_println_P(&secondarySerialInterface, F("Secondary Serial Interface - INPUT (NUMBERS) ONLY."));
    software_serial_println_P(&secondarySerialInterface, F("Refer to Main Serial Interface for Verbose Output."));
    #endif
    

    for (;;) {
        wdt_reset();

        // main input handler
        if (serial_available() > 0) {
            char input[RX_BUFFER_SIZE] = {0};
            serial_read_string_until('\n', input, sizeof(input));
            trim(input);
            displayController.processInput(input);
        }

        // secondary input handler
        if (software_serial_available(&secondarySerialInterface) > 0) {
            char input[SS_RX_BUFFER_SIZE] = {0};
            software_serial_read_string_until(&secondarySerialInterface, '\n', input, sizeof(input));
            trim(input);
            displayController.processSecondaryInput(input);
        }

        displayController.updateDisplay();
    }

    return 0;
}


