// ========================= GLOBAL PROGRAM CONFIGURATIONS =========================

#define USE_MINIMAL_WIRING
#define MODE_NO_SERIAL

const int BASE_ADDR = 0; // EEPROM address to start writing writing from
const int SLOT_SIZE = 6; // uint32_t for sequence number (for wear levelling) + uint16_t for number
const unsigned long countingInterval = 100;

// =================================================================================

#ifdef IS_ATTINY
  // ============================ ATTINY CONFIGURATIONS ============================

  const uint8_t pins[3] = {0, 1, 2}; // order of latchpin, clockpin, datapin
  const byte rxPin = 3;
  const byte txPin = 4;

  const int NUM_SLOTS = 85; // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

  // ===============================================================================
#else 
  // ============================ GENERIC CONFIGURATIONS ===========================

  const uint8_t gnd[2] = {2, 3}; // first two pins of display in order of connection
  const uint8_t pins[3] = {6, 7, 8}; // order of latchpin, clockpin, datapin
  const uint8_t temperaturePin = A7;

  const double resistorValue = 10000.0;
  const unsigned long temperatureUpdateInterval = 250;

  const int NUM_SLOTS = 170; // maximum number of slots to use for wear levelling (SLOT_SIZE*NUM_SLOTS must < EEPROM.size())

  // ============================ GENERIC CONFIGURATIONS ===========================
#endif