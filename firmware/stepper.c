#include "stepper.h"
#include "pins.h"
#include "acc_profile.h"
#include "statusled.h"
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <stdio.h>

const int states[][4] = {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}};
//const int states[][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
static int current_state = 0;
const int kNUMBER_OF_STATES = sizeof(states)/sizeof(states[0]);

volatile int stepper_current_pos = 0;

/* Functions internal to the stepper module. */
void update_stepper_pins(void);
void move_handler(void);
void home_handler(void);

void stepper_setup(void)
{
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);
    
    /* Set stepper output pins to 'output push-pull'. */
    gpio_mode_setup(STEPPER_PORT, GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE, STEPPER_BLACK_PIN | STEPPER_BROWN_PIN | STEPPER_ORANGE_PIN | STEPPER_YELLOW_PIN);
    
    /* Set home switch input to 'input'. */
    gpio_mode_setup(STEPPER_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, HOME_PIN);
    
    /* Switch off all stepper outputs. */
    gpio_clear(STEPPER_PORT, STEPPER_BLACK_PIN | STEPPER_BROWN_PIN | STEPPER_ORANGE_PIN | STEPPER_YELLOW_PIN);
    
    /* Enable timer 3 clock. */
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM3EN);
    
    /* Reset timer. */
    timer_reset(TIM3);
    
    /* Configure prescaler. */
    timer_set_prescaler(TIM3, 100);
    
    /* Enable IRQs */
    nvic_enable_irq(NVIC_TIM3_IRQ);
    timer_enable_irq(TIM3, TIM_DIER_UIE);
}

void update_stepper_pins(void)
{
    states[current_state][0] ? gpio_set(STEPPER_PORT, STEPPER_BROWN_PIN) : gpio_clear(STEPPER_PORT, STEPPER_BROWN_PIN);	
    states[current_state][2] ? gpio_set(STEPPER_PORT, STEPPER_BLACK_PIN) : gpio_clear(STEPPER_PORT, STEPPER_BLACK_PIN);
    states[current_state][1] ? gpio_set(STEPPER_PORT, STEPPER_YELLOW_PIN): gpio_clear(STEPPER_PORT, STEPPER_YELLOW_PIN);
    states[current_state][3] ? gpio_set(STEPPER_PORT, STEPPER_ORANGE_PIN): gpio_clear(STEPPER_PORT, STEPPER_ORANGE_PIN);
    
    set_status(LED_ORANGE, 1);
}

void stepper_step(int dir)
{
    int inc = dir ? 1 : -1;
    current_state = ((current_state + inc) + kNUMBER_OF_STATES) % kNUMBER_OF_STATES;
    stepper_current_pos -= inc;   // FIXME
    
    update_stepper_pins();
}

/* void stepper_on(void)
{
    update_stepper_pins();
} */

void stepper_off(void)
{
    gpio_clear(STEPPER_PORT, STEPPER_BLACK_PIN | STEPPER_BROWN_PIN | STEPPER_ORANGE_PIN | STEPPER_YELLOW_PIN);
    
    set_status(LED_ORANGE, 0);
}

volatile unsigned int move_state = 0;
volatile unsigned int home_state = 0;
volatile int stepper_is_homed = 0;

/* variables for motion planner */
volatile unsigned int mtn_pos_idx;
volatile unsigned int mtn_acc_end;
volatile unsigned int mtn_steady_end;
volatile unsigned int mtn_v_idx = 0;
volatile unsigned int move_dir;

int stepper_idle(void)
{
    return (move_state == 0 && home_state == 0);
}

void stepper_move(int delta_pos)
{
    move_dir = 0;
    if(delta_pos < 0) {
        delta_pos = -delta_pos;
        move_dir = 1;
    }
    
    if(delta_pos == 0)
        return;
    
    mtn_pos_idx = 0;
    mtn_v_idx = 0;
    
    if(delta_pos == 1) {
        move_state = 3;
    } else if(delta_pos == 2) {
        mtn_steady_end = 0;
        move_state = 2;
    } else if(delta_pos == 3) {
        mtn_acc_end = 0;
        mtn_steady_end = 1;
        move_state = 1;
    } else if((unsigned int) delta_pos < 2*n_acc_delays) {
        mtn_acc_end = delta_pos/2 - 1;
        mtn_steady_end = delta_pos - mtn_acc_end;
        move_state = 1;
    } else {
        mtn_acc_end = n_acc_delays - 1;
        mtn_steady_end = delta_pos - mtn_acc_end;
        move_state = 1;
    }
    
    timer_set_period(TIM3, 1000);
    timer_enable_counter(TIM3);
}

int stepper_move_to(int target_pos)
{
    if(!stepper_is_homed || target_pos < 0 || target_pos > 5000)
        return 0;
    
    stepper_move(target_pos - stepper_current_pos);
    return 1;
}

void stepper_home(void)
{
    home_state = 1;
    mtn_v_idx = 1;
    timer_set_period(TIM3, acc_delays[0]);
    timer_enable_counter(TIM3);
}

int home_pressed(void)
{
    return gpio_get(STEPPER_PORT, HOME_PIN);
}

void move_handler(void)
{
    switch(move_state) {
        case 1:   /* Accelerate */
            stepper_step(move_dir);
            mtn_pos_idx++;
            timer_set_period(TIM3, acc_delays[mtn_v_idx]);
            
            if(mtn_pos_idx >= mtn_acc_end)
                move_state = 2;
            else
                mtn_v_idx++;
        break;
        case 2:   /* Move steady */
            stepper_step(move_dir);
            mtn_pos_idx++;
            timer_set_period(TIM3, acc_delays[mtn_v_idx]);
            
            if(mtn_pos_idx >= mtn_steady_end)
                move_state = 3;
        break;
        case 3:   /* Decelerate */
            stepper_step(move_dir);
            mtn_pos_idx++;
            
            if(mtn_v_idx > 0) {
                mtn_v_idx--;
                timer_set_period(TIM3, acc_delays[mtn_v_idx]);
            } else {
                move_state = 0;
                timer_disable_counter(TIM3);
            }
        break;
        default:
        break;
    }
}

void home_handler(void)
{
    switch(home_state) {
        case 1:   /* Accelerate */
            stepper_step(1);
            timer_set_period(TIM3, acc_delays[mtn_v_idx]);
            
            if((mtn_v_idx < n_acc_delays-1) && !home_pressed())
                mtn_v_idx++;
            else
                home_state = 2;
        break;
        case 2:   /* Move steady */
            stepper_step(1);
            if(home_pressed())
                home_state = 3;
        break;
        case 3:   /* Decelerate */
            stepper_step(1);
            timer_set_period(TIM3, acc_delays[mtn_v_idx]);
            
            if(mtn_v_idx > 0) {
                mtn_v_idx--;
            } else {
                timer_set_period(TIM3, 30000);
                home_state = 4;
            }
        break;
        case 4:   /* Back off */
            stepper_step(0);
            
            if(!home_pressed()) {
                timer_disable_counter(TIM3);
                home_state = 0;
                stepper_current_pos = 0;
                stepper_is_homed = 1;
            }
        default:
        break;
    }
}

void tim3_isr(void)
{
    if(timer_get_flag(TIM3, TIM_SR_UIF)) {
        timer_clear_flag(TIM3, TIM_SR_UIF);
        
        if(move_state) move_handler();
        if(home_state) home_handler();
    }
}
