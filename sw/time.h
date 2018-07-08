#ifndef TIME_H
#define TIME_H
#include <avr/interrupt.h>
#include <inttypes.h>

#define TIME_TIMER_COUNT_REGISTER TCNT2

uint32_t time_counter;

void time_init(void);

uint32_t time_get(void);

// more recent time first
uint32_t time_delta(uint32_t a, uint32_t b);
#endif
