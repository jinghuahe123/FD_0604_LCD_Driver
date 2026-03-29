#include "DisplayDriver_FD0604.hpp"
#include "Digit_Patterns.hpp"

DisplayDriver_FD0604::DisplayDriver_FD0604(const DisplayDriver_FD0604::DriverParams& params) : 
    _params(&params), _params_minimal(nullptr), _params_directport(nullptr), _params_directport_minimal(nullptr), _pinConfig(NORMAL_WIRING)  {
  
  pinMode(_params->latchPin, OUTPUT);
  pinMode(_params->clockPin, OUTPUT);
  pinMode(_params->dataPin, OUTPUT);

  pinMode(_params->gnd0, OUTPUT);
  pinMode(_params->gnd1, OUTPUT);
}

DisplayDriver_FD0604::DisplayDriver_FD0604(const DisplayDriver_FD0604::DriverParams_MinimalWiring& params) : 
    _params(nullptr), _params_minimal(&params), _params_directport(nullptr), _params_directport_minimal(nullptr), _pinConfig(MINIMAL_WIRING)  {
  
  pinMode(_params_minimal->latchPin, OUTPUT);
  pinMode(_params_minimal->clockPin, OUTPUT);
  pinMode(_params_minimal->dataPin, OUTPUT);
}

DisplayDriver_FD0604::DisplayDriver_FD0604(const DisplayDriver_FD0604::DriverParams_DIRECTPORT& params) : 
    _params(nullptr), _params_minimal(nullptr), _params_directport(&params), _params_directport_minimal(nullptr), _pinConfig(NORMAL_WIRING_DIRECTPORT)  {
  
  *(_params_directport->DDRx_latchPin) |= (1 << _params_directport->PIN_latchPin);
	*(_params_directport->DDRx_clockPin) |= (1 << _params_directport->PIN_clockPin);
	*(_params_directport->DDRx_dataPin) |= (1 << _params_directport->PIN_dataPin);

  *(_params_directport->DDRx_GND1) |= (1 << _params_directport->PIN_GND1);
	*(_params_directport->DDRx_GND2) |= (1 << _params_directport->PIN_GND2);
}

DisplayDriver_FD0604::DisplayDriver_FD0604(const DisplayDriver_FD0604::DriverParams_DIRECTPORT_MinimalWiring& params) : 
    _params(nullptr), _params_minimal(nullptr), _params_directport(nullptr), _params_directport_minimal(&params), _pinConfig(MINIMAL_WIRING_DIRECTPORT)  {
  
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

void DisplayDriver_FD0604::checkClock(boolean& clock, uint16_t (&arr)[2]) {
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
  switch (_pinConfig) {
    case NORMAL_WIRING:  {
      digitalWrite(_params->gnd0, HIGH);
      digitalWrite(_params->gnd1, HIGH);
      break;
    }
    
    case MINIMAL_WIRING: {
      uint16_t pattern = (1 << 0) & (1 << 15);
      writeShiftRegister(pattern);
      break;
    }

    case NORMAL_WIRING_DIRECTPORT: {
      *(_params_directport->PORTx_GND1) |= (1 << _params_directport->PIN_GND1);
		  *(_params_directport->PORTx_GND2) |= (1 << _params_directport->PIN_GND2);
      break;
    }
      
    case MINIMAL_WIRING_DIRECTPORT: {
      uint16_t pattern = (1 << 0) & (1 << 15);
      writeShiftRegister(pattern);
      break;
    }
  }
}

/**
 * @details                 Parses each individual display number together.
 * @param number            The desired display number.
 * @param interval          The time the number should be displayed for.
 * @param leading_zeroes    Toggles whether the display will show leading zeroes.
 * @param clock             Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::showNumber(uint16_t number, unsigned long interval, bool leading_zeroes, bool clock) { 
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
  //getDisplayDigit(each_digit[0]+30, arr[0]);
  //getDisplayDigit(each_digit[1]+20, arr[1]);
  //getDisplayDigit(each_digit[2]+10, arr[2]);
  //getDisplayDigit(each_digit[3], arr[3]);
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

  writePins(interval, out);
}

/**
 * @details                 Parses each individual display number together.
 * @param letters           The desired display letters.
 * @param interval          The time the number should be displayed for.
 * @param clock             Toggle the clock LEDs. 
 * @note                    DEPRECATED
 */
void DisplayDriver_FD0604::showLetter(String letters, unsigned long interval, bool clock) { // change to char array? OR limit to size 3
  const String mask = "abcdef";
  char each_character[5] = {0}; // extra byte required for /n character
  uint16_t arr[5][2] = {0};
  uint16_t out[2] = {0};

  if (letters.length() > 4) letters = letters.substring(0, 4);

  letters.toLowerCase();
  letters.toCharArray(each_character, sizeof(each_character));

  for (uint8_t i=0; i<4; i++) {
    if (i < letters.length()) {
      int pos = mask.indexOf(each_character[i]);
      if (pos != -1) {
        if (displayOrientation == NORMAL_DISPLAY) {
          getLetter(pos + 6*(3-i), arr[i]);
        } else {
          getLetterUpsideDown(pos + 6*(3-i), arr[i]);
        }
      }
    }
    // getLetter(mask.indexOf(each_character[i]) + 6*(2-i), arr[i]);
  }

  checkClock(clock, arr[4]);

  for (int8_t i = 0; i < 2; i++) {
    // Use bitwise OR to combine the values from all arrays
    out[i] = arr[0][i] | arr[1][i] | arr[2][i] | arr[3][i] | arr[4][i];
  }

  writePins(interval, out);
}

/**
 * @details                 Parses each individual display sequence together.
 * @param letters           The desired display sequence.
 * @param interval          The time the number should be displayed for.
 * @param clock             Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::showDisplay(String to_display, unsigned long interval, bool leading_zeroes, bool clock) {
  char digits[4] = {' ', ' ', ' ', ' '};

  uint8_t length = to_display.length();
  uint8_t startPosition = 4 - length;

  for (uint8_t i=0; i<length && i<4; i++) {
    digits[startPosition + i] = to_display.charAt(i);
  }  

  charShowDisplay(digits, interval, leading_zeroes, clock);
}

/**
 * @details                 Parses each individual display sequence together.
 * @param letters           The desired display sequence.
 * @param interval          The time the number should be displayed for.
 * @param clock             Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::charShowDisplay(char (&digits)[4], unsigned long interval, bool leading_zeroes, bool clock) {
  uint16_t arr[5][2] = {0};
  uint16_t out[2] = {0};
  bool leading_digit = true;

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
      digits[i] = tolower(digits[i]);
      const String mask = "abcdef";

      int pos = mask.indexOf(digits[i]);
      if (pos != -1) {
        if (displayOrientation == NORMAL_DISPLAY) {
          getLetter(pos + 6*(3-i), arr[i]);
        } else {
          getLetterUpsideDown(pos + 6*(3-i), arr[i]);
        }
      } else if (digits[i] == 'o' && i == 3) {

        // FIX THIS BIT
        // what to fix, still need fixing?
        
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

  writePins(interval, out);
}


/**
 * @details           Parses the null display.
 * @param interval    The time the number should be displayed for.
 * @param clock       Toggle the clock LEDs. 
 */
void DisplayDriver_FD0604::showNull(unsigned long interval) {

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

  writePins(interval, out);
}

/**
 * @details             Multiplexes the display.
 * @param interval      The time the number should be displayed for.
 * @param displayPins   The pointer array of to-display LED's on/off pins. 
 */
void DisplayDriver_FD0604::writePins(unsigned long &interval, uint16_t* displayPins) {
  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while (currentMillis - previousMillis <= interval) {
    if (interval == 0) currentMillis = 0;
    currentMillis = millis();

    //clear();
    switch (_pinConfig) {
      case NORMAL_WIRING: {
        digitalWrite(_params->gnd0, _params->npn_transistor_enable ? HIGH : LOW); // Invert GND output if NPN is connected.
        digitalWrite(_params->gnd1, _params->npn_transistor_enable ? LOW : HIGH);
        writeShiftRegister(displayPins[0]);
        _delay_ms(MULTIPLEX_SPEED);
        
        digitalWrite(_params->gnd0, _params->npn_transistor_enable ? LOW : HIGH);
        digitalWrite(_params->gnd1, _params->npn_transistor_enable ? HIGH : LOW);
        writeShiftRegister(displayPins[1]);
        _delay_ms(MULTIPLEX_SPEED);

        break;
      }
      case MINIMAL_WIRING: {
        uint16_t mask = (1 << 0) | (1 << 15);
        uint16_t pattern = _params_minimal->npn_transistor_enable ? (1 << 0) : (1 << 15);

        displayPins[0] = (displayPins[0] & ~mask) | pattern;
        displayPins[1] = (displayPins[1] & ~mask) | (pattern ^ mask); // Invert pattern (gnd layout)

        writeShiftRegister(displayPins[0]);
        _delay_ms(MULTIPLEX_SPEED);
        writeShiftRegister(displayPins[1]);
        _delay_ms(MULTIPLEX_SPEED);

        break;
      }
      case NORMAL_WIRING_DIRECTPORT: {
        if (_params_directport->npn_transistor_enable) {
				  *_params_directport->PORTx_GND1 |= (1 << _params_directport->PIN_GND1);
				  *_params_directport->PORTx_GND2 &= ~(1 << _params_directport->PIN_GND2);
				} else {
				  *_params_directport->PORTx_GND1 &= ~(1 << _params_directport->PIN_GND1);
				  *_params_directport->PORTx_GND2 |= (1 << _params_directport->PIN_GND2);
			  }
			  writeShiftRegister(displayPins[0]);
			  _delay_ms(MULTIPLEX_SPEED);

			  if (_params_directport->npn_transistor_enable) {
				  *_params_directport->PORTx_GND1 &= ~(1 << _params_directport->PIN_GND1);
				  *_params_directport->PORTx_GND2 |= (1 << _params_directport->PIN_GND2);
				} else {
				  *_params_directport->PORTx_GND1 |= (1 << _params_directport->PIN_GND1);
				  *_params_directport->PORTx_GND2 &= ~(1 << _params_directport->PIN_GND2);
			  }
			  writeShiftRegister(displayPins[1]);
			  _delay_ms(MULTIPLEX_SPEED);

        break;
      }
      case MINIMAL_WIRING_DIRECTPORT: {
        uint16_t mask = (1 << 0) | (1 << 15);
        uint16_t pattern = _params_directport_minimal->npn_transistor_enable ? (1 << 0) : (1 << 15);

        displayPins[0] = (displayPins[0] & ~mask) | pattern;
        displayPins[1] = (displayPins[1] & ~mask) | (pattern ^ mask); // Invert pattern (gnd layout)

        writeShiftRegister(displayPins[0]);
        _delay_ms(MULTIPLEX_SPEED);
        writeShiftRegister(displayPins[1]);
        _delay_ms(MULTIPLEX_SPEED);

        break;
      }
    }
    
  }
}

/**
 * @details       Writes dual shift registers with display data
 * @param data    Entire two bytes of data for each ground pin. 
 */
void DisplayDriver_FD0604::writeShiftRegister(uint16_t data) {
  switch (_pinConfig) {
  case NORMAL_WIRING:
    digitalWrite(_params->latchPin, LOW);
    shiftOutLSBFirst((uint8_t)data);
    shiftOutLSBFirst((uint8_t)(data >> 8));
    digitalWrite(_params->latchPin, HIGH);
    break;

  case MINIMAL_WIRING:
    digitalWrite(_params_minimal->latchPin, LOW);
    shiftOutLSBFirst((uint8_t)data);
    shiftOutLSBFirst((uint8_t)(data >> 8));
    digitalWrite(_params_minimal->latchPin, HIGH);
    break;

  case NORMAL_WIRING_DIRECTPORT:
    *(_params_directport->PORTx_latchPin) &= ~(1 << _params_directport->PIN_latchPin);
    shiftOutLSBFirst((uint8_t)data);
    shiftOutLSBFirst((uint8_t)(data >> 8));
    *(_params_directport->PORTx_latchPin) |= (1 << _params_directport->PIN_latchPin);
    break;

  case MINIMAL_WIRING_DIRECTPORT:
    *(_params_directport_minimal->PORTx_latchPin) &= ~(1 << _params_directport_minimal->PIN_latchPin);
    shiftOutLSBFirst((uint8_t)data);
    shiftOutLSBFirst((uint8_t)(data >> 8));
    *(_params_directport_minimal->PORTx_latchPin) |= (1 << _params_directport_minimal->PIN_latchPin);
    break;
  }
}


void DisplayDriver_FD0604::shiftOutLSBFirst(uint8_t val) {
	for (uint8_t i=0; i<8; i++) {
    switch (_pinConfig) {
      case NORMAL_WIRING: {
        digitalWrite(_params->dataPin, val & 1);
        val >>= 1;
        digitalWrite(_params->clockPin, HIGH);
        digitalWrite(_params->clockPin, LOW);
        break;
      }

      case MINIMAL_WIRING: {
        digitalWrite(_params_minimal->dataPin, val & 1);
        val >>= 1;
        digitalWrite(_params_minimal->clockPin, HIGH);
        digitalWrite(_params_minimal->clockPin, LOW);
        break;
      }

      case NORMAL_WIRING_DIRECTPORT: { 
        if (val & 1) {
          *(_params_directport->PORTx_dataPin) |= (1 << _params_directport->PIN_dataPin);
        } else {
          *(_params_directport->PORTx_dataPin) &= ~(1 << _params_directport->PIN_dataPin);
        }
        val >>= 1;

        *(_params_directport->PORTx_clockPin) |= (1 << _params_directport->PIN_clockPin);
        *(_params_directport->PORTx_clockPin) &= ~(1 << _params_directport->PIN_clockPin);
        break;
      }

      case MINIMAL_WIRING_DIRECTPORT: { 
        if (val & 1) {
          *(_params_directport_minimal->PORTx_dataPin) |= (1 << _params_directport_minimal->PIN_dataPin);
        } else {
          *(_params_directport_minimal->PORTx_dataPin) &= ~(1 << _params_directport_minimal->PIN_dataPin);
        }
        val >>= 1;

        *(_params_directport_minimal->PORTx_clockPin) |= (1 << _params_directport_minimal->PIN_clockPin);
        *(_params_directport_minimal->PORTx_clockPin) &= ~(1 << _params_directport_minimal->PIN_clockPin);
        break;
      }
    }
	}
}

