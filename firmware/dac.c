#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/dac.h>
#include "dac.h"

void dac_setup(void)
{
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_DACEN);
    dac_enable(CHANNEL_1);
}

void set_motor_power(int power)
{
    dac_load_data_buffer_single(power, RIGHT12, CHANNEL_1);
}

