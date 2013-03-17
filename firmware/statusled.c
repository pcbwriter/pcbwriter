#include "statusled.h"

void led_setup(void)
{
    /* Enable GPIOD clock. */
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);

    /* Set GPIO12-15 (in GPIO port D) to 'output push-pull'. */
    gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, GPIO12 | GPIO13 | GPIO14 | GPIO15);
    
    /* Switch off all LEDs. */
    gpio_clear(GPIOD, GPIO12 | GPIO13 | GPIO14 | GPIO15);
}

void set_status(int led, int on)
{
    if(on)
        gpio_set(GPIOD, led);
    else
        gpio_clear(GPIOD, led);
}
