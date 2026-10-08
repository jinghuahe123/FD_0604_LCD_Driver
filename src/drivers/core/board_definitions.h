#ifndef DRIVER_CORE_BOARD_DEFINITIONS_H
#define DRIVER_CORE_BOARD_DEFINITIONS_H

#ifndef AVR
#error "This code is intended for AVR microcontrollers only."
#endif

// flag to treat similar microcontroller variants as equal
#ifndef TREAT_VARIANTS_AS_EQUAL
#define TREAT_VARIANTS_AS_EQUAL
#endif

#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
    #ifndef VARIANT_MEGA328
        #define VARIANT_MEGA328
    #endif
#endif

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__)
    #ifndef VARIANT_MEGA168
        #define VARIANT_MEGA168
    #endif
    #ifdef TREAT_VARIANTS_AS_EQUAL
        #ifndef VARIANT_MEGA328
            #define VARIANT_MEGA328
        #endif
        #warning "Treating ATmega168 and ATmega328 as equal. This may cause issues if the code uses features specific to one of these chips."
    #endif
#endif

#if defined(__AVR_ATmega328PB__)
    #ifndef VARIANT_MEGA328PB
        #define VARIANT_MEGA328PB
    #endif
    #ifdef TREAT_VARIANTS_AS_EQUAL
        #ifndef VARIANT_MEGA328
            #define VARIANT_MEGA328
        #endif
        #warning "Treating ATmega328PB and ATmega328 as equal. This may cause issues if the code uses features specific to one of these chips."
    #endif
#endif



#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    #ifndef VARIANT_TINYX5
        #define VARIANT_TINYX5
    #endif
#endif





#if defined(VARIANT_MEGA328)
    #define TIMER0_PWM_PIN_A 6  // OC0A
    #define TIMER0_PWM_PIN_B 5  // OC0B
    #define TIMER1_PWM_PIN_A 9  // OC1A
    #define TIMER1_PWM_PIN_B 10 // OC1B
    #define TIMER2_PWM_PIN_A 11 // OC2A
    #define TIMER2_PWM_PIN_B 3  // OC2B
#elif defined(VARIANT_TINYX5)
    #define TIMER0_PWM_PIN_A 0  // OC0A (PB0)
    #define TIMER0_PWM_PIN_B 1  // OC0B (PB1)
    #define TIMER1_PWM_PIN_A 4  // OC1A (PB4)
#endif





#endif // DRIVER_CORE_BOARD_DEFINITIONS_H
