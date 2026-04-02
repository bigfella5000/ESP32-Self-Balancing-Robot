#ifndef MOTORS_H
#define MOTORS_H

#include "useful_stuff.h"

// Initializes pwm connction to motor driver
void init_motor_driver();

// Set motor direction: 1(A) or 2(B), "cw" or "ccw"
int set_motor_dir(int motor, char* dir);

// Set motor speed: 1(A) or 2(B), value between 0-1024
int set_motor_speed(int motor, int duty);

// Gradually brings motor to a stop
void slow_decel(); // STILL NEEDS TO BE IMPLEMENTED

#endif