#include "hw.h"
#include "config.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "hw";

/* ---------------- Pyro ---------------- */
static const int s_pyro_pin[2] = { PIN_PYRO_DROGUE, PIN_PYRO_MAIN };
static bool     s_armed;
static bool     s_fired[2];
static uint32_t s_fire_end[2];
static bool     s_firing[2];

void pyro_safe_early(void)
{
    for (int i = 0; i < 2; i++) {
        gpio_reset_pin(s_pyro_pin[i]);
        gpio_set_level(s_pyro_pin[i], 0);
        gpio_set_direction(s_pyro_pin[i], GPIO_MODE_OUTPUT);
        gpio_set_level(s_pyro_pin[i], 0);
        gpio_pulldown_en(s_pyro_pin[i]);
        gpio_pullup_dis(s_pyro_pin[i]);
    }
}

void pyro_arm(void)         { s_armed = true; ESP_LOGW(TAG, "PYROS ARMED"); }
bool pyro_is_armed(void)    { return s_armed; }
bool pyro_fired(pyro_ch_t c){ return s_fired[c]; }

bool pyro_fire(pyro_ch_t ch, uint32_t now_ms)
{
    if (!s_armed || s_fired[ch]) return false;
    gpio_set_level(s_pyro_pin[ch], 1);
    s_firing[ch] = true;
    s_fired[ch] = true;
    s_fire_end[ch] = now_ms + PYRO_PULSE_MS;
    ESP_LOGW(TAG, "FIRE %s", ch == PYRO_DROGUE ? "DROGUE" : "MAIN");
    return true;
}

void pyro_update(uint32_t now_ms)
{
    for (int i = 0; i < 2; i++) {
        if (s_firing[i] && (int32_t)(now_ms - s_fire_end[i]) >= 0) {
            gpio_set_level(s_pyro_pin[i], 0);
            s_firing[i] = false;
        }
    }
}

/* ---------------- Servos (LEDC timer 0, 50 Hz) ---------------- */
#define SERVO_RES   LEDC_TIMER_14_BIT
#define SERVO_MAX   ((1 << 14) - 1)
#define SERVO_FREQ  50

void servos_init(void)
{
    ledc_timer_config_t t = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_0,
        .duty_resolution = SERVO_RES, .freq_hz = SERVO_FREQ, .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));
    const int pins[2] = { PIN_SERVO_PITCH, PIN_SERVO_YAW };
    for (int i = 0; i < 2; i++) {
        ledc_channel_config_t c = {
            .gpio_num = pins[i], .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0 + i, .timer_sel = LEDC_TIMER_0, .duty = 0,
        };
        ESP_ERROR_CHECK(ledc_channel_config(&c));
        servo_write_us(i, SERVO_CENTER_US);
    }
}

void servo_write_us(int idx, int us)
{
    if (us < 900)  us = 900;      /* hard limits protect the gimbal */
    if (us > 2100) us = 2100;
    uint32_t duty = (uint32_t)us * (SERVO_MAX + 1) / (1000000 / SERVO_FREQ);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0 + idx, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0 + idx);
}

/* ---------------- Buzzer (LEDC timer 1) ---------------- */
void buzzer_init(void)
{
    ledc_timer_config_t t = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_10_BIT, .freq_hz = BUZZER_FREQ_HZ, .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));
    ledc_channel_config_t c = {
        .gpio_num = PIN_BUZZER, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_2, .timer_sel = LEDC_TIMER_1, .duty = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

void buzzer_set(bool on)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, on ? 512 : 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}

/* ---------------- ADC1 ---------------- */
static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;
static const adc_channel_t s_ch[4] = {
    ADC_CH_BATT_VTEST, ADC_CH_PYRO_VTEST, ADC_CH_CONT_D, ADC_CH_CONT_M,
};

void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t u = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&u, &s_adc));
    adc_oneshot_chan_cfg_t cc = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    for (int i = 0; i < 4; i++) ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, s_ch[i], &cc));

    adc_cali_line_fitting_config_t cal = {
        .unit_id = ADC_UNIT_1, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cal, &s_cali) != ESP_OK) {
        s_cali = NULL;
        ESP_LOGW(TAG, "ADC calibration unavailable, using raw scaling");
    }
}

static float read_v(adc_channel_t ch)
{
    int raw = 0, mv = 0, sum = 0;
    for (int i = 0; i < 4; i++) {                 /* small average */
        adc_oneshot_read(s_adc, ch, &raw);
        if (s_cali) adc_cali_raw_to_voltage(s_cali, raw, &mv);
        else        mv = raw * 3100 / 4095;
        sum += mv;
    }
    return sum / 4.0f / 1000.0f;
}

void adc_read_all(adc_readings_t *r)
{
    r->v_batt    = read_v(ADC_CH_BATT_VTEST) / DIV_BATT;
    r->v_pyro    = read_v(ADC_CH_PYRO_VTEST) / DIV_PYRO;
    r->v_cont[0] = read_v(ADC_CH_CONT_D)     / DIV_CONT;
    r->v_cont[1] = read_v(ADC_CH_CONT_M)     / DIV_CONT;
    for (int i = 0; i < 2; i++)
        r->cont[i] = (r->v_pyro > 2.0f) && (r->v_cont[i] > CONT_THRESH_FRAC * r->v_pyro);
}
