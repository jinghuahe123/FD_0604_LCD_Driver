#include "DisplayDriver_FD0604.hpp"
#include "Digit_Patterns.hpp"

DisplayDriver_FD0604::DisplayDriver_FD0604(const DisplayDriver_FD0604::DriverParams_DIRECTPORT& params) : 
    _params_directport(&params), _params_directport_minimal(nullptr), _pinConfig(NORMAL_WIRING_DIRECTPORT)  {
  
  *(_params_directport->DDRx_latchPin) |= (1 << _params_directport->PIN_latchPin);
	*(_params_directport->DDRx_clockPin) |= (1 << _params_directport->PIN_clockPin);
	*(_params_directport->DDRx_dataPin) |= (1 << _params_directport->PIN_dataPin);

  *(_params_directport->DDRx_GND1) |= (1 << _params_directport->PIN_GND1);
	*(_params_directport->DDRx_GND2) |= (1 << _params_directport->PIN_GND2);
}

DisplayDriver_FD0604::DisplayDriver_FD0604(const DisplayDriver_FD0604::DriverParams_DIRECTPORT_MinimalWiring& params) : 
    _params_directport(nullptr), _params_directport_minimal(&params), _pinConfig(MINIMAL_WIRING_DIRECTPORT)  {
  
  *(_params_directport_minimal->DDRx_latchPin) |= (1 << _params_directport_minimal->PIN_latchPin);
	*(_params_directport_minimal->DDRx_clockPin) |= (1 << _params_directport_minimal->PIN_clockPin);
	*(_params_directport_minimal->DDRx_dataPin) |= (1 << _params_directport_minimal->PIN_dataPin);
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

void DisplayDriver_FD0604::checkClock(bool& clock, uint16_t (&arr)[2]) {
  if (clock)  {
    if (displayOrientation == NORMAL_DISPLAY) {
      getSpecialChar(0, arr);
    } else {
      getSpecialCharUpsideDown(0, arr);
    }
  }
}

/**
 * @details   Clears the display.
 */
void DisplayDriver_FD0604::clear() {
  displayingDigits[0] = {0};
  displayingDigits[1] = {0};
}

/**
 * @details                 Parses each individual display number together.
 * @param number            The desired display number.
 * @param interval          The time the number should be displayed for.
 * @param leading_zeroes    Toggles whether the display will show leading zeroes.
 * @param clock             Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::showNumber(uint16_t number, bool leading_zeroes, bool clock) { 
  uint16_t each_digit[4] = {0};
  uint16_t arr[5][2] = {0};
  uint16_t out[2] = {0};
  bool leading_digit = true;
  
  // Parse the number into the array
  for (int8_t i = 3; i >= 0; i--) {
    each_digit[i] = number % 10; // Extract the last digit
    number /= 10;         // Remove the last digit from the number
  }  

  // Addition for confining to digit patterns array layout
  for (int8_t i=0; i<4; i++) {
    if (leading_digit && each_digit[i] != 0) {
      leading_digit = false;
    } 
    if (!leading_digit || leading_zeroes) {
      if (displayOrientation == NORMAL_DISPLAY) {
        getNumber(each_digit[i] + 10*(3-i), arr[i]); // substitues the previous 4 commands into a loop
      } else {
        getNumberUpsideDown(each_digit[i] + 10*(3-i), arr[i]); // substitues the previous 4 commands into a loop
      }
    }
  }

  checkClock(clock, arr[4]);

  for (int8_t i = 0; i < 2; i++) {
    // Use bitwise OR to combine the values from all four arrays
    out[i] = arr[0][i] | arr[1][i] | arr[2][i] | arr[3][i] | arr[4][i];
  }

  handlePinConfigurations(out);
}

/**
 * @details                 Parses each individual display number together. 
 *                              MUST PASS A 4-DIGIT ARRAY + NULL TERMINATOR. FOR EMPTY DIGIT FILL WITH ' ' CHAR.
 * @param letters           The desired display letters.
 * @param interval          The time the number should be displayed for.
 * @param clock             Toggle the clock LEDs. 
 * @note                    DEPRECATED, reccomend to use showDisplay
 */
void DisplayDriver_FD0604::showLetter(const char letters[5], bool clock) { 
  // 5 required for a \n terminator

  const char mask[] = "abcdef";
  uint16_t arr[5][2] = {0};
  uint16_t out[2] = {0};

  char output_letters[5] = {0};
  for (uint8_t i=0; i<4; i++) output_letters[i] = tolower(letters[i]);

  for (uint8_t i=0; i<4; i++) {
    int8_t pos = -1; 
    char* ptr = strchr(mask, output_letters[i]);
    if (ptr != nullptr) pos = ptr - mask;

    if (pos != -1) {
      if (displayOrientation == NORMAL_DISPLAY) {
        getLetter(pos + 6*(3-i), arr[i]);
      } else {
        getLetterUpsideDown(pos + 6*(3-i), arr[i]);
      }
    }
  }

  checkClock(clock, arr[4]);

  for (int8_t i = 0; i < 2; i++) {
    // Use bitwise OR to combine the values from all arrays
    out[i] = arr[0][i] | arr[1][i] | arr[2][i] | arr[3][i] | arr[4][i];
  }

  handlePinConfigurations(out);
}

/**
 * @details                 Parses each individual display sequence together.
 *                              MUST PASS A 4-DIGIT ARRAY + NULL TERMINATOR. FOR EMPTY DIGIT FILL WITH ' ' CHAR.
 * @param letters           The desired display sequence.
 * @param interval          The time the number should be displayed for.
 * @param clock             Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::showDisplay(const char digits[5], bool leading_zeroes, bool clock) {
  uint16_t arr[5][2] = {0};
  uint16_t out[2] = {0};
  const char mask[] = "abcdef";
  bool leading_digit = true;

  char output_letters[5] = {0};

  for (int i=0; i<4; i++) {
    if (isdigit(digits[i])) {
      uint8_t number = digits[i] - '0';

      if (leading_digit && number != 0) {
        leading_digit = false;
      } 
      if (!leading_digit || leading_zeroes) {
        if (displayOrientation == NORMAL_DISPLAY) {
          getNumber(number + 10*(3-i), arr[i]); // substitues the previous 4 commands into a loop
        } else {
          getNumberUpsideDown(number + 10*(3-i), arr[i]); // substitues the previous 4 commands into a loop
        }
      }
    } else if (!isdigit(digits[i]) && digits[i] != ' ') {
      leading_digit = false;
      output_letters[i] = tolower(digits[i]);

      char* ptr = strchr(mask, output_letters[i]);

      if (ptr != NULL) {
        int8_t pos = ptr - mask;
        if (displayOrientation == NORMAL_DISPLAY) {
          getLetter(pos + 6*(3-i), arr[i]);
        } else {
          getLetterUpsideDown(pos + 6*(3-i), arr[i]);
        }
      } else if (digits[i] == 'o' && i == 3) {
        if (displayOrientation == NORMAL_DISPLAY) {
          getSpecialChar(3, arr[i]);
        } else {
          getSpecialCharUpsideDown(3, arr[i]);
        }
      }
    }
  }

  checkClock(clock, arr[4]);

  for (int8_t i = 0; i < 2; i++) {
    // Use bitwise OR to combine the values from all four arrays
    out[i] = arr[0][i] | arr[1][i] | arr[2][i] | arr[3][i] | arr[4][i];
  }

  handlePinConfigurations(out);
}

/**
 * @details           Parses the null display.
 * @param interval    The time the number should be displayed for.
 * @param clock       Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::showNull() {

  uint16_t null_digits[2], clock_digits[2], out[2];
  
  if (displayOrientation == NORMAL_DISPLAY) {
    getSpecialChar(1, null_digits);
    getSpecialChar(0, clock_digits);
  } else {
    getSpecialCharUpsideDown(1, null_digits);
    getSpecialCharUpsideDown(0, clock_digits);
  }

  for (int8_t i=0; i<2; i++) {
    out[i] = null_digits[i] | clock_digits[i];
  }

  handlePinConfigurations(out);
}


/**
 * @details         Writes the parsed pin data into object's digit store, ready for multiplexing.
 * @param data      The parsed pin data to write. 
 */
void DisplayDriver_FD0604::handlePinConfigurations(uint16_t (&data)[2]) {
  switch (_pinConfig) {
    case NORMAL_WIRING_DIRECTPORT: {
      displayingDigits[0] = data[0];
      displayingDigits[1] = data[1];
      break;
    }
    case MINIMAL_WIRING_DIRECTPORT: {
      uint16_t mask = (1 << 0) | (1 << 15);
      uint16_t pattern = _params_directport_minimal->npn_transistor_enable ? (1 << 0) : (1 << 15);

      displayingDigits[0] = (data[0] & ~mask) | pattern;
      displayingDigits[1] = (data[1] & ~mask) | (pattern ^ mask); // Invert pattern (gnd layout)
      break;
    }
  }
}






/*
  The following is called on each ISR Routine. 
  minimal display is much more efficient
*/

/**
 * @details       Static multiplex callback for running in ISR routine.
 * @param obj     The corresponding display object to multiplex. 
 */
void DisplayDriver_FD0604::isr_mutliplex_display_callback(DisplayDriver_FD0604* obj) {
  obj->multiplexdisplayHandler();
}

/**
 * @details       Handles whether normal or minimal display multiplexer is used. 
 */
void DisplayDriver_FD0604::multiplexdisplayHandler() {
  switch (_pinConfig) {
    case NORMAL_WIRING_DIRECTPORT:    multiplex_display_normal();   break;
    case MINIMAL_WIRING_DIRECTPORT:   multiplex_display_minimal();   break;
  }
}

/**
 * @details       Multiplexes the display based on normal display wiring. 
 */
void DisplayDriver_FD0604::multiplex_display_normal() {
  // could optimise this for better clarity.
  // also need to test if this works or not
  if (_params_directport->npn_transistor_enable && currentlyDisplayingGND == 0) {
    *_params_directport->PORTx_GND1 |= (1 << _params_directport->PIN_GND1);
    *_params_directport->PORTx_GND2 &= ~(1 << _params_directport->PIN_GND2);
  } else if (_params_directport->npn_transistor_enable && currentlyDisplayingGND == 1) {
    *_params_directport->PORTx_GND1 &= ~(1 << _params_directport->PIN_GND1);
    *_params_directport->PORTx_GND2 |= (1 << _params_directport->PIN_GND2);
  } else if (!_params_directport->npn_transistor_enable && currentlyDisplayingGND == 1) {
    *_params_directport->PORTx_GND1 |= (1 << _params_directport->PIN_GND1);
    *_params_directport->PORTx_GND2 &= ~(1 << _params_directport->PIN_GND2);
  } else if (!_params_directport->npn_transistor_enable && currentlyDisplayingGND == 0) {
    *_params_directport->PORTx_GND1 &= ~(1 << _params_directport->PIN_GND1);
    *_params_directport->PORTx_GND2 |= (1 << _params_directport->PIN_GND2);
  }

  *(_params_directport->PORTx_latchPin) &= ~(1 << _params_directport->PIN_latchPin);
  shiftOutLSBFirstNormalDisplay((uint8_t)displayingDigits[currentlyDisplayingGND]);
  shiftOutLSBFirstNormalDisplay((uint8_t)(displayingDigits[currentlyDisplayingGND] >> 8));
  *(_params_directport->PORTx_latchPin) |= (1 << _params_directport->PIN_latchPin);

  currentlyDisplayingGND = !currentlyDisplayingGND;
}

/**
 * @details       Multiplexes the display based on minimal display wiring. 
 */
void DisplayDriver_FD0604::multiplex_display_minimal() {
  // gnd pins handled by handlePinConfigurations when called by things like showNumber.

  *(_params_directport_minimal->PORTx_latchPin) &= ~(1 << _params_directport_minimal->PIN_latchPin);
  shiftOutLSBFirstMinimalDisplay((uint8_t)displayingDigits[currentlyDisplayingGND]);
  shiftOutLSBFirstMinimalDisplay((uint8_t)(displayingDigits[currentlyDisplayingGND] >> 8));
  *(_params_directport_minimal->PORTx_latchPin) |= (1 << _params_directport_minimal->PIN_latchPin);

  currentlyDisplayingGND = !currentlyDisplayingGND;
}

/**
 * @details       Shift register function based on LSB, with normal display port configuration. 
 *                    Separates from minimal display configuration for speed, sacrificing code size. 
 * @param val     Value to shift out (8-bits at a time).
 */
void DisplayDriver_FD0604::shiftOutLSBFirstNormalDisplay(uint8_t val) {
  for (uint8_t i=0; i<8; i++) {
    if (val & 1) {
      *(_params_directport->PORTx_dataPin) |= (1 << _params_directport->PIN_dataPin);
    } else {
      *(_params_directport->PORTx_dataPin) &= ~(1 << _params_directport->PIN_dataPin);
    }
    val >>= 1;

    *(_params_directport->PORTx_clockPin) |= (1 << _params_directport->PIN_clockPin);
    *(_params_directport->PORTx_clockPin) &= ~(1 << _params_directport->PIN_clockPin);
  }
}

/**
 * @details       Shift register function based on LSB, with minimal display port configuration. 
 *                    Separates from normal display configuration for speed, sacrificing code size. 
 * @param val     Value to shift out (8-bits at a time).
 */
void DisplayDriver_FD0604::shiftOutLSBFirstMinimalDisplay(uint8_t val) {
  for (uint8_t i=0; i<8; i++) {
    if (val & 1) {
      *(_params_directport_minimal->PORTx_dataPin) |= (1 << _params_directport_minimal->PIN_dataPin);
    } else {
      *(_params_directport_minimal->PORTx_dataPin) &= ~(1 << _params_directport_minimal->PIN_dataPin);
    }
    val >>= 1;

    *(_params_directport_minimal->PORTx_clockPin) |= (1 << _params_directport_minimal->PIN_clockPin);
    *(_params_directport_minimal->PORTx_clockPin) &= ~(1 << _params_directport_minimal->PIN_clockPin);
  }
}

