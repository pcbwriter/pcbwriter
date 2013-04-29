#include "stepper.h"
#include "pins.h"
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <stdio.h>

const int states[][4] = {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}};
//const int states[][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
static int current_state = 0;
const int kNUMBER_OF_STATES = sizeof(states)/sizeof(states[0]);

void stepper_setup(void)
{
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);
    
    /* Set stepper output pins to 'output push-pull'. */
    gpio_mode_setup(STEPPER_PORT, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, STEPPER_BLACK_PIN | STEPPER_BROWN_PIN | STEPPER_ORANGE_PIN | STEPPER_YELLOW_PIN);
    
    /* Switch off all stepper outputs. */
    gpio_clear(STEPPER_PORT, STEPPER_BLACK_PIN | STEPPER_BROWN_PIN | STEPPER_ORANGE_PIN | STEPPER_YELLOW_PIN);
}

void stepper_step(int dir)
{
    int inc = dir ? 1 : -1;
    current_state = ((current_state + inc) + kNUMBER_OF_STATES) % kNUMBER_OF_STATES;
    
    states[current_state][0] ? gpio_set(STEPPER_PORT, STEPPER_BROWN_PIN) : gpio_clear(STEPPER_PORT, STEPPER_BROWN_PIN);	
    states[current_state][2] ? gpio_set(STEPPER_PORT, STEPPER_BLACK_PIN) : gpio_clear(STEPPER_PORT, STEPPER_BLACK_PIN);
    states[current_state][1] ? gpio_set(STEPPER_PORT, STEPPER_YELLOW_PIN): gpio_clear(STEPPER_PORT, STEPPER_YELLOW_PIN);
    states[current_state][3] ? gpio_set(STEPPER_PORT, STEPPER_ORANGE_PIN): gpio_clear(STEPPER_PORT, STEPPER_ORANGE_PIN);
}
