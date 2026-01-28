#include <avr/pgmspace.h>

const uint16_t display[36][2] PROGMEM = { // [digit][gnd pin]
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
  
  {0x0001, 0x0000}, // clock region
  {0x0408, 0x2040}  // nullDisplay
};
