#include "timer.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef F_CPU
#error F_CPU not defined.
#endif

// automatically calculate prescaler and OCR0A based on F_CPU
#if F_CPU == 16500000UL
    // 16.5MHz: Use prescaler 128, OCR0A = 128
    // 16,500,000 / 128 / 1000 = 128.90625 - 1 = 127.90625 (round to 128)
    #define TIMER0_PRESCALER_OPTION (1 << CS02)  // Prescaler 128
    #define OCR0A_VALUE 64
#elif F_CPU == 16000000UL
    // 16MHz: Use prescaler 64, OCR0A = 249
    #define TIMER0_PRESCALER_OPTION (1 << CS01) | (1 << CS00)  // Prescaler 64
    #define OCR0A_VALUE 249
    
#elif F_CPU == 8000000UL
    // 8MHz: Use prescaler 64, OCR0A = 124
    // 8,000,000 / 64 / 1000 = 125 - 1 = 124
    #define TIMER0_PRESCALER_OPTION (1 << CS01) | (1 << CS00)  // Prescaler 64
    #define OCR0A_VALUE 124
    
#elif F_CPU == 20000000UL
    // 20MHz: Use prescaler 64, OCR0A = 311
    // 20,000,000 / 64 / 1000 = 312.5 
    // error of 0.16% due to non-integer value
    #define TIMER0_PRESCALER_OPTION (1 << CS01) | (1 << CS00)  // Prescaler 64
    #define OCR0A_VALUE 311 
    
#elif F_CPU == 12000000UL
    // 12MHz: Use prescaler 64, OCR0A = 186
    // 12,000,000 / 64 / 1000 = 187.5 - 1 = 186
    #define TIMER0_PRESCALER_OPTION (1 << CS01) | (1 << CS00)  // Prescaler 64
    #define OCR0A_VALUE 186
    
#elif F_CPU == 1000000UL
    // 1MHz: Use prescaler 8, OCR0A = 124
    // 1,000,000 / 8 / 1000 = 125 - 1 = 124
    #define TIMER0_PRESCALER_OPTION (1 << CS01)  // Prescaler 8
    #define OCR0A_VALUE 124
    
#elif F_CPU <= 500000UL
    // perscaler 1 for very low speeds
    #define TIMER0_PRESCALER_OPTION (1 << CS00)  // Prescaler 1
    #define OCR0A_VALUE ((F_CPU / 1000) - 1)
    
#else
    #error "Unsupported F_CPU for millis() implementation"
#endif

// Verify OCR0A fits in 8-bit register (0-255)
#if OCR0A_VALUE > 255
    #error "OCR0A_VALUE exceeds 255. Need different prescaler for this F_CPU"
#endif

volatile uint32_t timer0_millis = 0;

void init_timer0_millis(void) {
    TCCR0B = 0; // stop timer0 to configure

    TCCR0A = (1 << WGM01); // configure for CTC mode

    OCR0A = OCR0A_VALUE; // set compare value

    TCCR0B = TIMER0_PRESCALER_OPTION; // set prescaler

    //TIMSK0 |= (1 << OCIE0A); // enable match a interrupt

    /* Enable Output Compare A interrupt.
     * ATtiny25/45/85 use TIMSK (all timers share one register).
     * ATmega328P and most ATmega use TIMSK0 (one per timer).              */
    #if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
        TIMSK |= (1 << OCIE0A);
    #elif defined(TIMSK0)
        TIMSK0 |= (1 << OCIE0A);
    #elif defined(TIMSK)
        TIMSK  |= (1 << OCIE0A);          /* fallback for other ATtiny/ATmega    */
    #else
        #error "Cannot determine timer interrupt mask register (TIMSK0 / TIMSK)."
    #endif
}

void __attribute__((weak)) isr_ms_timer(void) {
    // default implementation is empty to be overridden
}

ISR(TIMER0_COMPA_vect) {
    timer0_millis++;
    isr_ms_timer();
}

uint32_t millis(void) {
    uint32_t ms;
    cli();
    ms = timer0_millis;
    sei();
    return ms;
}

void busy_delay(uint16_t ms) {
    uint32_t start = millis();
    while ((millis() - start) < ms) asm volatile("nop");
}