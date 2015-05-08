#ifndef _SYSTICK_H
#define _SYSTICK_H

#include <stddef.h>
#include <stdbool.h>

extern volatile size_t systicks;

extern void systick_init(void);

/**
 * Create a blocking delay.
 * @param duration How long, in ms, to delay for
 */
extern void systick_delay(size_t duration);

/**
 * Test to see if a certain duration has passed.  It is important to perform the
 * check this way, as it takes overflow into account.  It is not possible to
 * test for durations longer than 49 days!
 * @param timestamp The time to start testing from
 * @param duration The number of milliseconds before returning true
 * @returns true if the number of milliseconds has passed
 */
extern bool systick_test_duration(size_t timestamp, size_t duration);

#endif
