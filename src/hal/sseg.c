#include "sseg.h"

#include "systick.h"


uint8_t sseg_l_lut[] =
{
    0xeb, 0x60, 0xce, 0xec, 0x65, 0xad, 0xaf, 0xe0,
    0xef, 0xed, 0xe7, 0x2f, 0x8b, 0x6e, 0x8f, 0x87
};

uint8_t sseg_r_lut[] =
{
    0x77, 0x60, 0x5b, 0x7a, 0x6c, 0x3e, 0x3f, 0x70,
    0x7f, 0x7e, 0x7d, 0x2f, 0x17, 0x6b, 0x1f, 0x1d
};


void
sseg_init(void)
{
    GPIO_InitTypeDef gpio;

    /* Enable the GPIO clocks */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    /* Set all of the seven seg pins as outputs */
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
        GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio);
    GPIO_Init(GPIOC, &gpio);

    sseg_off();
}


void
sseg_set(uint16_t val)
{
    uint16_t gpio;

    /* Left SSD */
    gpio = GPIO_ReadInputData(GPIOC);
    gpio &= ~0xef;
    gpio |= sseg_l_lut[val >> 4];
    GPIO_Write(GPIOC, gpio);

    /* Right SSD */
    gpio = GPIO_ReadInputData(GPIOA);
    gpio &= ~0x7f;
    gpio |= sseg_r_lut[val & 0xf];
    GPIO_Write(GPIOA, gpio);
}


void
sseg_off(void)
{
    uint16_t val;

    /* Turn off the display segments */
    val = GPIO_ReadInputData(GPIOC);
    val &= ~0xef;
    GPIO_Write(GPIOC, val);
    val = GPIO_ReadInputData(GPIOA);
    val &= ~0x7f;
    GPIO_Write(GPIOA, val);;
}


void
sseg_set_dp(bool left, bool right)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_7, right ? Bit_SET : Bit_RESET);
    GPIO_WriteBit(GPIOC, GPIO_Pin_4, left ? Bit_SET : Bit_RESET);
}
