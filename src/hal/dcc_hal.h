#ifndef _DCC_HAL_H
#define _DCC_HAL_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "stm32f10x.h"


/*
 * GPIO
 */
#define DCC_HAL_GPIO         (GPIOB)
#define DCC_HAL_GPIO_RCC     (RCC_APB2Periph_GPIOB)
#define DCC_HAL_GPIO_PIN_1   (GPIO_Pin_0)
#define DCC_HAL_GPIO_PIN_2   (GPIO_Pin_1)


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
extern uint8_t
dcc_hal_write(uint8_t *data, uint8_t len);


#endif /* _DCC_HAL_H */
