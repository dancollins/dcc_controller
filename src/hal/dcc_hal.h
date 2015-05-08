#ifndef _DCC_HAL_H
#define _DCC_HAL_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "stm32f10x.h"


/**
 * Initialises the DCC low level driver
 */
extern void
dcc_hal_init(void);


/**
 * Writes the data out the DCC port
 * \param data memory buffer holding the data to write
 * \param len the number of octets to write
 * \return the number of octets written
 */
extern uint32_t
dcc_hal_write(uint8_t *data, uint32_t len);


#endif /* _DCC_HAL_H */
