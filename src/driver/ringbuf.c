#include "ringbuf.h"

#define min(a, b) a > b ? b : a

void ringbuf_init(ringbuf_t *ringbuf, volatile uint8_t *buf, size_t size) {
  ringbuf->buffer = buf;
  ringbuf->capacity = size;
  ringbuf->length = 0;
  ringbuf->head = 0;
  ringbuf->tail = 0;
}

size_t ringbuf_write(ringbuf_t *ringbuf, uint8_t *data, size_t len) {
  uint16_t freespace;

  /* Calculate how much space there is in the buffer */
  freespace = ringbuf->capacity - ringbuf->length;

  /* Constrain length to the space in the buffer */
  len = min(len, freespace);

  /* Copy the data into the buffer */
  for (int i = 0; i < len; i++) {
    ringbuf->buffer[ringbuf->head] = data[i];
    ringbuf->head = (ringbuf->head + 1) % ringbuf->capacity;
  }

  /* Update the buffer length */
  ringbuf->length += len;
  
  return len;
}

size_t ringbuf_read(ringbuf_t *ringbuf, uint8_t *data, size_t len) {
  /* Can't read from an empty buffer */
  if (!ringbuf_has_data(ringbuf))
    return 0;

  /* Calculate how many bytes we can read */
  len = min(len, ringbuf->length);

  /* Copy data from the buffer */
  for (int i = 0; i < len; i++) {
    data[i] = ringbuf->buffer[ringbuf->tail];
    ringbuf->tail = (ringbuf->tail + 1) % ringbuf->capacity;
  }

  /* Update the buffer length */
  ringbuf->length -= len;

  return len;
}

void ringbuf_flush(ringbuf_t *ringbuf) {
  ringbuf->head = 0;
  ringbuf->tail = 0;
  ringbuf->length = 0;
  ringbuf->buffer[0] = 0;
}

size_t ringbuf_get_len(ringbuf_t *ringbuf) {
  return ringbuf->length;
}

bool ringbuf_has_data(ringbuf_t * ringbuf) {
  if (ringbuf->length)
    return true;

  return false;
}
