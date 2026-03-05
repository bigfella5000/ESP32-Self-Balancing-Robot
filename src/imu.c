#include "imu.h"

#define I2C_MASTER_SCL_PIN 22
#define I2C_MASTER_SDA_PIN 21
#define I2C_MASTER_PORT 0
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_TIMEOUT_MS 5
#define IMU_I2C_ADDR 0x68
#define IMU_PWR_MGMT_1_ADDR 0x6B
#define IMU_DATA_ADDR 0x3B

i2c_master_dev_handle_t i2c_init() {
    i2c_master_bus_config_t bus_conf = {
        .i2c_port = I2C_MASTER_PORT,
        .sda_io_num = I2C_MASTER_SDA_PIN,
        .scl_io_num = I2C_MASTER_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT, // This is the APB clock and it can run at 400kHZ (Fast mode)
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_conf, &bus_handle));

    i2c_device_config_t dev_conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IMU_I2C_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };
    i2c_master_dev_handle_t imu_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_conf, &imu_handle));

    uint8_t wake_cmd[] = {0x6B, 0x08}; // 0x08 flips off the temp sensor
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_master_transmit(imu_handle, wake_cmd, sizeof(wake_cmd), 1000);

    return imu_handle;
}

imu_data_t get_imu_data(i2c_master_dev_handle_t imu_handle, imu_adj_t* imu_adj) {
    uint8_t reg_addr = IMU_DATA_ADDR;
    uint8_t data_buf[14] = {0};
    esp_err_t ret = i2c_master_transmit_receive(imu_handle, &reg_addr, 1, data_buf, 14, 100);
    
    if (ret != ESP_OK) {
        ESP_LOGE("IMU_DATA", "Data read failed: %s\n", esp_err_to_name(ret));
    }

    float ACCEL_SCALE = 16384.0;
    float GYRO_SCALE = 131.0;

    imu_data_t data = {
        .ax = (int16_t)(data_buf[0] << 8 | data_buf[1]) / ACCEL_SCALE - ((imu_adj) ? imu_adj -> ax : 0),
        .ay = (int16_t)(data_buf[2] << 8 | data_buf[3]) / ACCEL_SCALE - ((imu_adj) ? imu_adj -> ay : 0),
        .az = (int16_t)(data_buf[4] << 8 | data_buf[5]) / ACCEL_SCALE - ((imu_adj) ? imu_adj -> az : 0),
        .gx = (int16_t)(data_buf[8] << 8 | data_buf[9]) / GYRO_SCALE - ((imu_adj) ? imu_adj -> gx : 0),
        .gy = (int16_t)(data_buf[10] << 8 | data_buf[11]) / GYRO_SCALE - ((imu_adj) ? imu_adj -> gy : 0),
        .gz = (int16_t)(data_buf[12] << 8 | data_buf[13]) / GYRO_SCALE - ((imu_adj) ? imu_adj -> gz : 0)
    };
    
    return data;
}

void print_imu_data(imu_data_t data) {
    // printf("ax: %.5f, ay: %.5f, az: %.5f\n", data.ax, data.ay, data.az);
    printf("gx: %.2f, gy: %.2f, gz: %.2f\n\n", data.gx, data.gy, data.gz);
}

imu_adj_t calibrate_imu(i2c_master_dev_handle_t imu_handle) {
    int NUM_MEASUREMENTS = 40;
    int DELAY_BETWEEN_MEASUREMENTS = 100;


    // Calculate averages
    float avg_ax = 0;
    float avg_ay = 0;
    float avg_az = 0;
    float avg_gx = 0;
    float avg_gy = 0;
    float avg_gz = 0;

    for (int i = 0; i < NUM_MEASUREMENTS; i++) {
        imu_data_t data = get_imu_data(imu_handle, NULL);
        avg_ax += data.ax;
        avg_ay += data.ay;
        avg_az += data.az;
        avg_gx += data.gx;
        avg_gy += data.gy;
        avg_gz += data.gz;
        delay(DELAY_BETWEEN_MEASUREMENTS);
    }

    avg_ax /= NUM_MEASUREMENTS;
    avg_ay /= NUM_MEASUREMENTS;
    avg_az /= NUM_MEASUREMENTS;
    avg_gx /= NUM_MEASUREMENTS;
    avg_gy /= NUM_MEASUREMENTS;
    avg_gz /= NUM_MEASUREMENTS;

    // Adjustment for future reading values
    imu_adj_t adj = (imu_adj_t){
        .ax = avg_ax,
        .ay = avg_ay,
        .az = avg_az,
        .gx = avg_gx,
        .gy = avg_gy,
        .gz = avg_gz
    };

    printf("\nApplied accelerometer adjustments: ax -> %f   ay -> %f   az -> %f\n", adj.ax, adj.ay, adj.az);
    printf("Applied gyro adjustments: gx -> %f   gy -> %f   gz -> %f\n", adj.gx, adj.gy, adj.gz);

    return adj;
}