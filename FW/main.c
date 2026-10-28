// main.c (for rocket flight computer; ESP-IDF)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

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