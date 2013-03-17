#ifndef PCBWRITER_STATUSLED_H
#define PCBWRITER_STATUSLED_H

#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>

#define LED_GREEN  GPIO12
#define LED_ORANGE GPIO13
#define LED_RED    GPIO14
#define LED_BLUE   GPIO15

void led_setup(void);
void set_status(int led, int on);

#endif
