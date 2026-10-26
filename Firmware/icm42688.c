#include <string.h>
#include "icm42688.h"
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "icm42688";
static spi_device_handle_t s_dev;

#define REG_DEVICE_CONFIG  0x11
#define REG_TEMP_DATA1     0x1D   /* temp, accel XYZ, gyro XYZ follow (big-endian) */
#define REG_PWR_MGMT0      0x4E
#define REG_GYRO_CONFIG0   0x4F
#define REG_ACCEL_CONFIG0  0x50
#define REG_WHO_AM_I       0x75
#define REG_BANK_SEL       0x76
#define WHO_AM_I_VAL       0x47

#define ACCEL_LSB_PER_G    2048.0f  /* +-16 g    */
#define GYRO_LSB_PER_DPS   16.4f    /* +-2000 dps */

static esp_err_t wr(uint8_t reg, uint8_t val)
{
    spi_transaction_t t = {
        .length = 16,
        .flags = SPI_TRANS_USE_TXDATA,
        .tx_data = { reg & 0x7F, val },
    };
    return spi_device_polling_transmit(s_dev, &t);
}

static esp_err_t rd(uint8_t reg, uint8_t *buf, size_t n)
{
    uint8_t tx[16] = {0}, rx[16] = {0};
    if (n > 15) return ESP_ERR_INVALID_SIZE;
    tx[0] = reg | 0x80;
    spi_transaction_t t = { .length = 8 * (n + 1), .tx_buffer = tx, .rx_buffer = rx };
    esp_err_t e = spi_device_polling_transmit(s_dev, &t);
    if (e == ESP_OK) memcpy(buf, rx + 1, n);
    return e;
}

esp_err_t icm42688_init(spi_host_device_t host)
{
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_IMU_MOSI, .miso_io_num = PIN_IMU_MISO,
        .sclk_io_num = PIN_IMU_SCLK, .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host, &bus, SPI_DMA_DISABLED));

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 10 * 1000 * 1000,   /* pins go through GPIO matrix */
        .mode = 0,
        .spics_io_num = PIN_IMU_CS,
        .queue_size = 2,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host, &dev, &s_dev));

    wr(REG_BANK_SEL, 0);
    wr(REG_DEVICE_CONFIG, 0x01);              /* soft reset */
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t id = 0;
    rd(REG_WHO_AM_I, &id, 1);
    if (id != WHO_AM_I_VAL) {
        ESP_LOGE(TAG, "WHO_AM_I = 0x%02X (expected 0x47)", id);
        return ESP_ERR_NOT_FOUND;
    }

    wr(REG_GYRO_CONFIG0,  (0 << 5) | 0x06);   /* +-2000 dps, 1 kHz ODR */
    wr(REG_ACCEL_CONFIG0, (0 << 5) | 0x06);   /* +-16 g,     1 kHz ODR */
    wr(REG_PWR_MGMT0, 0x0F);                  /* gyro + accel low-noise mode */
    vTaskDelay(pdMS_TO_TICKS(50));            /* gyro start-up time */

    ESP_LOGI(TAG, "ICM-42688 ready");
    return ESP_OK;
}

esp_err_t icm42688_read(imu_sample_t *s)
{
    uint8_t b[14];
    esp_err_t e = rd(REG_TEMP_DATA1, b, sizeof(b));
    if (e != ESP_OK) return e;

    int16_t raw[7];
    for (int i = 0; i < 7; i++) raw[i] = (int16_t)((b[2 * i] << 8) | b[2 * i + 1]);

    s->temp_c = raw[0] / 132.48f + 25.0f;
    for (int i = 0; i < 3; i++) {
        s->accel_g[i]  = raw[1 + i] / ACCEL_LSB_PER_G;
        s->gyro_dps[i] = raw[4 + i] / GYRO_LSB_PER_DPS;
    }
    return ESP_OK;
}
