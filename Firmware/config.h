#pragma once
/*
 * Rocket flight computer - board configuration
 * Pin map taken from the Rocket.kicad_sch netlist (ESP32-WROOM-32).
 */
#include "esp_adc/adc_oneshot.h"

/* ---------------- Pins ---------------- */
/* ICM-42688-P IMU, SPI (SPI2_HOST) */
#define PIN_IMU_CS        17
#define PIN_IMU_SCLK      18
#define PIN_IMU_MOSI      19
#define PIN_IMU_MISO      21
#define PIN_IMU_INT1      16   /* wired, not used: IMU is polled */

/* BMP280 barometer, I2C (SDO->GND => addr 0x76, CSB->3V3 => I2C mode) */
#define PIN_I2C_SDA       23
#define PIN_I2C_SCL       22
#define BMP280_ADDR       0x76

/* microSD, SPI (SPI3_HOST) */
#define PIN_SD_CLK        27
#define PIN_SD_MOSI       26
#define PIN_SD_MISO       32
#define PIN_SD_CS         25

/* Pyro channels: GPIO HIGH -> optocoupler LED on -> MOSFET fires */
#define PIN_PYRO_DROGUE   33
#define PIN_PYRO_MAIN      5   /* WARNING: IO5 is a strapping pin, see README */

/* TVC servos (LEDC PWM) */
#define PIN_SERVO_PITCH    4   /* J8 pin 1, SERVO1_PWM */
#define PIN_SERVO_YAW     13   /* J8 pin 2, SERVO2_PWM */

/* Buzzer via Q6 */
#define PIN_BUZZER        14
#define BUZZER_FREQ_HZ  2700   /* works for passive; active buzzers also sound */

/* ADC1 channels (ADC1 keeps working with WiFi on) */
#define ADC_CH_PYRO_VTEST ADC_CHANNEL_0  /* IO36 */
#define ADC_CH_BATT_VTEST ADC_CHANNEL_3  /* IO39 */
#define ADC_CH_CONT_D     ADC_CHANNEL_6  /* IO34 */
#define ADC_CH_CONT_M     ADC_CHANNEL_7  /* IO35 */

/* Divider ratios (Vadc = Vin * ratio), values read from the schematic */
#define DIV_PYRO   (4.7f / (10.0f + 4.7f))    /* R28 10k / R30 4.7k */
#define DIV_CONT   (4.7f / (10.0f + 4.7f))    /* R39/R12, R40/R13   */
#define DIV_BATT   (33.0f / (100.0f + 33.0f)) /* R37 100k / R36 33k */

/* ---------------- Timing ---------------- */
#define CONTROL_HZ          200     /* IMU + TVC loop */
#define BARO_DIV              4     /* baro every 4th loop  -> 50 Hz */
#define ADC_DIV              20     /* ADC every 20th loop  -> 10 Hz */
#define LOG_DIV               2     /* log every 2nd loop   -> 100 Hz */

/* ---------------- Flight logic ---------------- */
#define LAUNCH_ACCEL_G       3.0f   /* |a| above this ...            */
#define LAUNCH_ACCEL_MS        50   /* ... for this long = launch    */
#define LAUNCH_ALT_M        20.0f   /* or baro climbs this much      */
#define BURNOUT_ACCEL_G      0.5f   /* axial accel below this ...    */
#define BURNOUT_MS            100   /* ... for this long = burnout   */
#define MAX_BURN_MS          8000   /* force COAST after this        */
#define APOGEE_LOCKOUT_MS    4000   /* no apogee before this (transonic baro spikes) */
#define APOGEE_CONFIRM          5   /* consecutive baro samples with v < 0 */
#define MAIN_DEPLOY_ALT_M  150.0f   /* main fires below this AGL on descent */
#define PYRO_PULSE_MS        1000   /* firing pulse length */
#define PYRO_MIN_V           6.0f   /* warn if pyro battery below this */
#define CONT_THRESH_FRAC     0.5f   /* CONT > frac * Vpyro => e-match present */
#define LANDED_SPEED_MS      1.0f
#define LANDED_HOLD_MS       5000

/* Baro alpha-beta filter */
#define AB_ALPHA             0.20f
#define AB_BETA              0.02f

/* ---------------- TVC ---------------- */
/* Gains: gimbal degrees per degree of tilt error. MUST be tuned for your airframe. */
#define TVC_KP               0.60f
#define TVC_KI               0.10f
#define TVC_KD               0.15f  /* per deg/s of body rate */
#define TVC_I_LIMIT          3.0f   /* integral clamp, gimbal deg */
#define TVC_MAX_DEG          5.0f   /* mechanical gimbal limit */

#define SERVO_CENTER_US      1500
#define SERVO_US_PER_DEG     10.0f  /* depends on linkage - CALIBRATE */
#define SERVO_PITCH_SIGN     (+1)   /* flip if servo corrects the wrong way */
#define SERVO_YAW_SIGN       (+1)
#define SERVO_PITCH_TRIM_US  0
#define SERVO_YAW_TRIM_US    0

/*
 * IMU axis mapping. Body frame: +Z = rocket nose, X/Y = gimbal axes.
 * Body axis n = SIGN_n * sensor axis IDX_n.  Verify with the pad telemetry:
 * standing the rocket upright must give az ~ +1 g and tilt ~ 0.
 */
#define BODY_X_IDX  0
#define BODY_X_SIGN (+1)
#define BODY_Y_IDX  1
#define BODY_Y_SIGN (+1)
#define BODY_Z_IDX  2
#define BODY_Z_SIGN (+1)
