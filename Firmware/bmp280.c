#include <math.h>
#include "bmp280.h"
#include "config.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bmp280";
static i2c_master_dev_handle_t s_dev;

static struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
} cal;

static esp_err_t rd(uint8_t reg, uint8_t *buf, size_t n)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, n, 20);
}

static esp_err_t wr(uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_transmit(s_dev, b, 2, 20);
}

esp_err_t bmp280_init(void)
{
    i2c_master_bus_config_t bc = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,   /* R32 / R33 on board */
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bc, &bus));

    i2c_device_config_t dc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BMP280_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dc, &s_dev));

    uint8_t id = 0;
    if (rd(0xD0, &id, 1) != ESP_OK || id != 0x58) {
        ESP_LOGE(TAG, "chip id 0x%02X (expected 0x58)", id);
        return ESP_ERR_NOT_FOUND;
    }
    wr(0xE0, 0xB6);                       /* soft reset */
    vTaskDelay(pdMS_TO_TICKS(5));

    uint8_t c[24];
    ESP_ERROR_CHECK(rd(0x88, c, sizeof(c)));
    uint16_t *u = (uint16_t *)&cal;       /* little-endian, packed in order */
    for (int i = 0; i < 12; i++) u[i] = (uint16_t)(c[2 * i] | (c[2 * i + 1] << 8));

    wr(0xF5, (0 << 5) | (2 << 2));        /* standby 0.5 ms, IIR x4 */
    wr(0xF4, (1 << 5) | (3 << 2) | 3);    /* T x1, P x4, normal mode (~75 Hz) */

    ESP_LOGI(TAG, "BMP280 ready");
    return ESP_OK;
}

esp_err_t bmp280_read(float *pressure_pa, float *temp_c)
{
    uint8_t b[6];
    esp_err_t e = rd(0xF7, b, sizeof(b));
    if (e != ESP_OK) return e;

    int32_t adc_P = (b[0] << 12) | (b[1] << 4) | (b[2] >> 4);
    int32_t adc_T = (b[3] << 12) | (b[4] << 4) | (b[5] >> 4);

    /* Bosch datasheet compensation (integer) */
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)cal.T1 << 1))) * cal.T2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)cal.T1) *
                    ((adc_T >> 4) - (int32_t)cal.T1)) >> 12) * cal.T3) >> 14;
    int32_t t_fine = v1 + v2;
    *temp_c = ((t_fine * 5 + 128) >> 8) / 100.0f;

    int64_t p1 = (int64_t)t_fine - 128000;
    int64_t p2 = p1 * p1 * cal.P6;
    p2 += (p1 * cal.P5) << 17;
    p2 += ((int64_t)cal.P4) << 35;
    p1 = ((p1 * p1 * cal.P3) >> 8) + ((p1 * cal.P2) << 12);
    p1 = ((((int64_t)1) << 47) + p1) * cal.P1 >> 33;
    if (p1 == 0) return ESP_ERR_INVALID_RESPONSE;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - p2) * 3125) / p1;
    p1 = ((int64_t)cal.P9 * (p >> 13) * (p >> 13)) >> 25;
    p2 = ((int64_t)cal.P8 * p) >> 19;
    p = ((p + p1 + p2) >> 8) + (((int64_t)cal.P7) << 4);
    *pressure_pa = (float)p / 256.0f;
    return ESP_OK;
}

float bmp280_altitude_m(float pressure_pa, float ref_pa)
{
    return 44330.0f * (1.0f - powf(pressure_pa / ref_pa, 0.1903f));
}
