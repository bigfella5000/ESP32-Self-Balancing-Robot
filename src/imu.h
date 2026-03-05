#ifndef IMU_H
#define IMU_H

#include "useful_shit.h"

// Initializes i2c connection to IMU
i2c_master_dev_handle_t i2c_init();

// Gets an imu_data_t struct. imu_adj can be NULL or you can provide an imu_adj_t struct which contains calibration adjustment parameters
imu_data_t get_imu_data(i2c_master_dev_handle_t imu_handle, imu_adj_t* imu_adj);

// Prints the imu_data_t struct
void print_imu_data(imu_data_t data);

// Calibrates the IMU so all values go towards 0.0
imu_adj_t calibrate_imu(i2c_master_dev_handle_t imu_handle);

#endif