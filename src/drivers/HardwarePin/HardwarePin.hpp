#pragma once

#include "drivers/core/avr_pins.h"

/**
 * @brief A concrete Pin backed by a physical AVR I/O pin.
 *
 * HardwarePin caches the three register pointers (PORTx / DDRx / PINx)
 * and the bit mask for its pin at construction time, so digitalWrite()
 * and digitalRead() reduce to a single masked load/store on the cached
 * pointer. The register-pointer lookup itself is delegated to the C
 * shim in avr_pins.h, which owns the variant-specific pin mapping.
 *
 * Analog reads are delegated to the ADC layer in avr_pins.h. The ADC is
 * a shared peripheral, so it must be initialized once via initADC()
 * before any analogRead() call. initADC() is idempotent.
 */
class HardwarePin {
    public:
        /** ADC reference voltage options. Mirrors AVR_AdcRef_t. */
        enum class adcRef_t : uint8_t {
            AREF     = AVR_ADC_REF_AREF,      ///< External reference on AREF pin
            AVCC     = AVR_ADC_REF_AVCC,      ///< AVcc (usually 5V), cap on AREF
            INTERNAL = AVR_ADC_REF_INTERNAL   ///< Internal 1.1V reference
        };

        /** Modes to configure the pin. */
        enum class Mode : uint8_t {
            INPUT,
            OUTPUT,
            INPUT_PULLUP
        };

                /**
         * @brief Construct a pin from its Arduino-style digital number.
         *
         * Resolves the pin to its (port, bit) pair via the variant's pin
         * map and caches the three register pointers (PORTx / DDRx / PINx)
         * and the bit mask, so digitalWrite() / digitalRead() / setMode()
         * reduce to a single masked load or store on the cached pointer.
         *
         * Also derives the capability flags. The invariant established
         * here — and relied on by the capability accessors — is:
         *
         *     capability bit set  <=>  the pointer it needs is non-null
         *
         * Specifically:
         *   - DIGITAL_WRITE is set only when _portReg is non-null.
         *   - DIGITAL_READ  is set only when _pinReg  is non-null.
         *   - ANALOG_WRITE  is set only on Timer0 comparator pins, which
         *                   are necessarily real digital pins, so it
         *                   implies DIGITAL_WRITE.
         *   - ANALOG_READ   is set only when avr_adc_pin_has_channel()
         *                   says so. This is the one capability that is
         *                   independent of the port registers: ADC-only
         *                   pins (A6/A7 on the 328P) have an ADC channel
         *                   but no PORTx/DDRx/PINx.
         *
         * Because of that invariant, callers may gate on a capability
         * accessor and then dereference the corresponding pointer without
         * a second null check.
         *
         * Invalid pin numbers still construct: the pin map lookup fails,
         * every register pointer stays null, and every digital capability
         * bit stays clear. Every operation on such a pin is then a no-op
         * (or returns false / UINT16_MAX for analogRead()). The only
         * capability that can still be set on a pin with no port registers
         * is ANALOG_READ, for ADC-only pins as noted above.
         */
        HardwarePin(uint8_t pin);
        ~HardwarePin() = default;

        /** @brief Configure the pin as input, output, or input-with-pullup. */
        void setMode(Mode mode);

        /** @brief Drive the pin high (true) or low (false). */
        void digitalWrite(bool value);

        /** @brief Read the instantaneous logic level on the pin. */
        uint8_t digitalRead();

        #ifdef ANALOG_WRITE_ENABLE
        /** @brief Set the PWM duty cycle (0..255) on a Timer0 PWM pin. */
        void analogWrite(uint8_t value);
        #endif

        /**
         * @brief Initialize the shared ADC peripheral.
         *
         * Sets the reference voltage, selects a prescaler from F_CPU,
         * and enables the ADC. Safe to call more than once; the most
         * recent call wins. Must be called before analogRead().
         *
         * @param ref  Reference voltage selection. Defaults to AVCC.
         */
        static void initADC(adcRef_t ref = adcRef_t::AVCC);

        /**
         * @brief Perform a single blocking 10-bit conversion on this pin.
         *
         * @return 0..1023 on success. For a pin that has no ADC channel
         *         on this variant, returns UINT16_MAX (a deliberate
         *         out-of-band value so a misconfigured pin is obvious
         *         rather than silently reading 0).
         */
        uint16_t analogRead();

        /**
         * @brief Get the pin number.
         * @return The pin number.
         */
        uint8_t getPin() const { return _pin; }

    private:
        const uint8_t _pin;
        volatile uint8_t* _portReg = nullptr;   ///< &PORTx (writes / pull-ups)
        volatile uint8_t* _pinReg  = nullptr;   ///< &PINx  (reads)
        volatile uint8_t* _ddrReg  = nullptr;   ///< &DDRx  (setMode)
        uint8_t           _mask    = 0;         ///< 1 << bit

        /**
         * Compile-time-known capability bits for this pin, packed into a byte. 
         */
        uint8_t pinCapabilities = 0;

        enum class CapabilityBit : uint8_t {
            DIGITAL_WRITE = 0x01,
            DIGITAL_READ  = 0x02,
            ANALOG_READ   = 0x04,
            #ifdef ANALOG_WRITE_ENABLE
            ANALOG_WRITE  = 0x08
            #endif
        };

        inline uint8_t isDigitalWriteCapable() const {
            return (pinCapabilities & static_cast<uint8_t>(CapabilityBit::DIGITAL_WRITE));
        }
        inline uint8_t isDigitalReadCapable() const {
            return (pinCapabilities & static_cast<uint8_t>(CapabilityBit::DIGITAL_READ));
        }
        inline uint8_t isAnalogReadCapable() const {
            return (pinCapabilities & static_cast<uint8_t>(CapabilityBit::ANALOG_READ));
        }
        #ifdef ANALOG_WRITE_ENABLE
        inline uint8_t isAnalogWriteCapable() const {
            return (pinCapabilities & static_cast<uint8_t>(CapabilityBit::ANALOG_WRITE));
        }
        #endif

        /**
         * @brief Detach this pin from Timer0 PWM, if it is currently
         *        driven by it.
         *
         * Called from digitalWrite() so that writing a digital level on
         * a pin that was previously used with analogWrite() takes over
         * the pin, matching Arduino semantics. No-op on non-PWM pins.
         */
        #ifdef ANALOG_WRITE_ENABLE
        void detachFromTimer0PWM();
        #endif
};