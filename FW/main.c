// main.c (for rocket flight computer; ESP-IDF)
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

// pins
// general pins
#define PYRO_DROGUE_PIN 33
#define PYRO_MAIN_PIN 5
#define SERVO_PITCH_PIN 4
#define SERVO_YAW_PIN 13
#define BUZZER_PIN 14
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

// stuff
typedef enum {
    PAD,
    BOOST,
    COAST,
    DROGUE,
    MAIN,
    LAND
} state_t;
static state_t current_state = PAD;
static spi_device_handle_t spi_device_handle;
static i2c_device_handle_t i2c_device_handle;
static const char *TAG = "MAIN";
#define BARO_I2C_ADDRESS 0x76


// Function prototypes
void pyro_init(void){
    gpio_set_direction(PYRO_DROGUE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(PYRO_MAIN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PYRO_DROGUE_PIN, 0);
    gpio_set_level(PYRO_MAIN_PIN, 0);
};

void imu_init(void){
    spi_bus_config_t spicfg = {
        .miso_io_num = MISO_PIN,
        .mosi_io_num = MOSI_PIN,
        .sclk_io_num = SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    }
     esp_err_t err_spi_bus = spi_bus_initialize(SPI2_HOST, &spicfg, SPI_DMA_CH_AUTO);
     if(err_spi_bus != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %d", err_spi_bus);
    }
    spi_device_interface_config_t imucfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 3,
        .spics_io_num = CS_PIN,
        .queue_size = 7
    }
    esp_err_t err_spi_device = spi_bus_add_device(SPI2_HOST, &imucfg, &spi_device_handle);
    if(err_spi_device != ESP_OK){
        ESP_LOGE(TAG, "Failed to add SPI device: %d", err_spi_device);
    }
}
void baro_init(void){
    i2c_master_bus_config_t i2ccfg = {
        .i2c_port = -1,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_time_cnt = 7,
        .flags.enable_internal_pullup = false
    };

    esp_err_t err_i2c_bus = i2c_new_master_bus(&i2ccfg, &i2c_device_handle);
    if(err_i2c_bus != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize I2C bus: %d", err_i2c_bus);
    }
    i2c_device_config_t barocfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BARO_I2C_ADDRESS,
        .scl_speed_hz = 400 * 1000
    };
    esp_err_t err_i2c_device = i2c_bus_device_create(i2c_device_handle, &barocfg);
    if(err_i2c_device != ESP_OK){
        ESP_LOGE(TAG, "Failed to add I2C device: 0x%02x, %d", BARO_I2C_ADDRESS, err_i2c_device);
    }
}
void sd_init(void){
    spi_bus_config_t spisdcfg{
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };

    esp_err_t err_spi_bus = spi_bus_initialize(SPI2_HOST, &spisdcfg, SPI_DMA_CH_AUTO);
    if(err_spi_bus != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize SPI bus for SD card: %d", err_spi_bus);
    }

    esp_vfs_fat_

}
void servo_init(void){
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t err_servo_timer = ledc_timer_config(&ledc_timer);
    if(err_servo_timer != ESP_OK){
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %d", err_servo_timer);
    }

    ledc_channel_config_t ledc_channel_pitch = {
        .gpio_num = SERVO_PITCH_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };

    esp_err_t err_servo_channel_pitch = ledc_channel_config(&ledc_channel_pitch);
    if(err_servo_channel_pitch != ESP_OK){
        ESP_LOGE(TAG, "Failed to configure LEDC channel for pitch servo: %d", err_servo_channel_pitch);
    }

    ledc_channel_config_t ledc_channel_yaw = {
        .gpio_num = SERVO_YAW_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };

    esp_err_t err_servo_channel_yaw = ledc_channel_config(&ledc_channel_yaw);
    if(err_servo_channel_yaw != ESP_OK){
        ESP_LOGE(TAG, "Failed to configure LEDC channel for yaw servo: %d", err_servo_channel_yaw);
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