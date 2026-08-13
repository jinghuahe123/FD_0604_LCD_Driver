#ifndef PERSISTENTSTORAGEMANAGER_TPP
#define PERSISTENTSTORAGEMANAGER_TPP

// IntelliSense fix: tell it the class is defined in the header
#ifndef __INTELLISENSE__
// normal compilation: class is already visible from the header include
#else
// IntelliSense: explicitly include the header so it sees the full class
#include "PersistentStorageManager.hpp"
#endif

#include "serial.h"

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
    uint16_t entryIndex = 0;

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

        entries[entryIndex++] = entry;
    }

    return uninitializedCount;
}

template <typename T>
void PersistentStorageManager<T>::printHistory(uint16_t count, void (*printValueParser)(T)) const {
    if (count == 0 || count > NUM_SLOTS) return; // invalid count

    uint16_t maxIndex = 0;
    uint32_t maxSequence = 0;
    bool foundAny = findLatestEntry(maxIndex, maxSequence);

    if (!foundAny) {
        serial_println_P(F("No valid data found in storage."));
        return;
    }

    uint16_t uninitializedCount = 0;
    uint16_t entryIndex = 0;

    serial_println_P(F("=============================================================="));
    serial_println_P(F("                     EEPROM STORAGE HISTORY"));
    serial_println_P(F("=============================================================="));

    serial_print_P(F("Base Address: 0x")); serial_print_hex16(BASE_ADDR); serial_ln();
    serial_print_P(F("Total Slots: ")); serial_print_u16(NUM_SLOTS); serial_ln();
    serial_println_P(F("--------------------------------------------------------------"));

    auto print_padded_u32 = [](uint32_t value, uint8_t digits) {
        char buffer[digits + 1]; // max 10 digits + null terminator
        char* ptr = buffer + digits; // point to the end of the buffer
        *ptr = '\0';

        for (uint8_t i=0; i<digits; i++) {
            ptr--;
            *ptr = '0' + (value % 10);
            value /= 10;
        }

        serial_print(ptr);
    };

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

        T value;
        EEPROM.get(address + sizeof(uint32_t), value);

        // print the entry here
        serial_print_P(F("[")); print_padded_u32(entryIndex++, 4);
        serial_print_P(F("] Address: 0x")); serial_print_hex16(address);
        serial_print_P(F(" | Sequence: ")); print_padded_u32(sequenceNumber, 10);
        serial_print_P(F(" | Value: ")); printValueParser(value);
        serial_ln();

    }
    serial_println_P(F("--------------------------------------------------------------"));
    serial_print_P(F("Total entries searched: ")); serial_print_u16(count - uninitializedCount); serial_ln();
    serial_print_P(F("Empty entries searched: ")); serial_print_u16(uninitializedCount); serial_ln();
    serial_println_P(F("=============================================================="));
}

#endif // PERSISTENTSTORAGEMANAGER_TPP