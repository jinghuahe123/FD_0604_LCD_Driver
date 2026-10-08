#include "timer0.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "drivers/core/board_definitions.h"

#ifndef F_CPU
#error F_CPU not defined.
#endif

#ifndef INC_FREERTOS_H

// Calculate microseconds per timer0 overflow (64 * 256 * 1e6 / F_CPU)
#define MICROSECONDS_PER_TIMER0_OVERFLOW \
    (64UL * 256UL * 100000UL / ((F_CPU + 5UL) / 10UL))

// Whole milliseconds per overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000U)

// Fractional milliseconds per overflow (shifted right by 3 to fit in byte)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000U) >> 3)
#define FRACT_MAX (1000U >> 3)


static volatile uint32_t timer0_millis = 0;
static volatile uint8_t timer0_fract = 0;

void init_timer0(void) {
    // Stop timer0
    TCCR0B = 0;

    // Configure for Fast PWM mode (Arduino standard)
    #if defined(TCCR0A) && defined(WGM01)
        //TCCR0A |= _BV(WGM01) | _BV(WGM00) | _BV(COM0A1) | _BV(COM0B1);
    #endif

    // Set timer 0 prescale factor to 64 (Arduino standard)
    #if defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__)
        TCCR0 |= _BV(WGM01) | _BV(WGM00) | _BV(CS02);
    #elif defined(TCCR0) && defined(CS01) && defined(CS00)
        TCCR0 |= _BV(CS01) | _BV(CS00);
        #if defined(WGM00) && defined(WGM01)
            TCCR0 |= _BV(WGM01) | _BV(WGM00);
        #endif
    #elif defined(TCCR0B) && defined(CS01) && defined(CS00)
        TCCR0B |= _BV(CS01) | _BV(CS00);
    #elif defined(TCCR0A) && defined(CS01) && defined(CS00)
        TCCR0A |= _BV(CS01) | _BV(CS00);
    #else
        #error "Timer 0 prescale factor 64 not set correctly"
    #endif

    // Enable timer 0 overflow interrupt
    #if defined(TIMSK) && defined(TOIE0)
        TIMSK |= _BV(TOIE0);
    #elif defined(TIMSK0) && defined(TOIE0)
        TIMSK0 |= _BV(TOIE0);
    #else
        #error "Timer 0 overflow interrupt not set correctly"
    #endif
}

void __attribute__((weak)) isr_ms_timer(void) {
    // default implementation is empty to be overridden
}

ISR(TIMER0_OVF_vect) {
    unsigned long m = timer0_millis;
    unsigned char f = timer0_fract;

    f += FRACT_INC;

    if (f >= FRACT_MAX) {
        f -= FRACT_MAX;
        m += MILLIS_INC + 1;
    } else {
        m += MILLIS_INC;
    }

    timer0_fract = f;
    timer0_millis = m;
    isr_ms_timer();
}

uint32_t millis(void) {
    uint32_t m;
    uint8_t oldSREG = SREG;

    cli();
    m = timer0_millis;
    SREG = oldSREG;

    return m;
}

void busy_delay(uint16_t ms) {
    uint32_t start = millis();
    while ((millis() - start) < ms) asm volatile("nop");
}



// ================================== HardwarePin public api ==================================

void timer0_pwm_enable_a(void) {
    #if defined(TCCR0A) && defined(WGM01) && defined(WGM00) && defined(COM0A1)
        // Enable Fast PWM waveform mode (shared by both channels,
        // safe to set even if channel B already set it)
        TCCR0A |= _BV(WGM01) | _BV(WGM00);

        // Connect OC0A to the pin
        TCCR0A |= _BV(COM0A1);

        OCR0A = 0;
    #endif
}

void timer0_pwm_enable_b(void) {
    #if defined(TCCR0A) && defined(WGM01) && defined(WGM00) && defined(COM0B1)
        TCCR0A |= _BV(WGM01) | _BV(WGM00);
        TCCR0A |= _BV(COM0B1);

        OCR0B = 0;
    #endif
}

void timer0_pwm_set_duty_a(uint8_t duty) {
    OCR0A = duty;
}

void timer0_pwm_set_duty_b(uint8_t duty) {
    OCR0B = duty;
}

void timer0_pwm_disable_a(void) {
    #if defined(TCCR0A) && defined(COM0A1)
        TCCR0A &= ~((1 << COM0A1) | (1 << COM0A0));
    #endif
}

void timer0_pwm_disable_b(void) {
    #if defined(TCCR0A) && defined(COM0B1)
        TCCR0A &= ~((1 << COM0B1) | (1 << COM0B0));
    #endif
}


#else // INC_FREERTOS_H

#include "FreeRTOS.h"
#include "task.h"

uint32_t millis(void) {
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

#endif // INC_FREERTOS_H