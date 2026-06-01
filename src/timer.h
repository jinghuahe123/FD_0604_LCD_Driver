#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_millis(void);
uint32_t millis(void);
void busy_delay(uint16_t ms);

extern void isr_ms_timer(void) __attribute__((weak));

#ifdef __cplusplus
}
#endif

#endif // TIMER_H