#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t t_ms;
    uint8_t  state;
    uint8_t  flags;          /* bit0 armed, bit1 drogue fired, bit2 main fired, bit3/4 cont D/M */
    float    accel_g[3];     /* body frame */
    float    gyro_dps[3];    /* body frame */
    float    alt_m, vel_ms, pressure_pa, temp_c;
    float    tilt_deg[2];
    int16_t  servo_us[2];
    float    v_batt, v_pyro;
} log_rec_t;

bool logger_init(void);              /* mounts SD, opens new file, starts task */
void logger_push(const log_rec_t *r);/* non-blocking, drops if queue full */
void logger_event(const char *msg);  /* event line in the log */
void logger_close(void);
