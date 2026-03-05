#ifndef STRUCTS_AND_INCLUDES_H
#define STRUCTS_AND_INCLUDES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"

typedef struct {
	float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} imu_data_t;

typedef struct {
	float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} imu_adj_t;

inline void delay(int ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

#endif