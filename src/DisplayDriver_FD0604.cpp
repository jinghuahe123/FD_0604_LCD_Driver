#include "DisplayDriver_FD0604.hpp"
#include "Digit_Patterns.hpp"

const char DisplayDriver_FD0604::letter_mask[] PROGMEM = "abcdef";
const uint16_t DisplayDriver_FD0604::gnd_mask = (1 << 0) | (1 << 15);

DisplayDriver_FD0604::DisplayDriver_FD0604(const DisplayDriver_FD0604::DriverParameters& params) 
    : gnd_pattern(params.npn_transistor_enable ? (1 << 0) : (1 << 15)),
      _params(params) 
{
    *(_params.DDRx_latchPin) |= (1 << _params.PIN_latchPin);
    *(_params.DDRx_clockPin) |= (1 << _params.PIN_clockPin);
    *(_params.DDRx_dataPin) |= (1 << _params.PIN_dataPin);

    displayingDigits.gnd0Mask = 0;
    displayingDigits.gnd1Mask = 0;
}

void DisplayDriver_FD0604::setDisplayOrientation(bool orientation) {
    displayOrientation = orientation;
}

void DisplayDriver_FD0604::flipDisplayOrientation() {
    displayOrientation = !displayOrientation;
}

bool DisplayDriver_FD0604::getDisplayOrientation() {
    return displayOrientation;
}

void DisplayDriver_FD0604::checkClock(bool& clock, DisplayMask& arr) {
    if (clock)  {
        if (displayOrientation == NORMAL_DISPLAY) {
            getSpecialChar(0, arr);
        } else {
            getSpecialCharUpsideDown(0, arr);
        }
    }
}

void DisplayDriver_FD0604::clear() {
    displayingDigits.gnd0Mask = 0;
    displayingDigits.gnd1Mask = 0;
}

void DisplayDriver_FD0604::showNumber(uint16_t number, bool leading_zeroes, bool clock) { 
    uint8_t each_digit_number[4] = {0};
    DisplayMask each_digit_mask[5] = {0}; // array to store each digit's display mask, including the clock mask, before it is combined into a single output mask
    DisplayMask output;
    bool leading_digit = true;
    
    // Parse the number into the array
    for (int8_t i = 3; i >= 0; i--) {
        each_digit_number[i] = number % 10; // Extract the last digit
        number /= 10;         // Remove the last digit from the number
    }  

    // Addition for confining to digit patterns array layout
    for (int8_t i=0; i<4; i++) {
        if (leading_digit && each_digit_number[i] != 0) {
            leading_digit = false;
        } 
        if (!leading_digit || leading_zeroes) {
            if (displayOrientation == NORMAL_DISPLAY) {
                getNumber(each_digit_number[i] + 10*(3-i), each_digit_mask[i]); // substitues the previous 4 commands into a loop
            } else {
                getNumberUpsideDown(each_digit_number[i] + 10*(3-i), each_digit_mask[i]); // substitues the previous 4 commands into a loop
            }
        }
    }

    checkClock(clock, each_digit_mask[4]);

    output.gnd0Mask = each_digit_mask[0].gnd0Mask | each_digit_mask[1].gnd0Mask | each_digit_mask[2].gnd0Mask | each_digit_mask[3].gnd0Mask | each_digit_mask[4].gnd0Mask;
    output.gnd1Mask = each_digit_mask[0].gnd1Mask | each_digit_mask[1].gnd1Mask | each_digit_mask[2].gnd1Mask | each_digit_mask[3].gnd1Mask | each_digit_mask[4].gnd1Mask;

    handlePinConfigurations(output);
}

/*
void DisplayDriver_FD0604::showLetter(const char letters[5], bool clock) { 
    // 5 required for a \n terminator

    //const char mask[] = "abcdef"; // TODO: make this progmem
    DisplayMask each_digit_mask[5] = {0}; // array to store each digit's display mask, including the clock mask, before it is combined into a single output mask
    DisplayMask output;

    char output_letters[5] = {0};
    for (uint8_t i=0; i<4; i++) output_letters[i] = tolower(letters[i]);

    for (uint8_t i=0; i<4; i++) {
        int8_t pos = -1; 
        const char* ptr = strchr_P(letter_mask, output_letters[i]);
        if (ptr != nullptr) pos = ptr - letter_mask;
        //char* ptr = strchr(mask, output_letters[i]);
        //if (ptr != nullptr) pos = ptr - mask;

        if (pos != -1) {
            if (displayOrientation == NORMAL_DISPLAY) {
                getLetter(pos + 6*(3-i), each_digit_mask[i]);
            } else {
                getLetterUpsideDown(pos + 6*(3-i), each_digit_mask[i]);
            }
        }
    }

    checkClock(clock, each_digit_mask[4]);

    output.gnd0Mask = each_digit_mask[0].gnd0Mask | each_digit_mask[1].gnd0Mask | each_digit_mask[2].gnd0Mask | each_digit_mask[3].gnd0Mask | each_digit_mask[4].gnd0Mask;
    output.gnd1Mask = each_digit_mask[0].gnd1Mask | each_digit_mask[1].gnd1Mask | each_digit_mask[2].gnd1Mask | each_digit_mask[3].gnd1Mask | each_digit_mask[4].gnd1Mask;

    handlePinConfigurations(output);
}*/

void DisplayDriver_FD0604::showDisplay(const char digits[5], bool leading_zeroes, bool clock) {
    DisplayMask each_digit_mask[5] = {0};
    DisplayMask output;
    //const char mask[] = "abcdef"; // TODO: make this progmem
    bool leading_digit = true;

    char output_letters[5] = {0};

    for (int i=0; i<4; i++) {
        if (isdigit(digits[i])) {
            const uint8_t number = digits[i] - '0';

            if (leading_digit && number != 0) {
                leading_digit = false;
            } 
            if (!leading_digit || leading_zeroes) {
                if (displayOrientation == NORMAL_DISPLAY) {
                    getNumber(number + 10*(3-i), each_digit_mask[i]); // substitues the previous 4 commands into a loop
                } else {
                    getNumberUpsideDown(number + 10*(3-i), each_digit_mask[i]); // substitues the previous 4 commands into a loop
                }
            }
        } else if (!isdigit(digits[i]) && digits[i] != ' ') {
            leading_digit = false;
            output_letters[i] = tolower(digits[i]);

            const char* ptr = strchr_P(letter_mask, output_letters[i]);

            if (ptr != NULL) {
                const int8_t pos = ptr - letter_mask;
                if (displayOrientation == NORMAL_DISPLAY) {
                    getLetter(pos + 6*(3-i), each_digit_mask[i]);
                } else {
                    getLetterUpsideDown(pos + 6*(3-i), each_digit_mask[i]);
                }
            } else if (digits[i] == 'o' && i == 3) {
                if (displayOrientation == NORMAL_DISPLAY) {
                    getSpecialChar(3, each_digit_mask[i]);
                } else {
                    getSpecialCharUpsideDown(3, each_digit_mask[i]);
                }
            }
        }
    }

    checkClock(clock, each_digit_mask[4]);

    output.gnd0Mask = each_digit_mask[0].gnd0Mask | each_digit_mask[1].gnd0Mask | each_digit_mask[2].gnd0Mask | each_digit_mask[3].gnd0Mask | each_digit_mask[4].gnd0Mask;
    output.gnd1Mask = each_digit_mask[0].gnd1Mask | each_digit_mask[1].gnd1Mask | each_digit_mask[2].gnd1Mask | each_digit_mask[3].gnd1Mask | each_digit_mask[4].gnd1Mask;

    handlePinConfigurations(output);
}

void DisplayDriver_FD0604::showNull() {
    DisplayMask null_digits, clock_digits, output;
  
    if (displayOrientation == NORMAL_DISPLAY) {
        getSpecialChar(1, null_digits);
        getSpecialChar(0, clock_digits);
    } else {
        getSpecialCharUpsideDown(1, null_digits);
        getSpecialCharUpsideDown(0, clock_digits);
    }

    output.gnd0Mask = null_digits.gnd0Mask | clock_digits.gnd0Mask;
    output.gnd1Mask = null_digits.gnd1Mask | clock_digits.gnd1Mask;

    handlePinConfigurations(output);
}


/**
 * @details         Writes the parsed pin data into object's digit store, ready for multiplexing.
 * @param data      The parsed pin data to write. 
 */
void DisplayDriver_FD0604::handlePinConfigurations(DisplayMask& data) {
    displayingDigits.gnd0Mask = (data.gnd0Mask & ~gnd_mask) | gnd_pattern;
    displayingDigits.gnd1Mask = (data.gnd1Mask & ~gnd_mask) | (gnd_pattern ^ gnd_mask); // Invert pattern (gnd layout)
}






/*
  The following is called on each ISR Routine. 
*/

void DisplayDriver_FD0604::multiplexDisplay() {
    multiplex_display();
}

/**
 * @details       Multiplexes the display based on minimal display wiring. 
 */
void DisplayDriver_FD0604::multiplex_display() {
    // gnd pins handled by handlePinConfigurations when called by things like showNumber.

    *(_params.PORTx_latchPin) &= ~(1 << _params.PIN_latchPin);

    if (currentlyDisplayingGND == 0) {
        shiftOutLSBFirst((uint8_t)displayingDigits.gnd0Mask);
        shiftOutLSBFirst((uint8_t)(displayingDigits.gnd0Mask >> 8));
        currentlyDisplayingGND = 1;
    } else {
        shiftOutLSBFirst((uint8_t)displayingDigits.gnd1Mask);
        shiftOutLSBFirst((uint8_t)(displayingDigits.gnd1Mask >> 8));
        currentlyDisplayingGND = 0;
    }

    *(_params.PORTx_latchPin) |= (1 << _params.PIN_latchPin);
}

/**
 * @details       Shift register function based on LSB, with minimal display port configuration. 
 *                    Separates from normal display configuration for speed, sacrificing code size. 
 * @param val     Value to shift out (8-bits at a time).
 */
void DisplayDriver_FD0604::shiftOutLSBFirst(uint8_t val) {
    for (uint8_t i=0; i<8; i++) {
        if (val & 1) {
            *(_params.PORTx_dataPin) |= (1 << _params.PIN_dataPin);
        } else {
            *(_params.PORTx_dataPin) &= ~(1 << _params.PIN_dataPin);
        }
        val >>= 1;

        *(_params.PORTx_clockPin) |= (1 << _params.PIN_clockPin);
        *(_params.PORTx_clockPin) &= ~(1 << _params.PIN_clockPin);
    }
}

