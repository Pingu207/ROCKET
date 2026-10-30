// main.c (for rocket flight computer; ESP-IDF)
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

// defs
// IMU pins
#define CS_PIN 17
#define SCLK_PIN 18
#define MOSI_PIN 19
#define MISO_PIN 21
// Baro pins
#define SDA_PIN 23
#define SCL_PIN 22
// SD pins
#define SD_CS_PIN 25
#define SD_MOSI_PIN 26
#define SD_CLK_PIN 27
#define SD_MISO_PIN 32
// other pins
#define PYRO_DROGUE_PIN 33
#define PYRO_MAIN_PIN 5
#define SERVO_PITCH_PIN 4
#define SERVO_YAW_PIN 13
#define BUZZER_PIN 14
// other defs
#define BARO_I2C_ADDRESS 0x76
#define SD_MOUNT "/sdcard"
#define SERVO_CENTER_DUTY 1229

// state enum
typedef enum {
    PAD,
    BOOST,
    COAST,
    DROGUE,
    MAIN,
    LAND
} state_t;

// static inits
static state_t current_state = PAD;
static spi_device_handle_t imu_handle;
static i2c_master_bus_handle_t i2c_bus_handle;
static i2c_master_dev_handle_t baro_handle;
static sdmmc_card_t *sd_card;
static const char *TAG = "MAIN";
static esp_err_t err;

// Subsystem inits
void pyro_init(void){
    gpio_reset_pin(PYRO_DROGUE_PIN);
    gpio_reset_pin(PYRO_MAIN_PIN);
    gpio_set_level(PYRO_DROGUE_PIN, 0);
    gpio_set_level(PYRO_MAIN_PIN, 0);
    gpio_set_direction(PYRO_DROGUE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(PYRO_MAIN_PIN, GPIO_MODE_OUTPUT);
};

void imu_init(void){
    spi_bus_config_t imu_bus_config = {
        .miso_io_num = MISO_PIN,
        .mosi_io_num = MOSI_PIN,
        .sclk_io_num = SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    err = spi_bus_initialize(SPI2_HOST, &imu_bus_config, SPI_DMA_CH_AUTO);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize IMU SPI bus: %s", esp_err_to_name(err));
        return;
    }

    spi_device_interface_config_t imu_device_config = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 3,
        .spics_io_num = CS_PIN,
        .queue_size = 7
    };
    err = spi_bus_add_device(SPI2_HOST, &imu_device_config, &imu_handle);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to add IMU SPI device: %s", esp_err_to_name(err));
        return;
    }
}

void baro_init(void){
    i2c_master_bus_config_t baro_bus_config = {
        .i2c_port = -1,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false   // do not set true(!!) board has external pullups
    };
    err = i2c_new_master_bus(&baro_bus_config, &i2c_bus_handle);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize I2C bus: %s", esp_err_to_name(err));
        return;
    }

    i2c_device_config_t baro_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BARO_I2C_ADDRESS,
        .scl_speed_hz = 400 * 1000
    };
    err = i2c_master_bus_add_device(i2c_bus_handle, &baro_device_config, &baro_handle);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to add I2C device 0x%02x: %s", BARO_I2C_ADDRESS, esp_err_to_name(err));
        return;
    }
}

void sd_init(void){
    sdmmc_mount_config_t sd_mount_config = {
        .format_if_mount_failed = true,
        .max_files = 4,
        .allocation_unit_size = 4096
    };

    ESP_LOGI(TAG, "Initializing SD Card");

    spi_bus_config_t spi_bus_configs[1] = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };

    err = spi_bus_initialize(SPI2_HOST, &spi_bus_configs[1], SDSPI_DEFAULT_DMA_CHAN);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize SPI bus for SD card: %d", err);
        return;
    }

    sd_device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    sd_device_config.gpio_cs = SD_SPI_CS_PIN;
    sd_device_config.host_id = SPI2_HOST;

    sd_sdmmc_host = SDSPI_HOST_DEFAULT();

    err = esp_vfs_fat_sdmmc_mount(SD_MOUNT, &sd_sdmmc_host, &sd_mount_config, NULL);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to mount SD card: %d", err);
        return;
    }
}
void servo_init(void){
    ledc_timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK
    };

    err = ledc_timer_config(&ledc_timer_config);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %d", err);
        return;
    }

    ledc_channel_configs[0] = {
        .gpio_num = SERVO_PITCH_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };

    err = ledc_channel_config(&ledc_channel_configs[0]);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to configure LEDC channel for pitch servo: %d", err);
        return;
    }

    ledc_channel_configs[1] = {
        .gpio_num = SERVO_YAW_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };

    err = ledc_channel_config(&ledc_channel_configs[1]);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Failed to configure LEDC channel for yaw servo: %d", err);
        return;
    }

}
void app_main(void){
    pyro_init(); // must be first(!!)
    imu_init();
    baro_init();
    sd_init();
    servo_init();
    ESP_LOGI(TAG, "All subsystems initialized.");
    


    while(1){
        switch(current_state){
            case PAD:
                ESP_LOGI(TAG, "Current state: PAD");
                // Detect launch, then arm pyros
                break;
            case BOOST:
                ESP_LOGI(TAG, "Current state: BOOST");
                // run TVC: if(tilt){adjust servos}, detect burnout
                break;
            case COAST:
                ESP_LOGI(TAG, "Current state: COAST");
                // detect apogee -> deploy drogue
                break;
            case DROGUE:
                ESP_LOGI(TAG, "Current state: DROGUE");
                // if(altitude < threshold){deploy main parachute}
                break;
            case MAIN:
                ESP_LOGI(TAG, "Current state: MAIN");
                // detect landing
                break;
            case LAND:
                ESP_LOGI(TAG, "Current state: LAND");
                // start buzzing so I can find you
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(5)); //200hz
    }
}