#ifndef IMU_H
#define IMU_H

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

// Initializes i2c connection to IMU
i2c_master_dev_handle_t i2c_init();

// Gets an imu_data_t struct. imu_adj can be NULL or you can provide an imu_adj_t struct which contains calibration adjustment parameters
imu_data_t get_imu_data(i2c_master_dev_handle_t imu_handle, imu_adj_t* imu_adj);

// Prints the imu_data_t struct
void print_imu_data(imu_data_t data);

// Calibrates the IMU so all values go towards 0.0
imu_adj_t calibrate_imu(i2c_master_dev_handle_t imu_handle);

#endif