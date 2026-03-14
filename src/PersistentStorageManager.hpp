#ifndef PERSISTENTSTORAGEMANAGER_H
#define PERSISTENTSTORAGEMANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include <StandardCplusplus.h>
#include <vector>

class PersistentStorageManager {
    private:
        const uint16_t BASE_ADDR;
        const uint8_t SLOT_SIZE;
        const uint16_t NUM_SLOTS;

    public:
        PersistentStorageManager(const uint16_t &base_address, const uint8_t &slot_size, const uint16_t &num_slots);

        struct writtenData {
            uint16_t writeSlot;
            uint16_t writeAddress;
        };

        struct StorageEntry {
            uint16_t address;
            uint32_t sequence;
            int16_t value;
        };

        uint16_t getBaseAddr();
        uint8_t getSlotSize();
        uint16_t getNumSlots();

        writtenData writeData_uint16(uint16_t value);
        uint16_t readData_uint16();

        void clearData();
        uint16_t getLastEntries(uint8_t count, std::vector<StorageEntry>& entries);

};

#endif
