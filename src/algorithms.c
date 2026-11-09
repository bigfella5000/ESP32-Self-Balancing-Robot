#include "freertos/FreeRTOS.h"

#include "algorithms.h"

void pid_control_task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 100Hz loop (10ms)

    while(1) {
        // 1. Read IMU
        // 2. Calculate Pitch/Roll
        // 3. Compute PID
        // 4. Update Motor Speeds
        
        // Strict delay ensures this runs exactly every 10ms
        vTaskDelayUntil(&xLastWakeTime, xFrequency); 
    }
}