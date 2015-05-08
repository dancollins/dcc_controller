#include "systick.h"

#include <stdio.h>

#include "stm32f10x.h"

volatile size_t systicks;

void systick_init(void) {
  /* Set SysTick timer for 1ms interrupts */
  if (SysTick_Config(SystemCoreClock / 1000)) {
    /* TODO: Assertions */
    fprintf(stderr, "Error setting systick clock!\n");
    while(1);
  }
}

void systick_delay(size_t duration) {
  size_t timestamp = systicks;

  /* It's important to clear the WDT while we're delaying otherwise long delays
   * restart the MCU. There's no point worrying about the systick timer failing
   * as the whole thing is broken at that point..! */
  /* TODO: Add the WDT */
  while (!systick_test_duration(timestamp, duration))
    ;
}

bool systick_test_duration(size_t timestamp, size_t duration) {
  size_t delta = systicks - timestamp;

  return delta >= duration;
}

void SysTick_Handler(void) {
  systicks++;
}
