#ifndef DISPLAY_DRIVER_FD0604_HPP
#define DISPLAY_DRIVER_FD0604_HPP

#include <ctype.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#define NORMAL_DISPLAY              0
#define FLIPPED_DISPLAY             1
#define INVERTED_DISPLAY            FLIPPED_DISPLAY

/**
 * @class DisplayDriver_FD0604
 * @brief Driver for FD-0604GN-24C 4-digit 7-segment LED display
 * 
 * This class handles the low-level control of the FD-0604GN-24C display, including:
 * - Multiplexing the display digits
 * - Displaying numbers, letters, and special characters
 * - Inverting display orientation
 * 
 * Assumes:
 * - Display is connected to the specified pins in DriverParameters
 * - ISR routine is set up and calls multiplex_display() at a regular interval
 */
class DisplayDriver_FD0604 {
    public:
        struct DriverParameters {
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

        DisplayDriver_FD0604(const DriverParameters& params);

        void setDisplayOrientation(bool orientation);
        void flipDisplayOrientation();
        bool getDisplayOrientation();

        // 5 required for a \n terminator
        /**
         * @details                 Parses each individual display number together. 
         *                              MUST PASS A 4-DIGIT ARRAY + NULL TERMINATOR. FOR EMPTY DIGIT FILL WITH ' ' CHAR.
         * @param letters           The desired display letters.
         * @param interval          The time the number should be displayed for.
         * @param clock             Toggle the clock LEDs. 
         * @note                    DEPRECATED, reccomend to use showDisplay
         */
        //void showLetter(const char letters[5], bool clock = false); 

        /**
         * @details                 Parses each individual display sequence together.
         *                              MUST PASS A 4-DIGIT ARRAY + NULL TERMINATOR. FOR EMPTY DIGIT FILL WITH ' ' CHAR.
         * @param letters           The desired display sequence.
         * @param interval          The time the number should be displayed for.
         * @param clock             Toggle the clock LEDs. 
         */
        void showDisplay(const char digits[5], bool leading_zeroes = false, bool clock = false);

        /**
         * @details   Clears the display.
         */
        void clear();

        /**
         * @details                 Parses each individual display number together.
         * @param number            The desired display number.
         * @param interval          The time the number should be displayed for.
         * @param leading_zeroes    Toggles whether the display will show leading zeroes.
         * @param clock             Toggle the clock LEDs. 
         */
        void showNumber(uint16_t number, bool leading_zeroes = false, bool clock = false);

        /**
         * @details           Parses the null display.
         * @param interval    The time the number should be displayed for.
         * @param clock       Toggle the clock LEDs. 
         */
        void showNull();

        /**
         * @details       ISR handler for multiplexing the display. Should be called in the main loop or timer interrupt.
         */
        void multiplexDisplay();
        

    private:
        typedef struct {
            uint16_t gnd0Mask;
            uint16_t gnd1Mask;
        } DisplayMask;

        static const DisplayMask number[40] PROGMEM;
        static const DisplayMask special_character[4] PROGMEM;
        static const DisplayMask letter[24] PROGMEM;
        static const DisplayMask number_upsidedown[40] PROGMEM;
        static const DisplayMask letter_upsidedown[24] PROGMEM;
        static const DisplayMask special_character_upsidedown[4] PROGMEM;

        static const char letter_mask[] PROGMEM;
        static const uint16_t gnd_mask;
        const uint16_t gnd_pattern;

        static void getNumber(uint8_t index, DisplayMask& output);
        static void getSpecialChar(uint8_t index, DisplayMask& output);
        static void getLetter(uint8_t index, DisplayMask& output);
        static void getNumberUpsideDown(uint8_t index, DisplayMask& output);
        static void getLetterUpsideDown(uint8_t index, DisplayMask& output);
        static void getSpecialCharUpsideDown(uint8_t index, DisplayMask& output);

        const DriverParameters _params;

        volatile uint8_t currentlyDisplayingGND = 0;
        volatile DisplayMask displayingDigits;
        bool displayOrientation = NORMAL_DISPLAY;

        void checkClock(bool &clock, DisplayMask& arr);

        void handlePinConfigurations(DisplayMask& data);
        void multiplex_display();
        void shiftOutLSBFirst(uint8_t val);

};

#endif // DISPLAY_DRIVER_FD0604_HPP
