#pragma once
#include "esp_err.h"

esp_err_t bmp280_init(void);
esp_err_t bmp280_read(float *pressure_pa, float *temp_c);
float bmp280_altitude_m(float pressure_pa, float ref_pa);
