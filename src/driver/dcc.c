#include "dcc.h"

#include "dcc_hal.h"


/**
 * Instruction types for the first data byte. These instructions are
 * left shifted to align with the MSB of the data byte
 */
typedef enum
{
    DCC_INST_TYPE_SPEED_FORWARD = 0x40,
    DCC_INST_TYPE_SPEED_REVERSE = 0x60,
} dcc_inst_type_t;


/**
 * Completed frame ready to pack and write to the wire
 */
typedef struct
{
    uint8_t address;
    uint8_t n_data_bytes;
    uint8_t data[3];
    uint8_t checksum;
} dcc_frame_t;


static bool initialised = false;

void
dcc_init(void)
{
    dcc_hal_init();

    initialised = true;
}


static uint8_t
send_frame(dcc_frame_t *frame)
{
    uint8_t data[5];
    uint8_t len = 0;
    int i;

    if (!initialised)
        dcc_init();

    /* Address */
    data[len++] = frame->address;

    /* Data */
    for (i = 0; i < frame->n_data_bytes; i++)
    {
        data[len++] = frame->data[i];
    }

    /* Checksum */
    data[len++] = frame->checksum;

    /* The sender will then take all the bytes and add an extra 0 bit between
     * each one */
    return dcc_hal_write(data, len);
}


static void
calculate_checksum(dcc_frame_t *frame)
{
    int i;

    frame->checksum = frame->address;
    for (i = 0; i < frame->n_data_bytes; i++)
    {
        frame->checksum ^= frame->data[i];
    }
}


bool
dcc_set_speed(uint8_t address, uint8_t speed, bool is_forward)
{
    dcc_frame_t f;
    uint8_t x;

    /* Configure the frame */
    f.address = address;
    f.n_data_bytes = 1;
    f.data[0] = is_forward ? DCC_INST_TYPE_SPEED_FORWARD :
                             DCC_INST_TYPE_SPEED_REVERSE;

    /* Calculate the speed value, swapping the MSB and LSB */
    speed &= 0x1f;
    x = ((speed >> 4) ^ speed) & 0x1;
    speed = speed ^ ((x << 4) | x);
    f.data[0] |= speed;

    /* Calculate the checksum */
    calculate_checksum(&f);

    /* Send the frame */
    return send_frame(&f) != 0;
}
