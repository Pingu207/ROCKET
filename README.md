# ROCKET (TVC)

A rocket using thrust vector control (TVC), with an onboard ESP32-WROOM-32 flight computer and sensors on a custom PCB.

## Overview
This project combines a gimbaled thrust-vectoring motor mount with a custom flight computer 
to actively stabilize the rocket during ascent. Progress is being logged as part of Hack 
Club's Stardance challenge.

## Hardware
- **MCU:** ESP32-WROOM-32
- **Barometer:** BMP280 (wired via I2C)
- **Servos:** 2x 20 kg·cm servos for TVC gimbal actuation
- **Power:** 2S LiPo battery for servo supply
- **Pyro channels:** Parachute ejection circuit (schematic complete)
- **PCB:** Custom board housing flight computer + sensors (schematic in progress)

## Mechanical
- TVC mount fully built, with weight-reduction holes machined in
- First servo mount CAD'd and finished
- Rocket body and nosecone modeled in CAD and assembled with the TVC assembly

## Flight Simulation
Simulated the rocket's altitude, velocity, and acceleration profile:
- Fit a polynomial thrust curve using regression (Desmos)
- Derived an ODE (Riccati equation) factoring in gravity and drag
- Solved numerically in Python (matplotlib) for performance
- **Result:** predicted apogee of 268.65 m at 7.66 seconds

## Project Status
- [x] TVC mount designed, built, and weight-optimized
- [x] Servo mount CAD finished
- [x] Body + nosecone CAD assembled with TVC
- [x] BMP280 wired (I2C)
- [x] Pyro firing (parachute ejection) circuit designed
- [x] Flight simulation (altitude/velocity/acceleration, apogee prediction)
- [x] Servo wiring for TVC actuation
- [x] GPS integration into schematic
- [x] Finish full PCB schematic
- [ ] PCB fabrication
- [ ] Static fire / ground testing  
- [ ] First test flight

## Devlog
Full build log with photos and hours logged: 
[Stardance project page](https://stardance.hackclub.com/projects/45265)

TIKI


