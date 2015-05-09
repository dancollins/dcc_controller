#ifndef _DCC_H
#define _DCC_H


#include <stdint.h>

#include "stm32f10x.h"

#include "dcc_hal.h"


extern void
dcc_init(void);


extern bool
dcc_set_speed(uint8_t address, uint8_t speed, bool is_forward);


#endif /* _DCC_H */
