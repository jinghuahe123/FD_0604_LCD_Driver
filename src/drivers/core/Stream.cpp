/**
 * @file Stream.cpp
 * @brief Implementation of Stream.
 *
 * All parsing is built on top of the three pure virtuals
 * available()/read()/peek(), so any subclass that implements those gets
 * the full input API for free.
 *
 * No dynamic allocation, no stdlib conversions, no Arduino millis().
 */

#include "Stream.hpp"

// Default tick source: null. Callers that want timeout behaviour set
// Stream::tickSource to their own millis-equivalent.
uint32_t (*Stream::tickSource)() = nullptr;

// ===========================================================================
// Timed single-byte helpers
// ===========================================================================

int Stream::timedRead() {
    if (_timeout == 0 || tickSource == nullptr) {
        return read();                      // single non-blocking attempt
    }
    unsigned long start = tickSource();
    int c;
    do {
        c = read();
        if (c >= 0) return c;
    } while ((tickSource() - start) < _timeout);
    return -1;
}

int Stream::timedPeek() {
    if (_timeout == 0 || tickSource == nullptr) {
        return peek();
    }
    unsigned long start = tickSource();
    int c;
    do {
        c = peek();
        if (c >= 0) return c;
    } while ((tickSource() - start) < _timeout);
    return -1;
}

// ===========================================================================
// Scanners
// ===========================================================================

bool Stream::find(char target) {
    int c;
    while ((c = timedRead()) >= 0) {
        if (static_cast<char>(c) == target) return true;
    }
    return false;
}

bool Stream::findUntil(char target, char terminator) {
    int c;
    while ((c = timedRead()) >= 0) {
        char ch = static_cast<char>(c);
        if (ch == target)     return true;
        if (ch == terminator) return false;
    }
    return false;
}

// ===========================================================================
// Block readers
// ===========================================================================

size_t Stream::readBytes(char* buffer, size_t length) {
    size_t n = 0;
    while (n < length) {
        int c = timedRead();
        if (c < 0) break;
        buffer[n++] = static_cast<char>(c);
    }
    return n;
}

size_t Stream::readBytesUntil(char terminator, char* buffer, size_t length) {
    size_t n = 0;
    while (n < length) {
        int c = timedRead();
        if (c < 0) break;
        char ch = static_cast<char>(c);
        if (ch == terminator) break;
        buffer[n++] = ch;
    }
    return n;
}

uint8_t Stream::readStringUntil(char delimiter, char* buffer, uint8_t max_len) {
    if (max_len == 0) return 0;

    uint8_t count = 0;
    // Leave room for the terminator.
    while (count < (max_len - 1)) {
        int c = timedRead();
        if (c < 0) break;                // block until a byte arrives
        if (static_cast<char>(c) == delimiter) break;
        buffer[count++] = static_cast<char>(c);
    }
    buffer[count] = '\0';
    return count;
}

// ===========================================================================
// Numeric parsers
// ===========================================================================

// Shared skip logic: discard bytes until we see a digit, a sign, or a
// character that is neither a skip char nor a leading whitespace. The
// first "real" byte is returned via *firstByte, and the number of skipped
// bytes (for comment handling) is not tracked beyond returning bool.
namespace {

inline bool isDigitChar(char c) { return c >= '0' && c <= '9'; }

} // namespace

long Stream::parseInt() {
    return parseInt(0, '\0');
}

long Stream::parseInt(char skipChar, char isComment) {
    long value = 0;
    bool negative = false;
    bool haveDigits = false;
    bool inNumber = false;

    int c;
    while ((c = timedRead()) >= 0) {
        char ch = static_cast<char>(c);

        // Comment handling: consume to end of line.
        if (isComment != '\0' && ch == isComment) {
            do {
                c = timedRead();
                if (c < 0) return 0;
                ch = static_cast<char>(c);
            } while (ch != '\n' && ch != '\r');
            continue;
        }

        if (!inNumber) {
            if (ch == '-') {
                negative = true;
                inNumber = true;
                continue;
            }
            if (ch == '+') {
                inNumber = true;
                continue;
            }
            if (isDigitChar(ch)) {
                inNumber = true;
                haveDigits = true;
                value = ch - '0';
                continue;
            }
            // Not part of a number: skip if explicitly allowed, else stop.
            if (skipChar != '\0' && ch == skipChar) continue;
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
            break;                          // unknown leading char: stop
        }

        if (isDigitChar(ch)) {
            haveDigits = true;
            value = value * 10 + (ch - '0');
        } else {
            break;                          // end of number
        }
    }

    if (!haveDigits) return 0;
    return negative ? -value : value;
}

float Stream::parseFloat() {
    return parseFloat(0, '\0');
}

float Stream::parseFloat(char skipChar, char isComment) {
    bool negative = false;
    bool haveDigits = false;
    bool inNumber = false;
    bool inFraction = false;

    long intPart = 0;
    long fracPart = 0;
    uint8_t fracDigits = 0;

    int c;
    while ((c = timedRead()) >= 0) {
        char ch = static_cast<char>(c);

        if (isComment != '\0' && ch == isComment) {
            do {
                c = timedRead();
                if (c < 0) goto done;
                ch = static_cast<char>(c);
            } while (ch != '\n' && ch != '\r');
            continue;
        }

        if (!inNumber) {
            if (ch == '-') { negative = true; inNumber = true; continue; }
            if (ch == '+') { inNumber = true; continue; }
            if (isDigitChar(ch)) {
                inNumber = true;
                haveDigits = true;
                intPart = ch - '0';
                continue;
            }
            if (skipChar != '\0' && ch == skipChar) continue;
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
            break;
        }

        if (ch == '.' && !inFraction) {
            inFraction = true;
            continue;
        }

        if (isDigitChar(ch)) {
            haveDigits = true;
            if (inFraction) {
                if (fracDigits < 6) {
                    fracPart = fracPart * 10 + (ch - '0');
                    ++fracDigits;
                }
                // else: silently truncate beyond 6 places
            } else {
                intPart = intPart * 10 + (ch - '0');
            }
            continue;
        }

        break;                              // end of number
    }

done:
    if (!haveDigits) return 0.0f;

    float value = static_cast<float>(intPart);
    if (fracDigits) {
        float scale = 1.0f;
        for (uint8_t i = 0; i < fracDigits; ++i) scale *= 10.0f;
        value += static_cast<float>(fracPart) / scale;
    }
    return negative ? -value : value;
}