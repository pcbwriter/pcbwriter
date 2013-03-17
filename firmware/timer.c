#include "timer.h"
#include "motorctrl.h"
#include "pins.h"
#include "dma_spi.h"
#include "statusled.h"

#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/f4/nvic.h>

void timer_setup(void)
{
    /* Enable timer clock. */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM1EN);
    
    /* Reset timer. */
    timer_reset(TIM1);
    
    /* Configure prescaler. */
    timer_set_prescaler(TIM1, 160);
    
    /* Configure PE11 (AF1: TIM1_CH2) (SYNC_IN). */
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPEEN);
    gpio_mode_setup(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11);
    gpio_set_af(GPIOE, GPIO_AF1, GPIO11);
    
    /* Configure input capture. */
    timer_ic_disable(TIM1, TIM_IC2);
    timer_ic_set_input(TIM1, TIM_IC2, TIM_IC_IN_TI2);
    timer_ic_set_polarity(TIM1, TIM_IC2, TIM_IC_RISING);
    timer_ic_set_prescaler(TIM1, TIM_IC2, TIM_IC_PSC_OFF);
    timer_ic_set_filter(TIM1, TIM_IC2, TIM_IC_OFF);
    timer_ic_enable(TIM1, TIM_IC2);
    
    /* Enable counter. */
    timer_enable_counter(TIM1);
    
    /* Enable IRQs */
    nvic_enable_irq(NVIC_TIM1_UP_TIM10_IRQ);
    timer_enable_irq(TIM1, TIM_DIER_UIE);
    
    nvic_enable_irq(NVIC_TIM1_CC_IRQ);
    timer_enable_irq(TIM1, TIM_DIER_CC2IE);
}

volatile int n_overflow = 0;

void tim1_up_tim10_isr(void)
{
    if(timer_get_flag(TIM1, TIM_SR_UIF)) {
        timer_clear_flag(TIM1, TIM_SR_UIF);
        if(n_overflow <= 8) n_overflow++;
    }
}

int last_ccr = 0;
int dma_enabled = 0;
int motor_ok = 16;

void tim1_cc_isr(void)
{
    if(timer_get_flag(TIM1, TIM_SR_CC2IF)) {
        int ccr = TIM_CCR2(TIM1);
        int save_n_overflow = n_overflow;
        n_overflow = 0;
        
        int delta = (ccr + (save_n_overflow<<16)) - last_ccr;
        
        if(dma_enabled)
            start_dma();
        
        timer_clear_flag(TIM1, TIM_SR_CC2IF);
        
        //gpio_set(DEBUG0_OUT_PORT, DEBUG0_OUT_PIN);
        //gpio_clear(DEBUG0_OUT_PORT, DEBUG0_OUT_PIN);
        
        last_ccr = ccr;
        
        int speed_delta = motor_ctrl_step(delta);
        // Wait for motor speed to get stable.
        if(!dma_enabled && speed_delta > -10 && speed_delta < 10) {
            if(motor_ok == 0) {
                set_status(LED_GREEN, 1);
                set_status(LED_RED, 0);
                dma_enabled = 1;
            } else {
                motor_ok--;
            }
        }
        
        if(dma_enabled && (speed_delta < -100 || speed_delta > 100)) {
            set_status(LED_GREEN, 0);
            set_status(LED_RED, 1);
            dma_enabled = 0;
            motor_ok = 16;
        }
        
        if(delta < 100) {
            gpio_set(GPIOB, GPIO0);
            __asm("nop");
            __asm("nop");
            __asm("nop");
            __asm("nop");
            gpio_clear(GPIOB, GPIO0);
        }
    }
}

