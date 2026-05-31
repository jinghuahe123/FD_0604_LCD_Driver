#include <EEPROM.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "configs.hpp"
#include "serial.h"
#include "char_helper.h"
#include "timer.h"
#include "io_helper.h"
#include "DisplayDriver_FD0604.hpp"
#include "PersistentStorageManager.hpp"
#include "DisplayController_FD0604.hpp"

DisplayController_FD0604 displayController(displayParams, controllerParams);

static void updateVersion(bool print=0) {
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

static void init_timer2_for_1000hz(void) {
    // clear timer2 control registers
    TCCR2A = 0x00; // normal mode, OC2 disconnected
    TCCR2B = 0x00;
    TCCR2A |= (1 << WGM21); // set CTC mode
    
    // Prescaler 128 
    TCCR2B |= (1 << CS22) | (1 << CS20);  // CS22=1, CS21=0, CS20=1 = /128
    OCR2A = 124;  // 16MHz/128 = 125kHz / 125 = 1000Hz
    
    TIMSK2 |= (1 << OCIE2A); // enable interrupts
    sei();
}

ISR(TIMER2_COMPA_vect) {
    DisplayDriver_FD0604::isr_mutliplex_display_callback(displayController.getDisplayDriverObject());
}

int main(void) {
    init_millis();
    init_adc(ADC_REF_AREF);
    init_timer2_for_1000hz();

    serial_init(HARDWARE_SERIAL_BAUD);
    updateVersion();

    wdt_enable(WDTO_2S);
    wdt_reset();

    displayController.showInfo();
    displayController.showAvailableCommands();
    
    for (;;) {
        wdt_reset();

        if (serial_available() > 0) {
            char input[RX_BUFFER_SIZE] = {0};
            serial_read_string_until('\n', input, sizeof(input));
            trim(input);
            displayController.processInput(input);
        }

        displayController.updateDisplay();
    }

    return 0;
}


