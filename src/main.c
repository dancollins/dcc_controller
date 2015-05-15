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
#include "sseg.h"


/* Number of milliseconds between tasks */
#define STATE_UPDATE (100)
#define DCC_UPDATE (25)
#define THROTTLE_UPDATE (7) /* units are 100 milliseconds */


/* Button indicies */
#define BLEFT    (0)
#define BRIGHT   (1)
#define BSELECT  (3)
#define BESTOP   (2)


/* State machine states for the controller */
typedef enum {
    STATE_SELECT,
    STATE_THROTTLE,
    STATE_E_STOP
} state_t;


/* This value is actively maintained by DMA. As soon as the ADC is ready,
 * this value gets updated. */
static uint32_t adc_value;


/* Each button has a count which increments when the button is pressed and
 * decrements when the button is released. This is used to debounce the
 * buttons */
static volatile uint8_t buttons[4];
static volatile bool buttons_checked[4];


static uint8_t
get_buttons(void)
{
    uint16_t ret = GPIO_ReadInputData(GPIOB);

    /* The buttons are pulled high, and we want a '1' bit when they're pressed.
     * So we mask them, toggle them, and then shift them such that the first
     * button is bit 0 */
    ret &= 0x7800;
    ret ^= 0x7800;
    ret >>= 11;

    return (uint8_t)ret;
}


static void
update_buttons(void)
{
    uint8_t state = get_buttons();
    int i;

    for (i = 0; i < 4; i++)
    {
        if (state & (1 << i))
        {
            buttons[i]++;
            if (buttons[i] > 5)
                buttons[i] = 5;
        }
        else
        {
            if (buttons[i] > 0)
                buttons[i]--;
            else
                buttons_checked[i] = false;
        }
    }
}


static void
calculate_throttle(uint8_t *throttle, bool *reverse)
{
    /* This is atomic, so the DMA won't overwrite the value in the middle
     * of our calculations */
    uint16_t adc_val = adc_value;

    /* Set the sign bit (ret), and then take the absolute */
    if (adc_val > 0x7ff)
    {
        *reverse = true;
        adc_val -= 0x7ff;
    }
    else
    {
        *reverse = false;
        adc_val = 0x800 - adc_val;
    }

    /* Transform the value into a 29 value range */
    adc_val /= 73;
    *throttle = (uint8_t)adc_val;

    /* Constain the throttle value */
    if (*throttle > 28)
        *throttle = 28;
}


int
main(void)
{
    /* STM lib variables */
    GPIO_InitTypeDef gpio_cfg;
    ADC_InitTypeDef adc_cfg;
    DMA_InitTypeDef dma_cfg;

    /* State variables */
    state_t state = STATE_SELECT;
    uint8_t selected_train = 1;
    uint8_t throttle = 0, old_throttle;
    bool reverse = false;
    bool blink = false;

    /* Timer variables */
    size_t state_time = 0;
    size_t dcc_time = 0;
    size_t throttle_time = 0;


    /* Init our drivers */
    systick_init(update_buttons);
    dcc_init();
    sseg_init();


    /* Prepare LED pins */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio_cfg.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    gpio_cfg.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_cfg.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio_cfg);


    /* Prepare button pins */
    gpio_cfg.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
    gpio_cfg.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio_cfg);


    /* Prepare potentiometer */
    gpio_cfg.GPIO_Pin = GPIO_Pin_0;
    gpio_cfg.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOB, &gpio_cfg);


    /* Prepare DMA */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_DeInit(DMA1_Channel1);
    dma_cfg.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    dma_cfg.DMA_MemoryBaseAddr = (uint32_t)&adc_value;
    dma_cfg.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma_cfg.DMA_BufferSize = 1;
    dma_cfg.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_cfg.DMA_MemoryInc = DMA_MemoryInc_Disable;
    dma_cfg.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma_cfg.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma_cfg.DMA_Mode = DMA_Mode_Circular;
    dma_cfg.DMA_Priority = DMA_Priority_High;
    dma_cfg.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &dma_cfg);

    /* Enable DMA1 channel1 */
    DMA_Cmd(DMA1_Channel1, ENABLE);


    /* Prepare ADC */
    RCC_ADCCLKConfig(RCC_PCLK2_Div2);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    adc_cfg.ADC_Mode = ADC_Mode_Independent;
    adc_cfg.ADC_ScanConvMode = ENABLE;
    adc_cfg.ADC_ContinuousConvMode = ENABLE;
    adc_cfg.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc_cfg.ADC_DataAlign = ADC_DataAlign_Right;
    adc_cfg.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc_cfg);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_55Cycles5);

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    /* Calibrate */
    ADC_ResetCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1))
        ;
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1))
        ;

    /* Start the continuous conversion */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);


    while(1)
    {
        calculate_throttle(&throttle, &reverse);

        /* Update state based on button presses */
        if (systick_test_duration(state_time, STATE_UPDATE))
        {
            /* Update state based on buttons */
            /* left and right */
            if (buttons[BLEFT] == 5 && !buttons_checked[BLEFT])
            {
                if (selected_train > 1)
                {
                    selected_train--;
                }
                else
                {
                    selected_train = DCC_N_TRAINS;
                }
                state = STATE_SELECT;

                buttons_checked[BLEFT] = true;
            } else if (buttons[BRIGHT] == 5 && !buttons_checked[BRIGHT])
            {
                if (selected_train < DCC_N_TRAINS)
                {
                    selected_train++;
                }
                else
                {
                    selected_train = 1;
                }
                state = STATE_SELECT;

                buttons_checked[BRIGHT] = true;
            }

            /* select */
            if (buttons[BSELECT] == 5 && !buttons_checked[BSELECT])
            {
                state = STATE_THROTTLE;

                /* This sets the initial throttle (enables the train) */
                dcc_set_speed(selected_train, throttle, reverse);

                buttons_checked[BSELECT] = true;
            }

            /* e stop */
            if (buttons[BESTOP] == 5 && !buttons_checked[BESTOP])
            {
                state = STATE_E_STOP;

				buttons_checked[BESTOP] = true;
            }

            /* Update the display and DCC */
            switch (state)
            {
            case STATE_SELECT:
                if (blink)
                {
                    sseg_set(selected_train);
                    sseg_set_dp(true, false);
                }
                else
                {
                    sseg_off();
                }
                blink = !blink;
                break;

            case STATE_THROTTLE:
                if (throttle != old_throttle)
                {
                    old_throttle = throttle;
                    throttle_time = THROTTLE_UPDATE;
                    dcc_set_speed(selected_train, throttle, reverse);
                }

                if (throttle_time > 0)
                {
                    sseg_set(throttle);
                    sseg_set_dp(false, reverse);
                    throttle_time--;
                }
                else
                {
                    sseg_set(selected_train);
                    sseg_set_dp(true, false);
                    blink = false;
                }
                break;

            case STATE_E_STOP:
                sseg_set(0xEE);
                sseg_set_dp(true, true);
                dcc_e_stop(true);
                break;
            }

            state_time = systicks;
        }

        /* Send the DCC controls */
        if (systick_test_duration(dcc_time, DCC_UPDATE))
        {
            dcc_update();

            dcc_time = systicks;
        }
    }

    return 0;
}


