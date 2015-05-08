#include "uart.h"
#include "ringbuf.h"

#include <stdio.h>

#include "stm32f10x.h"

#define RX_BUF_LEN 1024

static volatile size_t icount = 0;

static uint32_t baud_rate = 115200;
static bool flowcontrol = false;

typedef struct uart_port_t {
  ringbuf_t *rx_ringbuf;
  void *uart;
} uart_port_t;

/* Modem receive buffer */
static volatile uint8_t uart0_rx_buf[RX_BUF_LEN];
static ringbuf_t uart0_ringbuf;

/* UART ports */
#define UART_COUNT 2
static uart_port_t uart_ports[UART_COUNT];


void uart_init(void) {
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef uart;
  NVIC_InitTypeDef nvic;

  /* GPIO Clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

  /* GSM TX, Debug TX */
  gpio.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_2;
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &gpio);

  /* GSM RX */
  gpio.GPIO_Pin = GPIO_Pin_10;
  gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &gpio);

  /* Alternate Functionality */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  /* GSM Interrupt */
  nvic.NVIC_IRQChannel = USART1_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 0;
  nvic.NVIC_IRQChannelSubPriority = 0;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  /* USART */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

  uart.USART_BaudRate = baud_rate;
  uart.USART_WordLength = USART_WordLength_8b;
  uart.USART_StopBits = USART_StopBits_1;
  uart.USART_Parity = USART_Parity_No;
  uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(USART1, &uart);

  uart.USART_BaudRate = 115200;
  uart.USART_Mode = USART_Mode_Tx;
  USART_Init(USART2, &uart);

  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

  USART_Cmd(USART1, ENABLE);
  USART_Cmd(USART2, ENABLE);

  /* Receive buffer */
  ringbuf_init(&uart0_ringbuf, uart0_rx_buf, RX_BUF_LEN);

  /* UART Ports */
  uart_ports[0].rx_ringbuf = &uart0_ringbuf;
  uart_ports[0].uart = USART1;

  uart_ports[1].rx_ringbuf = NULL;
  uart_ports[1].uart = USART2;
}

void uart_set_baud(uint8_t port, uint32_t baud) {
  USART_InitTypeDef uart;

  baud_rate = baud;

  if (flowcontrol)
    uart.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
  else
    uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

  uart.USART_BaudRate = baud_rate;
  uart.USART_WordLength = USART_WordLength_8b;
  uart.USART_StopBits = USART_StopBits_1;
  uart.USART_Parity = USART_Parity_No;
  uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(uart_ports[port].uart, &uart);
}

void uart_enable_flow_control(uint8_t port) {
  USART_InitTypeDef uart;

  uart.USART_BaudRate = baud_rate;
  uart.USART_WordLength = USART_WordLength_8b;
  uart.USART_StopBits = USART_StopBits_1;
  uart.USART_Parity = USART_Parity_No;
  uart.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
  uart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(uart_ports[port].uart, &uart);

  flowcontrol = true;
}

uint16_t uart_send_data(uint8_t port, uint8_t *buf, uint16_t len) {
  if (port > UART_COUNT - 1)
    return 0;

  for (int i = 0; i < len; i++) {
    _uart_putch(port, buf[i]);
  }

  return len;
}

uint16_t uart_get_data(uint8_t port, uint8_t *buf, uint16_t len) {
  return ringbuf_read(uart_ports[port].rx_ringbuf, buf, len);
}

void uart_flush_rx_buffer(uint8_t port) {
  ringbuf_flush(uart_ports[port].rx_ringbuf);
}


uint16_t uart_get_buf_length(uint8_t port) {
  return ringbuf_get_len(uart_ports[port].rx_ringbuf);
}


/*
 * Interrupts
 */
static void uart_rx_handler(uint8_t port) {
  uint16_t c;

  /* There is an issue causing commands to always time out. Added printf so we
   * can see any overrun errors (which shouldn't happen when flow control is
   * enabled). I haven't found a way to produce the error reliably. */

  /* Read the character from the USART */
  c = (uint8_t)USART_ReceiveData(uart_ports[port].uart);

  /* Technically this shouldn't clear the flags, but it seems to be related to
   * the timeout issue. */
  if (USART_GetFlagStatus(uart_ports[port].uart, USART_FLAG_NE) ||
      USART_GetFlagStatus(uart_ports[port].uart, USART_FLAG_FE) ||
      USART_GetFlagStatus(uart_ports[port].uart, USART_FLAG_PE) ||
      USART_GetFlagStatus(uart_ports[port].uart, USART_FLAG_ORE)) {
    c = -1;
    fprintf(stderr, "UART: Error flag set.\n");
  }

  /* Add the character to the buffer */
  if (c != -1)
    ringbuf_write(uart_ports[port].rx_ringbuf, (uint8_t *)&c, 1);
}

void USART1_IRQHandler(void) {
  if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
    uart_rx_handler(UART_GSM_PORT);
  }
}


/*
 * Primitive functions
 */
void _uart_putch(uint8_t port, uint8_t c) {
  while(USART_GetFlagStatus(uart_ports[port].uart, USART_FLAG_TXE) == RESET);
  USART_SendData(uart_ports[port].uart, c);
}

uint8_t _uart_getch(uint8_t port) {
  while(USART_GetITStatus(uart_ports[port].uart, USART_IT_RXNE) == RESET);
  return USART_ReceiveData(uart_ports[port].uart);
}
