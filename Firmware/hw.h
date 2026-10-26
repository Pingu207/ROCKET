#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum { PYRO_DROGUE = 0, PYRO_MAIN = 1 } pyro_ch_t;

typedef struct {
    float v_batt;     /* logic battery, V */
    float v_pyro;     /* pyro rail, V     */
    float v_cont[2];  /* e-match nodes, V */
    bool  cont[2];    /* e-match present  */
} adc_readings_t;

void pyro_safe_early(void);              /* call FIRST in app_main */
void pyro_arm(void);
bool pyro_is_armed(void);
bool pyro_fire(pyro_ch_t ch, uint32_t now_ms);
void pyro_update(uint32_t now_ms);       /* ends firing pulses */
bool pyro_fired(pyro_ch_t ch);

void servos_init(void);
void servo_write_us(int idx, int us);    /* 0 = pitch, 1 = yaw */

void buzzer_init(void);
void buzzer_set(bool on);

void adc_init(void);
void adc_read_all(adc_readings_t *r);
