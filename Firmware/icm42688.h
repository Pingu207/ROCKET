#pragma once
#include "esp_err.h"
#include "driver/spi_master.h"

typedef struct {
    float accel_g[3];   /* sensor frame, g    */
    float gyro_dps[3];  /* sensor frame, deg/s */
    float temp_c;
} imu_sample_t;

esp_err_t icm42688_init(spi_host_device_t host);
esp_err_t icm42688_read(imu_sample_t *s);
