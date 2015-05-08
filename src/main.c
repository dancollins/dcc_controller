/*
 * DCC Controller
 *
 * Used to control the throttle of trains using the DCC signalling system.
 *
 * Dan Collins 2015
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "stm32f10x.h"

#include "systick.h"
#include "dcc.h"


int
main(void)
{
    systick_init();
    dcc_init();

    printf("\n\n");
    printf("DCC Controller\n");
    printf("Dan Collins 2015\n");

    /* Enable GPIO Clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* Prepare LED pins */
    GPIO_InitTypeDef gpio_cfg;
    gpio_cfg.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    gpio_cfg.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_cfg.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio_cfg);

    while(1)
    {
        GPIO_WriteBit(GPIOB, GPIO_Pin_8, SET);
        systick_delay(250);
        GPIO_WriteBit(GPIOB, GPIO_Pin_8, RESET);
        systick_delay(250);
    }

    return 0;
}


