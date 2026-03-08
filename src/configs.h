// ATTINY definitions
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || \
    defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || \
    defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__) || \
    defined(__AVR_ATtiny26__) || defined(__AVR_ATtiny261__) || defined(__AVR_ATtiny461__) || defined(__AVR_ATtiny861__) || \
    defined(__AVR_ATtiny43__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__) || \
    defined(__AVR_ATtiny48__) || defined(__AVR_ATtiny88__) || \
    defined(__AVR_ATtiny13__) || defined(__AVR_ATtiny13A__) || \
    defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__) || \
    defined(__AVR_ATtiny1634__) || \
    defined(__AVR_ATtiny828__) || \
    defined(__AVR_ATtiny441__) || defined(__AVR_ATtiny841__) || \
    defined(__AVR_ATtiny204__) || defined(__AVR_ATtiny404__) || defined(__AVR_ATtiny804__) || defined(__AVR_ATtiny1604__) || \
    defined(__AVR_ATtiny212__) || defined(__AVR_ATtiny412__) || defined(__AVR_ATtiny806__) || defined(__AVR_ATtiny1606__) || \
    defined(__AVR_ATtiny416__) || defined(__AVR_ATtiny417__) || defined(__AVR_ATtiny816__) || defined(__AVR_ATtiny817__) || defined(__AVR_ATtiny1616__) || defined(__AVR_ATtiny3216__) || \
    defined(__AVR_ATtiny1624__) || defined(__AVR_ATtiny1626__) || defined(__AVR_ATtiny1627__)

  #define IS_ATTINY
#endif


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