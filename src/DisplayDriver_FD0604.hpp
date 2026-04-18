#ifndef DISPLAY_DRIVER_FD0604_H
#define DISPLAY_DRIVER_FD0604_H

#include <ctype.h>
#include <string.h>
#include <util/delay.h>

#define MULTIPLEX_SPEED             1

#define NORMAL_WIRING_DIRECTPORT    2
#define MINIMAL_WIRING_DIRECTPORT   3

#define NORMAL_DISPLAY              0
#define FLIPPED_DISPLAY             1
#define INVERTED_DISPLAY            FLIPPED_DISPLAY


/**
 * @details         Remember to set up an ISR with function isr_mutliplex_display_callback() and pass the object through. 
 */
class DisplayDriver_FD0604 {
    public:
        struct DriverParams_DIRECTPORT {
            const bool npn_transistor_enable;
            
            volatile uint8_t* const DDRx_latchPin;
            volatile uint8_t* const PORTx_latchPin;
            const uint8_t PIN_latchPin;

            volatile uint8_t* const DDRx_clockPin;
            volatile uint8_t* const PORTx_clockPin;
            const uint8_t PIN_clockPin;

            volatile uint8_t* const DDRx_dataPin;
            volatile uint8_t* const PORTx_dataPin;
            const uint8_t PIN_dataPin;

            volatile uint8_t* const DDRx_GND1;
            volatile uint8_t* const PORTx_GND1;
            const uint8_t PIN_GND1;

            volatile uint8_t* const DDRx_GND2;
            volatile uint8_t* const PORTx_GND2;
            const uint8_t PIN_GND2;
        };

        struct DriverParams_DIRECTPORT_MinimalWiring {
            const bool npn_transistor_enable;
            
            volatile uint8_t* const DDRx_latchPin;
            volatile uint8_t* const PORTx_latchPin;
            const uint8_t PIN_latchPin;

            volatile uint8_t* const DDRx_clockPin;
            volatile uint8_t* const PORTx_clockPin;
            const uint8_t PIN_clockPin;

            volatile uint8_t* const DDRx_dataPin;
            volatile uint8_t* const PORTx_dataPin;
            const uint8_t PIN_dataPin;
        };


        DisplayDriver_FD0604(const DriverParams_DIRECTPORT& params);
        DisplayDriver_FD0604(const DriverParams_DIRECTPORT_MinimalWiring& params);

        void setDisplayOrientation(bool orientation);
        void flipDisplayOrientation();
        bool getDisplayOrientation();

        // 5 required for a \n terminator
        void showLetter(const char letters[5], bool clock = false); 
        void showDisplay(const char digits[5], bool leading_zeroes = false, bool clock = false);

        void clear();
        void showNumber(uint16_t number, bool leading_zeroes = false, bool clock = false);
        void showNull();

        static void isr_mutliplex_display_callback(DisplayDriver_FD0604* obj);
        void multiplexdisplayHandler();
        

    private:
        const DriverParams_DIRECTPORT* _params_directport;
        const DriverParams_DIRECTPORT_MinimalWiring* _params_directport_minimal;
        const uint8_t _pinConfig; // 0 for Arduino style normal, 1 for Arduino style minimal, 2 for direct register manipulation normal, 3 for direct register manipulation minimal

        volatile bool currentlyDisplayingGND = 0;
        volatile uint16_t displayingDigits[2] = {0};
        
        bool displayOrientation = NORMAL_DISPLAY;

        void getNumber(uint8_t index, uint16_t (&output)[2]);
        void getSpecialChar(uint8_t index, uint16_t (&output)[2]);

        void getLetter(uint8_t index, uint16_t (&output)[2]);
        void getNumberUpsideDown(uint8_t index, uint16_t (&output)[2]);
        void getLetterUpsideDown(uint8_t index, uint16_t (&output)[2]);
        void getSpecialCharUpsideDown(uint8_t index, uint16_t (&output)[2]);

        void checkClock(bool &clock, uint16_t (&arr)[2]);

        void handlePinConfigurations(uint16_t (&data)[2]);
        void multiplex_display_minimal();
        void multiplex_display_normal();

        void shiftOutLSBFirstNormalDisplay(uint8_t val);
        void shiftOutLSBFirstMinimalDisplay(uint8_t val);

};

#endif
