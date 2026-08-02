#ifndef DISPLAY_UTILS_HPP
#define DISPLAY_UTILS_HPP

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Check if a string contains only unsigned digits
 * @param str String to check (null-terminated)
 * @param number Output parameter for parsed number
 * @return true if valid unsigned number
 */
inline bool checkIfNumericUnsigned(const char* str, uint16_t& number) {
    if (str == nullptr || str[0] == '\0') return false;

    for (uint8_t i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') return false;
    }

    unsigned long val = strtoul(str, nullptr, 10);
    if (val > 65535) return false;

    number = (uint16_t)val;
    return true;
}

#endif // DISPLAY_UTILS_HPP