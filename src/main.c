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

#include "uart.h"
#include "systick.h"


static volatile bool led_state = false;


void
TIM2_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    led_state = !led_state;
    GPIO_WriteBit(GPIOB, GPIO_Pin_9, led_state ? Bit_SET : Bit_RESET);
}


void
timer_init(void)
{
    TIM_TimeBaseInitTypeDef tim_cfg;
    NVIC_InitTypeDef nvic_cfg;

    RCC_PCLK1Config(RCC_HCLK_Div4);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* Configure a timer to interrupt every 58 microseconds, as per NRMA
     * standard S-91
     *
     * Timer clock = 72MHz / 4 = 18MHz
     * 1/18MHz = 56ns
     * 58us / 56ns = 1044
     * => Interrupt every 1044 timer ticks
     * => load value = 1043
     */
    tim_cfg.TIM_Period = 1043;
    tim_cfg.TIM_Prescaler = 1;
    tim_cfg.TIM_ClockDivision = 0;
    tim_cfg.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tim_cfg);

    /* Interrupt on timer update */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /* Configure NVIC to generate the interrupt */
    nvic_cfg.NVIC_IRQChannel = TIM2_IRQn;
    nvic_cfg.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_cfg.NVIC_IRQChannelSubPriority = 1;
    nvic_cfg.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_cfg);

    /* Enable the timer */
    TIM_Cmd(TIM2, ENABLE);
}


int
main(void)
{
    systick_init();
    timer_init();

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


