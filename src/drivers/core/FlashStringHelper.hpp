#pragma once

/**
 * @brief Arduino's PROGMEM-string tag type.
 *
 * Declared (not defined) here so `F()` can produce a distinct pointer type
 * that selects the PROGMEM overloads of `print()` / `println()`. Normally
 * lives in <WString.h>; forward-declared here to avoid pulling in all of
 * Arduino.h. Always declared, even if `F` is already defined elsewhere.
 */
class __FlashStringHelper;

#ifndef F
/**
 * @brief Wrap a string literal so the PROGMEM overloads are selected.
 *
 * The literal is stored in flash by PSTR(); the cast to
 * `const __FlashStringHelper*` is only a type tag — the pointer still
 * points at the flash-resident characters.
 */
#define F(string_literal) \
    (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))
#endif