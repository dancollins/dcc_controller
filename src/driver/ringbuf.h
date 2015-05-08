#ifndef __RINGBUF_H
#define __RINGBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct ringbuf_t {
  volatile uint8_t *buffer;
  volatile size_t head;
  volatile size_t tail;
  volatile size_t length;
  size_t capacity;
} ringbuf_t;

/**
 * Initialises a ring buffer
 * @param buf A block of memory to use
 * @param size The size in octets of the buffer
 */
extern void ringbuf_init(ringbuf_t *ringbuf, volatile uint8_t *buf, size_t size);

/**
 * Write data into ring buffer
 * @param ringbuf The buffer to write into
 * @param data The data to write into the buffer
 * @param len The number of octets to write
 * @returns The number of octets written
 */
extern size_t ringbuf_write(ringbuf_t *ringbuf, uint8_t *data, size_t len);


/**
 * Read data from ring buffer
 * @param ringbuf The buffer to read from
 * @param data Where to store the read data
 * @param len The number of octets to read
 * @returns The number of octets read
 */
extern size_t ringbuf_read(ringbuf_t *ringbuf, uint8_t *data, size_t len);

/**
 * Flush all data from the ring buffer
 * @param ringbuf The buffer to flush */
extern void ringbuf_flush(ringbuf_t *ringbuf);


/**
 * Get the number of bytes in the ring buffer
 * @param ringbuf The ring buffer to test
 * @returns The number of bytes in the ring buffer
 */
extern size_t ringbuf_get_len(ringbuf_t *ringbuf);


/**
 * Test if the ring buffer contains data
 * @param ringbuf The ring buffer to test
 * @returns True if the ring buffer contains data
 */
extern bool ringbuf_has_data(ringbuf_t * ringbuf);

#endif
