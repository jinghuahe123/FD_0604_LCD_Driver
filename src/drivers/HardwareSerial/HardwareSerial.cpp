/**
 * @file HardwareSerial.cpp
 * @brief Implementation of HardwareSerialT<Traits>.
 *
 * The entire translation unit is compiled only on chips that have a USART.
 * On the ATtiny85 the file reduces to nothing, so no register references
 * or ISRs are emitted and the linker never looks for the methods.
 *
 * Method definitions are templated and explicitly instantiated at the
 * bottom for each supported port. This keeps the implementation out of the
 * header (smaller recompiles) while still allowing a single .cpp.
 *
 * All formatted output lives in Print.cpp; this file is only the sink:
 * write(), plus the lifecycle, input, and ISR plumbing.
 */

#include "HardwareSerial.hpp"

#if defined(UBRR0H) || defined(UBRRH)

#include <avr/interrupt.h>

// ===========================================================================
// Global instances
// ===========================================================================

#if defined(UBRR1H)
  // ATmega328PB: two ports.
  HardwareSerialT<Usart0Traits> Serial;
  HardwareSerialT<Usart1Traits> Serial1;
#else
  // ATmega328P (or any single-USART chip): one port.
  HardwareSerialT<Usart0Traits> Serial;
#endif

// ===========================================================================
// begin() / end()
// ===========================================================================

template <class T>
void HardwareSerialT<T>::begin(uint32_t baud) {
    // Disable the USART while reconfiguring so no stale interrupt fires.
    T::ucsrb() = 0;

    // --- Baud rate ---------------------------------------------------------
    // Double-speed mode (U2X = 1):
    //     UBRR = F_CPU / (8 * baud) - 1
    // This halves the divisor compared to normal speed, giving more headroom
    // at high baud rates and low F_CPU.
    uint32_t ubrr = (F_CPU / (8UL * baud)) - 1UL;
    if (ubrr > 4095UL) ubrr = 4095UL;   // clamp to 12-bit field

    // High byte first: writing UBRRH latches the full 12-bit value together
    // with UBRRL, so UBRRL must be written second.
    T::ubrrh() = static_cast<uint8_t>((ubrr >> 8) & 0x0F);
    T::ubrrl() = static_cast<uint8_t>(ubrr & 0xFF);

    // --- Frame format: 8 data bits, no parity, 1 stop bit (8N1) ------------
    // UCSRC: UMSEL=0 (async), UPM=00 (none), USBS=0 (1 stop),
    //        UCSZ=011 (8-bit), UCPOL=0.
    T::ucsrc() = (1 << T::UCSZ1_) | (1 << T::UCSZ0_);

    // --- U2X on ------------------------------------------------------------
    // Writing the whole register is fine here: the other writable bit
    // (MPCM0) is 0 at init.
    T::ucsra() = (1 << T::U2X);

    // --- Reset buffers before enabling interrupts -------------------------
    rx_head = rx_tail = 0;
    tx_head = tx_tail = 0;

    // Enable RX, TX and RX-complete IRQ. The TX-empty (UDRIE) interrupt is
    // deliberately left off; it is enabled lazily by _enqueueTx and disabled
    // by _udreISR when the TX buffer empties, so an idle USART stays quiet.
    T::ucsrb() = (1 << T::RXEN) | (1 << T::TXEN) | (1 << T::RXCIE);

    // Enable global interrupts so the RX/UDRE ISRs actually fire.
    // On bare-metal AVR, interrupts are disabled at reset; on the Arduino
    // core, this is redundant but harmless.
    sei();
}

template <class T>
void HardwareSerialT<T>::end() {
    T::ucsrb() = 0;                 // disable RX, TX and all USART IRQs
    rx_head = rx_tail = 0;
    tx_head = tx_tail = 0;
}

// ===========================================================================
// TX
// ===========================================================================

template <class T>
void HardwareSerialT<T>::_writeByteBlocking(uint8_t b) {
    while (!(T::ucsra() & (1 << T::UDRE))) { /* spin until data reg empty */ }
    T::udr() = b;
}

template <class T>
bool HardwareSerialT<T>::_writeByteDirect(uint8_t b) {
    // If the data register is already empty and the TX ring is empty too,
    // we can skip the ring entirely and feed UDR directly.
    if ((T::ucsra() & (1 << T::UDRE)) && (tx_head == tx_tail)) {
        T::udr() = b;
        return true;
    }
    return false;
}

template <class T>
void HardwareSerialT<T>::_enqueueTx(uint8_t b) {
    // Mask interrupts for the whole function. The UDRE ISR advances
    // tx_tail; if it fired between _writeByteBlocking() and the tail
    // update below, it would re-send the byte that was just written.
    // Masking closes that window.
    uint8_t oldSREG = SREG;
    cli();

    uint8_t next = (tx_head + 1) & (TX_BUFFER_SIZE - 1);

    if (next == tx_tail) {
        // Buffer full: stop the UDRE ISR and drain synchronously so nothing
        // already queued is lost and no byte is transmitted twice.
        T::ucsrb() &= ~(1 << T::UDRIE);

        while (tx_head != tx_tail) {
            _writeByteBlocking(tx_buf[tx_tail]);
            tx_tail = (tx_tail + 1) & (TX_BUFFER_SIZE - 1);
        }
        _writeByteBlocking(b);
    } else {
        tx_buf[tx_head] = b;
        tx_head = next;
        T::ucsrb() |= (1 << T::UDRIE);
    }

    SREG = oldSREG;
}

template <class T>
size_t HardwareSerialT<T>::write(uint8_t byte) {
    // Fast path: hardware idle and ring empty -> write straight through.
    if (_writeByteDirect(byte)) return 1;

    // Otherwise, queue it.
    _enqueueTx(byte);
    return 1;
}

template <class T>
size_t HardwareSerialT<T>::write(const uint8_t* buffer, size_t size) {
    size_t n = 0;
    while (size--) n += write(*buffer++);
    return n;
}

template <class T>
void HardwareSerialT<T>::flush() {
    // Wait for the software buffer to drain (the UDRE ISR does the work),
    // then for the hardware shift register to finish and TXC to set.
    while (tx_head != tx_tail) { /* ISR drains */ }
    while (!(T::ucsra() & (1 << T::TXC))) { /* spin */ }
    T::ucsra() |= (1 << T::TXC);    // clear TXC (write 1 to clear)
}

// ===========================================================================
// RX
// ===========================================================================

template <class T>
int HardwareSerialT<T>::available() {
    // (size + head - tail) mod size, branchless via the power-of-two mask.
    return static_cast<int>((RX_BUFFER_SIZE + rx_head - rx_tail) & (RX_BUFFER_SIZE - 1));
}

template <class T>
int HardwareSerialT<T>::read() {
    if (rx_head == rx_tail) return -1;          // empty
    uint8_t b = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_BUFFER_SIZE - 1);
    return b;
}

template <class T>
int HardwareSerialT<T>::peek() {
    if (rx_head == rx_tail) return -1;          // empty
    return rx_buf[rx_tail];
}

// ===========================================================================
// ISR hooks
// ===========================================================================

template <class T>
void HardwareSerialT<T>::_rxISR() {
    uint8_t b = T::udr();                       // reading UDR clears RXC
    uint8_t next = (rx_head + 1) & (RX_BUFFER_SIZE - 1);
    if (next != rx_tail) {
        rx_buf[rx_head] = b;
        rx_head = next;
    }
    // else: buffer full — drop the byte silently.
}

template <class T>
void HardwareSerialT<T>::_udreISR() {
    if (tx_head == tx_tail) {
        // Nothing left to send: disable the TX-empty interrupt so an idle
        // USART generates no further interrupts.
        T::ucsrb() &= ~(1 << T::UDRIE);
        return;
    }
    T::udr() = tx_buf[tx_tail];
    tx_tail = (tx_tail + 1) & (TX_BUFFER_SIZE - 1);
}

// ===========================================================================
// Explicit instantiations
//
// The definitions above are templated but live in this .cpp, so they must be
// instantiated here for each port. Add a line when adding a new port.
// ===========================================================================

template class HardwareSerialT<Usart0Traits>;
#if defined(UBRR1H)
template class HardwareSerialT<Usart1Traits>;
#endif

// ===========================================================================
// Vector handlers
//
// The vector names differ between the 328P and the 328PB:
//   328P : USART_RX_vect   / USART_UDRE_vect
//   328PB: USART0_RX_vect  / USART0_UDRE_vect   and   USART1_* for the second
// ===========================================================================

#if defined(USART_RX_vect)
  // ATmega328P
  ISR(USART_RX_vect)   { Serial._rxISR(); }
  ISR(USART_UDRE_vect) { Serial._udreISR(); }
#elif defined(USART0_RX_vect)
  // ATmega328PB — USART0
  ISR(USART0_RX_vect)   { Serial._rxISR(); }
  ISR(USART0_UDRE_vect) { Serial._udreISR(); }

  // ATmega328PB — USART1
  ISR(USART1_RX_vect)   { Serial1._rxISR(); }
  ISR(USART1_UDRE_vect) { Serial1._udreISR(); }
#endif

#endif  // UBRR0H / UBRRH