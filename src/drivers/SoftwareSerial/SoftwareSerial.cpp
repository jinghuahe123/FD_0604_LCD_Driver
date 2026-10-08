/**
 * @file SoftwareSerial.cpp
 * @brief Implementation of SoftwareSerial.
 *
 * Structure of this file, in order:
 *   1. PROGMEM delay table  — per-F_CPU timing constants
 *   2. Static storage       — the shared RX ring and active-listener pointer
 *   3. Construction         — pin setup via HardwarePin + avr_pins
 *   4. tunedDelay           — the cycle-accurate busy-wait primitive
 *   5. begin() / end()      — PCINT wiring and delay lookup
 *   6. listen() / overflow()— single-listener arbitration
 *   7. Receive ISR          — start-bit detection and bit sampling
 *   8. PCINT vector handlers— dispatch to the active listener
 *   9. Transmit             — bit-banged TX with interrupts masked
 *  10. Input                — read / peek / available / readStringUntil
 *
 * Output formatting (print/println/printHex/printBin/printOct, integer
 * and float paths) lives in Print.cpp and reaches the wire through the
 * virtual write() override below.
 *
 * The timing-critical parts (tunedDelay, recvISR, _enqueueByte, and the
 * delay table) are deliberately kept verbatim from the original Arduino
 * SoftwareSerial library, because the constants in the table are tuned
 * to the exact cycle counts of those functions. Changing the code without
 * re-measuring on a scope will break the timing.
 *
 * Efficiency notes for the integer paths (now in Print.cpp):
 *   - print(uint8_t) and print(uint16_t) are hand-written so the compiler
 *     turns the small-type divisions into multiply-shifts rather than
 *     linking __udivmodsi4.
 *   - print(uint32_t) uses a reverse-generation loop; the divisions are
 *     still 32-bit but the value is bounded by the input.
 *   - print(uint64_t) has a fast path for values that fit in 32 bits,
 *     which is the overwhelmingly common case in embedded code. Only
 *     genuinely large values fall through to __udivmoddi4, which costs
 *     ~200-300 bytes of flash and ~1000 cycles per digit.
 */

#include "SoftwareSerial.hpp"


// ---------------------------------------------------------------------------
// 1. PROGMEM delay table
//
// Each row gives the per-baud loop counts for one delay site in the state
// machine. The values were derived empirically for the tunedDelay()
// implementation below, at the specific F_CPU of the section.
//
// The table is PROGMEM-resident because it is only read once per begin()
// call and would otherwise waste 60+ bytes of precious SRAM.
// ---------------------------------------------------------------------------

/**
 * @brief One row of the per-F_CPU baud-rate delay table.
 *
 * All fields are loop counts for tunedDelay(), not microseconds. The
 * mapping from loop count to wall-clock time depends on the compiler's
 * code generation for tunedDelay(), which is why these are hardcoded
 * rather than computed at runtime.
 */
struct DelayTable {
    uint32_t baud;                ///< The baud rate this row applies to
    uint16_t rx_delay_centering;  ///< Start-bit → first data-bit centre
    uint16_t rx_delay_intrabit;   ///< Data-bit centre → next data-bit centre
    uint16_t rx_delay_stopbit;    ///< Last data-bit centre → end of stop bit
    uint16_t tx_delay;            ///< Loop count per transmitted bit
};

#if F_CPU == 16000000
static const DelayTable PROGMEM ss_table[] = {
    //  baud    rxcenter   rxintra    rxstop    tx
    { 115200,   1,         17,        17,       12    },
    { 57600,    10,        37,        37,       33    },
    { 38400,    25,        57,        57,       54    },
    { 31250,    31,        70,        70,       68    },
    { 28800,    34,        77,        77,       74    },
    { 19200,    54,        117,       117,      114   },
    { 14400,    74,        156,       156,      153   },
    { 9600,     114,       236,       236,      233   },
    { 4800,     233,       474,       474,      471   },
    { 2400,     471,       950,       950,      947   },
    { 1200,     947,       1902,      1902,     1899  },
    { 600,      1902,      3804,      3804,     3800  },
    { 300,      3804,      7617,      7617,     7614  },
};
#elif F_CPU == 8000000
static const DelayTable PROGMEM ss_table[] = {
    //  baud    rxcenter    rxintra    rxstop   tx
    { 115200,   1,          5,         5,       3     },
    { 57600,    1,          15,        15,      13    },
    { 38400,    2,          25,        26,      23    },
    { 31250,    7,          32,        33,      29    },
    { 28800,    11,         35,        35,      32    },
    { 19200,    20,         55,        55,      52    },
    { 14400,    30,         75,        75,      72    },
    { 9600,     50,         114,       114,     112   },
    { 4800,     110,        233,       233,     230   },
    { 2400,     229,        472,       472,     469   },
    { 1200,     467,        948,       948,     945   },
    { 600,      948,        1895,      1895,    1890  },
    { 300,      1895,       3805,      3805,    3802  },
};
#elif F_CPU == 20000000
static const DelayTable PROGMEM ss_table[] = {
    //  baud    rxcenter    rxintra    rxstop   tx
    { 115200,   3,          21,        21,      18    },
    { 57600,    20,         43,        43,      41    },
    { 38400,    37,         73,        73,      70    },
    { 31250,    45,         89,        89,      88    },
    { 28800,    46,         98,        98,      95    },
    { 19200,    71,         148,       148,     145   },
    { 14400,    96,         197,       197,     194   },
    { 9600,     146,        297,       297,     294   },
    { 4800,     296,        595,       595,     592   },
    { 2400,     592,        1189,      1189,    1186  },
    { 1200,     1187,       2379,      2379,    2376  },
    { 600,      2379,       4759,      4759,    4755  },
    { 300,      4759,       9523,      9523,    9520  },
};
#endif


// ---------------------------------------------------------------------------
// 2. Static storage
//
// All of these are shared across instances because the PCINT vectors are
// global. Only one SoftwareSerial can be the active listener at a time;
// that one owns the buffer.
// ---------------------------------------------------------------------------

/** @brief Shared RX ring buffer. Owned by whichever object is listening. */
char             SoftwareSerial::_receive_buffer[_SS_MAX_RX_BUFF];

/** @brief ISR write index. Advanced by recvISR(), reset by listen(). */
volatile uint8_t SoftwareSerial::_receive_buffer_tail = 0;

/** @brief Reader drain index. Advanced by read(), reset by listen(). */
volatile uint8_t SoftwareSerial::_receive_buffer_head = 0;

/** @brief The single object currently entitled to receive. */
SoftwareSerial*  SoftwareSerial::active_object_ = nullptr;


// ---------------------------------------------------------------------------
// 3. Construction / destruction
// ---------------------------------------------------------------------------

/**
 * @brief Construct and immediately configure both pins.
 *
 * The initializer list zeroes every cached pointer and delay so a
 * subsequent begin() has a clean starting state. The body then calls
 * setTX() and setRX() to cache the port pointers.
 */
SoftwareSerial::SoftwareSerial(uint8_t rxPin, uint8_t txPin, bool inverse_logic)
    : _receivePin(rxPin),
      _receiveBitMask(0),
      _receivePortRegister(nullptr),
      _transmitBitMask(0),
      _transmitPortRegister(nullptr),
      _rx_delay_centering(0),
      _rx_delay_intrabit(0),
      _rx_delay_stopbit(0),
      _tx_delay(0),
      _inverse_logic(inverse_logic),
      _buffer_overflow(false)
{
    setTX(txPin);
    setRX(rxPin);
}

/** @brief Detach from the PCINT, if still attached. */
SoftwareSerial::~SoftwareSerial() { end(); }


// ---------------------------------------------------------------------------
// 4. Pin setup
// ---------------------------------------------------------------------------

/**
 * @brief Configure the TX pin and cache its PORTx pointer and bit mask.
 *
 * The initial mode/write uses HardwarePin so the "what does OUTPUT mean"
 * question is answered in exactly one place (HardwarePin::setMode). We
 * then reach past that abstraction to cache the raw register pointer:
 * txPinWrite() runs once per bit inside the waveform loop, where a
 * function call would cost more cycles than we have.
 *
 * If @p txPin is not a valid digital pin on this variant,
 * avr_digital_to_pin() returns false and the cached pointer stays null.
 * txPinWrite() would then dereference null — but the constructor does
 * not enforce validity, so it is the caller's responsibility to pass a
 * pin that exists.
 */
void SoftwareSerial::setTX(uint8_t txPin) {
    HardwarePin pin(txPin);
    // preload port=1 whilst still an input (enables pullpup on some MCUs)
    pin.digitalWrite(true);                 // idle high
    pin.setMode(HardwarePin::Mode::OUTPUT);

    AVR_Port_t port;
    uint8_t    bit;
    if (avr_digital_to_pin(txPin, &port, &bit)) {
        _transmitBitMask       = avr_get_pin_mask(bit);
        _transmitPortRegister  = avr_get_port_reg(port);
    }
}

/**
 * @brief Configure the RX pin and cache its PINx pointer and bit mask.
 *
 * Note the pull-up: for normal (non-inverted) logic, the RX line idles
 * high, so we enable the internal pull-up to keep it defined when the
 * driver is disconnected. For inverted logic (idle-low), a pull-up would
 * fight the idle state, so we leave the pin floating (INPUT only).
 *
 * Same caching rationale as setTX().
 */
void SoftwareSerial::setRX(uint8_t rxPin) {
    HardwarePin pin(rxPin);
    pin.setMode(_inverse_logic ? HardwarePin::Mode::INPUT
                               : HardwarePin::Mode::INPUT_PULLUP);

    AVR_Port_t port;
    uint8_t    bit;
    if (avr_digital_to_pin(rxPin, &port, &bit)) {
        _receivePin           = rxPin;
        _receiveBitMask       = avr_get_pin_mask(bit);
        _receivePortRegister  = avr_get_pin_reg(port);
    }
}


// ---------------------------------------------------------------------------
// 5. tunedDelay — the cycle-accurate busy-wait
// ---------------------------------------------------------------------------

/**
 * @brief Busy-wait for @p delay iterations of a four-instruction loop.
 *
 * The loop body:
 *     sbiw    r24, 1        ; decrement the 16-bit counter
 *     ldi     rXX, 0xFF     ; load a temporary (used by the cpi/cpc pair)
 *     cpi     r24, 0xFF     ; low byte != 0xFF ?
 *     cpc     r25, rXX      ; and high byte == 0xFF ?
 *     brne    .-10          ; loop while (counter-1) != 0
 *
 * The `cpi`/`cpc` pair is a 16-bit compare-against-0xFF that accounts for
 * the sbiw wrap: after sbiw, the counter is zero and the Z flag would
 * normally branch on the first iteration. Comparing against 0xFF for
 * both bytes instead tests for "wrapped to zero", which is what we want.
 *
 * The asm block uses "+r" (r24:r25) and "+a" (r16) so the compiler keeps
 * the operands in registers across the whole block — no reloads, no
 * spills, deterministic cycle count.
 *
 * Cost: 4 cycles per iteration on a 1-cycle-per-instruction AVR (most
 * modern parts). A delay of 0 still executes the loop once.
 *
 * Verbatim from the original Arduino SoftwareSerial library.
 */
inline void SoftwareSerial::tunedDelay(uint16_t delay) {
    // Pin the operand to r24:r25, which is one of the four register
    // pairs that sbiw can address. Without this, -O2 may allocate any
    // pair and avr-as rejects the instruction.
    register uint16_t delay_reg asm("r24") = delay;
    uint8_t tmp = 0;
    asm volatile(
        "sbiw    %0, 0x01 \n\t"
        "ldi     %1, 0xFF \n\t"
        "cpi     %A0, 0xFF \n\t"
        "cpc     %B0, %1 \n\t"
        "brne    .-10     \n\t"
        : "+r" (delay_reg), "+a" (tmp)
        : "0"  (delay_reg)
    );
}


// ---------------------------------------------------------------------------
// 6. begin() / end()
// ---------------------------------------------------------------------------

/**
 * @brief Look up the delay values for @p baud, wire the PCINT, listen.
 *
 * The PCINT helpers from avr_pins.c handle the port → group mapping and
 * return the right register/bit for either the 328P (PCICR / PCMSK0..2,
 * PCIE0..2) or the TinyX5 (GIMSK / PCMSK, PCIE). If the pin has no PCINT
 * capability (e.g. it is not on a PCINT-capable port), the helpers
 * return NULL and we skip the wiring — the port would be TX-only.
 *
 * The final tunedDelay(_tx_delay) is a small settling wait: if TX was
 * left low by a previous transmission, this gives the line time to
 * return to its idle level before we start listening.
 */ 
#include <util/delay.h>
void SoftwareSerial::begin(uint32_t baud) {
    // Reset to a known "not configured" state. If the lookup below fails,
    // these zeros make write() a no-op and mark the baud as unsupported.
    _rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;

    // Linear scan of the table. There are at most 13 rows, and this runs
    // once per begin() call, so linear is fine; a binary search would
    // save nothing measurable.
    for (uint8_t i = 0; i < sizeof(ss_table) / sizeof(ss_table[0]); ++i) {
        if (pgm_read_dword(&ss_table[i].baud) == baud) {
            _rx_delay_centering = pgm_read_word(&ss_table[i].rx_delay_centering);
            _rx_delay_intrabit  = pgm_read_word(&ss_table[i].rx_delay_intrabit);
            _rx_delay_stopbit   = pgm_read_word(&ss_table[i].rx_delay_stopbit);
            _tx_delay           = pgm_read_word(&ss_table[i].tx_delay);
            break;
        }
    }
    if (_rx_delay_stopbit == 0) return;   // unsupported baud

    // Attach the pin-change interrupt for the RX pin. Two registers must
    // be configured:
    //   PCICR  |= enable bit for the group
    //   PCMSKx |= pin mask within the group
    // Both registers must exist and the pin must have a valid PCINT bit,
    // or we simply don't wire up RX (TX still works).
    volatile uint8_t* pcicr = avr_get_pcicr_reg(_receivePin);
    volatile uint8_t* pcmsk = avr_get_pcmsk_reg(_receivePin);
    uint8_t pcicr_bit       = avr_get_pcicr_bit(_receivePin);
    uint8_t pcmsk_bit       = avr_get_pcmsk_bit(_receivePin);

    if (pcicr && pcmsk && pcicr_bit != 0xFF && pcmsk_bit != 0xFF) {
        *pcicr |= (uint8_t)(1u << pcicr_bit);
        *pcmsk |= (uint8_t)(1u << pcmsk_bit);
    }

    // If the TX line was driven low by a partial previous transmission,
    // one bit-time of idle high restores the expected idle level.
    //tunedDelay(_tx_delay);

    txPinWrite(_inverse_logic ? 0 : 1);   // make sure TX is idle
    _delay_ms(50);                        // let the receiver see a solid idle period

    listen();
}

/**
 * @brief Detach the PCINT for this object's RX pin.
 *
 * Only touches PCMSKx (the per-pin mask), not PCICR (the group enable):
 * another SoftwareSerial instance on a different pin in the same group
 * may still need the group enabled. Leaving PCICR set with no matching
 * PCMSK bits simply causes no interrupts.
 *
 * Clears active_object_ if we were the listener, so a subsequent
 * handleInterrupt() becomes a harmless no-op.
 */
void SoftwareSerial::end() {
    volatile uint8_t* pcmsk = avr_get_pcmsk_reg(_receivePin);
    uint8_t pcmsk_bit       = avr_get_pcmsk_bit(_receivePin);
    if (pcmsk && pcmsk_bit != 0xFF) {
        *pcmsk &= (uint8_t)~(1u << pcmsk_bit);
    }
    if (active_object_ == this) active_object_ = nullptr;
}


// ---------------------------------------------------------------------------
// 7. listen() / overflow()
// ---------------------------------------------------------------------------

/**
 * @brief Take over the receive path (single-instance arbitration).
 *
 * The buffer is cleared under a critical section so the ISR cannot see
 * a half-reset state (e.g. tail==0, head!=0, which would look like a
 * full-ish buffer). SREG is saved and restored rather than blindly
 * calling sei(), so we don't re-enable interrupts that were already off.
 *
 * @return true if we displaced a different object, false if we were
 *         already listening.
 */
bool SoftwareSerial::listen() {
    if (active_object_ != this) {
        _buffer_overflow = false;
        uint8_t oldSREG = SREG;
        cli();
        _receive_buffer_head = _receive_buffer_tail = 0;
        active_object_ = this;
        SREG = oldSREG;
        return true;
    }
    return false;
}

/**
 * @brief Test-and-clear the RX overflow flag.
 *
 * Returns the flag's prior value and clears it, so consecutive calls
 * report "has overflowed since the previous call" rather than "has ever
 * overflowed".
 */
bool SoftwareSerial::overflow() {
    bool ret = _buffer_overflow;
    _buffer_overflow = false;
    return ret;
}


// ---------------------------------------------------------------------------
// 8. Receive ISR
// ---------------------------------------------------------------------------

/**
 * @brief Dispatch a PCINT event to the active listener.
 *
 * Called from all four PCINT vectors. If no object is listening (all
 * have been end()'d, or the interrupt fired between end() and
 * active_object_ being cleared), this returns immediately.
 */
void SoftwareSerial::handleInterrupt() {
    if (active_object_) active_object_->recvISR();
}

/**
 * @brief Sample one incoming byte, if this edge is a start bit.
 *
 * Called by the PCINT ISR on every change of any pin in the group. The
 * first thing we do is check whether *our* RX pin is at the start-bit
 * level; if not, the edge was caused by a different pin in the same
 * group and we return immediately (a common case when two soft-serial
 * ports share a port, or when a PWM or encoder output shares it).
 *
 * The bit sampling works like this, for a standard 8N1 frame:
 *
 *     idle  ─┐ ┌───┬───┬───┬───┬───┬───┬───┬───┐ ┌── idle
 *            └─┘   │   │   │   │   │   │   │   └──┘
 *            ^     ^   ^   ^   ^   ^   ^   ^   ^
 *            │     │   │   │   │   │   │   │   │
 *            │     LSB ...                  MSB│
 *            │                                 │
 *           start                            stop
 *
 *     Edge ─► tunedDelay(rx_delay_centering) ─► sample D0
 *          ─► tunedDelay(rx_delay_intrabit)  ─► sample D1
 *          ─► ...
 *          ─► tunedDelay(rx_delay_intrabit)  ─► sample D7
 *          ─► tunedDelay(rx_delay_stopbit)   ─► return
 *
 * rx_delay_centering is the offset from the start-bit edge to the
 * *centre* of the first data bit, not its leading edge. This gives the
 * maximum tolerance to line noise and baud-rate mismatch.
 *
 * The RX PCINT for this pin is disabled for the duration of the
 * sampling, so that edges within the byte being received do not
 * re-enter this routine and corrupt the timing. It is re-enabled
 * before returning.
 *
 * After the byte is assembled, if inverse_logic is set it is inverted
 * (because we sampled the inverted line). Then it is pushed into the
 * ring buffer. If the buffer is full, the byte is dropped and
 * _buffer_overflow is set for the application to notice later.
 */
void SoftwareSerial::recvISR() {
    uint8_t d = 0;

    // Is this actually our start bit?
    if (_inverse_logic ? rxPinRead() : !rxPinRead()) {

        // IMPORTANT:
        // Disable PCINT for our RX pin while sampling the byte.
        volatile uint8_t* pcmsk = avr_get_pcmsk_reg(_receivePin);
        uint8_t pcmsk_bit = avr_get_pcmsk_bit(_receivePin);

        if (pcmsk && pcmsk_bit != 0xFF)
            *pcmsk &= (uint8_t)~(1u << pcmsk_bit);

        tunedDelay(_rx_delay_centering);

        for (uint8_t i = 0x01; i; i <<= 1) {
            tunedDelay(_rx_delay_intrabit);

            if (rxPinRead())
                d |= i;
            else
                d &= (uint8_t)~i;
        }

        tunedDelay(_rx_delay_stopbit);

        if (_inverse_logic)
            d = (uint8_t)~d;

        uint8_t next =
            (uint8_t)((_receive_buffer_tail + 1)
                      & (_SS_MAX_RX_BUFF - 1));

        if (next != _receive_buffer_head) {
            _receive_buffer[_receive_buffer_tail] = (char)d;
            _receive_buffer_tail = next;
        } else {
            _buffer_overflow = true;
        }

        // Re-enable our RX PCINT.
        if (pcmsk && pcmsk_bit != 0xFF)
            *pcmsk |= (uint8_t)(1u << pcmsk_bit);
    }
}


// ---------------------------------------------------------------------------
// 9. PCINT vector handlers
//
// The 328P has three vectors (one per port); the Tiny85 has one (the
// whole port B). We only define a handler for a vector that the current
// variant actually has.
//
// All handlers do the same thing: forward to the active listener. The
// work of deciding whether the edge was for our pin is done inside
// recvISR(), which lets two SoftwareSerial objects on the same port share
// the same vector.
// ---------------------------------------------------------------------------

#if defined(PCINT0_vect)
ISR(PCINT0_vect) { SoftwareSerial::handleInterrupt(); }
#endif
#if defined(PCINT1_vect)
ISR(PCINT1_vect) { SoftwareSerial::handleInterrupt(); }
#endif
#if defined(PCINT2_vect)
ISR(PCINT2_vect) { SoftwareSerial::handleInterrupt(); }
#endif
#if defined(PCINT3_vect)
ISR(PCINT3_vect) { SoftwareSerial::handleInterrupt(); }
#endif


// ---------------------------------------------------------------------------
// 10. Transmit
// ---------------------------------------------------------------------------

/**
 * @brief Send one byte using the classic UART frame, interrupts masked.
 *
 * Frame layout for standard polarity (inverse_logic = false):
 *
 *     idle ─┐    ┌───┬───┬───┬───┬───┬───┬───┬───┐    ┌─── idle
 *           └────┘ D0│ D1│ D2│ D3│ D4│ D5│ D6│ D7└────┘
 *           start                                stop
 *
 * For inverted logic, the levels are simply flipped (start is high, stop
 * is low, idle is low).
 *
 * Interrupts are disabled for the whole byte because a PCINT firing
 * mid-frame would be serviced by recvISR(), which busy-waits for up to
 * one bit-time, and that delay would stretch the transmitted bits past
 * tolerance. Re-enabling them afterwards lets any pending RX edge be
 * serviced before we start the next byte.
 *
 * The final tunedDelay(_tx_delay) holds the line at the stop level long
 * enough that a following byte's start bit is cleanly separated.
 *
 * If tx_delay is zero (begin() was never called, or the baud was
 * unsupported), this is a no-op — important because otherwise we would
 * bit-bang with a zero-bit delay, which would produce garbage.
 */
void SoftwareSerial::_enqueueByte(uint8_t b) {
    if (_tx_delay == 0) return;

    // Save and disable interrupts so the transmit waveform is not
    // disturbed by a receive ISR.
    uint8_t oldSREG = SREG;
    cli();

    // Start bit. For standard polarity this is LOW; for inverted, HIGH.
    txPinWrite(_inverse_logic ? 1 : 0);

    // The +SS_XMIT_START_ADJUSTMENT compensates for the fixed latency
    // between the SBI/CBI instruction and the first iteration of
    // tunedDelay. See the constant's definition in the header.
    tunedDelay(_tx_delay + SS_XMIT_START_ADJUSTMENT);

    // 8 data bits, LSB first. The branch on (b & mask) is deliberately
    // balanced: both paths are the same length so bit-time does not
    // jitter with the data pattern.
    if (_inverse_logic) {
        for (uint8_t mask = 0x01; mask; mask <<= 1) {
            txPinWrite((b & mask) ? 0 : 1);
            tunedDelay(_tx_delay);
        }
        txPinWrite(0);   // stop bit (inverted: LOW)
    } else {
        for (uint8_t mask = 0x01; mask; mask <<= 1) {
            txPinWrite((b & mask) ? 1 : 0);
            tunedDelay(_tx_delay);
        }
        txPinWrite(1);   // stop bit (standard: HIGH)
    }

    // Restore the previous interrupt-enable state. If a PCINT became
    // pending during the transmit, it fires here and recvISR() runs
    // before we return.
    SREG = oldSREG;

    // Hold the line at the stop level for one more bit-time. This
    // guarantees a clean gap between bytes even if the caller calls
    // write() again immediately.
    tunedDelay(_tx_delay);
}

/**
 * @brief Public Print sink: transmit one byte.
 *
 * This is the single virtual function that the Print base class calls
 * for every character of every print()/println()/printHex()/... call.
 * It delegates to _enqueueByte(), which does the actual bit-banging.
 *
 * @param b  The byte to transmit.
 * @return 1 (always succeeds from the caller's perspective).
 */
size_t SoftwareSerial::write(uint8_t b) {
    _enqueueByte(b);
    return 1;
}

/**
 * @brief No-op, kept for API symmetry.
 *
 * Because _enqueueByte() blocks until the byte has fully shifted out
 * (including the trailing stop-bit hold), there is never anything
 * pending by the time write() returns.
 */
void SoftwareSerial::flush() { /* synchronous TX — no-op */ }


// ---------------------------------------------------------------------------
// 11. Input
// ---------------------------------------------------------------------------

/**
 * @brief Remove and return the oldest byte from the RX ring.
 *
 * The head/tail comparison is done first (single load of tail, single
 * load of head). If they're equal, the buffer is empty. Note that the
 * ring is *never* allowed to fill completely (the ISR requires
 * tail+1 != head), so head==tail uniquely means empty, not full.
 */
int SoftwareSerial::read() {
    if (!isListening()) return -1;
    if (_receive_buffer_head == _receive_buffer_tail) return -1;
    uint8_t d = (uint8_t)_receive_buffer[_receive_buffer_head];
    _receive_buffer_head = (uint8_t)((_receive_buffer_head + 1)
                                     & (_SS_MAX_RX_BUFF - 1));
    return d;
}

/**
 * @brief Return the oldest byte without removing it.
 *
 * Safe to call repeatedly: the head index is not advanced. The value
 * returned may become stale if the ISR adds more bytes between calls,
 * but the *oldest* unread byte is stable.
 */
int SoftwareSerial::peek() {
    if (!isListening()) return -1;
    if (_receive_buffer_head == _receive_buffer_tail) return -1;
    return (uint8_t)_receive_buffer[_receive_buffer_head];
}

/**
 * @brief Count of bytes available to read.
 *
 * The expression `(tail + N - head) & (N-1)` is the ring-buffer length
 * formula, computed without a modulo. It works because both indices are
 * already masked to the buffer size and N is a power of two.
 */
int SoftwareSerial::available() {
    if (!isListening()) return 0;
    return (_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head)
           & (_SS_MAX_RX_BUFF - 1);
}
