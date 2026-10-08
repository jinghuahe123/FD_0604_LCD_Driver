/**
 * @file Print.cpp
 * @brief Implementation of the Print base class.
 *
 * Every formatting routine here emits its characters one at a time via
 * the virtual `write(uint8_t)`, so a subclass's fast path (e.g. a USART
 * with an empty data register) applies to every character of every
 * formatted value.
 *
 * The integer paths are written to avoid pulling in the AVR 32-bit
 * division routine where possible: 8- and 16-bit values use successive
 * divide/modulo on small types, which the compiler lowers to
 * multiply-shift. 32- and 64-bit values use the generic divide-by-10
 * loop, which does link the division routine.
 *
 * printf / printf_P (optional, see ENABLE_PRINTF) wrap an avr-libc FILE
 * around the object and forward vfprintf's output through write().
 */

#include "Print.hpp"

#if ENABLE_PRINTF
#include <stdio.h>
#endif

// ===========================================================================
// Bulk write
// ===========================================================================

size_t Print::write(const uint8_t* buffer, size_t size) {
    size_t n = 0;
    while (size--) n += write(*buffer++);
    return n;
}

// ===========================================================================
// Strings
// ===========================================================================

size_t Print::print(const char* str) {
    // Route through write() so the subclass's fast path applies per char.
    size_t n = 0;
    while (*str) n += write(static_cast<uint8_t>(*str++));
    return n;
}

size_t Print::print(const __FlashStringHelper* str) {
    const char* p = reinterpret_cast<const char*>(str);
    size_t n = 0;
    char c;
    while ((c = static_cast<char>(pgm_read_byte(p++))) != '\0') {
        n += write(static_cast<uint8_t>(c));
    }
    return n;
}

size_t Print::println(const char* str) {
    size_t n = print(str);
    return n + print("\r\n");
}

size_t Print::println(const __FlashStringHelper* str) {
    size_t n = print(str);
    return n + print("\r\n");
}

size_t Print::println() {
    return print("\r\n");
}

// ===========================================================================
// Integers
//
// 8- and 16-bit paths avoid the 32-bit division routine by emitting
// digits from the most significant end with successive divide/modulo.
// ===========================================================================

size_t Print::print(uint8_t v) {
    if (v >= 100) {
        size_t n = write('0' + v / 100); v %= 100;
        n += write('0' + v / 10);        v %= 10;
        n += write('0' + v);
        return n;
    }
    if (v >= 10) {
        size_t n = write('0' + v / 10); v %= 10;
        n += write('0' + v);
        return n;
    }
    return write('0' + v);
}

size_t Print::print(uint16_t v) {
    if (v >= 10000) {
        size_t n = write('0' + v / 10000); v %= 10000;
        n += write('0' + v / 1000);        v %= 1000;
        n += write('0' + v / 100);         v %= 100;
        n += write('0' + v / 10);          v %= 10;
        n += write('0' + v);
        return n;
    }
    if (v >= 1000) {
        size_t n = write('0' + v / 1000); v %= 1000;
        n += write('0' + v / 100);        v %= 100;
        n += write('0' + v / 10);         v %= 10;
        n += write('0' + v);
        return n;
    }
    if (v >= 100) {
        size_t n = write('0' + v / 100); v %= 100;
        n += write('0' + v / 10);        v %= 10;
        n += write('0' + v);
        return n;
    }
    if (v >= 10) {
        size_t n = write('0' + v / 10); v %= 10;
        n += write('0' + v);
        return n;
    }
    return write('0' + v);
}

size_t Print::writeDec(uint32_t v, bool negative) {
    if (v == 0) return write('0');
    char buf[10];                   // max 10 digits for uint32_t
    uint8_t i = 0;
    while (v) { buf[i++] = static_cast<char>('0' + (v % 10)); v /= 10; }
    size_t n = 0;
    if (negative) n += write('-');
    while (i) n += write(static_cast<uint8_t>(buf[--i]));
    return n;
}

size_t Print::print(uint32_t v) { return writeDec(v, false); }

// Signed prints: negate in a wider type to avoid signed overflow at the
// minimum value (e.g. -128 for int8_t).

size_t Print::print(int8_t v) {
    if (v < 0) {
        size_t n = write('-');
        return n + print(static_cast<uint8_t>(-static_cast<int16_t>(v)));
    }
    return print(static_cast<uint8_t>(v));
}

size_t Print::print(int16_t v) {
    if (v < 0) {
        size_t n = write('-');
        return n + print(static_cast<uint16_t>(-static_cast<int32_t>(v)));
    }
    return print(static_cast<uint16_t>(v));
}

size_t Print::print(int32_t v) {
    if (v < 0) {
        size_t n = write('-');
        return n + print(static_cast<uint32_t>(-static_cast<int64_t>(v)));
    }
    return print(static_cast<uint32_t>(v));
}

size_t Print::writeU64(uint64_t v) {
    if (v == 0) return write('0');
    char buf[20];                   // max 20 digits for uint64_t
    uint8_t i = 0;
    while (v) { buf[i++] = static_cast<char>('0' + (v % 10)); v /= 10; }
    size_t n = 0;
    while (i) n += write(static_cast<uint8_t>(buf[--i]));
    return n;
}

size_t Print::writeI64(int64_t v) {
    if (v < 0) {
        size_t n = write('-');
        // Negate in uint64_t to avoid overflow at INT64_MIN.
        return n + writeU64(static_cast<uint64_t>(-(v + 1)) + 1ULL);
    }
    return writeU64(static_cast<uint64_t>(v));
}

size_t Print::print(uint64_t v) { return writeU64(v); }
size_t Print::print(int64_t  v) { return writeI64(v); }

// println variants delegate to print + newline.

size_t Print::println(uint8_t  v) { return print(v) + println(); }
size_t Print::println(int8_t   v) { return print(v) + println(); }
size_t Print::println(uint16_t v) { return print(v) + println(); }
size_t Print::println(int16_t  v) { return print(v) + println(); }
size_t Print::println(uint32_t v) { return print(v) + println(); }
size_t Print::println(int32_t  v) { return print(v) + println(); }
size_t Print::println(uint64_t v) { return print(v) + println(); }
size_t Print::println(int64_t  v) { return print(v) + println(); }

// ===========================================================================
// Floats (optional)
// ===========================================================================

#if ENABLE_FLOAT_PRINT

size_t Print::writeFloat(float v, uint8_t decimals) {
    if (decimals > 6) decimals = 6;     // beyond 6 the scale overflows uint32

    size_t n = 0;

    if (v < 0.0f) { n += write('-'); v = -v; }

    // scale = 10^decimals
    uint32_t scale = 1;
    for (uint8_t i = 0; i < decimals; ++i) scale *= 10;

    // Round to nearest at the requested precision.
    uint32_t scaled = static_cast<uint32_t>(v * static_cast<float>(scale) + 0.5f);
    uint32_t int_part  = scaled / scale;
    uint32_t frac_part = scaled % scale;

    n += writeDec(int_part, false);

    if (decimals) {
        n += write('.');
        // Emit each fractional digit, most significant first, zero-padded.
        uint32_t divisor = scale / 10;
        while (divisor) {
            n += write(static_cast<uint8_t>('0' + ((frac_part / divisor) % 10)));
            divisor /= 10;
        }
    }
    return n;
}

size_t Print::print(float v, uint8_t decimals) {
    return writeFloat(v, decimals);
}

size_t Print::println(float v, uint8_t decimals) {
    return writeFloat(v, decimals) + println();
}

#endif  // ENABLE_FLOAT_PRINT

// ===========================================================================
// Hex / binary / octal
//
// Fixed-width hex and binary (leading zeros included); octal has no
// leading zeros. No prefixes are emitted. All output goes through
// write() one byte at a time.
// ===========================================================================

size_t Print::printHex(uint8_t v) {
    static const char hex[] PROGMEM = "0123456789ABCDEF";
    size_t n = 0;
    n += write(static_cast<uint8_t>(pgm_read_byte(&hex[(v >> 4) & 0x0F])));
    n += write(static_cast<uint8_t>(pgm_read_byte(&hex[v & 0x0F])));
    return n;
}

size_t Print::printHex(uint16_t v) {
    size_t n = printHex(static_cast<uint8_t>(v >> 8));
    n += printHex(static_cast<uint8_t>(v & 0xFF));
    return n;
}

size_t Print::printHex(uint32_t v) {
    size_t n = printHex(static_cast<uint16_t>(v >> 16));
    n += printHex(static_cast<uint16_t>(v & 0xFFFF));
    return n;
}

size_t Print::printBin(uint8_t v) {
    size_t n = 0;
    for (int8_t i = 7; i >= 0; --i) {
        n += write((v & (1u << i)) ? '1' : '0');
    }
    return n;
}

size_t Print::printBin(uint16_t v) {
    size_t n = 0;
    for (int8_t i = 15; i >= 0; --i) {
        n += write((v & (1u << i)) ? '1' : '0');
    }
    return n;
}

size_t Print::printBin(uint32_t v) {
    size_t n = 0;
    for (int8_t i = 31; i >= 0; --i) {
        n += write((v & (1UL << i)) ? '1' : '0');
    }
    return n;
}

size_t Print::printOct(uint32_t v) {
    if (v == 0) return write('0');
    char buf[11];                 // 32 bits -> at most 11 octal digits
    uint8_t i = 0;
    while (v) { buf[i++] = static_cast<char>('0' + (v & 7)); v >>= 3; }
    size_t n = 0;
    while (i) n += write(static_cast<uint8_t>(buf[--i]));
    return n;
}

// ===========================================================================
// printf / printf_P (optional)
//
// Strategy, following the Arduino / Teensy cores:
//
//   1. Set up an avr-libc FILE on the stack. This costs a few bytes of
//      stack per call, but it makes printf reentrant and needs no
//      global state.
//   2. Bind the FILE's user-data pointer to `this`, so the putchar hook
//      can recover the Print object.
//   3. Bind the FILE's putchar hook to _printfPutChar, which forwards
//      each character to write(uint8_t).
//   4. Call vfprintf (RAM format) or vfprintf_P (flash format).
//
// The FILE itself is uninitialized by design; fdev_setup_stream fills
// in every field it needs. We don't use fdev_setup_stream's read-side
// arguments because we're write-only.
//
// Caveat: avr-libc's vfprintf respects the project's link-time stdio
// configuration. By default it supports the common integer conversions
// but not `%f`; to enable float formatting you need `-lprintf_flt` and
// the appropriate `-Wl,-u,vfprintf`. See avr-libc's documentation for
// the -lprintf_min / -lprintf_flt variants.
// ===========================================================================

#if ENABLE_PRINTF

/**
 * @brief avr-libc FILE putchar hook. Forwards one character to the
 *        bound Print object.
 *
 * The FILE's user-data pointer is set by fdev_set_udata() in printf()
 * and printf_P() to point at the Print object. We recover it here and
 * call write(). The return value is ignored by vfprintf; we return 0
 * as the Arduino core does.
 *
 * @param c   Character to emit.
 * @param fp  FILE whose user-data is the target Print object.
 * @return 0 (the return value is not used by vfprintf).
 */
int Print::_printfPutChar(char c, FILE* fp) {
    Print* self = static_cast<Print*>(fdev_get_udata(fp));
    self->write(static_cast<uint8_t>(c));
    return 0;
}

int Print::printf(const char* format, ...) {
    FILE f;
    va_list ap;

    // Bind the FILE's write hook and user-data pointer. _FDEV_SETUP_WRITE
    // tells fdev_setup_stream this is a write-only stream (no read side).
    fdev_setup_stream(&f, _printfPutChar, nullptr, _FDEV_SETUP_WRITE);
    fdev_set_udata(&f, this);

    va_start(ap, format);
    int n = vfprintf(&f, format, ap);
    va_end(ap);

    return static_cast<int16_t>(n);
}

int Print::printf_P(const char* format, ...) {
    FILE f;
    va_list ap;

    fdev_setup_stream(&f, _printfPutChar, nullptr, _FDEV_SETUP_WRITE);
    fdev_set_udata(&f, this);

    va_start(ap, format);
    int n = vfprintf_P(&f, format, ap);
    va_end(ap);

    return static_cast<int16_t>(n);
}

#endif  // ENABLE_PRINTF