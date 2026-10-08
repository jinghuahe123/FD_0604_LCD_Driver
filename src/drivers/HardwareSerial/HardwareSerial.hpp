/**
 * @file HardwareSerial.hpp
 * @brief Interrupt-driven USART driver for AVR, exposed as a Print sink.
 *
 * Provides a `HardwareSerialT<Traits>` class template that binds to a
 * specific USART peripheral via a traits struct (register references +
 * bit names). Templating on traits means every register access compiles
 * to a direct LDS/STS — no per-instance register pointers, no RAM cost.
 *
 * All formatted output (print/println/printHex/printBin/printOct, all
 * integer widths, optional floats, flash strings) is inherited from the
 * Print base class. HardwareSerialT only implements the byte sink:
 * `write(uint8_t)` and `write(const uint8_t*, size_t)`.
 *
 * Supported targets:
 *   - ATmega328P : USART0 (UBRR0H/UCSR0A/..., vectors USART_*)
 *   - ATmega328PB: USART0 + USART1 (vectors USART0_* / USART1_*)
 *   - ATtiny85   : no USART — nothing in this file is compiled
 *
 * Design notes:
 *   - RX and TX are both ring-buffered. RX is filled by the RX-complete
 *     interrupt; TX is drained by the TX-empty (UDRE) interrupt, which is
 *     enabled lazily on the first queued byte and disabled once the TX
 *     buffer empties. An idle USART therefore generates no interrupts.
 *   - `write()` is non-blocking as long as the TX buffer has room. If the
 *     buffer is full, `write()` stops the UDRE interrupt and drains the
 *     ring synchronously, so no byte is ever lost or duplicated. Increase
 *     TX_BUFFER_SIZE if you need larger non-blocking bursts.
 *   - `write()` has a fast path: if the hardware is idle and the ring is
 *     empty, the byte goes straight to UDR, bypassing the ring and the
 *     UDRE interrupt entirely.
 *   - `_enqueueTx()` masks interrupts for its duration. This is required
 *     to prevent the UDRE ISR from racing the ring-tail update (which
 *     would re-send the byte currently being written).
 *   - `readStringUntil()` is non-blocking: it returns whatever is
 *     currently buffered and does not spin waiting for the delimiter.
 *   - Float printing is inherited from Print and gated by
 *     ENABLE_FLOAT_PRINT (see Print.hpp) so the AVR soft-float routines
 *     are only linked when actually used.
 */

#pragma once

#include "drivers/core/Stream.hpp"

#include "drivers/core/board_definitions.h"
#include "drivers/core/FlashStringHelper.hpp" // __FlashStringHelper

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

/**
 * @brief RX ring buffer size, in bytes.
 *
 * Must be a power of two in [2, 256]. The RX ISR fills this buffer;
 * `read()` and `available()` drain it. Larger = more burst tolerance at
 * the cost of RAM.
 */
#ifndef RX_BUFFER_SIZE
#define RX_BUFFER_SIZE 64
#endif

/**
 * @brief TX ring buffer size, in bytes.
 *
 * Must be a power of two in [2, 256]. `write()` / `print()` enqueue here;
 * the UDRE interrupt drains it. Larger = more non-blocking burst capacity.
 */
#ifndef TX_BUFFER_SIZE
#define TX_BUFFER_SIZE 64
#endif

static_assert((RX_BUFFER_SIZE & (RX_BUFFER_SIZE - 1)) == 0,
              "RX_BUFFER_SIZE must be a power of two");
static_assert((TX_BUFFER_SIZE & (TX_BUFFER_SIZE - 1)) == 0,
              "TX_BUFFER_SIZE must be a power of two");
static_assert(RX_BUFFER_SIZE >= 2 && RX_BUFFER_SIZE <= 256,
              "RX_BUFFER_SIZE out of range");
static_assert(TX_BUFFER_SIZE >= 2 && TX_BUFFER_SIZE <= 256,
              "TX_BUFFER_SIZE out of range");

/**
 * @brief Interrupt-driven USART driver.
 *
 * @tparam T Traits struct describing one USART peripheral. See
 *           Usart0Traits and Usart1Traits below. The traits supply
 *           register references, bit positions, and (via the .cpp) the
 *           matching vector handlers.
 *
 * The class is non-copyable: the ISRs service the one global instance,
 * so copying would produce a second object whose buffers the ISRs never
 * touch.
 *
 * Typical use:
 * @code
 *     void setup() {
 *         Serial.begin(9600);          // also enables global interrupts
 *         Serial.println(F("hello"));
 *         Serial.printHex(0xDEADBEEF);
 *     }
 *     void loop() {
 *         if (Serial.available()) {
 *             int c = Serial.read();
 *             Serial.write(static_cast<uint8_t>(c));
 *         }
 *     }
 * @endcode
 */
template <class T>
class HardwareSerialT : public Stream {
public:
    HardwareSerialT() = default;

    // Non-copyable: the ISRs service the one global instance.
    HardwareSerialT(const HardwareSerialT&) = delete;
    HardwareSerialT& operator=(const HardwareSerialT&) = delete;

    // =====================================================================
    // Lifecycle
    // =====================================================================

    /**
     * @brief Initialise the USART and enable RX/TX at the given baud rate.
     *
     * Configures 8N1 in double-speed mode (U2X = 1):
     *     UBRR = F_CPU / (8 * baud) - 1
     * Resets both ring buffers, then enables RX, TX, and the RX-complete
     * interrupt. The TX-empty interrupt is left disabled until the first
     * byte is queued.
     *
     * Calls `sei()` on exit so the RX/UDRE ISRs actually fire on a
     * bare-metal target (on reset, global interrupts are disabled).
     *
     * @param baud Desired baud rate (e.g. 9600, 115200). If the computed
     *             divisor overflows the 12-bit UBRR, it is clamped to
     *             4095.
     */
    void begin(uint32_t baud);

    /**
     * @brief Disable the USART and reset both ring buffers.
     *
     * Does not wait for pending TX to complete. Call flush() first if
     * you need the last bytes to go out.
     */
    void end();

    /**
     * @brief Wait until all queued TX has been sent and the shift register
     *        has emptied.
     *
     * Spins on the software TX buffer first (drained by the UDRE ISR),
     * then on the hardware TXC flag. Clears TXC before returning.
     */
    void flush();

    // =====================================================================
    // Print sink — the only functions HardwareSerialT must implement
    // =====================================================================

    /**
     * @brief Queue one byte for transmission.
     *
     * Non-blocking if the TX buffer has room; otherwise drains the ring
     * synchronously so nothing is lost or duplicated. Has a fast path:
     * if the hardware data register is empty and the ring is empty, the
     * byte goes straight to UDR, bypassing the ring and the UDRE
     * interrupt entirely.
     *
     * @param byte  The byte to transmit.
     * @return 1 (always succeeds from the caller's perspective).
     */
    size_t write(uint8_t byte) override;

    /**
     * @brief Transmit a buffer of bytes.
     *
     * Default loop over `write(uint8_t)`. On AVR this is usually optimal
     * because the single-byte fast path already handles idle hardware;
     * override only if you have a cheaper bulk path.
     *
     * @param buffer  Pointer to the bytes to transmit.
     * @param size    Number of bytes to transmit.
     * @return Total number of bytes queued.
     */
    size_t write(const uint8_t* buffer, size_t size) override;

    // =====================================================================
    // Input
    // =====================================================================

    /**
     * @brief Number of bytes currently available to read.
     * @return Count in [0, RX_BUFFER_SIZE).
     */
    int available() override;

    /**
     * @brief Read one byte from the RX buffer.
     * @return The byte (0..255), or -1 if the buffer is empty.
     *
     * Returns `int`, not `uint8_t`, so that 0x00 data is distinguishable
     * from "no data".
     */
    int read() override;

    /**
     * @brief Return the next byte without consuming it.
     * @return The byte (0..255), or -1 if the buffer is empty.
     */
    int peek() override;

    // =====================================================================
    // ISR hooks
    //
    // Called from the vector handlers at the bottom of HardwareSerial.cpp.
    // Public so the free ISR functions can reach them; not intended for
    // application use.
    // =====================================================================

    /** @brief RX-complete: move one byte from UDR into the RX ring. */
    void _rxISR();

    /** @brief TX-empty: send the next buffered byte, or disable UDRIE. */
    void _udreISR();

private:
    // Ring indices are uint8_t because both buffers are <= 256 bytes
    // (enforced by static_assert above). Masking wraps them branchlessly.
    // volatile because they are shared with the ISRs.
    volatile uint8_t rx_head = 0;
    volatile uint8_t rx_tail = 0;
    volatile uint8_t rx_buf[RX_BUFFER_SIZE];

    volatile uint8_t tx_head = 0;
    volatile uint8_t tx_tail = 0;
    volatile uint8_t tx_buf[TX_BUFFER_SIZE];

    /** @brief Write directly to UDR, spinning on UDRE. Used on TX overflow. */
    void _writeByteBlocking(uint8_t b);

    /**
     * @brief Fast path: write straight to UDR if the hardware is ready.
     * @return true if the byte was sent, false if the caller must buffer it.
     *
     * Non-blocking. Checks UDRE; if set and the TX ring is empty, writes
     * UDR and returns true. Otherwise returns false so the caller can
     * fall back to the ring buffer.
     */
    bool _writeByteDirect(uint8_t b);

    /**
     * @brief Enqueue one byte into the TX ring.
     *
     * Runs with interrupts masked so the UDRE ISR cannot race the ring-tail
     * update. On overflow, disables UDRIE and drains the ring synchronously.
     */
    void _enqueueTx(uint8_t b);
};

// ---------------------------------------------------------------------------
// Per-port traits.
//
// Each traits struct exposes:
//   - static volatile uint8_t& <reg>()  — one accessor per register
//   - static constexpr uint8_t <BIT>    — per-chip bit-position names
//
// The accessors return references to the real SFR, so the compiler emits a
// direct LDS/STS rather than an indirect load through a stored pointer.
// The bit-name constants exist because 328P and 328PB spell the same bits
// differently (e.g. UCSZ00 vs UCSZ0).
// ---------------------------------------------------------------------------

#if defined(UBRR0H) || defined(UBRRH)
/**
 * @brief Traits for USART0 (present on ATmega328P and ATmega328PB).
 */
struct Usart0Traits {
  #if defined(UBRR0H)
    // ATmega328PB (and most modern AVRs): suffixed register names.
    static volatile uint8_t& ubrrh() { return UBRR0H; }
    static volatile uint8_t& ubrrl() { return UBRR0L; }
    static volatile uint8_t& ucsra() { return UCSR0A; }
    static volatile uint8_t& ucsrb() { return UCSR0B; }
    static volatile uint8_t& ucsrc() { return UCSR0C; }
    static volatile uint8_t& udr()   { return UDR0;   }
    static constexpr uint8_t RXEN   = RXEN0;
    static constexpr uint8_t TXEN   = TXEN0;
    static constexpr uint8_t RXCIE  = RXCIE0;
    static constexpr uint8_t UDRIE  = UDRIE0;
    static constexpr uint8_t UDRE   = UDRE0;
    static constexpr uint8_t TXC    = TXC0;
    static constexpr uint8_t U2X    = U2X0;
    static constexpr uint8_t UCSZ0_ = UCSZ00;
    static constexpr uint8_t UCSZ1_ = UCSZ01;
  #else
    // Older AVRs that name USART0's registers without a suffix.
    static volatile uint8_t& ubrrh() { return UBRRH; }
    static volatile uint8_t& ubrrl() { return UBRRL; }
    static volatile uint8_t& ucsra() { return UCSRA; }
    static volatile uint8_t& ucsrb() { return UCSRB; }
    static volatile uint8_t& ucsrc() { return UCSRC; }
    static volatile uint8_t& udr()   { return UDR;   }
    static constexpr uint8_t RXEN   = RXEN;
    static constexpr uint8_t TXEN   = TXEN;
    static constexpr uint8_t RXCIE  = RXCIE;
    static constexpr uint8_t UDRIE  = UDRIE;
    static constexpr uint8_t UDRE   = UDRE;
    static constexpr uint8_t TXC    = TXC;
    static constexpr uint8_t U2X    = U2X;
    static constexpr uint8_t UCSZ0_ = UCSZ0;
    static constexpr uint8_t UCSZ1_ = UCSZ1;
  #endif
};
#endif

#if defined(UBRR1H)
/**
 * @brief Traits for USART1 (ATmega328PB only).
 */
struct Usart1Traits {
    static volatile uint8_t& ubrrh() { return UBRR1H; }
    static volatile uint8_t& ubrrl() { return UBRR1L; }
    static volatile uint8_t& ucsra() { return UCSR1A; }
    static volatile uint8_t& ucsrb() { return UCSR1B; }
    static volatile uint8_t& ucsrc() { return UCSR1C; }
    static volatile uint8_t& udr()   { return UDR1;   }
    static constexpr uint8_t RXEN   = RXEN1;
    static constexpr uint8_t TXEN   = TXEN1;
    static constexpr uint8_t RXCIE  = RXCIE1;
    static constexpr uint8_t UDRIE  = UDRIE1;
    static constexpr uint8_t UDRE   = UDRE1;
    static constexpr uint8_t TXC    = TXC1;
    static constexpr uint8_t U2X    = U2X1;
    static constexpr uint8_t UCSZ0_ = UCSZ10;
    static constexpr uint8_t UCSZ1_ = UCSZ11;
};
#endif

// ---------------------------------------------------------------------------
// Global instances.
//
// Only declared where the corresponding peripheral exists. On the ATtiny85
// neither macro is defined, so `Serial` simply does not exist — referring
// to it is a compile-time error, which is the intended behaviour.
// ---------------------------------------------------------------------------

#if defined(UBRR0H) || defined(UBRRH)
/** @brief The default hardware serial port. */
extern HardwareSerialT<Usart0Traits> Serial;
#endif

#if defined(UBRR1H)
/** @brief The second hardware serial port (ATmega328PB only). */
extern HardwareSerialT<Usart1Traits> Serial1;
#endif