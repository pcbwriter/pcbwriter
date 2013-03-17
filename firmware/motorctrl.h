#ifndef PCBWRITER_MOTORCTRL_H
#define PCBWRITER_MOTORCTRL_H

void set_speed(int speed);
void enable_debug_out(int enable);
void motor_startup(void);
int motor_ctrl_step(int delta);

#endif
