# include "motors.h"

// Pin Definitions for motor driver
#define STBY_PIN 27
#define PWMA_PIN 16
#define AIN1_PIN 14
#define AIN2_PIN 13
#define PWMB_PIN 17
#define BIN1_PIN 26
#define BIN2_PIN 25

void init_motor_driver() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<STBY_PIN) | (1ULL<<AIN1_PIN) | (1ULL<<AIN2_PIN) | (1ULL<<BIN1_PIN) | (1ULL<<BIN2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel0 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PWMA_PIN,
        .duty           = 0, // Start at 0% speed
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel0);

    ledc_channel_config_t ledc_channel1 = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_1,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PWMB_PIN,
        .duty           = 0, // Start at 0% speed
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel1);

    // Enable the driver
    gpio_set_level(STBY_PIN, 1);
}

int set_motor_dir(int motor, char* dir) {
    if (motor != 1 && motor != 2) {
        ESP_LOGE("MOTOR", "Invalid motor.\n");
        return 1;
    }

    if (strcmp(dir, "cw") == 0) {
        gpio_set_level((motor == 1) ? AIN1_PIN : BIN1_PIN, 1);
        gpio_set_level((motor == 1) ? AIN2_PIN : BIN2_PIN, 0);
    }
    else if (strcmp(dir, "ccw") == 0) {
        gpio_set_level((motor == 1) ? AIN1_PIN : BIN1_PIN, 0);
        gpio_set_level((motor == 1) ? AIN2_PIN : BIN2_PIN, 1);
    }
    else {
        ESP_LOGE("MOTOR", "Invalid direction.\n");
        return 1;
    }

    return 0;
}

int set_motor_speed(int motor, int duty) {
    if (motor != 1 && motor != 2) {
        ESP_LOGE("MOTOR", "Invalid motor.\n");
        return 1;
    }

    if (!(duty >= 0 && duty <= 1023)) {
        ESP_LOGE("MOTOR", "Invalid duty.\n");
        return 1;
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, (motor == 1) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (motor == 1) ? LEDC_CHANNEL_0 : LEDC_CHANNEL_1);
    return 0;
}