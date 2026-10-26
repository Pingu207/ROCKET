#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "logger.h"
#include "config.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "logger";
#define MOUNT "/sdcard"

typedef struct { bool is_event; union { log_rec_t rec; char msg[64]; }; } item_t;

static QueueHandle_t s_q;
static FILE *s_f;
static volatile bool s_closing;

static void logger_task(void *arg)
{
    item_t it;
    TickType_t last_flush = xTaskGetTickCount();
    for (;;) {
        if (xQueueReceive(s_q, &it, pdMS_TO_TICKS(100)) == pdTRUE && s_f) {
            if (it.is_event) {
                fprintf(s_f, "#EVENT,%s\n", it.msg);
            } else {
                const log_rec_t *r = &it.rec;
                fprintf(s_f, "%lu,%u,%u,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.2f,"
                             "%.2f,%.2f,%d,%d,%.2f,%.2f\n",
                        (unsigned long)r->t_ms, r->state, r->flags,
                        r->accel_g[0], r->accel_g[1], r->accel_g[2],
                        r->gyro_dps[0], r->gyro_dps[1], r->gyro_dps[2],
                        r->alt_m, r->vel_ms, r->pressure_pa, r->temp_c,
                        r->tilt_deg[0], r->tilt_deg[1], r->servo_us[0], r->servo_us[1],
                        r->v_batt, r->v_pyro);
            }
        }
        if (s_f && (xTaskGetTickCount() - last_flush) > pdMS_TO_TICKS(1000)) {
            fflush(s_f);
            fsync(fileno(s_f));
            last_flush = xTaskGetTickCount();
        }
        if (s_closing && uxQueueMessagesWaiting(s_q) == 0 && s_f) {
            fclose(s_f);
            s_f = NULL;
            ESP_LOGI(TAG, "log closed");
        }
    }
}

bool logger_init(void)
{
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    host.max_freq_khz = 10000;

    spi_bus_config_t bus = {
        .mosi_io_num = PIN_SD_MOSI, .miso_io_num = PIN_SD_MISO, .sclk_io_num = PIN_SD_CLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 4096,
    };
    if (spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK) return false;

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = PIN_SD_CS;
    slot.host_id = SPI3_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mc = {
        .format_if_mount_failed = false, .max_files = 2, .allocation_unit_size = 16 * 1024,
    };
    sdmmc_card_t *card;
    if (esp_vfs_fat_sdspi_mount(MOUNT, &host, &slot, &mc, &card) != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed");
        return false;
    }

    char path[32];
    for (int n = 0; n < 1000; n++) {               /* next free FLTnnn.CSV */
        snprintf(path, sizeof(path), MOUNT "/FLT%03d.CSV", n);
        if (access(path, F_OK) != 0) break;
    }
    s_f = fopen(path, "w");
    if (!s_f) { ESP_LOGE(TAG, "open %s failed", path); return false; }
    setvbuf(s_f, NULL, _IOFBF, 8192);
    fprintf(s_f, "t_ms,state,flags,ax,ay,az,gx,gy,gz,alt,vel,press,temp,"
                 "tilt_x,tilt_y,servo_p,servo_y,vbatt,vpyro\n");

    s_q = xQueueCreate(256, sizeof(item_t));
    xTaskCreatePinnedToCore(logger_task, "logger", 4096, NULL, 5, NULL, 0);
    ESP_LOGI(TAG, "logging to %s", path);
    return true;
}

void logger_push(const log_rec_t *r)
{
    if (!s_q || s_closing) return;
    item_t it = { .is_event = false };
    it.rec = *r;
    xQueueSend(s_q, &it, 0);
}

void logger_event(const char *msg)
{
    if (!s_q || s_closing) return;
    item_t it = { .is_event = true };
    strncpy(it.msg, msg, sizeof(it.msg) - 1);
    xQueueSend(s_q, &it, 0);
}

void logger_close(void) { s_closing = true; }
