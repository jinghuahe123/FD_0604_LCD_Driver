#ifndef PERSISTENTSTORAGEMANAGER_HPP
#define PERSISTENTSTORAGEMANAGER_HPP

#include <EEPROM.h>
#include <avr/wdt.h>

template <typename T>
class PersistentStorageManager {
    public: 
        struct writtenData {
            uint16_t writeSlot;
            uint16_t writeAddress;
        };

        struct StorageEntry {
            uint16_t address;
            uint32_t sequence;
            T value;
        };

        PersistentStorageManager(uint16_t base_addr, uint16_t num_slots);

        writtenData write(const T& value) const;
        T read() const;

        void erase() const;
        uint16_t readHistory(uint16_t count, StorageEntry* entries) const;

    private:
        const uint16_t BASE_ADDR;
        const uint8_t SLOT_SIZE;
        const uint16_t NUM_SLOTS; 
        
        bool findLatestEntry(uint16_t& latestIndex, uint32_t& latestSequence) const;
};

#include "PersistentStorageManager.tpp"

#endif // PERSISTENTSTORAGEMANAGER_HPP
