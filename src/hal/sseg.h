#ifndef _HAL_SSEG_H
#define _HAL_SSEG_H

#include "stm32f10x.h"
#include <stdint.h>
#include <stdbool.h>


extern void
sseg_init(void);


extern void
sseg_set(uint16_t val);


extern void
sseg_off(void);


extern void
sseg_set_dp(bool left, bool right);


#endif
