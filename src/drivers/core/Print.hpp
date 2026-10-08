/**
 * @file Print.hpp
 * @brief Abstract base class for byte sinks (USART, LCD, SoftSerial, ...).
 *
 * `Print` owns the *what* of formatted output: how to turn an integer, a
 * float, a string, or a raw byte sequence into a stream of characters.
 * It owns nothing about the *where* — that is delegated to a single pure
 * virtual function, `write(uint8_t)`.
 *
 * Any class that can emit one byte at a time can inherit from `Print` and
 * inherit every `print()` / `println()` / `printHex()` / ... overload for
 * free. This mirrors the Arduino core's `Print` class, with three small
 * differences:
 *
 *   - Every method returns `size_t` (the number of bytes written), so
 *     callers can check or accumulate output length.
 *   - Float printing is gated behind `ENABLE_FLOAT_PRINT` so the AVR
 *     soft-float routines are only linked when actually used.
 *   - `printf` / `printf_P` are provided, gated behind `ENABLE_PRINTF`,
 *     because they pull in the avr-libc stdio formatting machinery
 *     (~1.5-2 KB of flash).
 *
 * Design notes:
 *   - `write(uint8_t)` is the only function a subclass must implement.
 *   - `write(const uint8_t*, size_t)` has a default loop implementation;
 *     subclasses with a cheap bulk path (DMA, page write) may override it.
 *   - All formatting routines emit one byte at a time via `write()`, so a
 *     subclass's fast path applies uniformly to every character.
 *   - No prefixes (`0x`, `0b`) are emitted by `printHex`/`printBin`/
 *     `printOct`; print them yourself if you want them.
 *   - Hex and binary output is fixed-width (leading zeros included);
 *     octal has no leading zeros.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <avr/pgmspace.h>

#include "drivers/core/FlashStringHelper.hpp"   // __FlashStringHelper

/**
 * @brief Set to 0 to drop float printing.
 *
 * Float formatting pulls in the AVR soft-float routines, which cost
 * several hundred bytes of flash. Define to 0 if you never print floats.
 *
 * Must be defined before including this header (or here, if you prefer
 * a single point of truth; the `#ifndef` guard makes either work).
 */
#ifndef ENABLE_FLOAT_PRINT
#define ENABLE_FLOAT_PRINT 1
#endif

/**
 * @brief Set to 0 to drop printf() / printf_P().
 *
 * `printf` is implemented on top of avr-libc's `vfprintf` and
 * `vfprintf_P`, which together pull in the stdio formatting machinery:
 * format-string parsing, all the `%d`/`%x`/`%s`/`%f` conversion
 * routines, and (if you use `%f`) the floating-point formatter. That is
 * roughly 1.5-2 KB of flash on AVR.
 *
 * Keep it at 1 if you want the familiar C-style formatted output:
 * @code
 *     Serial.printf("x=%d y=%d\n", x, y);
 *     Serial.printf_P(PSTR("addr=%04X\n"), addr);
 * @endcode
 *
 * Set to 0 to drop both functions and reclaim the flash.
 */
#ifndef ENABLE_PRINTF
#define ENABLE_PRINTF 1
#endif

#if ENABLE_PRINTF
#include <stdio.h>
#endif

/**
 * @brief Abstract base for anything that can emit a stream of bytes.
 *
 * Subclasses implement exactly one function: `write(uint8_t)`. Everything
 * else — strings, integers of every width, floats, hex/bin/oct, printf —
 * is implemented here in terms of that one function.
 *
 * @code
 *     class MySink : public Print {
 *     public:
 *         size_t write(uint8_t b) override {
 *             // ... emit b somewhere ...
 *             return 1;
 *         }
 *     };
 *
 *     MySink sink;
 *     sink.println(F("hello"));
 *     sink.printHex(0xDEADBEEF);
 *     sink.printf("value = %d\n", 42);
 * @endcode
 */
class Print {
public:
    // No virtual destructor. On AVR this avoids emitting a reference to
    // operator delete, which does not exist in the freestanding runtime.
    // Nothing deletes a Print through a base pointer, so this is safe.

    // =====================================================================
    // The one function a subclass must provide
    // =====================================================================

    /**
     * @brief Emit one byte.
     *
     * @param byte  The byte to emit (0..255).
     * @return Number of bytes written: 1 on success, 0 on failure.
     *
     * Implementations may block, buffer, or drop the byte; the base class
     * does not care. The return value is used by the bulk `write()`
     * default implementation and by every `print*` overload to compute
     * the total length written.
     */
    virtual size_t write(uint8_t byte) = 0;

    /**
     * @brief Emit a buffer of bytes.
     *
     * Default implementation loops `write(uint8_t)` one byte at a time.
     * Subclasses with a cheaper bulk path (DMA, block transfer) should
     * override this.
     *
     * @param buffer  Pointer to the bytes to emit.
     * @param size    Number of bytes to emit.
     * @return Total number of bytes written.
     */
    virtual size_t write(const uint8_t* buffer, size_t size);

    // =====================================================================
    // String output
    // =====================================================================

    /**
     * @brief Print a NUL-terminated string from RAM.
     * @param str  Pointer to a NUL-terminated string in SRAM.
     * @return Number of bytes written (excluding the NUL).
     */
    size_t print(const char* str);

    /**
     * @brief Print a NUL-terminated string from flash.
     *
     * Use with the `F()` macro:
     * @code
     *     Serial.println(F("hello from flash"));
     * @endcode
     *
     * @param str  Pointer to a NUL-terminated string in PROGMEM.
     * @return Number of bytes written (excluding the NUL).
     */
    size_t print(const __FlashStringHelper* str);

    /** @brief `print(str)` followed by CRLF. */
    size_t println(const char* str);

    /** @brief `print(str)` followed by CRLF. */
    size_t println(const __FlashStringHelper* str);

    /** @brief Emit CRLF. */
    size_t println();

    // =====================================================================
    // Integer output
    //
    // Dedicated 8- and 16-bit paths avoid the 32-bit division routine;
    // the compiler turns small-type divisions into multiply-shift. Signed
    // overloads emit a leading '-' for negative values. All widths from
    // 8 to 64 bits are supported.
    // =====================================================================

    size_t print(uint8_t  v);
    size_t print(int8_t   v);
    size_t print(uint16_t v);
    size_t print(int16_t  v);
    size_t print(uint32_t v);
    size_t print(int32_t  v);
    size_t print(uint64_t v);
    size_t print(int64_t  v);

    size_t println(uint8_t  v);
    size_t println(int8_t   v);
    size_t println(uint16_t v);
    size_t println(int16_t  v);
    size_t println(uint32_t v);
    size_t println(int32_t  v);
    size_t println(uint64_t v);
    size_t println(int64_t  v);

    // =====================================================================
    // Float output (optional)
    //
    // Formats by scaling to an integer, rounding, then printing the
    // integer and zero-padded fractional parts. Uses AVR soft-float;
    // keep ENABLE_FLOAT_PRINT = 0 if you don't need it.
    // =====================================================================

#if ENABLE_FLOAT_PRINT
    /**
     * @brief Print a float with the given number of decimal places.
     *
     * @param v        Value to print.
     * @param decimals Digits after the decimal point (0..6). Rounded, not
     *                 truncated. Values beyond 6 are clamped to 6, since
     *                 the scale factor would overflow uint32_t.
     * @return Number of bytes written.
     */
    size_t print(float v, uint8_t decimals = 2);

    /** @brief `print(v, decimals)` followed by CRLF. */
    size_t println(float v, uint8_t decimals = 2);
#endif

    // =====================================================================
    // Base-formatted output
    //
    // Fixed-width, no prefix. If you want "0x" or "0b" prefixes, print
    // them yourself first:
    //     Serial.print("0x"); Serial.printHex(v);
    //
    // Hex and binary are fixed-width (leading zeros included). Octal has
    // no leading zeros (like the C driver's serial_print_oct).
    // =====================================================================

    /** @brief Print a byte as exactly 2 hex digits (e.g. 0x0F -> "0F"). */
    size_t printHex(uint8_t v);

    /** @brief Print a 16-bit value as exactly 4 hex digits. */
    size_t printHex(uint16_t v);

    /** @brief Print a 32-bit value as exactly 8 hex digits. */
    size_t printHex(uint32_t v);

    /** @brief Print a byte as exactly 8 binary digits. */
    size_t printBin(uint8_t v);

    /** @brief Print a 16-bit value as exactly 16 binary digits. */
    size_t printBin(uint16_t v);

    /** @brief Print a 32-bit value as exactly 32 binary digits. */
    size_t printBin(uint32_t v);

    /**
     * @brief Print an unsigned 32-bit value in octal.
     *
     * No leading zeros; zero prints as "0".
     */
    size_t printOct(uint32_t v);

    // =====================================================================
    // printf / printf_P (optional)
    //
    // Implemented on top of avr-libc's vfprintf / vfprintf_P. A FILE is
    // set up on the stack and its "putchar" hook is bound to this object,
    // so every character vfprintf produces is routed through write().
    //
    // Cost: ~1.5-2 KB of flash for the stdio machinery plus (if you use
    // %f) the floating-point formatter. Gate with ENABLE_PRINTF.
    // =====================================================================

#if ENABLE_PRINTF
    /**
     * @brief Print a formatted string, C printf-style.
     *
     * The format string lives in RAM. For a format string in flash, use
     * printf_P() with PSTR() or F().
     *
     * Supported conversions are whatever avr-libc's vfprintf supports:
     *   %d %i %u %o %x %X %c %s %p %%
     *   %ld %lu %lo %lx %lX (long)
     *   %f %e %g (float/double; pulls in more flash)
     *   %S (string in flash)
     * Plus the usual width, precision, and flag modifiers.
     *
     * avr-libc's stdio is configured by the project's link flags. The
     * default is usually enough; if you need long/long-long or %f and
     * they're missing, check for `-Wl,-u,vfprintf` and the corresponding
     * `-lprintf_flt` / `-lprintf_min` selections in your Makefile.
     *
     * @param format  printf-style format string, NUL-terminated, in RAM.
     * @param ...     Arguments matching the format specifiers.
     * @return Number of characters written, or a negative value on error.
     */
    int printf(const char* format, ...)
        __attribute__((format(printf, 2, 3)));

    /**
     * @brief Print a formatted string with the format in flash.
     *
     * Identical to printf(), except the format string is read from
     * PROGMEM. Use this for format strings that never change, to keep
     * them out of SRAM:
     * @code
     *     Serial.printf_P(PSTR("addr=%04X\n"), addr);
     *     Serial.printf_P(F("value = %d\n"), value);
     * @endcode
     *
     * The `format` attribute is not applied here because GCC's printf
     * checker assumes a RAM pointer; it still works, it just won't warn.
     *
     * @param format  printf-style format string, NUL-terminated, in flash.
     * @param ...     Arguments matching the format specifiers.
     * @return Number of characters written, or a negative value on error.
     */
    int printf_P(const char* format, ...);
#endif

private:
    // Formatting helpers. All output through write().
    size_t writeDec(uint32_t v, bool negative);
    size_t writeU64(uint64_t v);
    size_t writeI64(int64_t v);
#if ENABLE_FLOAT_PRINT
    size_t writeFloat(float v, uint8_t decimals);
#endif
#if ENABLE_PRINTF
    // avr-libc FILE hook. Bound to `this` via fdev_set_udata(); every
    // character vfprintf produces lands here.
    static int _printfPutChar(char c, FILE* fp);
#endif
};