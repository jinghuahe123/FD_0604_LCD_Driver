/*
    AVR Pin <-> Arduino Digital Pin mapping library.

    Provides:
      - Arduino-style digital pin <-> (port, bit) conversion
      - Register access (PORTx, DDRx, PINx) by logical port
      - Pin-mode and pin-I/O convenience functions
      - Pin-change interrupt (PCINT) register/bit lookup
      - ADC (analog read) channel mapping, reference selection, and
        polled conversions
      - Debug-friendly pin name helpers

    Supported variants:
      - VARIANT_MEGA328 : ATmega328/P (Arduino Uno, Nano, etc.)
      - VARIANT_TINYX5  : ATtiny25/45/85

    All functions are pure C, safe to call from C and C++. The header is
    wrapped in `extern "C"` so symbol names are stable across the two.
*/

#ifndef DRIVER_CORE_AVR_PINS_H
#define DRIVER_CORE_AVR_PINS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "board_definitions.h"


//=============================================================================
// ATmega328P Digital Pin Mapping (Arduino-style)
//
// The mapping follows the Arduino Uno pinout so code can be shared between
// the AVR core and this library. D14..D19 alias the analog pins A0..A5.
//=============================================================================

// Port B pins (digital pins 8-13)
#define PIN_D8   8   // PB0
#define PIN_D9   9   // PB1
#define PIN_D10  10  // PB2
#define PIN_D11  11  // PB3
#define PIN_D12  12  // PB4
#define PIN_D13  13  // PB5

// Port C pins (analog pins A0-A5, can be used as digital 14-19)
#define PIN_D14  14  // PC0 (A0)
#define PIN_D15  15  // PC1 (A1)
#define PIN_D16  16  // PC2 (A2)
#define PIN_D17  17  // PC3 (A3)
#define PIN_D18  18  // PC4 (A4)
#define PIN_D19  19  // PC5 (A5)

// Port D pins (digital pins 0-7)
#define PIN_D0   0   // PD0 (RX)
#define PIN_D1   1   // PD1 (TX)
#define PIN_D2   2   // PD2
#define PIN_D3   3   // PD3
#define PIN_D4   4   // PD4
#define PIN_D5   5   // PD5
#define PIN_D6   6   // PD6
#define PIN_D7   7   // PD7


//=============================================================================
// Generic Port Definitions
//
// AVR_Port_t is the canonical "logical port" identifier. Only the ports
// that physically exist on the selected variant are enumerable, so a
// switch over AVR_Port_t on a given variant needs only handle those.
//=============================================================================

/** @brief Logical port identifier (letter of the PORTx register). */
typedef enum {
    #if defined(PORTB)
    PORT_B = 0,
    #endif
    #if defined(PORTC)
    PORT_C = 1,
    #endif
    #if defined(PORTD)
    PORT_D = 2
    #endif
} AVR_Port_t;


//=============================================================================
// Pin-Change Interrupt Groups
//
// On AVR, a "pin change interrupt" is not per-pin; it is per-group. Each
// group is enabled by one bit in PCICR (or GIMSK on the ATtiny) and has
// one mask register (PCMSKx) whose bits select which pins in the group
// can trigger the interrupt.
//
// Group assignments (see the datasheet for your chip):
//
//   ATmega328P:
//     PCINT0 -> Port B (PB0..PB7)  -> D8..D13  (+ XTAL pins PB6/PB7)
//     PCINT1 -> Port C (PC0..PC6)  -> D14..D19 (+ RESET on PC6)
//     PCINT2 -> Port D (PD0..PD7)  -> D0..D7
//
//   ATtiny85:
//     PCINT0 -> Port B (PB0..PB5)  -> D0..D5   (single group)
//
// The ISR vectors are named PCINT0_vect, PCINT1_vect, PCINT2_vect on the
// 328P. On the ATtiny85 there is only PCINT0_vect (even though the name
// may be aliased).
//=============================================================================

/** @brief Pin-change interrupt group identifier. */
typedef enum {
    PCINT_GROUP_0 = 0,   ///< Port B group on 328P; only group on TinyX5
    PCINT_GROUP_1 = 1,   ///< Port C group on 328P (unused on TinyX5)
    PCINT_GROUP_2 = 2,   ///< Port D group on 328P (unused on TinyX5)
    PCINT_GROUP_INVALID = 0xFF
} AVR_PcintGroup_t;


//=============================================================================
// ADC (Analog Read) Definitions
//
// The ADC is a single shared peripheral, not a per-pin resource. Channel
// selection is by *ADC channel number*, which is wired to a physical pin
// differently on each variant:
//
//   ATmega328P (DIP/TQFP):
//     ADC0..ADC5 -> PC0..PC5 -> D14..D19 (A0..A5)
//     ADC6, ADC7 exist only on TQFP/MLF packages and are not brought out
//     on the Uno/Nano, so we do not expose them.
//
//   ATtiny85 (DIP/SOIC):
//     ADC0 -> PB5 -> D5
//     ADC1 -> PB2 -> D2
//     ADC2 -> PB4 -> D4
//     ADC3 -> PB3 -> D3
//     (ADC2/ADC3 are shared with the internal temperature sensor and
//      the 1.1V bandgap, selected via MUX bits 3:0; we leave those
//      advanced modes out of this layer.)
//=============================================================================

/** @brief ADC reference voltage selection. */
typedef enum {
    AVR_ADC_REF_AREF     = 0,   ///< External reference on AREF pin
    AVR_ADC_REF_AVCC     = 1,   ///< AVcc (usually 5V), external cap on AREF
    AVR_ADC_REF_INTERNAL = 2    ///< Internal 1.1V reference
} AVR_AdcRef_t;


//=============================================================================
// Core Conversion Functions
//=============================================================================

/**
 * @brief Convert a (port, bit) pair to an Arduino-style digital pin number.
 *
 * @param port  Logical port (PORT_B, PORT_C, PORT_D).
 * @param pin   Bit index within the port (0..7).
 * @return      Digital pin number, or 0xFF if the combination is invalid
 *              on the current variant.
 *
 * Examples (ATmega328P):
 *   avr_pin_to_digital(PORT_B, 3) -> 11   (D11)
 *   avr_pin_to_digital(PORT_D, 2) -> 2    (D2)
 *   avr_pin_to_digital(PORT_C, 0) -> 14   (D14 / A0)
 */
uint8_t avr_pin_to_digital(AVR_Port_t port, uint8_t pin);

/**
 * @brief Convert an Arduino-style digital pin number to (port, bit).
 *
 * @param[in]  digital_pin  Digital pin number (0..19).
 * @param[out] port         Set to the logical port on success.
 * @param[out] pin          Set to the bit index within the port on success.
 * @return     true on success, false if @p digital_pin is out of range
 *             for the current variant.
 *
 * Examples (ATmega328P):
 *   avr_digital_to_pin(11, &port, &pin) -> port=PORT_B, pin=3
 *   avr_digital_to_pin(2,  &port, &pin) -> port=PORT_D, pin=2
 *   avr_digital_to_pin(14, &port, &pin) -> port=PORT_C, pin=0
 */
bool avr_digital_to_pin(uint8_t digital_pin, AVR_Port_t *port, uint8_t *pin);

/**
 * @brief Test whether a (port, bit) pair is valid on the current variant.
 *
 * @param port  Logical port.
 * @param pin   Bit index within the port.
 * @return      true if the pin exists and is usable.
 */
bool avr_pin_valid(AVR_Port_t port, uint8_t pin);

/**
 * @brief Test whether an Arduino-style digital pin number is valid.
 *
 * @param digital_pin  Digital pin number.
 * @return             true if within the variant's D0..DN range.
 */
bool avr_digital_pin_valid(uint8_t digital_pin);


//=============================================================================
// Register Access Functions
//
// Each returns a pointer into the AVR's I/O space. The pointers are
// stable for the lifetime of the program, so they may be cached (e.g. by
// a hot loop or an ISR that needs to bypass the function-call overhead).
// All return NULL for an invalid port.
//=============================================================================

/** @brief Pointer to the PORTx register (data direction output latch). */
volatile uint8_t* avr_get_port_reg(AVR_Port_t port);

/** @brief Pointer to the DDRx register (data direction). */
volatile uint8_t* avr_get_ddr_reg(AVR_Port_t port);

/** @brief Pointer to the PINx register (pin state, always readable). */
volatile uint8_t* avr_get_pin_reg(AVR_Port_t port);

/**
 * @brief Bit mask for a bit index within a port.
 *
 * @param pin  Bit index (0..7).
 * @return     (1 << pin).
 */
uint8_t avr_get_pin_mask(uint8_t pin);


//=============================================================================
// Pin Operation Functions (Convenience)
//
// These go through the register-access helpers, so they take a port/bit
// pair rather than a digital pin number. Use avr_digital_to_pin() first
// if you only have a digital pin number.
//
// IMPORTANT: these are intended for setup / low-frequency use. Each call
// does a switch + pointer dereference. In an ISR or a bit-banging loop,
// cache the pointer from avr_get_port_reg() / avr_get_pin_reg() instead.
//=============================================================================

/** @brief Configure a pin as a low-impedance output (DDRx bit = 1). */
void avr_pin_output(AVR_Port_t port, uint8_t pin);

/** @brief Configure a pin as an input (DDRx bit = 0). */
void avr_pin_input(AVR_Port_t port, uint8_t pin);

/**
 * @brief Enable or disable the internal pull-up on an input pin.
 *
 * Only meaningful when the pin is configured as an input (DDRx bit = 0);
 * on an output this controls whether the pin sources or sinks current.
 */
void avr_pin_pullup(AVR_Port_t port, uint8_t pin, bool enable);

/** @brief Drive an output pin high (true) or low (false). */
void avr_pin_write(AVR_Port_t port, uint8_t pin, bool value);

/** @brief Read the instantaneous logic level on a pin. */
bool avr_pin_read(AVR_Port_t port, uint8_t pin);

/** @brief Toggle an output pin (writes 1 to the PINx bit). */
void avr_pin_toggle(AVR_Port_t port, uint8_t pin);


//=============================================================================
// Pin-Change Interrupt (PCINT) Helpers
//
// A PCINT interrupt is configured by:
//   1. Clearing the corresponding bit in PCMSKx (disable while configuring).
//   2. Setting the PCICR bit for the group to enable the interrupt.
//   3. Setting the PCMSKx bit for each pin that should trigger.
//
// These helpers hide the group-selection logic so callers only need a
// digital pin number. All return NULL / 0xFF for pins that have no PCINT
// capability on the current variant.
//=============================================================================

/**
 * @brief Pointer to the pin-change interrupt control register.
 *
 * On the ATmega328P this is PCICR. On the ATtiny85 the equivalent is
 * GIMSK (the enable bit lives at a different position; see
 * avr_get_pcicr_bit()).
 *
 * @param digital_pin  Digital pin number.
 * @return             Pointer, or NULL if the pin has no PCINT group.
 */
volatile uint8_t* avr_get_pcicr_reg(uint8_t digital_pin);

/**
 * @brief Bit position within the PCICR / GIMSK register for a digital pin.
 *
 * @param digital_pin  Digital pin number.
 * @return             Bit index (PCIE0..PCIE2 on 328P, PCIE on TinyX5),
 *                     or 0xFF if the pin has no PCINT group.
 */
uint8_t avr_get_pcicr_bit(uint8_t digital_pin);

/**
 * @brief Pointer to the pin-change mask register for a digital pin.
 *
 * @param digital_pin  Digital pin number.
 * @return             Pointer to PCMSK0/1/2 (328P) or PCMSK (TinyX5),
 *                     or NULL if the pin has no PCINT group.
 */
volatile uint8_t* avr_get_pcmsk_reg(uint8_t digital_pin);

/**
 * @brief Bit position within the PCMSK register for a digital pin.
 *
 * @param digital_pin  Digital pin number.
 * @return             Bit index (PCINT0..PCINT23 on 328P, PCINT0..5 on
 *                     TinyX5), or 0xFF if the pin has no PCINT group.
 */
uint8_t avr_get_pcmsk_bit(uint8_t digital_pin);

/**
 * @brief Group identifier for a digital pin.
 *
 * Useful when a single handler needs to know which vector fired, though
 * in practice the ISR is fixed per vector so this is mostly diagnostic.
 *
 * @param digital_pin  Digital pin number.
 * @return             AVR_PcintGroup_t, or PCINT_GROUP_INVALID.
 */
AVR_PcintGroup_t avr_get_pcint_group(uint8_t digital_pin);


//=============================================================================
// ADC (Analog Read) Functions
//
// The ADC is a single shared peripheral. Initialization is explicit and
// idempotent: call avr_adc_init() once before any avr_adc_read() call.
// It sets the reference, the prescaler (derived from F_CPU so the ADC
// clock lands in the 50..200 kHz band), and enables the ADC in free-run
// mode with auto-trigger off.
//
// Channel selection is variant-specific:
//   ATmega328P: channel = digital_pin - 14   (D14..D19 -> ADC0..ADC5)
//   ATtiny85:   channel = {D5->0, D2->1, D4->2, D3->3}
//=============================================================================

/**
 * @brief Test whether a digital pin has an ADC channel on this variant.
 *
 * Use this instead of hand-rolling range checks in higher layers so the
 * "is this pin analog-capable?" question has exactly one answer.
 *
 * @param digital_pin  Digital pin number.
 * @return             true if the pin can be used with avr_adc_read().
 */
bool avr_adc_pin_has_channel(uint8_t digital_pin);

/**
 * @brief Map a digital pin to its ADC channel number.
 *
 * @param digital_pin  Digital pin number.
 * @return             ADC channel (0..7), or 0xFF if the pin has no
 *                     ADC channel on this variant.
 */
uint8_t avr_adc_channel_for_pin(uint8_t digital_pin);

/**
 * @brief Initialize the ADC with the given reference and a prescaler
 *        chosen from F_CPU.
 *
 * Safe to call multiple times; each call re-applies the reference and
 * re-enables the ADC. If F_CPU is unknown (not defined), the prescaler
 * defaults to /128, which is safe for all AVRs up to 20 MHz.
 *
 * @param ref  Reference voltage selection.
 */
void avr_adc_init(AVR_AdcRef_t ref);

/**
 * @brief Change the ADC reference without touching the prescaler or the
 *        enable bit.
 *
 * Includes the internal-reference settling delay when
 * AVR_ADC_REF_INTERNAL is selected.
 *
 * @param ref  Reference voltage selection.
 */
void avr_adc_set_reference(AVR_AdcRef_t ref);

/**
 * @brief Start a conversion on the given channel (non-blocking).
 *
 * The result is available from avr_adc_ready() / avr_adc_result().
 * Intended for callers that want to overlap the conversion with other
 * work. HardwarePin uses the blocking avr_adc_read() instead.
 *
 * @param channel  ADC channel number (use avr_adc_channel_for_pin()).
 */
void avr_adc_start(uint8_t channel);

/** @brief true once the conversion started by avr_adc_start() is done. */
bool avr_adc_ready(void);

/** @brief Read the result of the last completed conversion. */
uint16_t avr_adc_result(void);

/**
 * @brief Blocking single-shot conversion on a channel.
 *
 * Starts a conversion and busy-waits for ADSC to clear. This is the
 * function HardwarePin::analogRead() delegates to.
 *
 * @param channel  ADC channel number.
 * @return         10-bit conversion result (0..1023). Returns 0 if the
 *                 channel is out of range for the variant.
 */
uint16_t avr_adc_read(uint8_t channel);


//=============================================================================
// String Names for Debugging
//=============================================================================

/** @brief Write the register name (e.g. "PORTB") into @p buffer. */
void avr_port_name(AVR_Port_t port, char* buffer, size_t buflen);

/** @brief Write a human-readable pin name (e.g. "D11", "D14/A0") into @p buffer. */
void avr_pin_name(AVR_Port_t port, uint8_t pin, char* buffer, size_t buflen);


//=============================================================================
// Pin Information Structure
//=============================================================================

/** @brief Snapshot of everything this library knows about a pin. */
typedef struct {
    uint8_t  digital_pin;       ///< Arduino-style digital pin number
    AVR_Port_t port;            ///< Logical port
    uint8_t  pin;               ///< Bit index within the port

    char port_name[8];          ///< "PORTB", "PORTC", "PORTD"
    char pin_name[8];           ///< "D11", "D14/A0", ...

    volatile uint8_t *port_reg; ///< &PORTx
    volatile uint8_t *ddr_reg;  ///< &DDRx
    volatile uint8_t *pin_reg;  ///< &PINx
} AVR_PinInfo_t;

/**
 * @brief Fill in an AVR_PinInfo_t for the given (port, bit).
 *
 * The returned struct contains cached register pointers, so it may be
 * kept and used in a hot loop without paying the switch cost again.
 */
AVR_PinInfo_t avr_get_pin_info(AVR_Port_t port, uint8_t pin);


//=============================================================================
// Helper Macros for Common Configurations
//
// These expand to brace-initializers for { AVR_Port_t, bit } pairs, which
// is handy when populating static tables.
//=============================================================================

#if defined(PORTD)
#define AVR_PIN_D0  { PORT_D, 0 }
#define AVR_PIN_D1  { PORT_D, 1 }
#define AVR_PIN_D2  { PORT_D, 2 }
#define AVR_PIN_D3  { PORT_D, 3 }
#define AVR_PIN_D4  { PORT_D, 4 }
#define AVR_PIN_D5  { PORT_D, 5 }
#define AVR_PIN_D6  { PORT_D, 6 }
#define AVR_PIN_D7  { PORT_D, 7 }
#endif
#if defined(PORTB)
#define AVR_PIN_D8  { PORT_B, 0 }
#define AVR_PIN_D9  { PORT_B, 1 }
#define AVR_PIN_D10 { PORT_B, 2 }
#define AVR_PIN_D11 { PORT_B, 3 }
#define AVR_PIN_D12 { PORT_B, 4 }
#define AVR_PIN_D13 { PORT_B, 5 }
#endif
#if defined(PORTC)
#define AVR_PIN_A0  { PORT_C, 0 }
#define AVR_PIN_A1  { PORT_C, 1 }
#define AVR_PIN_A2  { PORT_C, 2 }
#define AVR_PIN_A3  { PORT_C, 3 }
#define AVR_PIN_A4  { PORT_C, 4 }
#define AVR_PIN_A5  { PORT_C, 5 }
#endif

#ifdef __cplusplus
}
#endif

#endif // DRIVER_CORE_AVR_PINS_H