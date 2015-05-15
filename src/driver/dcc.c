#include "dcc.h"

#include <string.h>

#include "dcc_hal.h"
#include "systick.h"


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


/**
 * We hold the latest packets for each of the trains, and transmit them in
 * sequence in the update function.
 */
static dcc_frame_t trains[DCC_N_TRAINS];


/** True when the emergency stop is called */
static bool stopped = false;


/* Look up table for the speed values */
static uint8_t speed_lut[] =
{
    0x00, /* Stop */
    0x02, 0x12, 0x03, 0x13, 0x04, 0x14, 0x05, 0x15,
    0x06, 0x16, 0x07, 0x17, 0x08, 0x18, 0x09, 0x19,
    0x0a, 0x1a, 0x0b, 0x1b, 0x0c, 0x1c, 0x0d, 0x1d,
    0x0e, 0x1e, 0x0f, 0x1f
};


static bool initialised = false;

void
dcc_init(void)
{
    dcc_hal_init();

    memset(trains, 0, DCC_N_TRAINS * sizeof(dcc_frame_t));

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


void
dcc_update(void)
{
    int i;

    static dcc_frame_t e_stop =
    {
        .address = 0x00,
        .n_data_bytes = 1,
        .data[0] = 0x41,
        .checksum = 0x41
    };

    if (!stopped)
    {
        for (i = 0; i < DCC_N_TRAINS; i++)
        {
            if (trains[i].address != 0)
            {
                send_frame(&trains[i]);
                /* We delay 10 ms (5ms required) to be safe */
                systick_delay(10);
            }
        }
    }
    else
    {
        send_frame(&e_stop);
    }
}


void
dcc_set_speed(uint8_t address, uint8_t speed, bool is_forward)
{
    dcc_frame_t f;

    /* Make sure the address is valid */
    if (address > DCC_N_TRAINS || address == 0)
    {
        printf("Invalid train address: %02x\n", address);
        return;
    }

    /* Configure the frame */
    f.address = address & 0x7f;
    f.n_data_bytes = 1;
    f.data[0] = is_forward ? DCC_INST_TYPE_SPEED_FORWARD :
                             DCC_INST_TYPE_SPEED_REVERSE;

    /* Make sure the speed is a valid value */
    if (speed > 28)
        speed = 28;
    f.data[0] |= speed_lut[speed];

    /* Calculate the checksum */
    calculate_checksum(&f);

    /* Store the speed */
    memcpy(&trains[address-1], &f, sizeof(dcc_frame_t));
}


void
dcc_e_stop(bool enable)
{
    stopped = enable;

    if (stopped)
    {
        /* Make sure trains don't boost off again when we start back up */
        memset(trains, 0, DCC_N_TRAINS * sizeof(dcc_frame_t));
    }
}
