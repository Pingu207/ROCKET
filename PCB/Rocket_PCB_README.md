# Rocket Flight Computer

A custom flight computer for a TVC model rocket that steers the rocket during boost and deploys its parachutes on the way down.

## What it does
- **Steers the rocket** with thrust vector control: two servos gimbal the motor to keep the rocket flying straight.
- **Deploys recovery** with two pyro channels: a drogue chute at apogee and the main chute at a set altitude.
- **Logs the flight** (acceleration, rotation, altitude, servo positions, battery voltages) to a microSD card.

## How it works
An ESP32 reads an ICM-42688 IMU and a BMP280 barometer. It detects launch, tracks the rocket's orientation and drives the servos to correct any tilt. When the barometer shows the rocket has stopped climbing, it fires the drogue; the main fires lower on the way down. Each pyro channel is optoisolated by an optocoupler to protect the ESP32 and has continuity sensing, so the board confirms the e-matches are connected before launch.

The logic, servos and pyros each run from their own battery, so a servo stall or a pyro firing can't brown out the computer. The board is programmed over USB-C

## Computer Module Subsystem
<img width="452" height="356" alt="image" src="https://github.com/user-attachments/assets/0567e726-d5c2-4fbe-b5c0-68e1d3ce4786" />
Contains decoupling capacitors for the ESP power pins, RC decoupling for sensing pins, 2 SPI and an I2C communication circuit, 2 switches for RESET/BOOT on the ESP, and a buzzer.


## Why I made it
I decided to design a PCB for this project in order to get experience with designing a more complex PCB with multiple subsystems, learn about embedded control, and fly a rocket without having to buy a COTS flight computer
