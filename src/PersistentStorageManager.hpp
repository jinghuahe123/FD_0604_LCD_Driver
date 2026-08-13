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

        /**
         * @brief Write a value to EEPROM with sequence tracking
         * @param value The value to write
         * @return A struct containing the slot index and EEPROM address where the value was written
         */
        writtenData write(const T& value) const;

        /**
         * @brief Read the latest stored value from EEPROM
         * @return The latest stored value
         */
        T read() const;

        /**
         * @brief Erase all stored data in EEPROM
         */
        void erase() const;
        
        /**
         * @brief Read the history of stored values
         * @param count Number of history entries to read
         * @param entries Pointer to an array of StorageEntry to store the results
         * @return Number of uninitialized entries found
         */
        uint16_t readHistory(uint16_t count, StorageEntry* entries) const;

        /**
         * @brief Print the history of stored values to serial
         * @param count Number of history entries to print
         * @param printValueParser A lambda to format the value being printed
         */
        void printHistory(uint16_t count, void (*printValueParser)(T)) const;

    private:
        const uint16_t BASE_ADDR;
        const uint8_t SLOT_SIZE;
        const uint16_t NUM_SLOTS; 
        
        bool findLatestEntry(uint16_t& latestIndex, uint32_t& latestSequence) const;
};

#include "PersistentStorageManager.tpp"

#endif // PERSISTENTSTORAGEMANAGER_HPP
