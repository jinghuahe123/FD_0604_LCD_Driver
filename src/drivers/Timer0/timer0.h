#ifndef DRIVER_CORE_TIMER_H
#define DRIVER_CORE_TIMER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INC_FREERTOS_H
    #warning "FreeRTOS is included, timer0 PWM functions are disabled. millis() falls back to using the FreeRTOS tick count. Use FreeRTOS delay functions instead of busy_delay()."
#else

// Note: PWM frequency = F_CPU / (prescaler * 256)
// For 16MHz with prescaler 64: ~976.56Hz
void init_timer0(void);
extern void isr_ms_timer(void);

void busy_delay(uint16_t ms);

// left public for HardwarePin pwm functionability, but not intended for direct use by user code
void timer0_pwm_enable_a(void);
void timer0_pwm_enable_b(void);
void timer0_pwm_set_duty_a(uint8_t duty);
void timer0_pwm_set_duty_b(uint8_t duty);
void timer0_pwm_disable_a(void);
void timer0_pwm_disable_b(void);

#endif // INC_FREERTOS_H

uint32_t millis(void);

#ifdef __cplusplus
}
#endif

#endif // DRIVER_CORE_TIMER_H