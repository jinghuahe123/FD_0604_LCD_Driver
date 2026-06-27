#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __AVR_ATmega328P__
#warning Chip is not certified for this code. ADC bits may be set incorrectly.
#endif

typedef enum {
    ADC_REF_AREF = 0,      // external reference on AREF pin
    ADC_REF_AVCC = 1,      // AVcc (5V) with external capacitor on AREF
    ADC_REF_INTERNAL = 3   // internal 1.1V reference
} adc_reference_t;

void init_adc(adc_reference_t ref);
uint16_t analog_read(uint8_t channel);


#ifdef __cplusplus
}
#endif

#endif // ADC_H