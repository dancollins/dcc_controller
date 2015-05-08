#ifndef _UART_H
#define _UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


typedef enum uart_file_t {
  UART_GSM_PORT = 0,
  UART_DEBUG_PORT
} uart_file_t;


/** Write a character to the UART.  This is blocking! */
extern void _uart_putch(uint8_t port, uint8_t ch);

/** Read a character from the UART.  This is blocking! */
extern uint8_t _uart_getch(uint8_t port);

/** Initiate the UART ports */
extern void uart_init(void);

/**
 * Change the UART baud rate
 * @param port The port to set the baud for
 * @param baud The new baud rate
 */
extern void uart_set_baud(uint8_t port, uint32_t baud);

/**
 * Enable hardware flow control on the specified uart port
 * @param The port to enable flow control for
 */
extern void uart_enable_flow_control(uint8_t port);

/**
 * Send data out the UART
 * @param port The UART port to send data to
 * @param buf The buffer to read data from
 * @param len The number of bytes to send
 */
extern uint16_t uart_send_data(uint8_t port, uint8_t *buf, uint16_t len);

/**
 * Get data from the UART
 * @param port The UART port to read from
 * @param buf The buffer to save data to
 * @param len The number of bytes to read
 * @returns The number of bytes read
 */
extern uint16_t uart_get_data(uint8_t port, uint8_t *buf, uint16_t len);

/**
 * Clear data from the UART receive buffer
 * @param port The UART port to flush
 */
extern void uart_flush_rx_buffer(uint8_t port);

/**
 * Get the amount of available data in the receive buffer
 * @param port The UART port to test
 * @returns The number of available bytes
 */
extern uint16_t uart_get_buf_length(uint8_t port);

#endif /* _UART_H */
