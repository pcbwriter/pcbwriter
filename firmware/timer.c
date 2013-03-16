#include "timer.h"
#include "motorctrl.h"
#include "pins.h"
#include "dma_spi.h"

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
    timer_clear_flag(TIM1, TIM_SR_UIF);
    if(n_overflow <= 2) n_overflow++;
}

int last_ccr = 0;

void tim1_cc_isr(void)
{
    int ccr = TIM_CCR2(TIM1);
    int save_n_overflow = n_overflow;
    n_overflow = 0;
    
    timer_clear_flag(TIM1, TIM_SR_CC2IF);
    
    start_dma();
    
    int delta = (ccr + (save_n_overflow<<16)) - last_ccr;
    
    //gpio_set(DEBUG0_OUT_PORT, DEBUG0_OUT_PIN);
    //gpio_clear(DEBUG0_OUT_PORT, DEBUG0_OUT_PIN);
    
    last_ccr = ccr;
    
    if(delta < 40000)
        motor_ctrl_step(delta);
}

