#ifndef DISPLAY_DRIVER_FD0604_H
#define DISPLAY_DRIVER_FD0604_H

#include <ctype.h>
#include <string.h>
#include <util/delay.h>

#define NORMAL_DISPLAY              0
#define FLIPPED_DISPLAY             1
#define INVERTED_DISPLAY            FLIPPED_DISPLAY


/**
 * @details         Need to set up an ISR with function isr_mutliplex_display_callback() and pass the object through. 
 */
class DisplayDriver_FD0604 {
    public:
        struct DriverParams {
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

        DisplayDriver_FD0604(const DriverParams& params);

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
        const DriverParams* _params;

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
        void multiplex_display();
        void shiftOutLSBFirst(uint8_t val);

};

#endif
