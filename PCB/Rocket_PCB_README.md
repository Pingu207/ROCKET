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

Contains the ESP32 as the 'brain' of the board, contains decoupling/bulk capacitors for the ESP power pins, RC decoupling for sensing pins, 2 SPI and an I2C communication circuit, 2 switches for RESET/BOOT on the ESP, various other resistors for signal lines and pull up on I2C/SPI lines, and a buzzer for detection of the apparatus after landing.

## USB Input and Conversion(USB-UART) Subsystem
<img width="445" height="227" alt="image" src="https://github.com/user-attachments/assets/206f9ba8-55dc-47a7-9ec5-1eb3abca64a8" />

Uses a 14 pin USB-C Receptacle for programming of the ESP32 over USB(Note: On the PCB, only one side of the receptacle footprint is routed correctly due to space constraints, so it will only work when the cable is plugged in one specific way, not both). Contains a USBLC6-2SC6 transient voltage suppression diode array for ESD/Surge protection. The USB-UART conversion is handled by the CH340K, an auto reset transistor circuit is also in place to make the ESP32 enter boot mode when data is transferred. The VBUS from the USB can also power logic on the chip by feeding into the logic power buck converter(ORing with the battery), the line also contains a Schottky diode for protection.

## Barometer(BMP280) Subsystem
<img width="322" height="167" alt="image" src="https://github.com/user-attachments/assets/d94b6c0b-8884-4121-bca9-b40823f66f48" />

Uses the BMP280 as the barometer, contains 2 decoupling caps for the power pins and a pull up resistor for the SCL line. Wired for I2C communication with the ESP32. Since SDO is tied to GND and CSB is pulled high to +3V3, the BMP280 will be set to the I2C adress 0x76.

## Logic Power and GND subsystem
<img width="301" height="223" alt="image" src="https://github.com/user-attachments/assets/17b80f0b-cb33-4d97-9ba8-dd8b858d9b9e" />

Uses a generic connector symbol(JST PH 2.0 connector is used on the PCB). Contains reverse protection and voltage testing(facilitated by ESP ADC pin(Sensor_VN or pin 5)) before the buck converter. The buck converter input is OR'd between the battery and the USBC VBUS input, so either can power the chip. The buck converter used is the AP63203WU, a 2A synchronous step-down (buck) DC-DC switching voltage regulator, which is used here to step down the voltage of the battery(2S LiPO, 7.4V nominal) to a usable 3.3V for the ESP32. It is fitted with its correct passives(Input and Output capacitors, inductors, and bootstrap capacitor) as specified by its datasheet(Note: No feedback resistors are required here because the chip outputs a fixed 3.3V due to its internal feedback loop). The output of the chip supplies power for all logic functions on the chip.

## SD Card Logging Subsystem
<img width="362" height="132" alt="image" src="https://github.com/user-attachments/assets/2bbd533f-f4d0-4129-9d1c-c2e801113459" />

Uses an SD card slot for flight data logging. Wired SPI from the ESP32. Contains pullup resistors and proper decoupling/bulk caps.

## Servo Power Subsystem
<img width="328" height="257" alt="image" src="https://github.com/user-attachments/assets/842bd762-43dc-4d45-96fa-a6c87584238f" />

Uses generic pin connectors here on the schematic and JST PH 2.0 on the PCB again. Uses an IRF7404 transistor for reverse protection. Power feeds into 2 TPS563200s, 3A step down voltage regulators. They have proper passives(Input and Output capacitors, inductors, bootstrap capacitor, and feedback resistor). They also have LEDs alongside each for indication. Power feeds into 2x3 pin headers where the servos will feed from, each power line contains a 470uF bulk cap. Each servo is also controlled via PWM from the ESP32.

## Pyro Firing Subsystem
<img width="280" height="194" alt="image" src="https://github.com/user-attachments/assets/f56551a3-49d3-4c73-b927-95b6d0f44ae1" />

Uses a generic connector symbol (JST PH 2.0 on the PCB). An SS34 Schottky diode provides reverse protection, and an LED shows when the pyro battery is connected. The battery voltage is measured by the ESP32. There are two firing channel(Drogue/Main). Each is driven by an ESP32 GPIO through a PC817 optocoupler. The optocoupler switches the gate of an IRLZ44N, with a 10k pulldown. The MOSFET switches the low side of the e-match, which connects to the board through 2-pin screw terminals. Each e-match has a flyback diode across it for protection. Each channel also has continuity sensing via an ESP32 ADC pin. The pyro ground is connected to the main GND through a 0Ω resistor. (Note: The continuity and voltage sensing need the grounds tied together. Also, the main channel uses an ESP strapping pin(IO5) so the pyro battery should only be connected once the board is powered on and on the pad.)
## Why I made it
I decided to design a PCB for this project in order to get experience with designing a more complex PCB with multiple subsystems, learn about embedded control, and fly a rocket without having to buy a COTS flight computer
