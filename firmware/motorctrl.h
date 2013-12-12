#ifndef PCBWRITER_MOTORCTRL_H
#define PCBWRITER_MOTORCTRL_H

#include <stdint.h>

void set_speed(int speed);
void enable_debug_out(int enable);
void motor_startup(void);
int motor_ctrl_step(uint32_t delta);

#endif
