#include "dcc_hal.h"

#include "ringbuf.h"


/* Dead time to allow transistors in the H bridge to switch off completely
 * before the next set is switched on */
#define DEAD_TIME (10)


/* Data buffer */
#define BUF_SIZE (256)
static uint8_t data[BUF_SIZE];
static ringbuf_t buf;


/*
 * ISR state variables
 */
bool output_state = false;
bool transmitting = false;
int8_t bit_shift;
uint16_t byte;
uint8_t irq_count;


void
dcc_hal_init(void)
{
    GPIO_InitTypeDef gpio_cfg;
    TIM_TimeBaseInitTypeDef tim_cfg;
    NVIC_InitTypeDef nvic_cfg;

    /* Prepare ring buffer */
    ringbuf_init(&buf, data, BUF_SIZE);

    /* Peripheral clock = HCLK/4, enable GPIO and timer 2 clock */
    RCC_PCLK1Config(RCC_HCLK_Div4);
    RCC_APB2PeriphClockCmd(DCC_HAL_GPIO_RCC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* Set DCC_PIN_1 and DCC_PIN_2 as outputs */
    gpio_cfg.GPIO_Pin = DCC_HAL_GPIO_PIN_1 | DCC_HAL_GPIO_PIN_2;
    gpio_cfg.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_cfg.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(DCC_HAL_GPIO, &gpio_cfg);

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


uint8_t
dcc_hal_write(uint8_t *data, uint8_t len)
{
    printf("writing %d bytes\n", len);

    /* This check is NOT safe, however the length value will only ever go
     * down. This means the worst case is we fail to write data. */
    if (ringbuf_get_space(&buf) < len)
    {
        printf("buffer too full!\n");
        return 0;
    }

    return (uint8_t)ringbuf_write(&buf, data, len);
}


static void
set_output(bool output_state)
{
#if (DEAD_TIME > 0)
    int i;
#endif

    /* Set the new output state. */
    if (output_state)
    {
        GPIO_WriteBit(DCC_HAL_GPIO, DCC_HAL_GPIO_PIN_2, Bit_RESET);
#if (DEAD_TIME > 0)
        for (i = 0; i < DEAD_TIME; i++)
            __NOP();
#endif
        GPIO_WriteBit(DCC_HAL_GPIO, DCC_HAL_GPIO_PIN_1, Bit_SET);
    }
    else
    {
        GPIO_WriteBit(DCC_HAL_GPIO, DCC_HAL_GPIO_PIN_1, Bit_RESET);
#if (DEAD_TIME > 0)
        for (i = 0; i < DEAD_TIME; i++)
            __NOP();
#endif
        GPIO_WriteBit(DCC_HAL_GPIO, DCC_HAL_GPIO_PIN_2, Bit_SET);
    }
}


void
TIM2_IRQHandler(void)
{
    int bit;

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    irq_count++;
    if (irq_count > 254)
        irq_count = 254;

    /* If we're not transmitting, figure out if we should be */
    if (!transmitting)
    {
        if (ringbuf_has_data(&buf) && irq_count > 15)
        {
            /* Get ready for the new transfer */
            transmitting = true;
            irq_count = 0;
            bit_shift = 8;
            /* The 9th bit will always be zero, which gets us an easy way
             * to send the inter-byte zero */
            ringbuf_pop(&buf, (uint8_t *)&byte);
        }
        else
        {
            /* Keep sending preamble */
            output_state = !output_state;
        }
    }

    GPIO_WriteBit(GPIOB, GPIO_Pin_9, transmitting ? Bit_RESET : Bit_SET);

    /* Figure out the output state */
    if (transmitting)
    {
        bit = ((1 << bit_shift) & byte) >> bit_shift;

        if (irq_count == 1)
        {
            output_state = false;
        }
        else if (bit)
        {
            if (irq_count == 2)
            {
                output_state = true;
                irq_count = 0;
                bit_shift--;
            }
        }
        else
        {
            if (irq_count == 3)
            {
                output_state = true;
            }
            else if (irq_count == 4)
            {
                irq_count = 0;
                bit_shift--;
            }
        }

        /* If we've reached the end of the byte, then set the state so we
         * prepare the next byte */
        if (bit_shift < 0)
        {
            transmitting = false;
            irq_count = ringbuf_has_data(&buf) ? 15 : 0;
        }
    }

    set_output(output_state);
}
