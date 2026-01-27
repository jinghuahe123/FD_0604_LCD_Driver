#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>

#define MULTIPLEX_SPEED 4

#define NORMAL_WIRING 0
#define MINIMAL_WIRING 1

class DisplayDriver {
    private:
        const uint8_t* gnd;
        uint8_t latchPin;
        uint8_t clockPin;
        uint8_t dataPin;
        bool npn;
        uint8_t wiring_style;
        unsigned long previousMillis = 0;

        void getDisplayDigit(uint8_t digit, uint16_t (&output)[2]);
        void writeShiftRegister(uint16_t data);
        void writePins(unsigned long interval, uint16_t* displayPins);

    public:
        DisplayDriver(const uint8_t* gnds, const uint8_t* pins, bool npn_toggle);
        DisplayDriver(const uint8_t* pins, bool npn_toggle);

        void clear();
        void writeArray(uint16_t number, unsigned long interval, bool clock=true);
        void writeNull(unsigned long interval, bool clock=true);

};

#endif