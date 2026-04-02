#ifndef USEFUL_STUFF_H
#define USEFUL_STUFF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"

// Holds the values for the imu readings
typedef struct {
	float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} imu_data_t;

// Holds the adjustment values for the imu readings after performing a calibration
typedef struct {
	float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} imu_adj_t;

// Shorthand notation for performing an RTOS delay
inline void delay(int ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

#endif