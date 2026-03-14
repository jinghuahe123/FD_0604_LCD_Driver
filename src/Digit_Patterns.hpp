#include <avr/pgmspace.h>

#include "DisplayDriver_FD0604.hpp"

// for vcc pin:
// -- ENTRY NO. -- | -- PIN --
//       0               6
//       1               7
//       2               8
//       3               9
//       4               10
//       5               12
//       6               13
//       7               15
//       8               16
//       9               17
//       10              18
//       11              19
//       12              20
//       13              21
//       14              30
//
// ref:
//{0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14}
//{6, 7, 8, 9,10,12,13,15,16,17,18,19,20,21,30}
//
// Pattern:
// {<gnds1>, <gnds2>}

static const uint16_t number[40][2] PROGMEM = { // [digit][gnd pin]
  {0x0016, 0x000E}, // number 0 in ones digit
  {0x0000, 0x000C}, // number 1 in ones digit
  {0x001C, 0x000A}, // number 2 in ones digit
  {0x000C, 0x000E}, // number 3 in ones digit
  {0x000A, 0x000C}, // number 4 in ones digit
  {0x000E, 0x0006}, // number 5 in ones digit
  {0x001E, 0x0006}, // number 6 in ones digit
  {0x0000, 0x000E}, // number 7 in ones digit
  {0x001E, 0x000E}, // number 8 in ones digit
  {0x000E, 0x000E}, // number 9 in ones digit

  {0x00E0, 0x00B0}, // number 0 in tens digit
  {0x0060, 0x0000}, // number 1 in tens digit
  {0x00C0, 0x0070}, // number 2 in tens digit
  {0x00E0, 0x0060}, // number 3 in tens digit
  {0x0060, 0x00C0}, // number 4 in tens digit
  {0x00A0, 0x00E0}, // number 5 in tens digit
  {0x00A0, 0x00F0}, // number 6 in tens digit
  {0x00E0, 0x0000}, // number 7 in tens digit
  {0x00E0, 0x00F0}, // number 8 in tens digit
  {0x00E0, 0x00E0}, // number 9 in tens digit

  {0x0B00, 0x0700}, // number 0 in hundreds digit
  {0x0000, 0x0600}, // number 1 in hundreds digit
  {0x0E00, 0x0500}, // number 2 in hundreds digit
  {0x0600, 0x0700}, // number 3 in hundreds digit
  {0x0500, 0x0600}, // number 4 in hundreds digit
  {0x0700, 0x0300}, // number 5 in hundreds digit
  {0x0F00, 0x0300}, // number 6 in hundreds digit
  {0x0000, 0x0700}, // number 7 in hundreds digit
  {0x0F00, 0x0700}, // number 8 in hundreds digit
  {0x0700, 0x0700}, // number 9 in hundreds digit

  {0x0000, 0x0000}, // number 0 in thousands digit - CAUTION: is not supported
  {0x0000, 0x4800}, // number 1 in thousands digit
  {0x3000, 0x7000}, // number 2 in thousands digit
  {0x3000, 0x6800}, // number 3 in thousands digit
  {0x0000, 0x0000}, // number 4 in thousands digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 5 in thousands digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 6 in thousands digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 7 in thousands digit - CAUTION: Available but not implemented
  {0x0000, 0x0000}, // number 8 in thousands digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 9 in thousands digit - CAUTION: is not supported
};

static const uint16_t special_character[4][2] PROGMEM {
  {0x0001, 0x0000}, // clock region
  {0x0408, 0x2040}, // nullDisplay
  {0x0000, 0x1800}, // highValue Symbol
  {0x000A, 0x0002} // degree symbol
};

static const uint16_t letter[24][2] PROGMEM = {
  {0x001A, 0x000E}, // A in ones digit
  {0x001E, 0x0004}, // B in ones digit
  {0x001C, 0x0000}, // C in ones digit
  {0x001C, 0x000C}, // D in ones digit
  {0x001E, 0x0002}, // E in ones digit
  {0x001A, 0x0002}, // F in ones digit

  {0x00E0, 0x00D0}, // A in tens digit
  {0x0020, 0x00F0}, // B in tens digit
  {0x0000, 0x0070}, // C in tens digit
  {0x0060, 0x0070}, // D in tens digit
  {0x0080, 0x00F0}, // E in tens digit
  {0x0080, 0x00D0}, // F in tens digit

  {0x0D00, 0x0700}, // A in hundreds digit
  {0x0F00, 0x0200}, // B in hundreds digit
  {0x0E00, 0x0000}, // C in hundreds digit
  {0x0E00, 0x0600}, // D in hundreds digit
  {0x0F00, 0x0100}, // E in hundreds digit
  {0x0D00, 0x0100}, // F in hundreds digit

  {0x0000, 0x0000}, // A in thousands digit - CAUTION is not supported
  {0x0000, 0x0000}, // B in thousands digit - CAUTION is not supported
  {0x0000, 0x0000}, // C in thousands digit - CAUTION is not supported
  {0x0000, 0x0000}, // D in thousands digit - CAUTION is not supported
  {0x0000, 0x0000}, // E in thousands digit - CAUTION is not supported
  {0x0000, 0x0000}, // F in thousands digit - CAUTION is not supported
};

static const uint16_t number_upsidedown[40][2] PROGMEM {
  {0x0000, 0x0000}, // number 0 in ones digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 1 in ones digit - CAUTION: is not supported
  {0x3000, 0x7000}, // number 2 in ones digit
  {0x0000, 0x0000}, // number 3 in ones digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 4 in ones digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 5 in ones digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 6 in ones digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 7 in ones digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 8 in ones digit - CAUTION: is not supported
  {0x0000, 0x0000}, // number 9 in ones digit - CAUTION: is not supported

  {0x0B00, 0x0700}, // number 0 in tens digit
  {0x0900, 0x0000}, // number 1 in tens digit
  {0x0E00, 0x0500}, // number 2 in tens digit
  {0x0F00, 0x0100}, // number 3 in tens digit
  {0x0D00, 0x0200}, // number 4 in tens digit
  {0x0700, 0x0300}, // number 5 in tens digit
  {0x0700, 0x0700}, // number 6 in tens digit
  {0x0B00, 0x0000}, // number 7 in tens digit
  {0x0F00, 0x0700}, // number 8 in tens digit
  {0x0F00, 0x0300}, // number 9 in tens digit

  {0x00E0, 0x00B0}, // number 0 in hundreds digit
  {0x0000, 0x0090}, // number 1 in hundreds digit
  {0x00C0, 0x0070}, // number 2 in hundreds digit
  {0x0080, 0x00F0}, // number 3 in hundreds digit
  {0x0020, 0x00D0}, // number 4 in hundreds digit
  {0x00A0, 0x00E0}, // number 5 in hundreds digit
  {0x00E0, 0x00E0}, // number 6 in hundreds digit
  {0x0000, 0x00B0}, // number 7 in hundreds digit
  {0x00E0, 0x00F0}, // number 8 in hundreds digit
  {0x00A0, 0x00F0}, // number 9 in hundreds digit

  //{0b000000000010110, 0b000000000001110}, // number 0 in thousands digit
  {0x0016, 0x000E}, // number 0 in thousands digit
  {0x0012, 0x0000}, // number 1 in thousands digit
  {0x001C, 0x000A}, // number 2 in thousands digit
  {0x001E, 0x0002}, // number 3 in thousands digit
  {0x001A, 0x0004}, // number 4 in thousands digit
  {0x000E, 0x0006}, // number 5 in thousands digit
  {0x000E, 0x000E}, // number 6 in thousands digit
  {0x0016, 0x0000}, // number 7 in thousands digit
  {0x001E, 0x000E}, // number 8 in thousands digit
  {0x001E, 0x0006}, // number 9 in thousands digit
};

static const uint16_t letter_upsidedown[24][2] PROGMEM = {
  {0x0000, 0x0000}, // A in ones digit - CAUTION: is not supported
  {0x0000, 0x0000}, // B in ones digit - CAUTION: is not supported
  {0x2000, 0x6000}, // C in ones digit 
  {0x0000, 0x0000}, // D in ones digit - CAUTION: is not supported
  {0x3000, 0x6800}, // E in ones digit 
  {0x1000, 0x6800}, // F in ones digit 

  {0x0F00, 0x0600}, // A in tens digit
  {0x0500, 0x0700}, // B in tens digit
  {0x0400, 0x0500}, // C in tens digit 
  {0x0D00, 0x0500}, // D in tens digit
  {0x0600, 0x0700}, // E in tens digit 
  {0x0600, 0x0600}, // F in tens digit 

  {0x0060, 0x00F0}, // A in hundreds digit
  {0x00E0, 0x00C0}, // B in hundreds digit
  {0x00C0, 0x0040}, // C in hundreds digit 
  {0x00C0, 0x00D0}, // D in hundreds digit
  {0x00E0, 0x0060}, // E in hundreds digit 
  {0x0060, 0x0060}, // F in hundreds digit 
  
  {0x001E, 0x000C}, // A in thousands digit
  {0x000A, 0x000E}, // B in thousands digit
  {0x0008, 0x000A}, // C in thousands digit 
  {0x001A, 0x000A}, // D in thousands digit
  {0x000C, 0x000E}, // E in thousands digit 
  {0x000C, 0x000C}, // F in thousands digit 
}; 

static const uint16_t special_character_upsidedown[4][2] PROGMEM = {
  {0x0001, 0x0000}, // clock region
  {0x0408, 0x2040}, // nullDisplay
  {0x0000, 0x0000}, // highValue Symbol - deprecated & removed
  {0x1000, 0x2800} // degree symbol
};


void DisplayDriver_FD0604::getNumber(uint8_t index, uint16_t (&output)[2]) {
  memcpy_P(output, &number[index], sizeof(number[index]));
}

void DisplayDriver_FD0604::getNumberUpsideDown(uint8_t index, uint16_t (&output)[2]) {
  memcpy_P(output, &number_upsidedown[index], sizeof(number_upsidedown[index]));
}


void DisplayDriver_FD0604::getSpecialChar(uint8_t index, uint16_t (&output)[2]) {
  memcpy_P(output, &special_character[index], sizeof(special_character[index]));
}

void DisplayDriver_FD0604::getSpecialCharUpsideDown(uint8_t index, uint16_t (&output)[2]) {
  memcpy_P(output, &special_character_upsidedown[index], sizeof(special_character_upsidedown[index]));
}


void DisplayDriver_FD0604::getLetter(uint8_t index, uint16_t (&output)[2]) {
  memcpy_P(output, &letter[index], sizeof(letter[index]));
}

void DisplayDriver_FD0604::getLetterUpsideDown(uint8_t index, uint16_t (&output)[2]) {
  memcpy_P(output, &letter_upsidedown[index], sizeof(letter_upsidedown[index]));
}

