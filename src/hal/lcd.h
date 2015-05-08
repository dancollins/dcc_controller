#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>
#include <stdbool.h>

extern void lcd_init(void);
extern void lcd_clear(void);
extern void lcd_put_string(char *str);
extern void lcd_set_cursor(uint8_t x, uint8_t y);

#endif /* __LCD_H */
