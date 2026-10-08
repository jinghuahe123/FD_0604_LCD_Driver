/**
 * @file Stream.hpp
 * @brief Buffered-input extension of Print.
 *
 * Stream sits between Print (output formatting) and the concrete serial
 * sinks (HardwareSerialT, SoftwareSerial). It contributes the *input*
 * side: peek/read/available plus the parsing helpers that every serial
 * port wants but none should reimplement.
 *
 * Pure AVR, no Arduino runtime:
 *   - No String class. readStringUntil() writes into a caller-supplied
 *     buffer with a length cap, matching the signature already used by
 *     HardwareSerialT and SoftwareSerial.
 *   - No millis(). Timeout defaults to a caller-set tick count using
 *     whatever timebase you provide via setTimeout(); timedRead() uses
 *     it. If you don't call setTimeout(), the timeout is 0 and timedRead()
 *     is equivalent to read().
 *   - parseInt()/parseFloat() skip leading non-numeric characters, honour
 *     a leading '-', stop at the first character that can't extend the
 *     number, and (for parseFloat) call the caller-supplied "peek the
 *     next char" via peek(). No stdlib strtol/atof.
 *
 * Subclasses must implement:
 *   - size_t write(uint8_t)          (inherited from Print)
 *   - int    available()
 *   - int    read()
 *   - int    peek()
 *
 * Stream adds no data members, so adding it to the hierarchy costs no
 * RAM and one extra vtable slot per pure-virtual override.
 */

#pragma once

#include "Print.hpp"

#include <stdint.h>
#include <stddef.h>

class Stream : public Print {
public:
    // No virtual destructor, same reasoning as Print.

    // =====================================================================
    // Input primitives — subclasses must provide these
    // =====================================================================

    /** @brief Number of bytes available to read without blocking. */
    virtual int available() = 0;

    /** @brief Read one byte, or -1 if none is available. */
    virtual int read() = 0;

    /** @brief Return the next byte without consuming it, or -1. */
    virtual int peek() = 0;

    // =====================================================================
    // Timeout
    //
    // No millis() in this library, so "timeout" is whatever unit you feed
    // it. Set a function pointer to your own tick source if you want
    // wall-clock semantics, or leave it null and setTimeout() disables
    // timeout-based waiting.
    // =====================================================================

    /**
     * @brief Set the per-byte timeout for the timed readers.
     *
     * @param ms  Timeout in whatever unit your timebase uses. 0 means
     *            "no timeout" — timedRead() returns immediately if no
     *            byte is waiting.
     */
    void setTimeout(uint32_t ms) { _timeout = ms; }

    /** @brief Current timeout value. */
    uint32_t getTimeout() const { return _timeout; }

    /**
     * @brief Function pointer to the current-tick source used by timedRead.
     *
     * Set to your own millis-equivalent. If null, timedRead() degenerates
     * to read() (single non-blocking attempt).
     */
    static uint32_t (*tickSource)();

    // =====================================================================
    // Timed single-byte helpers
    // =====================================================================

    /**
     * @brief Read one byte, waiting up to _timeout ticks for it.
     * @return The byte (0..255), or -1 on timeout.
     */
    int timedRead();

    /**
     * @brief Peek the next byte, waiting up to _timeout ticks.
     * @return The byte (0..255), or -1 on timeout.
     */
    int timedPeek();

    // =====================================================================
    // Scanners
    // =====================================================================

    /**
     * @brief Consume input until @p target is found.
     *
     * Reads and discards bytes until a byte equal to @p target is seen
     * (and consumed), or the timeout expires. Respects _timeout.
     *
     * @param target  Byte to look for.
     * @return true if @p target was found and consumed, false on timeout.
     */
    bool find(char target);

    /**
     * @brief Consume input until @p target is found or @p terminator is hit.
     *
     * Like find(), but stops early (and returns false) if @p terminator
     * is seen first. The terminator is consumed.
     *
     * @param target      Byte to look for.
     * @param terminator  Byte that aborts the search.
     * @return true if @p target was found, false on terminator or timeout.
     */
    bool findUntil(char target, char terminator);

    // =====================================================================
    // Block readers
    // =====================================================================

    /**
     * @brief Read bytes into @p buffer until it holds @p length bytes or
     *        the timeout expires.
     *
     * @param buffer  Destination.
     * @param length  Exact number of bytes to read.
     * @return Number of bytes actually stored (== length on success).
     */
    size_t readBytes(char* buffer, size_t length);

    /**
     * @brief Read bytes into @p buffer until @p terminator or @p length.
     *
     * The terminator is consumed but not stored. Stops early on timeout.
     *
     * @param buffer       Destination.
     * @param length       Capacity of @p buffer.
     * @param terminator   Byte that ends the read.
     * @return Number of bytes stored (excluding the terminator).
     */
    size_t readBytesUntil(char terminator, char* buffer, size_t length);

    /**
     * @brief Read bytes into @p buffer until @p delimiter or capacity-1.
     *
     * Always NUL-terminates. Same contract as the readStringUntil() that
     * HardwareSerialT and SoftwareSerial already expose; provided here so
     * every Stream has it.
     *
     * @param delimiter  Byte that ends the read (consumed, not stored).
     * @param buffer     Destination; must have room for @p max_len bytes.
     * @param max_len    Capacity including the NUL.
     * @return Number of data bytes stored (excluding the NUL).
     */
    uint8_t readStringUntil(char delimiter, char* buffer, uint8_t max_len);

    // =====================================================================
    // Numeric parsers
    //
    // Both skip leading non-numeric characters (whitespace and any other
    // non-digit/non-sign byte), honour a single leading '-', and stop at
    // the first character that cannot extend the number. That character
    // is left in the input (i.e. peek() still returns it).
    // =====================================================================

    /**
     * @brief Parse a signed long from the stream.
     *
     * Leading non-numeric bytes are discarded. A leading '-' or '+' is
     * honoured. Parsing stops at the first non-digit. If no digit is
     * found before the timeout, returns 0.
     *
     * @return Parsed value, or 0 if nothing parseable arrived.
     */
    long parseInt();

    /**
     * @brief Parse a signed long with an explicit stream terminator.
     *
     * @param skipChar  Byte treated as a leading skip character (e.g. ','
     *                  or ' '). Pass 0 for default whitespace-only skip.
     * @param isComment Pass '\0' for no comment handling. Otherwise, a
     *                  byte equal to this value starts a comment that runs
     *                  to end-of-line and is discarded.
     * @return Parsed value, or 0 if nothing parseable arrived.
     */
    long parseInt(char skipChar, char isComment);

    /**
     * @brief Parse a float from the stream.
     *
     * Same leading-skip and sign rules as parseInt(); additionally accepts
     * a decimal point and (for the fractional digits) up to 6 places.
     * Does not accept exponent notation.
     *
     * @return Parsed value, or 0.0f if nothing parseable arrived.
     */
    float parseFloat();

    /**
     * @brief Parse a float with explicit terminator handling.
     * @see parseInt(char, char)
     */
    float parseFloat(char skipChar, char isComment);

protected:
    unsigned long _timeout = 0;
};