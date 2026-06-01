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
DisplayDriver_FD0604* displayDriver = displayController.getDisplayDriverObject();

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

// function of timer.h that runs every ms
void isr_ms_timer(void) {
    DisplayDriver_FD0604::isr_multiplex_display_callback(displayDriver);
}

int main(void) {
    init_millis();
    init_adc(ADC_REF_AREF);

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


