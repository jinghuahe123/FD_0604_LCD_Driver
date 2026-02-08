#ifndef DISPLAY_DRIVER_FD0604_H
#define DISPLAY_DRIVER_FD0604_H

#include <Arduino.h>
#include <util/delay.h>

#define MULTIPLEX_SPEED 4

#define NORMAL_WIRING 0
#define MINIMAL_WIRING 1

class DisplayDriver_FD0604 {
    private:
        const uint8_t* gnd;
        const uint8_t latchPin;
        const uint8_t clockPin;
        const uint8_t dataPin;
        const bool npn;
        const uint8_t wiring_style;
        
        unsigned long previousMillis = 0;

        void getNumber(uint8_t index, uint16_t (&output)[2]);
        void getSpecialChar(uint8_t index, uint16_t (&output)[2]);

        void writeShiftRegister(uint16_t data);
        void writePins(unsigned long interval, uint16_t* displayPins);

    public:
        DisplayDriver_FD0604(const uint8_t* gnds, const uint8_t* pins, bool npn_toggle);
        DisplayDriver_FD0604(const uint8_t* pins, bool npn_toggle);

        void clear();
        void writeNumber(uint16_t number, unsigned long interval, bool leading_zeroes = false, bool clock = false);
        void writeNull(unsigned long interval, bool clock = false);

};

#endif