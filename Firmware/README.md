# Rocket flight computer firmware (ESP32, ESP-IDF v5.2+)

## Build / flash
    idf.py set-target esp32
    idf.py build flash monitor      # over the board's USB-C (CH340)

## What it does
| State  | Behaviour |
|--------|-----------|
| PAD    | Calibrates gyro bias + gravity alignment while still, servos centred, continuity beeps (1 = drogue, 2 = main, 3 = both, long tone = none), telemetry on serial at 2 Hz |
| BOOST  | Launch = >3 g for 50 ms or +20 m. **Pyros auto-arm here.** TVC active (quaternion attitude, PID per gimbal axis) |
| COAST  | Burnout = axial accel <0.5 g for 100 ms (or 8 s). Servos centred. Apogee = baro velocity <0 for 5 samples, locked out for 4 s after launch |
| DROGUE | Drogue fired at apogee (1 s pulse). Main fires below 150 m AGL while descending |
| MAIN / LANDED | Landed after 5 s at <1 m/s, log closed, recovery beacon |

Logs go to `/sdcard/FLTnnn.CSV` at 100 Hz (FAT32 card).

## MUST do before flight
1. **IMU axis mapping** (`config.h`, BODY_*): stand the rocket upright; pad telemetry must show az ≈ +1 g, tilt ≈ 0.
   Tilt the nose toward each gimbal axis and check the sign of tilt_x / tilt_y.
2. **Servo direction / scale**: hold the rocket, tilt it, and confirm the gimbal deflects to push the nose back.
   Flip SERVO_*_SIGN if not. Measure µs per gimbal degree (SERVO_US_PER_DEG) and set the trims.
3. **TVC gains** (TVC_KP/KI/KD) are placeholders. Tune them on a test stand / in simulation for your motor and airframe.
4. **Bench-test recovery** with LEDs or bulbs instead of e-matches (simulate launch by shaking >3 g).
5. Set MAIN_DEPLOY_ALT_M for your field.

## Hardware notes
- **IO5 (MAIN pyro) is an ESP32 strapping pin** with an internal pull-up during reset. The firmware drives it low as
  the very first instruction, but keep the pyro battery (J3) disconnected until the rocket is on the pad.
  The clean fix on the next revision is to move MAIN to a non-strapping GPIO.
- Continuity / pyro-voltage sensing only works when BATT_GND is tied to GND (R38 fitted).
- The ADC on the ESP32 is ±~3 % even calibrated; voltages are for go/no-go, not precision.
