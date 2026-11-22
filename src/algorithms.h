#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "components/imu.h"
#include "components/motors.h"

void pid_control_task(void *pvParameters);

#endif