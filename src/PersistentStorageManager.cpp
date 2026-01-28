#include <PersistentStorageManager.hpp>

/**
 * @param base_address          The EEPROM address to start logging data from.
 * @param slot_size             The size of each slot of data.
 * @param num_slots             The total number of slots to use for wear-levelling.
 */
PersistentStorageManager::PersistentStorageManager(const uint16_t &base_address, const uint8_t &slot_size, const uint16_t &num_slots) {
  BASE_ADDR = base_address;
  SLOT_SIZE = slot_size;
  NUM_SLOTS = num_slots;

  if (NUM_SLOTS*SLOT_SIZE > EEPROM.length()) {
    Serial.println(F("Error: Too much EEPROM space allocated for wear levelling."));
    while (1);
  }
}

/**
 * @param value                 The data to write to EEPROM.
 */
void PersistentStorageManager::writeData_uint16(uint16_t value) {
  uint32_t maxSequence = 0; 
  uint16_t newestSlot = -1; 

  for(uint16_t i=0; i<NUM_SLOTS; i++) {
    uint16_t address = BASE_ADDR + i * SLOT_SIZE;
    uint32_t sequence = 0;
    
    for(int8_t j=0; j<4; j++) {
      uint8_t b = EEPROM.read(address + j);
      if(b == 0xFF && j == 0) break;
      ((uint8_t*)&sequence)[j] = b;
    }

    if(sequence != 0xFFFFFFFF && (newestSlot == (uint8_t)-1 || sequence > maxSequence)) {
      maxSequence = sequence;
      newestSlot = i;
    }
  }

  uint16_t writeSlot = (newestSlot + 1) % NUM_SLOTS;
  uint16_t writeAddress = BASE_ADDR + writeSlot * SLOT_SIZE;
  
  uint32_t newSequence = (newestSlot == (uint8_t)-1) ? 1 : maxSequence + 1;

  EEPROM.put(writeAddress, newSequence);
  EEPROM.put(writeAddress + 4, value);

  Serial.println(F("====================="));
  Serial.print(F("Wrote Data: ")); Serial.println(value);
  Serial.print(F("Written Slot: ")); Serial.println(writeSlot);
  Serial.print(F("EEPROM Address: 0x")); Serial.println(writeAddress, HEX);
  // Serial.print(F("Sequence Number: ")); Serial.println(newSeq);
  Serial.println(F("====================="));
  
}

/**
 * @return                  Returns the latest piece of data that was written. 
 */
uint16_t PersistentStorageManager::readData_uint16() {
  uint32_t maxSequence = 0;
  uint16_t newestSlot = -1;

  for(uint16_t i=0; i<NUM_SLOTS; i++) {
    uint16_t address = BASE_ADDR + i * SLOT_SIZE;
    uint32_t sequence = 0;
    
    if(EEPROM.read(address) == 0xFF) continue;
    EEPROM.get(address, sequence);
    
    if(sequence != 0xFFFFFFFF && sequence >= maxSequence) {
      maxSequence = sequence;
      newestSlot = i;
    }
  }

  if(newestSlot == (uint8_t)-1) return 0;

  uint16_t data;
  EEPROM.get(BASE_ADDR + newestSlot * SLOT_SIZE + 4, data);

  Serial.println(F("====================="));
  Serial.print(F("Read Data: ")); Serial.println(data);
  Serial.print(F("Read Slot: ")); Serial.println(newestSlot);
  Serial.print(F("EEPROM Address: 0x")); Serial.println(BASE_ADDR + newestSlot * SLOT_SIZE, HEX);
  Serial.println(F("====================="));

  return data;
}