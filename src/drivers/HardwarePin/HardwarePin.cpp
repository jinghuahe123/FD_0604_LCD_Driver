#include "HardwarePin.hpp"

#ifdef ANALOG_WRITE_ENABLE
#include <drivers/Timer0/timer0.h>
#ifdef USE_FREERTOS
    // Timer0 is already owned by the FreeRTOS tick; analogWrite() on
    // Timer0 pins would fight the scheduler, so disable it entirely.
    #undef ANALOG_WRITE_ENABLE
#endif
#endif

#ifndef F_CPU
#error F_CPU not defined.
#endif


//=============================================================================
// Construction
//=============================================================================

HardwarePin::HardwarePin(uint8_t pin) : _pin(pin) {
    // --- cache register pointers + bit mask -------------------------------
    // avr_digital_to_pin() owns the variant-specific pin -> (port, bit)
    // mapping; we just cache the resulting register pointers here so the
    // hot paths (digitalRead/digitalWrite) don't pay for the switch.
    AVR_Port_t port;
    uint8_t    bit;
    const bool validIOPin = avr_digital_to_pin(_pin, &port, &bit);

    // this check ensures that digitalRead, digitalWrite and analogWrite capability
    // pins are only ever set if the pin is a valid IO pin with a valid port and bit.
    // It speeds up the hot paths by avoiding null pointer checks in those functions.
    if (validIOPin) {
        _portReg = avr_get_port_reg(port);
        _ddrReg  = avr_get_ddr_reg(port);
        _pinReg  = avr_get_pin_reg(port);
        _mask    = avr_get_pin_mask(bit);

        // --- capability flags -------------------------------------------------
        // Digital capability is variant-wide: every brought-out pin supports
        // digital read and write. Analog-read capability is per-pin and comes
        // from the ADC layer, so there is exactly one source of truth for
        // "does this pin have an ADC channel?".
        #if defined(VARIANT_MEGA328)
            if (_pin <= 13 || (_pin >= 14 && _pin <= 19)) {
                pinCapabilities |= static_cast<uint8_t>(CapabilityBit::DIGITAL_WRITE);
                pinCapabilities |= static_cast<uint8_t>(CapabilityBit::DIGITAL_READ);
            }
            #ifdef ANALOG_WRITE_ENABLE
            if (_pin == 5 || _pin == 6) {
                pinCapabilities |= static_cast<uint8_t>(CapabilityBit::ANALOG_WRITE);
            }
            #endif
        #elif defined(VARIANT_TINYX5)
            if (_pin <= 5) {
                pinCapabilities |= static_cast<uint8_t>(CapabilityBit::DIGITAL_WRITE);
                pinCapabilities |= static_cast<uint8_t>(CapabilityBit::DIGITAL_READ);
            }
            #ifdef ANALOG_WRITE_ENABLE
            if (_pin == 0 || _pin == 1) {
                pinCapabilities |= static_cast<uint8_t>(CapabilityBit::ANALOG_WRITE);
            }
            #endif
        #endif
    }

    if (avr_adc_pin_has_channel(_pin)) {
        pinCapabilities |= static_cast<uint8_t>(CapabilityBit::ANALOG_READ);
    }
}


//=============================================================================
// Digital I/O
//=============================================================================

void HardwarePin::setMode(Mode mode) {
    if (!_ddrReg || !_portReg) return;

    switch (mode) {
        case Mode::INPUT:
            *_ddrReg  &= static_cast<uint8_t>(~_mask);
            *_portReg &= static_cast<uint8_t>(~_mask);   // pull-up off
            break;
        case Mode::OUTPUT:
            *_ddrReg |= _mask;
            break;
        case Mode::INPUT_PULLUP:
            *_ddrReg  &= static_cast<uint8_t>(~_mask);
            *_portReg |= _mask;
            break;
    }
}


void HardwarePin::digitalWrite(bool value) {
    if (!isDigitalWriteCapable()) return;

    #ifdef ANALOG_WRITE_ENABLE
    // Take the pin back from Timer0 if analogWrite() was previously used
    // on it.
    if (isAnalogWriteCapable()) detachFromTimer0PWM();
    #endif

    if (value) *_portReg |=  _mask;
    else       *_portReg &= static_cast<uint8_t>(~_mask);
}


uint8_t HardwarePin::digitalRead() {
    if (!isDigitalReadCapable()) return 0;
    return (*_pinReg & _mask) != 0;
}


//=============================================================================
// Timer0 PWM (analogWrite)
//=============================================================================

#ifdef ANALOG_WRITE_ENABLE

void HardwarePin::detachFromTimer0PWM() {
    #if defined(VARIANT_MEGA328)
        // OC0A = digital pin 6, OC0B = digital pin 5
        switch (_pin) {
            case 5: timer0_pwm_disable_b(); break;   // OC0B
            case 6: timer0_pwm_disable_a(); break;   // OC0A
            default: break;                          // not a Timer0 pin
        }
    #elif defined(VARIANT_TINYX5)
        // OC0A = PB0 = digital pin 0, OC0B = PB1 = digital pin 1
        switch (_pin) {
            case 0: timer0_pwm_disable_a(); break;   // OC0A
            case 1: timer0_pwm_disable_b(); break;   // OC0B
            default: break;
        }
    #endif
}


void HardwarePin::analogWrite(uint8_t value) {
    if (!isAnalogWriteCapable()) return;

    switch (_pin) {
        #if defined(VARIANT_MEGA328)
            case 5: // OC0B
                timer0_pwm_enable_b();
                timer0_pwm_set_duty_b(value);
                break;
            case 6: // OC0A
                timer0_pwm_enable_a();
                timer0_pwm_set_duty_a(value);
                break;
        #elif defined(VARIANT_TINYX5)
            case 0: // OC0A
                timer0_pwm_enable_a();
                timer0_pwm_set_duty_a(value);
                break;
            case 1: // OC0B
                timer0_pwm_enable_b();
                timer0_pwm_set_duty_b(value);
                break;
        #endif
        default:
            return;   // not a PWM-capable pin
    }
}

#endif // ANALOG_WRITE_ENABLE


//=============================================================================
// ADC (analogRead)
//=============================================================================

void HardwarePin::initADC(adcRef_t ref) {
    // The ADC is a shared peripheral; the actual register work lives in
    // the C shim so the variant-specific bits (prescaler, reference
    // encoding) stay in one place.
    avr_adc_init(static_cast<AVR_AdcRef_t>(ref));
}


uint16_t HardwarePin::analogRead() {
    if (!isAnalogReadCapable()) {
        // Deliberately out-of-band so a misconfigured pin is obvious.
        // (A plain 0 would be indistinguishable from a grounded input.)
        return UINT16_MAX;
    }

    uint8_t channel = avr_adc_channel_for_pin(_pin);
    if (channel == 0xFF) return UINT16_MAX;

    return avr_adc_read(channel);
}