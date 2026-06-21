#ifndef PERSISTENTSTORAGEMANAGER_TPP
#define PERSISTENTSTORAGEMANAGER_TPP

// IntelliSense fix: tell it the class is defined in the header
#ifndef __INTELLISENSE__
// normal compilation: class is already visible from the header include
#else
// IntelliSense: explicitly include the header so it sees the full class
#include "PersistentStorageManager.hpp"
#endif

template <typename T>
PersistentStorageManager<T>::PersistentStorageManager(uint16_t base_addr, uint16_t num_slots) : BASE_ADDR(base_addr), SLOT_SIZE(sizeof(T) + sizeof(uint32_t)), NUM_SLOTS(num_slots) {
    //static_assert(BASE_ADDR + SLOT_SIZE * NUM_SLOTS <=E2END, "EEPROM size exceeded by PersistentStorageManager configuration.");
}

template <typename T>
bool PersistentStorageManager<T>::findLatestEntry(uint16_t& latestIndex, uint32_t& latestSequence) const {
    uint16_t maxIndex = 0;
    uint32_t maxSequence = 0;
    bool foundAny = false;

    for (uint16_t i=0; i<NUM_SLOTS; i++) {
        uint16_t address = BASE_ADDR + i * SLOT_SIZE;
        uint32_t sequenceNumber = 0;
        EEPROM.get(address, sequenceNumber);

        if (sequenceNumber == 0xFFFFFFFF) continue; // skip uninitialized slots

        if (!foundAny || sequenceNumber > maxSequence) {
            maxSequence = sequenceNumber;
            maxIndex = i;
            foundAny = true;
        }
    }

    if (!foundAny) {
        // no valid data found
        return false;
    }

    latestIndex = maxIndex;
    latestSequence = maxSequence;

    return foundAny;
}

template <typename T>
typename PersistentStorageManager<T>::writtenData PersistentStorageManager<T>::write(const T& value) const {
    uint16_t maxIndex = 0;
    uint32_t maxSequence = 0;
    bool foundAny = findLatestEntry(maxIndex, maxSequence);

    uint16_t nextIndex = foundAny ? (maxIndex + 1) % NUM_SLOTS : 0;
    uint16_t toWriteAddress = BASE_ADDR + nextIndex * SLOT_SIZE;

    if (foundAny) {
        T lastValue{};
        uint16_t lastAddress = BASE_ADDR + maxIndex * SLOT_SIZE;
        EEPROM.get(lastAddress + sizeof(uint32_t), lastValue);
        if (lastValue == value) {
            // if the value is the same as the last written value, do not write again
            return {maxIndex, lastAddress};
        }
    }

    EEPROM.put(toWriteAddress, maxSequence + 1);
    EEPROM.put(toWriteAddress + sizeof(uint32_t), value);

    return {nextIndex, toWriteAddress};
}

template <typename T>
T PersistentStorageManager<T>::read() const {
    uint16_t maxIndex = 0;
    uint32_t maxSequence = 0;
    bool foundAny = findLatestEntry(maxIndex, maxSequence);

    if (!foundAny) {
        // no valid data found, return default value
        return T();
    }

    T value{};
    uint16_t address = BASE_ADDR + maxIndex * SLOT_SIZE;
    EEPROM.get(address + sizeof(uint32_t), value);

    return value;
}

template <typename T>
void PersistentStorageManager<T>::erase() const {
    for (uint16_t i=0; i<NUM_SLOTS * SLOT_SIZE; i++) {
        EEPROM.write(BASE_ADDR + i, 0xFF);
        wdt_reset(); // reset watchdog timer to prevent reset during long erase
    }
}

template <typename T>
uint16_t PersistentStorageManager<T>::readHistory(uint16_t count, StorageEntry* entries) const {
    if (count == 0 || count > NUM_SLOTS) {
        return NUM_SLOTS; // invalid count
    }

    uint16_t maxIndex = 0;
    uint32_t maxSequence = 0;
    bool foundAny = findLatestEntry(maxIndex, maxSequence);

    if (!foundAny) {
        // no valid data found
        return NUM_SLOTS;
    }

    //uint16_t maxAddress = BASE_ADDR + maxIndex * SLOT_SIZE;
    uint16_t uninitializedCount = 0;

    uint16_t entryNumber = 0;

    for (uint16_t i=count; i>0; i--) {
        uint16_t backwardIndexCount = i-1;
        uint16_t slotIndex = (maxIndex + NUM_SLOTS - backwardIndexCount) % NUM_SLOTS; // modulo to wrap around
        uint16_t address = BASE_ADDR + slotIndex * SLOT_SIZE;

        uint32_t sequenceNumber = 0;
        EEPROM.get(address, sequenceNumber);
        if (sequenceNumber == 0xFFFFFFFF) {
            // uninitialized slot, skip
            uninitializedCount++;
            continue;
        }
        if (backwardIndexCount > maxSequence || sequenceNumber != maxSequence - backwardIndexCount) {
            // if the sequence number does not match the expected sequence, skip
            // ensure the slot belongs to current generation of data
            uninitializedCount++;
            continue;
        }

        StorageEntry entry;
        entry.address = address;
        entry.sequence = sequenceNumber;
        EEPROM.get(address + sizeof(uint32_t), entry.value);

        entries[entryNumber++] = entry;
    }

    return uninitializedCount;
}

#endif // PERSISTENTSTORAGEMANAGER_TPP