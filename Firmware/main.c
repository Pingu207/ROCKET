/*
 * Rocket flight computer - ESP32 / ESP-IDF v5.2+
 * TVC (2 servos) + dual-deploy recovery + SD logging.
 */
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "config.h"
#include "icm42688.h"
#include "bmp280.h"
#include "hw.h"
#include "logger.h"

static const char *TAG = "flight";

#define DEG2RAD (3.14159265f / 180.0f)
#define RAD2DEG (180.0f / 3.14159265f)

typedef enum {
    ST_PAD = 0, ST_BOOST, ST_COAST, ST_DROGUE, ST_MAIN, ST_LANDED, ST_FAULT
} state_t;
static const char *ST_NAME[] = { "PAD", "BOOST", "COAST", "DROGUE", "MAIN", "LANDED", "FAULT" };

/* ---------- shared state ---------- */
static state_t s_state = ST_PAD;
static uint32_t s_t_launch, s_t_state;

static inline uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static void set_state(state_t st, uint32_t t)
{
    s_state = st;
    s_t_state = t;
    char m[48];
    snprintf(m, sizeof(m), "STATE %s t=%lu", ST_NAME[st], (unsigned long)t);
    ESP_LOGW(TAG, "%s", m);
    logger_event(m);
}

/* ---------- axis mapping ---------- */
static void to_body(const float s[3], float b[3])
{
    b[0] = BODY_X_SIGN * s[BODY_X_IDX];
    b[1] = BODY_Y_SIGN * s[BODY_Y_IDX];
    b[2] = BODY_Z_SIGN * s[BODY_Z_IDX];
}

/* ---------- attitude: quaternion, body->world ---------- */
static float q[4] = { 1, 0, 0, 0 };

static void quat_norm(float *v)
{
    float n = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
    if (n > 1e-9f) for (int i = 0; i < 4; i++) v[i] /= n;
}

/* align body measured gravity (accel, points up when still) with world +Z */
static void quat_from_accel(const float a[3])
{
    float n = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    if (n < 0.5f) return;
    float ax = a[0] / n, ay = a[1] / n, az = a[2] / n;
    /* shortest arc from a to z: axis = a x z, w = 1 + a.z */
    q[0] = 1.0f + az; q[1] = ay; q[2] = -ax; q[3] = 0.0f;
    if (q[0] < 1e-6f) { q[0] = 0; q[1] = 1; q[2] = 0; q[3] = 0; } /* upside down */
    quat_norm(q);
}

static void quat_integrate(const float w[3], float dt) /* w: body rates, rad/s */
{
    float qw = q[0], qx = q[1], qy = q[2], qz = q[3];
    q[0] += 0.5f * dt * (-qx * w[0] - qy * w[1] - qz * w[2]);
    q[1] += 0.5f * dt * ( qw * w[0] + qy * w[2] - qz * w[1]);
    q[2] += 0.5f * dt * ( qw * w[1] - qx * w[2] + qz * w[0]);
    q[3] += 0.5f * dt * ( qw * w[2] + qx * w[1] - qy * w[0]);
    quat_norm(q);
}

/* tilt of the body Z axis from vertical, expressed about body X and Y (deg) */
static void tilt_body(float *tx, float *ty)
{
    float vx = 2.0f * (q[1] * q[3] - q[0] * q[2]);  /* world up in body frame */
    float vy = 2.0f * (q[2] * q[3] + q[0] * q[1]);
    float vz = 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]);
    *tx = atan2f(vy, vz) * RAD2DEG;
    *ty = atan2f(-vx, vz) * RAD2DEG;
}

/* ---------- TVC PID ---------- */
typedef struct { float i; } pid_t_;
static pid_t_ s_pid[2];

static float pid_step(pid_t_ *p, float err_deg, float rate_dps, float dt)
{
    p->i += TVC_KI * err_deg * dt;
    if (p->i >  TVC_I_LIMIT) p->i =  TVC_I_LIMIT;
    if (p->i < -TVC_I_LIMIT) p->i = -TVC_I_LIMIT;
    float u = -(TVC_KP * err_deg + p->i + TVC_KD * rate_dps); /* oppose the tilt */
    if (u >  TVC_MAX_DEG) u =  TVC_MAX_DEG;
    if (u < -TVC_MAX_DEG) u = -TVC_MAX_DEG;
    return u;
}

static int16_t s_servo_us[2] = { SERVO_CENTER_US, SERVO_CENTER_US };

static void servos_center(void)
{
    s_servo_us[0] = SERVO_CENTER_US + SERVO_PITCH_TRIM_US;
    s_servo_us[1] = SERVO_CENTER_US + SERVO_YAW_TRIM_US;
    servo_write_us(0, s_servo_us[0]);
    servo_write_us(1, s_servo_us[1]);
}

static void servos_gimbal(float pitch_deg, float yaw_deg)
{
    s_servo_us[0] = SERVO_CENTER_US + SERVO_PITCH_TRIM_US +
                    (int)(SERVO_PITCH_SIGN * pitch_deg * SERVO_US_PER_DEG);
    s_servo_us[1] = SERVO_CENTER_US + SERVO_YAW_TRIM_US +
                    (int)(SERVO_YAW_SIGN * yaw_deg * SERVO_US_PER_DEG);
    servo_write_us(0, s_servo_us[0]);
    servo_write_us(1, s_servo_us[1]);
}

/* ---------- buzzer patterns (non-blocking) ---------- */
static uint32_t s_beep_t0;
static void buzzer_pattern(uint32_t t, int beeps, uint32_t period_ms)
{
    uint32_t ph = (t - s_beep_t0) % period_ms;
    int slot = ph / 300;                    /* 150 ms on / 150 ms off */
    buzzer_set(slot < beeps && (ph % 300) < 150);
}

/* ---------- control task ---------- */
static void control_task(void *arg)
{
    imu_sample_t imu;
    adc_readings_t adc = { 0 };
    float acc[3] = { 0 }, gyr[3] = { 0 }, bias[3] = { 0 };
    float p0 = 101325.0f, press = 101325.0f, temp = 0.0f;
    float alt = 0.0f, vel = 0.0f, tilt[2] = { 0 };
    int launch_cnt = 0, burnout_cnt = 0, apogee_cnt = 0;
    uint32_t still_since = 0, tick = 0;
    int64_t t_prev = esp_timer_get_time();

    /* ground pressure reference: average 1 s */
    float sum = 0; int n = 0;
    for (int i = 0; i < 50; i++) {
        if (bmp280_read(&press, &temp) == ESP_OK) { sum += press; n++; }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (n) p0 = sum / n;

    TickType_t wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000 / CONTROL_HZ);
    s_beep_t0 = now_ms();

    for (;;) {
        vTaskDelayUntil(&wake, period);
        int64_t t_us = esp_timer_get_time();
        float dt = (t_us - t_prev) * 1e-6f;
        t_prev = t_us;
        uint32_t t = (uint32_t)(t_us / 1000);
        tick++;

        /* ---- sensors ---- */
        if (icm42688_read(&imu) == ESP_OK) {
            to_body(imu.accel_g, acc);
            to_body(imu.gyro_dps, gyr);
        }
        float amag = sqrtf(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
        float w[3] = { (gyr[0] - bias[0]) * DEG2RAD, (gyr[1] - bias[1]) * DEG2RAD,
                       (gyr[2] - bias[2]) * DEG2RAD };

        bool baro_new = false;
        if (tick % BARO_DIV == 0 && bmp280_read(&press, &temp) == ESP_OK) {
            float meas = bmp280_altitude_m(press, p0);
            float bdt = BARO_DIV / (float)CONTROL_HZ;
            alt += vel * bdt;                       /* alpha-beta filter */
            float r = meas - alt;
            alt += AB_ALPHA * r;
            vel += (AB_BETA / bdt) * r;
            baro_new = true;
        }
        if (tick % ADC_DIV == 0) adc_read_all(&adc);

        pyro_update(t);

        /* ---- state machine ---- */
        switch (s_state) {
        case ST_PAD: {
            /* keep gyro bias and gravity alignment fresh while sitting still */
            bool still = fabsf(amag - 1.0f) < 0.05f &&
                         fabsf(gyr[0] - bias[0]) < 3 && fabsf(gyr[1] - bias[1]) < 3 &&
                         fabsf(gyr[2] - bias[2]) < 3;
            if (still) {
                for (int i = 0; i < 3; i++) bias[i] += 0.01f * (gyr[i] - bias[i]);
                quat_from_accel(acc);
                if (baro_new) p0 += 0.002f * (press - p0);   /* slow ground drift */
            }
            servos_center();

            /* continuity beeps: 1 = drogue, 2 = main, 3 = both, long tone = none */
            int code = (adc.cont[0] ? 1 : 0) + (adc.cont[1] ? 2 : 0);
            if (code) buzzer_pattern(t, code, 3000);
            else      buzzer_set(((t - s_beep_t0) % 3000) < 600);

            launch_cnt = (amag > LAUNCH_ACCEL_G) ? launch_cnt + 1 : 0;
            if (launch_cnt * 1000 / CONTROL_HZ >= LAUNCH_ACCEL_MS || alt > LAUNCH_ALT_M) {
                buzzer_set(false);
                s_t_launch = t;
                pyro_arm();                                  /* auto-arm at launch */
                if (adc.v_pyro < PYRO_MIN_V) logger_event("WARN pyro battery low/absent");
                memset(s_pid, 0, sizeof(s_pid));
                set_state(ST_BOOST, t);
            }
            break;
        }
        case ST_BOOST:
            quat_integrate(w, dt);
            tilt_body(&tilt[0], &tilt[1]);
            servos_gimbal(pid_step(&s_pid[0], tilt[0], gyr[0] - bias[0], dt),
                          pid_step(&s_pid[1], tilt[1], gyr[1] - bias[1], dt));
            burnout_cnt = (acc[2] < BURNOUT_ACCEL_G) ? burnout_cnt + 1 : 0;
            if (burnout_cnt * 1000 / CONTROL_HZ >= BURNOUT_MS || t - s_t_launch > MAX_BURN_MS) {
                servos_center();
                set_state(ST_COAST, t);
            }
            break;

        case ST_COAST:
            quat_integrate(w, dt);
            tilt_body(&tilt[0], &tilt[1]);
            servos_center();
            if (baro_new && t - s_t_launch > APOGEE_LOCKOUT_MS) {
                apogee_cnt = (vel < 0.0f) ? apogee_cnt + 1 : 0;
                if (apogee_cnt >= APOGEE_CONFIRM) {
                    pyro_fire(PYRO_DROGUE, t);
                    set_state(ST_DROGUE, t);
                }
            }
            break;

        case ST_DROGUE:
            servos_center();
            if (baro_new && vel < 0.0f && alt < MAIN_DEPLOY_ALT_M) {
                pyro_fire(PYRO_MAIN, t);
                set_state(ST_MAIN, t);
            }
            break;

        case ST_MAIN:
            servos_center();
            if (fabsf(vel) < LANDED_SPEED_MS) {
                if (!still_since) still_since = t;
                if (t - still_since > LANDED_HOLD_MS) {
                    set_state(ST_LANDED, t);
                    logger_close();
                }
            } else {
                still_since = 0;
            }
            break;

        case ST_LANDED:
            servos_center();
            buzzer_pattern(t, 3, 5000);                     /* recovery beacon */
            break;

        default:
            servos_center();
            buzzer_pattern(t, 5, 2000);
            break;
        }

        /* ---- logging ---- */
        if (tick % LOG_DIV == 0) {
            log_rec_t r = {
                .t_ms = t, .state = s_state,
                .flags = (pyro_is_armed() << 0) | (pyro_fired(PYRO_DROGUE) << 1) |
                         (pyro_fired(PYRO_MAIN) << 2) | (adc.cont[0] << 3) | (adc.cont[1] << 4),
                .alt_m = alt, .vel_ms = vel, .pressure_pa = press, .temp_c = temp,
                .tilt_deg = { tilt[0], tilt[1] },
                .servo_us = { s_servo_us[0], s_servo_us[1] },
                .v_batt = adc.v_batt, .v_pyro = adc.v_pyro,
            };
            memcpy(r.accel_g, acc, sizeof(acc));
            memcpy(r.gyro_dps, gyr, sizeof(gyr));
            logger_push(&r);
        }

        /* ---- pad telemetry over USB serial, 2 Hz ---- */
        if (s_state == ST_PAD && tick % (CONTROL_HZ / 2) == 0) {
            float tx, ty; tilt_body(&tx, &ty);
            ESP_LOGI(TAG, "a=(%.2f %.2f %.2f)g tilt=(%.1f %.1f) alt=%.1f vbat=%.2f vpyro=%.2f cont D%d M%d",
                     acc[0], acc[1], acc[2], tx, ty, alt, adc.v_batt, adc.v_pyro,
                     adc.cont[0], adc.cont[1]);
        }
    }
}

void app_main(void)
{
    pyro_safe_early();          /* FIRST: force both pyro outputs LOW */
    buzzer_init();
    servos_init();
    adc_init();

    bool ok = true;
    if (icm42688_init(SPI2_HOST) != ESP_OK) { ESP_LOGE(TAG, "IMU init failed"); ok = false; }
    if (bmp280_init() != ESP_OK)            { ESP_LOGE(TAG, "baro init failed"); ok = false; }
    if (!logger_init())                     ESP_LOGE(TAG, "SD logging unavailable");

    if (!ok) {
        s_state = ST_FAULT;     /* never arms: no launch detection in FAULT */
        logger_event("FAULT sensor init");
    }

    buzzer_set(true); vTaskDelay(pdMS_TO_TICKS(500)); buzzer_set(false);   /* boot beep */
    xTaskCreatePinnedToCore(control_task, "control", 6144, NULL, 20, NULL, 1);
}
