#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>

class DisplayDriver {
    private:
        const int* gnd;
        const int* vcc;
        unsigned long previousMillis = 0;

        void getDisplayDigit(int digit, int (&output)[2][15]);

    public:
        DisplayDriver(const int* gnds, const int* vccs);

        void clear();
        void writeArray(int number, unsigned long interval, bool npn, bool clock);
};

#endif