#include "lcd.h"
#include "systick.h"

#include <string.h>

#include "stm32f10x.h"

static GPIO_InitTypeDef gpio;

/**
 * Toggle the enable line
 */
static inline void toggle_e(void) {
  GPIO_WriteBit(GPIOB, GPIO_Pin_2, SET);
  GPIO_WriteBit(GPIOB, GPIO_Pin_2, RESET);
}

/**
 * Blocking delay for the LCD busy bit
 */
#if 0
static void wait_busy(void) {
  gpio.GPIO_Pin = GPIO_Pin_3;
  gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &gpio);

  /* Read in command mode */
  GPIO_WriteBit(GPIOB, GPIO_Pin_1, SET);
  GPIO_WriteBit(GPIOB, GPIO_Pin_0, RESET);

  toggle_e();

  /* Wait until the busy flag is clear */
  while(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == Bit_SET) {
	systick_delay(1);
	toggle_e();
  }

  gpio.GPIO_Pin = GPIO_Pin_3;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &gpio);
}
#endif

/**
 * Send data to the LCD
 */
static void send_data(bool command, uint8_t data) {
  uint8_t stat;
  uint16_t port;

  systick_delay(1);

  /* Set command mode */
  stat = command ? RESET : SET;
  GPIO_WriteBit(GPIOB, GPIO_Pin_0, stat);

  /* Set RW */
  GPIO_WriteBit(GPIOB, GPIO_Pin_1, RESET);

  systick_delay(1);

  /* Write the data */
  port = GPIO_ReadInputData(GPIOC) & 0xfff0;
  port |= (data >> 4);
  GPIO_Write(GPIOC, port);

  toggle_e();

  port = GPIO_ReadInputData(GPIOC) & 0xfff0;
  port |= (data & 0xf);
  GPIO_Write(GPIOC, port);

  toggle_e();
}

void lcd_init(void) {
  /* GPIO Clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC,
						 ENABLE);

  /* Configure everything as output */
  gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &gpio);

  gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_Init(GPIOC, &gpio);

  /* Clear the control and data lines */
  GPIO_ResetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2);
  GPIO_ResetBits(GPIOC, GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3);

  /* Wait for the LCD to turn on */
  systick_delay(45);

  /* Set the function mode 3 times to start the display. We can't use the
   * busy flag yet */
  GPIO_SetBits(GPIOC, GPIO_Pin_0 | GPIO_Pin_1);
  toggle_e();
  systick_delay(5);
  toggle_e();
  systick_delay(100);
  toggle_e();

  /* 4 bit mode */
  systick_delay(2);
  GPIO_ResetBits(GPIOC, GPIO_Pin_0);
  toggle_e();

  /* 4bit mode, 1/16 duty, 5x7 characters */
  send_data(true, 0x28);
  /* Increment cursor after each character */
  send_data(true, 0x06);
  /* Turn the display on with no cursor */
  send_data(true, 0x0C);
  /* Clear the display */
  send_data(true, 0x01);
  /* Set the cursor to the first position */
  lcd_set_cursor(0, 0);
}

void lcd_clear(void) {
  send_data(true, 0x01);
}

void lcd_put_string(char *str) {
  int i;

  for (i = 0; i < strlen(str); i++) {
	send_data(false, str[i]);
  }
}

void lcd_set_cursor(uint8_t x, uint8_t y) {
  /* Constrain the values to fit on the screen */
  x = x <= 15 ? x : 15;
  y = y <= 1 ? y : 1;

  send_data(true, 0x80 | (x + 40 * y));
}
