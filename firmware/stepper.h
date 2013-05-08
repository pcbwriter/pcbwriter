#ifndef PCBWRITER_STEPPER_H
#define PCBWRITER_STEPPER_H

void stepper_setup(void);
void stepper_step(int dir);
void stepper_off(void);
void stepper_home(void);
void stepper_move(int delta_pos);
int stepper_move_to(int target_pos);

int home_pressed(void);
int stepper_idle(void);

extern volatile int stepper_current_pos;
extern volatile int stepper_is_homed;

#endif
