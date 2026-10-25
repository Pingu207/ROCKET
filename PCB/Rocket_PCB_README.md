# Rocket Flight Computer

A custom flight computer for model rockets that steers the rocket during boost and deploys its parachutes on the way down.

## What it does
- **Steers the rocket** with thrust vector control: two servos gimbal the motor to keep the rocket flying straight.
- **Deploys recovery** with two pyro channels: a drogue chute at apogee and the main chute at a set altitude.
- **Logs the flight** (acceleration, rotation, altitude, servo positions, battery voltages) to a microSD card.

## How it works
An ESP32 reads an ICM-42688 IMU and a BMP280 barometer. It detects launch, tracks the rocket's orientation and drives the servos to correct any tilt. When the barometer shows the rocket has stopped climbing, it fires the drogue; the main fires lower on the way down. Each pyro channel is optocoupler-isolated and has continuity sensing, so the board beeps to confirm the e-matches are connected before launch.

The logic, servos and pyros each run from their own battery, so a servo stall or a pyro firing can't brown out the computer. The board is programmed over USB-C.

## Why I made it
[Your reason here: e.g. to learn PCB design and embedded control, and to fly a TVC rocket without buying an off-the-shelf flight computer.]
