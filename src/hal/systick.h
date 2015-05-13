#ifndef _SYSTICK_H
#define _SYSTICK_H

#include <stddef.h>
#include <stdbool.h>


typedef void (*systick_callback_t)(void);

extern volatile size_t systicks;

/**
 * Prepare the systick timer, with an optional tick callback */
extern void systick_init(systick_callback_t cb);

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
