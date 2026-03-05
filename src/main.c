#include "motors.h"
#include "imu.h"

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
    char* dir;
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
        speed = (data.gx > 5 || data.gx < -5) ? fmin(1023, (int) ((fabsf(data.gx) / MAX_GX) * 1023)) : 0;
        set_motor_dir(2, dir);
        set_motor_speed(2, speed);
    }
}

//--------------------MAIN--------------------//
void app_main(void) {
    test2();
}