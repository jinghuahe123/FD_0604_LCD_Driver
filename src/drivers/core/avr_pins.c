/*
    AVR Pin <-> Arduino Digital Pin mapping library — implementation.

    See avr_pins.h for the API contract. The implementation is split into
    five sections:
        1. Core conversion        (pin number <-> port/bit)
        2. Register access        (PORTx / DDRx / PINx pointers)
        3. Pin operations         (mode, write, read, toggle)
        4. PCINT helpers          (group / register / bit lookup)
        5. ADC helpers            (channel mapping, init, conversion)
    plus string-name helpers and the pin-info struct filler.
*/

#include "avr_pins.h"

#ifndef F_CPU
#warning "F_CPU not defined; ADC prescaler defaults to /128 (safe up to 20 MHz)."
#endif


//=============================================================================
// Core Conversion Functions
//=============================================================================

uint8_t avr_pin_to_digital(AVR_Port_t port, uint8_t pin) {
    // Reject obviously-invalid bit indices up front. Each variant then
    // maps (port, bit) to the Arduino-style numbering it uses.
    if (pin > 7) return 0xFF;

    switch (port) {
    #ifdef VARIANT_MEGA328
        // Uno pinout: D0..D7 = PD, D8..D13 = PB, D14..D19 = PC (A0..A5).
        // Only PB0..PB5 and PC0..PC5 are brought out; PB6/PB7 are XTAL
        // and PC6 is RESET, so we deliberately reject them here.
        case PORT_B:
            return (pin <= 5) ? (uint8_t)(8 + pin)  : 0xFF;
        case PORT_C:
            return (pin <= 5) ? (uint8_t)(14 + pin) : 0xFF;
        case PORT_D:
            return (uint8_t)pin;                    // PD0..PD7 = D0..D7
    #elif defined(VARIANT_TINYX5)
        // ATtiny85: PB0..PB5 are D0..D5.
        case PORT_B:
            return (pin <= 5) ? (uint8_t)pin        : 0xFF;
    #endif
        default:
            return 0xFF;
    }
}


bool avr_digital_to_pin(uint8_t digital_pin, AVR_Port_t *port, uint8_t *pin) {
    // Guard against NULL outputs so a caller mistake is a clean failure
    // rather than a hard fault.
    if (!port || !pin) return false;

#ifdef VARIANT_MEGA328
    if (digital_pin <= 7) {
        // D0..D7 -> Port D
        *port = PORT_D;
        *pin  = digital_pin;
        return true;
    }
    if (digital_pin <= 13) {
        // D8..D13 -> Port B
        *port = PORT_B;
        *pin  = (uint8_t)(digital_pin - 8);
        return true;
    }
    if (digital_pin <= 19) {
        // D14..D19 -> Port C (aliases A0..A5)
        *port = PORT_C;
        *pin  = (uint8_t)(digital_pin - 14);
        return true;
    }
#elif defined(VARIANT_TINYX5)
    if (digital_pin <= 5) {
        // D0..D5 -> Port B
        *port = PORT_B;
        *pin  = digital_pin;
        return true;
    }
#endif

    return false;
}


bool avr_pin_valid(AVR_Port_t port, uint8_t pin) {
    if (pin > 7) return false;

    switch (port) {
    #ifdef VARIANT_MEGA328
        case PORT_B: return (pin <= 5);   // PB0..PB5 brought out
        case PORT_C: return (pin <= 5);   // PC0..PC5 brought out
        case PORT_D: return (pin <= 7);   // PD0..PD7 brought out
    #elif defined(VARIANT_TINYX5)
        case PORT_B: return (pin <= 5);   // PB0..PB5 brought out
    #endif
        default:     return false;
    }
}


bool avr_digital_pin_valid(uint8_t digital_pin) {
#ifdef VARIANT_MEGA328
    return (digital_pin <= 19);           // D0..D19
#elif defined(VARIANT_TINYX5)
    return (digital_pin <= 5);            // D0..D5
#else
    (void)digital_pin;
    return false;
#endif
}



//=============================================================================
// Register Access Functions
//=============================================================================

volatile uint8_t* avr_get_port_reg(AVR_Port_t port) {
    switch (port) {
        #if defined(PORTB)
        case PORT_B: return &PORTB;
        #endif
        #if defined(PORTC)
        case PORT_C: return &PORTC;
        #endif
        #if defined(PORTD)
        case PORT_D: return &PORTD;
        #endif
        default:     return NULL;
    }
}


volatile uint8_t* avr_get_ddr_reg(AVR_Port_t port) {
    switch (port) {
        #if defined(DDRB)
        case PORT_B: return &DDRB;
        #endif
        #if defined(DDRC)
        case PORT_C: return &DDRC;
        #endif
        #if defined(DDRD)
        case PORT_D: return &DDRD;
        #endif
        default:     return NULL;
    }
}


volatile uint8_t* avr_get_pin_reg(AVR_Port_t port) {
    switch (port) {
        #if defined(PINB)
        case PORT_B: return &PINB;
        #endif
        #if defined(PINC)
        case PORT_C: return &PINC;
        #endif
        #if defined(PIND)
        case PORT_D: return &PIND;
        #endif
        default:     return NULL;
    }
}


uint8_t avr_get_pin_mask(uint8_t pin) {
    return (uint8_t)(1u << (pin & 7));
}



//=============================================================================
// Pin Operation Functions
//=============================================================================

void avr_pin_output(AVR_Port_t port, uint8_t pin) {
    volatile uint8_t *ddr = avr_get_ddr_reg(port);
    if (ddr) *ddr |= (uint8_t)(1u << pin);
}


void avr_pin_input(AVR_Port_t port, uint8_t pin) {
    volatile uint8_t *ddr = avr_get_ddr_reg(port);
    if (ddr) *ddr &= (uint8_t)~(1u << pin);
}


void avr_pin_pullup(AVR_Port_t port, uint8_t pin, bool enable) {
    volatile uint8_t *port_reg = avr_get_port_reg(port);
    if (!port_reg) return;
    if (enable) *port_reg |=  (uint8_t)(1u << pin);
    else        *port_reg &= (uint8_t)~(1u << pin);
}


void avr_pin_write(AVR_Port_t port, uint8_t pin, bool value) {
    volatile uint8_t *port_reg = avr_get_port_reg(port);
    if (!port_reg) return;
    if (value) *port_reg |=  (uint8_t)(1u << pin);
    else       *port_reg &= (uint8_t)~(1u << pin);
}


bool avr_pin_read(AVR_Port_t port, uint8_t pin) {
    volatile uint8_t *pin_reg = avr_get_pin_reg(port);
    if (!pin_reg) return false;
    return (*pin_reg & (uint8_t)(1u << pin)) ? true : false;
}


void avr_pin_toggle(AVR_Port_t port, uint8_t pin) {
    // Writing a 1 to a PINx bit toggles the corresponding PORTx bit. This
    // is a documented AVR feature and is faster than read-modify-write.
    volatile uint8_t *pin_reg = avr_get_pin_reg(port);
    if (pin_reg) *pin_reg |= (uint8_t)(1u << pin);
}



//=============================================================================
// Pin-Change Interrupt (PCINT) Helpers
//
// Group selection is by *port*, not by pin. A pin's PCINT bit index is the
// position within its group's PCMSK register; the group's enable bit lives
// in PCICR (328P) or GIMSK (TinyX5).
//
// Datasheet references:
//   328P:   §13 "I/O-Ports", §12 "External Interrupts"
//   Tiny85: §9  "I/O-Ports", §10 "External Interrupts"
//=============================================================================


AVR_PcintGroup_t avr_get_pcint_group(uint8_t digital_pin) {
#ifdef VARIANT_MEGA328
    // D8..D13 -> Port B -> PCINT0
    // D14..D19 -> Port C -> PCINT1
    // D0..D7  -> Port D -> PCINT2
    if (digital_pin <= 7)  return PCINT_GROUP_2;
    if (digital_pin <= 13) return PCINT_GROUP_0;
    if (digital_pin <= 19) return PCINT_GROUP_1;
#elif defined(VARIANT_TINYX5)
    // Single group covering PB0..PB5 -> D0..D5.
    if (digital_pin <= 5)  return PCINT_GROUP_0;
#endif
    (void)digital_pin;
    return PCINT_GROUP_INVALID;
}


volatile uint8_t* avr_get_pcicr_reg(uint8_t digital_pin) {
    AVR_PcintGroup_t group = avr_get_pcint_group(digital_pin);

#if defined(VARIANT_MEGA328)
    // One PCICR register, three enable bits (PCIE0..PCIE2).
    return (group == PCINT_GROUP_INVALID) ? NULL : &PCICR;

#elif defined(VARIANT_TINYX5)
    // ATtiny uses GIMSK with a single PCIE bit. Same idea, different name
    // and different bit position; see avr_get_pcicr_bit().
    return (group == PCINT_GROUP_INVALID) ? NULL : &GIMSK;

#else
    (void)group;
    return NULL;
#endif
}


uint8_t avr_get_pcicr_bit(uint8_t digital_pin) {
    AVR_PcintGroup_t group = avr_get_pcint_group(digital_pin);
    if (group == PCINT_GROUP_INVALID) return 0xFF;

#if defined(VARIANT_MEGA328)
    // PCIE0/PCIE1/PCIE2 are numerically 0/1/2, so the group number *is*
    // the bit index on the 328P.
    return (uint8_t)group;

#elif defined(VARIANT_TINYX5)
    // GIMSK's PCINT enable is bit 5 (PCIE), regardless of group number.
    return PCIE;

#else
    (void)group;
    return 0xFF;
#endif
}


volatile uint8_t* avr_get_pcmsk_reg(uint8_t digital_pin) {
    AVR_PcintGroup_t group = avr_get_pcint_group(digital_pin);
    if (group == PCINT_GROUP_INVALID) return NULL;

#if defined(VARIANT_MEGA328)
    switch (group) {
        case PCINT_GROUP_0: return &PCMSK0;   // Port B
        case PCINT_GROUP_1: return &PCMSK1;   // Port C
        case PCINT_GROUP_2: return &PCMSK2;   // Port D
        default:            return NULL;
    }

#elif defined(VARIANT_TINYX5)
    // Single PCMSK register; group index is always 0 here.
    return &PCMSK;

#else
    return NULL;
#endif
}


uint8_t avr_get_pcmsk_bit(uint8_t digital_pin) {
    // The bit index within PCMSKx is the bit index of the physical pin
    // within its port. For the 328P:
    //    D8..D13 (PB0..PB5) -> PCINT0..PCINT5
    //    D14..D19 (PC0..PC5) -> PCINT8..PCINT13
    //    D0..D7  (PD0..PD7) -> PCINT16..PCINT23
    //
    // In all three cases the *offset from the base of the group* is just
    // the bit index within the port, so we can derive it directly.
#ifdef VARIANT_MEGA328
    if (digital_pin <= 7)  return (uint8_t)(digital_pin);         // PD0..PD7
    if (digital_pin <= 13) return (uint8_t)(digital_pin - 8);     // PB0..PB5
    if (digital_pin <= 19) return (uint8_t)(digital_pin - 14);    // PC0..PC5
#elif defined(VARIANT_TINYX5)
    if (digital_pin <= 5)  return (uint8_t)(digital_pin);         // PB0..PB5
#endif
    (void)digital_pin;
    return 0xFF;
}



//=============================================================================
// ADC (Analog Read) Helpers
//
// The ADC is a single shared peripheral. Channel selection is by channel
// number, not by port/bit, so this section is intentionally independent of
// the port-mapping code above. The only link is avr_adc_channel_for_pin(),
// which translates a digital pin to its channel using the variant's
// fixed wiring.
//
// Prescaler selection is compile-time (driven by F_CPU) so the ADC clock
// lands between 50 and 200 kHz. The target is ~125 kHz, which is the
// datasheet's recommended value for 10-bit accuracy.
//=============================================================================


bool avr_adc_pin_has_channel(uint8_t digital_pin) {
    return avr_adc_channel_for_pin(digital_pin) != 0xFF;
}


uint8_t avr_adc_channel_for_pin(uint8_t digital_pin) {
#if defined(VARIANT_MEGA328)
    if (digital_pin >= 14 && digital_pin <= 21) {
        return (uint8_t)(digital_pin - 14);
    }
    return 0xFF;

#elif defined(VARIANT_TINYX5)
    // ATtiny85 ADC wiring (see datasheet §17 "Analog-to-Digital Converter"):
    //   PB5 -> ADC0 -> D5
    //   PB2 -> ADC1 -> D2
    //   PB4 -> ADC2 -> D4
    //   PB3 -> ADC3 -> D3
    // ADC2/ADC3 also alias the temperature sensor and 1.1V bandgap; those
    // advanced modes are not exposed here.
    switch (digital_pin) {
        case 5: return 0;   // PB5 / ADC0
        case 2: return 1;   // PB2 / ADC1
        case 4: return 2;   // PB4 / ADC2
        case 3: return 3;   // PB3 / ADC3
        default: return 0xFF;
    }

#else
    (void)digital_pin;
    return 0xFF;
#endif
}


// --- Compile-time prescaler selection -------------------------------------
//
// We want the ADC clock in 50..200 kHz. AVR's ADPS2:0 field selects
// division factors 2, 4, 8, 16, 32, 64, 128. The table below picks the
// smallest factor that keeps the ADC clock at or above 125 kHz when
// possible, otherwise the largest safe one.
//
// The result is a compile-time constant so the compiler can fold it into
// the ADCSRA write. No macros leak into other translation units.

#if !defined(F_CPU)
    // No F_CPU: assume a conservative 16 MHz.
    #define AVR_ADC_PRESCALER_BITS  ((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))  // /128
    #define AVR_ADC_PRESCALER_DIV   128u
#elif F_CPU <= 200000UL
    #define AVR_ADC_PRESCALER_BITS  ((1 << ADPS0))                                 // /2
    #define AVR_ADC_PRESCALER_DIV   2u
#elif F_CPU <= 400000UL
    #define AVR_ADC_PRESCALER_BITS  ((1 << ADPS1))                                 // /4
    #define AVR_ADC_PRESCALER_DIV   4u
#elif F_CPU <= 1000000UL
    #define AVR_ADC_PRESCALER_BITS  ((1 << ADPS1) | (1 << ADPS0))                  // /8
    #define AVR_ADC_PRESCALER_DIV   8u
#elif F_CPU <= 2000000UL
    #define AVR_ADC_PRESCALER_BITS  ((1 << ADPS2))                                 // /16
    #define AVR_ADC_PRESCALER_DIV   16u
#elif F_CPU <= 4000000UL
    #define AVR_ADC_PRESCALER_BITS  ((1 << ADPS2) | (1 << ADPS0))                  // /32
    #define AVR_ADC_PRESCALER_DIV   32u
#elif F_CPU <= 8000000UL
    #define AVR_ADC_PRESCALER_BITS  ((1 << ADPS2) | (1 << ADPS1))                  // /64
    #define AVR_ADC_PRESCALER_DIV   64u
#else
    // 16 MHz and up: /128 keeps the ADC clock at 125 kHz at 16 MHz and
    // 156.25 kHz at 20 MHz, both within spec.
    #define AVR_ADC_PRESCALER_BITS  ((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))  // /128
    #define AVR_ADC_PRESCALER_DIV   128u
#endif

// Sanity-check the resulting ADC clock at compile time. These are not
// errors because some off-spec values still work in practice, but they
// are worth surfacing.
#if defined(F_CPU)
    #if (F_CPU / AVR_ADC_PRESCALER_DIV) > 200000UL
        #warning "ADC clock exceeds 200 kHz; accuracy may be reduced."
    #elif (F_CPU / AVR_ADC_PRESCALER_DIV) < 50000UL
        #warning "ADC clock below 50 kHz; conversions will be slow."
    #endif
#endif


void avr_adc_init(AVR_AdcRef_t ref) {
    // Apply the reference first so the internal-reference settling delay
    // (if any) overlaps with the rest of the setup.
    avr_adc_set_reference(ref);

    // Enable the ADC, set the prescaler, leave auto-trigger off.
    // Writing ADCSRA wholesale is safe here because we are the only
    // writer at init time; later conversions only touch ADSC.
    ADCSRA = (uint8_t)((1 << ADEN) | AVR_ADC_PRESCALER_BITS);

    // Free-running mode is disabled; we start each conversion explicitly.
    ADCSRB = 0;
}


void avr_adc_set_reference(AVR_AdcRef_t ref) {
#if defined(VARIANT_TINYX5)
    // ATtiny85: REFS2 = bit4, REFS1 = bit7, REFS0 = bit6
    ADMUX &= (uint8_t)~((1 << REFS1) | (1 << REFS0) | (1 << REFS2));

    switch (ref) {
        case AVR_ADC_REF_AREF:
            // REFS2:0 = 001 -> 外部 AREF
            ADMUX |= (1 << REFS0);
            break;
        case AVR_ADC_REF_AVCC:
            // REFS2:0 = 000 -> VCC
            break;
        case AVR_ADC_REF_INTERNAL:
            // REFS2:0 = 010 -> 内部 1.1V
            ADMUX |= (1 << REFS1);
            break;
        default:
            break;
    }

    if (ref == AVR_ADC_REF_INTERNAL) {
        for (volatile uint16_t i = 0; i < 100; i++) {
            __asm__ __volatile__("nop");
        }
    }
#else
    ADMUX &= (uint8_t)~((1 << REFS1) | (1 << REFS0));
    switch (ref) {
        case AVR_ADC_REF_AREF:     break;
        case AVR_ADC_REF_AVCC:     ADMUX |= (1 << REFS0); break;
        case AVR_ADC_REF_INTERNAL: ADMUX |= (1 << REFS1) | (1 << REFS0); break;
        default: break;
    }
    if (ref == AVR_ADC_REF_INTERNAL) {
        for (volatile uint8_t i = 0; i < 100; i++) {
            __asm__ __volatile__("nop");
        }
    }
#endif
}


void avr_adc_start(uint8_t channel) {
    // Select the channel while preserving the reference bits. ADMUX's
    // MUX field width differs between variants:
    //   328P: MUX3:0 (bits 3:0)
    //   TinyX5: MUX3:0 (bits 3:0), but only MUX1:0 are channel bits and
    //           MUX3:2 select the temperature sensor / bandgap. Since we
    //           only expose the four single-ended channels, masking to
    //           bits 3:0 is correct on both.
    ADMUX = (uint8_t)((ADMUX & 0xF0) | (channel & 0x0F));

    // Start the conversion. ADSC stays set until the conversion completes,
    // then clears itself in single-conversion mode.
    ADCSRA |= (1 << ADSC);
}


bool avr_adc_ready(void) {
    return (ADCSRA & (1 << ADSC)) == 0;
}


uint16_t avr_adc_result(void) {
    // ADC is a 16-bit register pair; reading it clears ADIF.
    return ADC;
}


uint16_t avr_adc_read(uint8_t channel) {
#if defined(VARIANT_MEGA328)
    if (channel > 7) return 0;   // only ADC0..ADC7 are brought out
#elif defined(VARIANT_TINYX5)
    if (channel > 3) return 0;   // only ADC0..ADC3 are brought out
#else
    (void)channel;
    return 0;
#endif

    avr_adc_start(channel);
    while (!avr_adc_ready()) {
        // Busy-wait. A single 10-bit conversion takes ~13 ADC clocks,
        // i.e. ~104 us at 125 kHz. Nothing else to do here.
    }
    return avr_adc_result();
}



//=============================================================================
// String Names for Debugging
//=============================================================================

void avr_port_name(AVR_Port_t port, char* buffer, size_t buflen) {
    if (!buffer || buflen == 0) return;

    const char* name;
    switch (port) {
        #if defined(PORTB)
        case PORT_B: name = "PORTB"; break;
        #endif
        #if defined(PORTC)
        case PORT_C: name = "PORTC"; break;
        #endif
        #if defined(PORTD)
        case PORT_D: name = "PORTD"; break;
        #endif
        default:     name = "INVALID"; break;
    }
    strlcpy(buffer, name, buflen);
}


void avr_pin_name(AVR_Port_t port, uint8_t pin, char* buffer, size_t buflen) {
    if (!buffer || buflen == 0) return;

    uint8_t digital = avr_pin_to_digital(port, pin);
    if (digital == 0xFF) {
        strlcpy(buffer, "INVALID", buflen);
        return;
    }

    // D14..D19 are the analog pins A0..A5 on the 328P. Show both names.
#ifdef VARIANT_MEGA328
    if (digital >= 14 && digital <= 19) {
        snprintf(buffer, buflen, "D%d/A%d", digital, digital - 14);
        return;
    }
#endif
    snprintf(buffer, buflen, "D%d", digital);
}



//=============================================================================
// Pin Information Structure
//=============================================================================

AVR_PinInfo_t avr_get_pin_info(AVR_Port_t port, uint8_t pin) {
    AVR_PinInfo_t info;

    info.port        = port;
    info.pin         = pin;
    info.digital_pin = avr_pin_to_digital(port, pin);

    avr_port_name(port, info.port_name, sizeof(info.port_name));
    avr_pin_name(port, pin, info.pin_name, sizeof(info.pin_name));

    info.port_reg = avr_get_port_reg(port);
    info.ddr_reg  = avr_get_ddr_reg(port);
    info.pin_reg  = avr_get_pin_reg(port);

    return info;
}