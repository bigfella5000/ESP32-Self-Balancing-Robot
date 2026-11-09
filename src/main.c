#include <stdio.h>
#include <string.h>
#include <math.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"

#include "algorithms.h"
#include "connections.h"
#include "motors.h"
#include "imu.h"
#include "lcd.h"

cv_data_t cv_data = {0};

//--------------------TESTS--------------------//
// Calibrate and read IMU
void test1() {
    i2c_master_dev_handle_t imu_handle = i2c_init();

    imu_adj_t imu_adj = calibrate_imu(imu_handle);
    ESP_LOGI("IMU_CALIBRATION", "----------IMU CALIBRATION COMPLETE----------");

    while (1) {
        print_imu_data(get_imu_data(imu_handle, &imu_adj));
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// Control motor speed by rotating IMU
void test2() {
    int MAX_GX = 100;
    char *dir;
    int speed;
    imu_data_t data;

    init_motor_driver();
    vTaskDelay(pdMS_TO_TICKS(25));
    i2c_master_dev_handle_t imu_handle = i2c_init();
    printf("\nKeep table still. Calibration beginning...\n\n");
    imu_adj_t adj = calibrate_imu(imu_handle);
    printf("\nInitialization complete.\n");

    while (1) {
        data = get_imu_data(imu_handle, &adj);

        dir = (data.gx > 0.0) ? "cw" : "ccw";
        speed = (data.gx > 5 || data.gx < -5) ? fmin(1023, (int)((fabsf(data.gx) / MAX_GX) * 1023)) : 0;
        set_motor_dir(2, dir);
        set_motor_speed(2, speed);
    }
}

// Read gesture commands from cv_controller.py
void test3() {
    cv_data.mutex = xSemaphoreCreateMutex();

    init_motor_driver();
    set_motor_dir(2, "cw");
    vTaskDelay(pdMS_TO_TICKS(25));

    wifi_init();

    xTaskCreate(udp_listener_task, "udp_listener", 4096, &cv_data, 5, NULL);

    while (1) {
        xSemaphoreTake(cv_data.mutex, portMAX_DELAY);
        char gesture[32];
        strncpy(gesture, cv_data.gesture, sizeof(gesture));
        float dx = cv_data.dx;
        float dy = cv_data.dy;
        xSemaphoreGive(cv_data.mutex);

        int speed = 0;
        if (strcmp(gesture, "none") == 0 || strcmp(gesture, "") == 0 ) {
            speed = 0;
        } else if (strcmp(gesture, "palm") == 0) {
            speed = 1023 / 4;
        } else if (strcmp(gesture, "back") == 0) {
            speed = 1023 / 2;
        } else if (strcmp(gesture, "fist") == 0) {
            speed = 1023 * 3/4;
        } else if (strcmp(gesture, "point") == 0) {
            speed = 1023;
        }

        set_motor_speed(2, speed);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void ui_task(void *pvParameters) {
    ili9341_init();
    ili9341_fill_screen(COLOR_BLACK);
    
    char current_gesture[32] = "";

    while(1) {
        xSemaphoreTake(cv_data.mutex, portMAX_DELAY);
        char new_gesture[32];
        strncpy(new_gesture, cv_data.gesture, sizeof(new_gesture));
        xSemaphoreGive(cv_data.mutex);

        if (strcmp(current_gesture, new_gesture) != 0) {
            draw_face_reaction(new_gesture);
            strncpy(current_gesture, new_gesture, sizeof(current_gesture));
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

//--------------------MAIN--------------------//
void app_main(void) {
    init_motor_driver();
    i2c_master_dev_handle_t imu_handle = i2c_init();
    wifi_init();
    
    cv_data.mutex = xSemaphoreCreateMutex();
    
    // Core 0: UDP Listener
    xTaskCreatePinnedToCore(udp_listener_task, "udp", 4096, &cv_data, 5, NULL, 0);
    // Core 0: LCD Drawing
    xTaskCreatePinnedToCore(ui_task, "ui", 4096, NULL, 4, NULL, 0);
    // Core 1: PID Control
    xTaskCreatePinnedToCore(pid_control_task, "pid", 4096, NULL, configMAX_PRIORITIES - 1, NULL, 1);
}