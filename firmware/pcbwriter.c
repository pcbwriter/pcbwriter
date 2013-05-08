#include "dac.h"
#include "usart.h"
#include "timer.h"
#include "dma_spi.h"
#include "motorctrl.h"
#include "usb.h"
#include "statusled.h"
#include "flash.h"
#include "stepper.h"
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <stdio.h>

void gpio_setup(void);

void gpio_setup(void)
{
    // Enable PB15 (LASER_OUT) as output (FIXME: remove)
    //rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPBEN);
    //gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO15);
    
    // Enable PB0, PB1 (DEBUG_0_OUT, DEBUG_1_OUT) as output.
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPBEN);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0 | GPIO1);
}

int main(void)
{
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
    
    gpio_setup();
    led_setup();
    usart_setup();
    timer_setup();
    dma_setup();
    spi_setup();
    dac_setup();
    usb_setup();
    pcb_flash_setup();
    stepper_setup();
    
    printf("\nPCBWriter starting...\n");
    
    set_speed(20000);
    motor_startup();
    
    while(1)
        usb_poll();
}
