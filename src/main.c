#include "connections.h"
#include "motors.h"
#include "imu.h"

cv_data_t cv_data = {0};

//--------------------TESTS--------------------//
void test1() {
    i2c_master_dev_handle_t imu_handle = i2c_init();

    imu_adj_t imu_adj = calibrate_imu(imu_handle);
    ESP_LOGI("IMU_CALIBRATION", "----------IMU CALIBRATION COMPLETE----------");

    while (1) {
        print_imu_data(get_imu_data(imu_handle, &imu_adj));
        delay(200);
    }
}

void test2() {
    int MAX_GX = 100;
    char *dir;
    int speed;
    imu_data_t data;

    init_motor_driver();
    delay(25);
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

void test3() {
    cv_data.mutex = xSemaphoreCreateMutex();

    init_motor_driver();
    set_motor_dir(2, "cw");
    delay(25);

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

//--------------------MAIN--------------------//
void app_main(void) {
    test3();
}