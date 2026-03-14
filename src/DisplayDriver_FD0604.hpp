#ifndef DISPLAY_DRIVER_FD0604_H
#define DISPLAY_DRIVER_FD0604_H

#include <Arduino.h>
#include <util/delay.h>

#define MULTIPLEX_SPEED     1

#define NORMAL_WIRING       0
#define MINIMAL_WIRING      1

#define NORMAL_DISPLAY      0
#define FLIPPED_DISPLAY     1
#define INVERTED_DISPLAY    FLIPPED_DISPLAY


class DisplayDriver_FD0604 {
    private:
        const uint8_t* gnd;
        const uint8_t latchPin;
        const uint8_t clockPin;
        const uint8_t dataPin;
        const bool npn;
        const uint8_t wiring_style;
        
        unsigned long previousMillis = 0;
        bool displayOrientation = NORMAL_DISPLAY;

        void getNumber(uint8_t index, uint16_t (&output)[2]);
        void getSpecialChar(uint8_t index, uint16_t (&output)[2]);

        void getLetter(uint8_t index, uint16_t (&output)[2]);
        void getNumberUpsideDown(uint8_t index, uint16_t (&output)[2]);
        void getLetterUpsideDown(uint8_t index, uint16_t (&output)[2]);
        void getSpecialCharUpsideDown(uint8_t index, uint16_t (&output)[2]);

        void charShowDisplay(char (&digits)[4], unsigned long interval,  bool leading_zeroes = false, bool clock = false);

        void checkClock(boolean &clock, uint16_t (&arr)[2]);

        void writeShiftRegister(uint16_t data);
        void writePins(unsigned long &interval, uint16_t* displayPins);

    public:
        DisplayDriver_FD0604(const uint8_t* gnds, const uint8_t* pins, bool npn_toggle);
        DisplayDriver_FD0604(const uint8_t* pins, bool npn_toggle);

        void setDisplayOrientation(bool orientation);
        void flipDisplayOrientation();
        bool getDisplayOrientation();

        void showLetter(String letters, unsigned long interval, bool clock = false);
        void showDisplay(String to_display, unsigned long interval, bool leading_zeroes = false, bool clock = false);
        // void showDisplay(char (&to_display)[4], unsigned long interval, bool leading_zeroes, bool clock);

        void clear();
        void showNumber(uint16_t number, unsigned long interval, bool leading_zeroes = false, bool clock = false);
        void showNull(unsigned long interval);

};

#endif
