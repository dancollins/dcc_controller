#ifndef _DCC_H
#define _DCC_H


#include <stdint.h>

#include "stm32f10x.h"

#include "dcc_hal.h"


#define DCC_N_TRAINS (10)


/**
 * Prepare the DCC library for use. Also powers the track!
 */
extern void
dcc_init(void);


/**
 * This needs to be called periodically to write the train speeds to the
 * rail
 */
extern void
dcc_update(void);


/**
 * Update the speed value for the given train. No signal is sent for a train
 * unless the speed has been set here
 */
extern void
dcc_set_speed(uint8_t address, uint8_t speed, bool is_forward);


extern void
dcc_e_stop(bool enabled);


#endif /* _DCC_H */
