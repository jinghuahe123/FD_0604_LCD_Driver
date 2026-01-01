#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>

class DisplayDriver {
    private:
        const int* gnd;
        const int* vcc;
        bool npn;
        unsigned long previousMillis = 0;

        void getDisplayDigit(int digit, int16_t (&output)[2]);
        void writePins(unsigned long interval, int16_t* displayPins);

    public:
        DisplayDriver(const int* gnds, const int* vccs, bool npn);

        void clear();
        void writeArray(int number, unsigned long interval, bool clock);
        void writeNull(unsigned long interval, bool clock);
};

#endif