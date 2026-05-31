#include "io_helper.h"
#include <avr/io.h>

#ifndef F_CPU
#error F_CPU not defined.
#endif

// automatically select optimal ADC prescaler based on F_CPU
// target ADC clock from 50kHz - 200kHz (125kHz ideal)
#if F_CPU <= 200000UL
    // 200kHz and below: Prescaler 2 (ADC clock = F_CPU/2)
    #define ADC_PRESCALER ((1 << ADPS0))
    #define ADC_PRESCALER_VALUE 2
    
#elif F_CPU <= 400000UL
    // 400kHz: Prescaler 4 (ADC clock = 100kHz)
    #define ADC_PRESCALER ((1 << ADPS1))
    #define ADC_PRESCALER_VALUE 4
    
#elif F_CPU <= 1000000UL
    // 1MHz: Prescaler 8 (ADC clock = 125kHz)
    #define ADC_PRESCALER ((1 << ADPS1) | (1 << ADPS0))
    #define ADC_PRESCALER_VALUE 8
    
#elif F_CPU <= 2000000UL
    // 2MHz: Prescaler 16 (ADC clock = 125kHz)
    #define ADC_PRESCALER ((1 << ADPS2))
    #define ADC_PRESCALER_VALUE 16
    
#elif F_CPU <= 4000000UL
    // 4MHz: Prescaler 32 (ADC clock = 125kHz)
    #define ADC_PRESCALER ((1 << ADPS2) | (1 << ADPS0))
    #define ADC_PRESCALER_VALUE 32
    
#elif F_CPU <= 8000000UL
    // 8MHz: Prescaler 64 (ADC clock = 125kHz)
    #define ADC_PRESCALER ((1 << ADPS2) | (1 << ADPS1))
    #define ADC_PRESCALER_VALUE 64
    
#elif F_CPU <= 16000000UL
    // 16MHz: Prescaler 128 (ADC clock = 125kHz)
    #define ADC_PRESCALER ((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))
    #define ADC_PRESCALER_VALUE 128
    
#elif F_CPU <= 20000000UL
    // 20MHz: Prescaler 128 (ADC clock = 156.25kHz)
    #define ADC_PRESCALER ((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))
    #define ADC_PRESCALER_VALUE 128
    
#else
    // above 20MHz use max prescaler
    #warning "F_CPU > 20MHz may exceed max ADC clock of 200kHz"
    #define ADC_PRESCALER ((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0))
    #define ADC_PRESCALER_VALUE 128
#endif

// calculate actual ADC clock
// safety check
#define ADC_CLOCK (F_CPU / ADC_PRESCALER_VALUE)
#if ADC_CLOCK > 200000
    #warning "ADC clock exceeds 200kHz - accuracy may be reduced"
#elif ADC_CLOCK < 50000
    #warning "ADC clock below 50kHz - conversion time may be slow"
#endif

// current reference setting (store for potential reconfiguration)
static adc_reference_t current_reference = ADC_REF_AVCC;

void analog_reference(adc_reference_t ref) {
    ADMUX &= ~((1 << REFS1) | (1 << REFS0)); // clear reference bits in ADMUX
    
    switch(ref) {
        case ADC_REF_AREF:
            // external reference on AREF pin
            // bits cleared, no need to set anything
            break;
            
        case ADC_REF_AVCC:
            // AVcc (5V usually) with external capacitor on AREF pin
            ADMUX |= (1 << REFS0);
            break;
            
        case ADC_REF_INTERNAL:
            // internal 1.1V reference
            ADMUX |= (1 << REFS1) | (1 << REFS0);
            break;
            
        default:
            // default to AVcc
            ADMUX |= (1 << REFS0);
            ref = ADC_REF_AVCC;
            break;
    }
    
    current_reference = ref;
    
    // wait for reference to stabilize (>100us req. for internal ref)
    if (ref == ADC_REF_INTERNAL) {
        for (volatile uint8_t i = 0; i < 100; i++) {
            asm volatile("nop");
        }
    }
}

void init_adc(adc_reference_t ref) {
    analog_reference(ref); // set reference voltage

    // enable adc with auto prescaler
    ADCSRA = (1 << ADEN) | ADC_PRESCALER;

    ADCSRB = 0; // auto trigger disabled
}

uint16_t analog_read(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); // select ADC channel by clearing low 4 bits, and set new channel

    ADCSRA |= (1 << ADSC); // start conversion by setting ADSC bit

    while (ADCSRA & (1 << ADSC)); // wait for conversion to finish

    return ADC;
}