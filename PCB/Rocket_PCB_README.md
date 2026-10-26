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

Contains decoupling capacitors for the ESP power pins, RC decoupling for sensing pins, 2 SPI and an I2C communication circuit, 2 switches for RESET/BOOT on the ESP, various other resistors for signal lines and pull up on I2C/SPI lines, and a buzzer for detection of the apparatus after landing. The ESP32 used here is acting as the 'brain' of this device.

## USB Input and Conversion(USB-UART) Subsystem
<img width="445" height="227" alt="image" src="https://github.com/user-attachments/assets/206f9ba8-55dc-47a7-9ec5-1eb3abca64a8" />

Uses a 14 pin USB-C Receptacle for programming of the ESP32 over USB(Note: On the PCB, only one side of the receptacle footprint is routed correctly due to space constraints, so it will only work when the cable is plugged in one specific way, not both). Contains a USBLC6-2SC6 transient voltage suppression diode array, which is an application specific integrated circuit designed for the purpose of electrostatic discharge and surge protection, used here because of the risk of ESD/Surge coming in from the USBC receptacle. The USB-UART conversion is handled by the CH340K, an auto reset transistor circuit is also in place to make the ESP32 enter boot mode when data is transferred. The VBUS from the USB can also power logic on the chip by feeding into the logic power buck converter(ORing with the battery), the line also contains a Schottky diode for protection.

## Barometer(BMP280) Subsystem
<img width="322" height="167" alt="image" src="https://github.com/user-attachments/assets/d94b6c0b-8884-4121-bca9-b40823f66f48" />

Uses the BMP280 as the barometer, contains 2 decoupling caps for the power pins and a pull up resistor for the SCL line. Wired for I2C communication with the ESP32. Since SDO is tied to GND and CSB is pulled high to +3V3, the BMP280 will be set to the I2C adress 0x76.

## Logic Power and GND subsystem
<img width="301" height="223" alt="image" src="https://github.com/user-attachments/assets/17b80f0b-cb33-4d97-9ba8-dd8b858d9b9e" />

Uses a generic connector symbol(JST PH 2.0 connector is used on the PCB). Contains reverse protection and voltage testing(facilitated by ESP ADC pin(Sensor_VN or pin 5)) before the buck converter. The buck converter input is OR'd between the battery and the USBC VBUS input, so either can power the chip. The buck converter used is the AP63203WU, a 2A synchronous step-down (buck) DC-DC switching voltage regulator, which is used here to step down the voltage of the battery(2S LiPO, 7.4V nominal) to a usable 3.3V for the ESP32. It is fitted with its correct passives(I/O capacitors, inductors, and bootstrap capacitor) as specified by its datasheet for this application(Note: No feedback resistors are required here because the chip outputs a fixed 3.3V due to its internal feedback loop). The output of the chip supplies power for all logic functions on the chip. This part of the schematic also shows the global GND tag being tied to GND.

## Why I made it
I decided to design a PCB for this project in order to get experience with designing a more complex PCB with multiple subsystems, learn about embedded control, and fly a rocket without having to buy a COTS flight computer
