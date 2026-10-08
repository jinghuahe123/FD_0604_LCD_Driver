/**
 * @file SoftwareSerial.hpp
 * @brief Interrupt-driven software serial for AVR, exposed as a Print sink.
 *
 * A pure-AVR C++ port of the Arduino SoftwareSerial library (originally
 * by ladyada, Mikal Hart, Paul Stoffregen, Garrett Mace and others).
 * No Arduino runtime is required: pin configuration goes through
 * HardwarePin / avr_pins.h, and the receive path is driven directly by
 * the PCINT vectors.
 *
 * All formatted output (print/println/printHex/printBin/printOct, all
 * integer widths, optional floats, flash strings) is inherited from the
 * Print base class. SoftwareSerial only implements the byte sink:
 * `write(uint8_t)`, which bit-bangs the byte.
 *
 * -----------------------------------------------------------------------
 * Design constraints (read before using)
 * -----------------------------------------------------------------------
 *  - TX is *blocking*: bit-banging a UART on the same MCU that must
 *    service an RX interrupt means interrupts are disabled for the
 *    duration of each byte. A byte at 9600 baud takes ~1.04 ms during
 *    which no other ISR can run. This is inherent to software serial,
 *    not a limitation of this port.
 *
 *  - RX is interrupt-driven but *single-instance*: only one SoftwareSerial
 *    object can be "listening" at a time (see listen()). This is because
 *    all SoftwareSerial objects share the same PCINT vectors.
 *
 *  - The RX buffer is small (default 64 bytes) because the ISR runs on
 *    every start-bit edge. If the buffer fills, the newest byte is
 *    dropped and overflow() reports it.
 *
 *  - Baud rates are fixed by a PROGMEM delay table tuned per F_CPU
 *    (8, 16, or 20 MHz). Unsupported baud rates cause begin() to return
 *    without configuring anything.
 *
 *  - Float printing is optional and controlled by ENABLE_FLOAT_PRINT,
 *    the same macro used by HardwareSerialT and Print. Setting it to 0
 *    in the build system disables float printing in all three at once.
 *
 * -----------------------------------------------------------------------
 * Example
 * -----------------------------------------------------------------------
 * @code
 *     SoftwareSerial ss(2, 3);           // RX on D2, TX on D3
 *
 *     void setup() {
 *         ss.begin(9600);
 *         ss.println(F("Hello from software serial"));
 *         ss.print(3.14159f, 3);         // requires ENABLE_FLOAT_PRINT
 *     }
 *
 *     void loop() {
 *         if (ss.available()) {
 *             ss.write((uint8_t)ss.read());   // echo
 *         }
 *     }
 * @endcode
 */

#pragma once

#include "drivers/core/Stream.hpp"
#include "drivers/HardwarePin/HardwarePin.hpp"               ///< Pin::Mode, HardwarePin
#include "drivers/core/avr_pins.h"             ///< AVR_Port_t, avr_* helpers
#include "drivers/core/FlashStringHelper.hpp"  ///< __FlashStringHelper

#include <stdint.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>


// ---------------------------------------------------------------------------
// Buffer size
// ---------------------------------------------------------------------------

/**
 * @brief RX ring buffer size, in bytes.
 *
 * Must be a power of two: the ISR and the reader use `(idx + 1) & (N-1)`
 * to advance the ring pointer without a branch or a modulo.
 *
 * Smaller = less RAM. 64 bytes is enough for ~66 ms at 9600 baud, which
 * is plenty for typical command-and-response protocols.
 *
 * Override by defining _SS_MAX_RX_BUFF before including this header, or
 * via the build system's `-D` flags.
 */
#ifndef _SS_MAX_RX_BUFF
#define _SS_MAX_RX_BUFF 64
#endif

static_assert(_SS_MAX_RX_BUFF >= 2, "RX buffer too small");
static_assert((_SS_MAX_RX_BUFF & (_SS_MAX_RX_BUFF - 1)) == 0,
              "_SS_MAX_RX_BUFF must be a power of two");


// ---------------------------------------------------------------------------
// Float printing
// ---------------------------------------------------------------------------

// ENABLE_FLOAT_PRINT is defined in Print.hpp (with an #ifndef guard) so
// that Print, HardwareSerialT and SoftwareSerial all agree on the value.
// See Print.hpp for the rationale.


// ---------------------------------------------------------------------------
// Timing constants
// ---------------------------------------------------------------------------

/**
 * @brief Extra delay added to the TX start bit.
 *
 * The start bit must be exactly one bit-time long. In practice there is a
 * small, fixed latency between writing the start bit and the first call
 * to tunedDelay() returning, so the delay table's tx_delay for the start
 * bit is augmented by this constant. The value depends on F_CPU because
 * the loop iteration cost scales with the clock.
 *
 * These values are verbatim from the original Arduino SoftwareSerial
 * library; they have been tuned empirically and should not be changed
 * without re-measuring on a scope.
 */
#if F_CPU == 16000000
  #define SS_XMIT_START_ADJUSTMENT 5
#elif F_CPU == 8000000
  #define SS_XMIT_START_ADJUSTMENT 4
#elif F_CPU == 20000000
  #define SS_XMIT_START_ADJUSTMENT 6
#else
  #error "SoftwareSerial supports F_CPU = 8, 16 or 20 MHz"
#endif


// ---------------------------------------------------------------------------
// SoftwareSerial
// ---------------------------------------------------------------------------

/**
 * @brief Interrupt-driven software serial port.
 *
 * Non-copyable: the ISR services a single global instance (active_object_),
 * so copying would produce a second object whose buffers the ISR never
 * touches.
 *
 * Typical use: construct once at global scope, call begin(baud) in setup(),
 * then use read/write/print/println. Only one instance may listen() at a
 * time; if you need two software serial ports, alternate between them with
 * listen().
 *
 * Because SoftwareSerial inherits from Print, any function that accepts
 * `Print&` can be handed a SoftwareSerial, a HardwareSerialT, or anything
 * else that implements write(uint8_t):
 * @code
 *     void logLine(Print& out, int value) {
 *         out.print(F("value = "));
 *         out.println(value);
 *     }
 *     logLine(Serial, 42);
 *     logLine(ss, 42);
 * @endcode
 */
class SoftwareSerial : public Stream {
public:
    /**
     * @brief Construct a software serial port bound to two pins.
     *
     * Sets up the pins immediately (TX as output-idle-high, RX as
     * input-with-pullup unless inverse_logic). The baud rate is not set
     * until begin() is called.
     *
     * @param rxPin         Digital pin number to receive on.
     * @param txPin         Digital pin number to transmit on.
     * @param inverse_logic If true, the line is inverted: idle-low, start
     *                      bit is high. Used for RS-232 level converters
     *                      without an inverting buffer, or some single-wire
     *                      buses. Default false (standard UART polarity).
     */
    SoftwareSerial(uint8_t rxPin, uint8_t txPin, bool inverse_logic = false);

    /** @brief Detach from the PCINT. Does not wait for pending TX. */
    ~SoftwareSerial();

    SoftwareSerial(const SoftwareSerial&) = delete;
    SoftwareSerial& operator=(const SoftwareSerial&) = delete;

    // =====================================================================
    // Lifecycle
    // =====================================================================

    /**
     * @brief Configure the port for the given baud rate.
     *
     * Looks up the delay values for @p baud in the PROGMEM delay table,
     * attaches the pin-change interrupt to the RX pin, and makes this
     * object the active listener.
     *
     * If @p baud is not in the table (which is variant/F_CPU specific),
     * this function returns without configuring anything: subsequent
     * write() calls are no-ops and available() returns 0.
     *
     * @param baud Desired baud rate (e.g. 9600, 57600, 115200).
     */
    void begin(uint32_t baud);

    /**
     * @brief Detach the RX pin-change interrupt and clear the active
     *        listener slot if this object held it.
     *
     * Does not reset the TX pin state or the delay values, so a following
     * begin() with the same baud is cheap.
     */
    void end();

    /**
     * @brief Claim the receive path for this object.
     *
     * Because all SoftwareSerial objects share the same PCINT vectors,
     * only one object can own the receive path at a time. Calling
     * listen() clears the RX buffer and makes this object the active
     * listener, displacing any previous one.
     *
     * @return true if this object replaced a different active listener,
     *         false if it was already the active listener.
     */
    bool listen();

    /** @brief True if this object currently owns the receive path. */
    bool isListening() const { return this == active_object_; }

    /**
     * @brief Test-and-clear the RX overflow flag.
     *
     * The flag is set when the ISR tried to store a byte but the ring
     * buffer was full. The byte was dropped. Reading the flag clears it,
     * so a single call reports "has overflowed since last checked".
     *
     * @return true if the buffer overflowed since the last call.
     */
    bool overflow();

    // =====================================================================
    // Print sink — the only function SoftwareSerial must implement
    // =====================================================================

    /**
     * @brief Transmit one byte.
     *
     * Blocks for the duration of the byte (start + 8 data + stop bits),
     * with interrupts disabled. See the class-level notes on why TX
     * cannot be non-blocking for software serial.
     *
     * This is the override that the Print base class dispatches to for
     * every character of every print()/println()/printHex()/... call.
     *
     * @param byte  The byte to transmit.
     * @return 1 (always succeeds from the caller's perspective).
     */
    size_t write(uint8_t byte) override;

    /**
     * @brief No-op, kept for API symmetry with HardwareSerialT.
     *
     * Bit-banged TX is synchronous: by the time write() returns, the byte
     * has left the wire. There is nothing to flush.
     */
    void flush();

    // =====================================================================
    // Input
    // =====================================================================

    /**
     * @brief Number of bytes waiting in the RX ring buffer.
     * @return Count in [0, _SS_MAX_RX_BUFF - 1], or 0 if not listening.
     */
    int available() override;

    /**
     * @brief Remove and return one byte from the RX ring buffer.
     * @return The byte (0..255), or -1 if the buffer is empty or this
     *         object is not listening.
     *
     * Returns `int` (not `uint8_t`) so that 0x00 data is distinguishable
     * from "no data".
     */
    int read() override;

    /**
     * @brief Return the next byte without removing it.
     * @return The byte (0..255), or -1 if empty / not listening.
     */
    int peek() override;

    // =====================================================================
    // ISR hooks
    //
    // Public so the free PCINT vector handlers can reach them. Not part
    // of the application-facing API.
    // =====================================================================

    /**
     * @brief Sample one byte from the RX line.
     *
     * Called from the PCINT ISR. Reads the current pin level, and if it
     * matches the expected start-bit polarity, samples 8 data bits at
     * bit-centre points and pushes the result into the RX ring buffer.
     *
     * The timing relies on the per-baud delay values configured by
     * begin(). It runs with interrupts disabled for the whole byte
     * (roughly one bit-time budget of slack before the next edge).
     *
     * The RX PCINT for this pin is disabled for the duration of the
     * sampling, so that edges within the byte being received do not
     * re-enter this routine and corrupt the timing. It is re-enabled
     * before returning.
     */
    void recvISR();

    /**
     * @brief Dispatch to the active listener's recvISR(), if any.
     *
     * Static because all four PCINT vectors share this entry point; the
     * per-object work happens via active_object_.
     */
    static void handleInterrupt();

private:
    // ---- Setup ----------------------------------------------------------
    // Called from the constructor. setTX/setRX use HardwarePin for the
    // initial mode/pullup configuration, then cache the raw PORTx/PINx
    // pointer and bit mask so the ISR and TX waveform can bypass the
    // abstraction's per-call overhead.

    /** @brief Configure the TX pin and cache its PORTx / mask. */
    void setTX(uint8_t txPin);

    /** @brief Configure the RX pin and cache its PINx / mask. */
    void setRX(uint8_t rxPin);

    // ---- Hot-path helpers ----------------------------------------------
    // Deliberately defined inline here, not in the .cpp: they run in the
    // ISR and the TX waveform loop where a function call's overhead is
    // unacceptable. They assume the cached pointers are non-null, which
    // setTX/setRX guarantee for any pin accepted by avr_digital_to_pin.

    /**
     * @brief Read the current logic level on the RX pin.
     *
     * Bitwise AND is used rather than a comparison so the result is the
     * raw masked value (0 or the mask), which the caller tests for
     * truthiness.
     */
    uint8_t rxPinRead() const {
        return *_receivePortRegister & _receiveBitMask;
    }

    /**
     * @brief Drive the TX pin to a specific logic level.
     *
     * Uses the bit-set / bit-clear atomic instructions (SBI/CBI) the
     * compiler emits for `PORTx |= mask` and `PORTx &= ~mask` when the
     * mask is a compile-time constant. Both forms are single-instruction
     * on AVR, with no read-modify-write hazard.
     */
    void txPinWrite(uint8_t state) {
        if (state == 0) *_transmitPortRegister &= ~_transmitBitMask;
        else            *_transmitPortRegister |=  _transmitBitMask;
    }

    /**
     * @brief Busy-wait for approximately @p delay loop iterations.
     *
     * The loop body is hand-tuned assembly (sbiw / brne) so the delay is
     * predictable to the cycle. This is what makes software serial work:
     * every delay in the state machine is a multiple of this primitive,
     * and the per-baud constants in the PROGMEM table were measured to
     * make those multiples land on bit-centre sample points.
     *
     * Verbatim from the original Arduino SoftwareSerial library.
     */
    static inline void tunedDelay(uint16_t delay);

    // ---- Output helpers -------------------------------------------------
    //
    // The formatting helpers (writeDec, writeU64, writeI64, writeFloat)
    // and every print()/println() overload live in Print. SoftwareSerial
    // only provides the byte-level write() they all call.

    /**
     * @brief Transmit a single byte with the configured polarity.
     *
     * This is the actual bit-banger: start bit, 8 data bits (LSB first),
     * stop bit. Interrupts are disabled for the duration so a PCINT
     * cannot corrupt the waveform, and re-enabled afterwards. If the
     * baud rate was not configured (tx_delay == 0), this is a no-op.
     */
    void _enqueueByte(uint8_t b);

    // ---- Per-instance data ---------------------------------------------
    // All cached for the ISR hot path. Initialized by setRX/setTX in the
    // constructor's initializer list / body.

    uint8_t  _receivePin;                   ///< Digital pin number for RX
    uint8_t  _receiveBitMask;               ///< Bit mask within PINx
    volatile uint8_t* _receivePortRegister; ///< Cached &PINx

    uint8_t  _transmitBitMask;              ///< Bit mask within PORTx
    volatile uint8_t* _transmitPortRegister;///< Cached &PORTx

    uint16_t _rx_delay_centering;           ///< Ticks to start-bit centre
    uint16_t _rx_delay_intrabit;            ///< Ticks between data-bit samples
    uint16_t _rx_delay_stopbit;             ///< Ticks to consume the stop bit
    uint16_t _tx_delay;                     ///< Ticks per transmitted bit

    bool _inverse_logic     : 1;            ///< Line is inverted (RS-232 style)
    bool _buffer_overflow   : 1;            ///< Set by the ISR on a full buffer

    // ---- Static (shared) state -----------------------------------------
    // Shared across all instances because the PCINT vectors are global.
    // volatile because the ISR reads/writes them concurrently with the
    // main loop.

    static char             _receive_buffer[_SS_MAX_RX_BUFF];
    static volatile uint8_t _receive_buffer_tail;   ///< ISR write index
    static volatile uint8_t _receive_buffer_head;   ///< Reader drain index
    static SoftwareSerial*  active_object_;         ///< Current listener
};