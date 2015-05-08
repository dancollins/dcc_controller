#include "dcc_hal.h"


void
dcc_hal_init(void)
{
    TIM_TimeBaseInitTypeDef tim_cfg;
    NVIC_InitTypeDef nvic_cfg;

    /* Peripheral clock = HCLK/4, enable the timer clock */
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


void
TIM2_IRQHandler(void)
{
    static bool led_state = false;

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    led_state = !led_state;
    GPIO_WriteBit(GPIOB, GPIO_Pin_9, led_state ? Bit_SET : Bit_RESET);
}
