# Rocket Flight Computer PCB

ESP32-based flight computer for model/high-power rockets, with thrust vector control (TVC), dual-deploy pyro recovery, sensor logging to microSD and USB-C programming.

| | |
|---|---|
| **MCU** | ESP32-WROOM-32 (antenna overhangs the top board edge) |
| **Sensors** | ICM-42688-P 6-axis IMU (SPI), BMP280 barometer (I2C) |
| **Storage** | microSD (SPI, push-push socket) |
| **Actuation** | 2 TVC servo outputs with independent buck regulators; 2 opto-isolated pyro channels |
| **Programming** | USB-C via CH340K USB-UART with auto-reset / auto-boot |
| **Board** | 98.3 × 76.3 mm, 4 layers, all SMD on top side (single-side PCBA) |
| **Fab target** | JLCPCB (≤100 × 100 mm price tier) |

---

## 1. Stackup and design rules

| Layer | Use |
|---|---|
| F.Cu | Components, signal routing |
| In1 (`GND`) | Solid GND plane |
| In2 (`+3V3`) | Solid 3.3 V plane |
| B.Cu | Signal routing, through-hole pins |

Design rules (JLCPCB 4-layer): 0.2 mm default trace/space (0.09 mm minimum), vias 0.6 mm / 0.3 mm drill, ≥0.3 mm copper to board edge.

Recommended trace widths for power nets: **1–1.5 mm** for `S1_PWR`, `S2_PWR`, the J4 input / servo VIN rail, `+7V4(PYRO)`, `BATT_GND` returns and `VBUS`.

---

## 2. Connectors

| Ref | Type | Location | Function | Pinout |
|---|---|---|---|---|
| **J1** | USB-C receptacle | Right edge, top | Programming / serial console, 5 V input | USB 2.0 FS, CC pulled down (5.1 k) |
| **J2** | JST-PH 2.0 mm, 2-pin | Right edge | **Logic battery** (2S LiPo, 7.4 V nominal) | 1 = BATT+, 2 = GND |
| **J3** | JST-PH 2.0 mm, 2-pin | Bottom edge | **Pyro battery** | 1 = PYRO BATT+, 2 = BATT_GND |
| **J4** | JST-PH 2.0 mm, 2-pin | Left edge, bottom | **Servo battery** | 1 = SERVO BATT+, 2 = GND |
| **J5** | 5.0 mm screw terminal, 2-pos | Bottom edge | **Drogue e-match** | 1 = E_MATCH_D(+) (FET drain), 2 = +7V4(PYRO) |
| **J6** | 5.0 mm screw terminal, 2-pos | Bottom edge | **Main e-match** | 1 = E_MATCH_M(+) (FET drain), 2 = +7V4(PYRO) |
| **J7** | microSD, push-push | Top-left corner | Data logging | SPI mode |
| **J8** | 2 × 3 pin header, 2.54 mm | Left edge, top | **TVC servos** | See below |

**J8 servo header.** Each servo plugs into one column (signal / power / GND):

| | Servo 1 (pitch) | Servo 2 (yaw) |
|---|---|---|
| Signal | Pin 1 (`SERVO1_PWM`, IO4 via R22) | Pin 2 (`SERVO2_PWM`, IO13 via R23) |
| Power | Pin 3 (`S1_PWR`) | Pin 4 (`S2_PWR`) |
| GND | Pin 5 | Pin 6 |

---

## 3. Power architecture

There are three separate battery inputs, so a servo stall or a pyro firing can't brown out the flight computer.

```
J1 USB 5V ──► D6 ─┐
                  ├─► VBUS ──► U6 AP63203 buck ──► +3V3 (ESP32, sensors, SD, CH340)
J2 2S LiPo ─► D7 ─┘                │
              └─► R37/R36 divider ─┴─► BATT_VTEST (IO39)

J4 servo batt ─► Q4 (P-FET reverse protection) ─► VIN ─┬─► U10 TPS563200 ─► S1_PWR ─► Servo 1
                                                      └─► U9  TPS563200 ─► S2_PWR ─► Servo 2

J3 pyro batt ─► D8 ─► +7V4(PYRO) ─► e-matches J5/J6 ─► Q3/Q5 low-side FETs ─► BATT_GND
```

| Rail | Source | Notes |
|---|---|---|
| **+3V3** | U6 AP63203 synchronous buck, fed by VBUS | Diode-OR of USB (D6) and logic battery (D7). Board runs from USB alone for bench work. |
| **S1_PWR / S2_PWR** | U10 / U9 TPS563200 bucks | Independent per servo. Output voltage set by R18/R19 and R16/R17. D5 / D3 indicate the rails are up. 470 µF bulk caps (C7, C22) absorb servo current spikes. |
| **+7V4(PYRO)** | J3 via D8 (SS34) | D4 lights when the pyro battery is present. |

**Ground.** `BATT_GND` (pyro return) joins `GND` through **R38 (0 Ω)**. See §6.

---

## 4. Functional blocks

### Computer module (U1, ESP32-WROOM-32)
- C1 (0.1 µF) and C2 (10 µF) decouple the 3V3 supply pin.
- R1 / C5 form the EN power-on reset; **SW2 = RESET**, **SW1 = BOOT** (IO0).
- Q6 + R4 drive the buzzer BZ1 from IO14.
- The four analog sense inputs each have an RC filter at the pin.
- The antenna overhangs the top board edge, so no copper or keepout is needed under it.

### USB-C / UART (J1, U3, U4, Q1, Q2)
- U4 (USBLC6) provides ESD protection on D+/D−.
- U3 (CH340K) is the USB-UART bridge to UART0 (`ESP_TX` / `ESP_RX`).
- Q1/Q2 with R8/R9 form the standard DTR/RTS auto-reset circuit, so `idf.py flash` works without pressing buttons.

### IMU (U2, ICM-42688-P)
- SPI on its own bus, with INT1 wired to IO16.
- R3 pulls CS up so the part stays in SPI mode during boot.
- It sits away from the buck converters to limit thermal and switching noise.

### Barometer (U5, BMP280)
- I2C at address **0x76** (SDO tied to GND, CSB tied to 3V3).
- The SDA and SCL pull-ups are R32 and R33.
- It sits away from the ESP32 module and the regulators to avoid self-heating error.

### SD card (J7)
- Separate SPI bus from the IMU.
- R26/R27 pull up CS and MISO; R34/R35 pull up DAT1/DAT2.

### Servo / TVC (U9, U10, Q4, J4, J8)
- Two mirrored TPS563200 buck channels feed two servo outputs.
- R22/R23 are series resistors on the PWM lines.
- Q4 (IRF7404) blocks reverse battery connection on J4.

### Pyro channels (U7, U8, Q3, Q5, J3, J5, J6)
- Each channel: ESP32 GPIO → PC817 optocoupler → gate resistor → logic-level N-MOSFET (low-side switch).
- D1/D2 are flyback diodes across each e-match.
- R39/R12 and R40/R13 sense continuity; R28/R30 sense the pyro battery voltage.

---

## 5. ESP32 pin map

| GPIO | Net | Function |
|---|---|---|
| IO0 | ESP_IO0 | BOOT button / auto-boot |
| EN | ESP_EN | RESET button / auto-reset |
| IO1 / IO3 | ESP_TX / ESP_RX | UART0 ↔ CH340K |
| IO4 | SERVO1_PWM | Servo 1 (pitch) |
| IO13 | SERVO2_PWM | Servo 2 (yaw) |
| IO5 | IO5_MAIN | **Main pyro fire** (HIGH = fire) |
| IO33 | IO33_DROGUE | **Drogue pyro fire** (HIGH = fire) |
| IO14 | — | Buzzer (via Q6) |
| IO16 | IMU_INT1 | IMU interrupt |
| IO17 / IO18 / IO19 / IO21 | SPI_CS / SCLK / MOSI / MISO | IMU SPI |
| IO22 / IO23 | I2C_SCL / I2C_SDA | Barometer |
| IO25 / IO26 / IO27 / IO32 | SD_CS / MOSI / CLK / MISO | microSD SPI |
| IO34 | CONT_D | Drogue continuity (ADC1_CH6) |
| IO35 | CONT_M | Main continuity (ADC1_CH7) |
| IO36 | PYRO_VTEST | Pyro battery voltage (ADC1_CH0) |
| IO39 | BATT_VTEST | Logic battery voltage (ADC1_CH3) |

**Analog scaling** (Vin = Vadc / ratio):

| Signal | Divider | Ratio | 8.4 V full charge reads |
|---|---|---|---|
| PYRO_VTEST | R28 10 k / R30 4.7 k | 0.320 | 2.69 V |
| CONT_D / CONT_M | R39 / R12, R40 / R13 (10 k / 4.7 k) | 0.320 | 2.69 V |
| BATT_VTEST | R37 100 k / R36 33 k | 0.248 | 2.08 V |

**Continuity logic.** With an e-match connected and the FET off, the drain node is pulled up to the pyro rail, so CONT reads close to PYRO_VTEST. With no e-match, the divider pulls the node to 0 V.

---

## 6. Safety notes

> ⚠️ **Pyro channels can ignite e-matches. Treat the board as live whenever J3 is connected.**

1. **IO5 strapping pin (errata).** The MAIN pyro output is on IO5, an ESP32 strapping pin with an internal pull-up during reset. The firmware drives it low immediately, but there's a short window at power-up before code runs. **Connect the pyro battery (J3) only on the pad, after the logic battery is on.** Next revision: move MAIN to a non-strapping GPIO.
2. **Arming.** There's no hardware arm switch. The firmware auto-arms only after it detects launch, and the pyro battery connection acts as the physical safe/arm.
3. **Ground isolation trade-off.**
   - **R38 fitted (default):** BATT_GND and GND are common. Continuity and pyro-voltage sensing work.
   - **R38 removed:** The pyro side is fully isolated, but the continuity and voltage readings become invalid, because the sense dividers reference the logic ground.
4. Bench-test both channels with LEDs or low-current bulbs before using e-matches.

---

## 7. Assembly (JLCPCB PCBA)

- **SMD:** All SMD parts, including the LGA sensors U2 and U5, are on the **top side**. Single-side assembly is enough.
- **Through-hole:** J2–J8, Q3, Q5, U7, U8, BZ1, Q6 and J1's shell pins. Hand-solder these or order THT assembly.
- **Polarity checks before ordering:**
  - **C7 / C22** (470 µF electrolytic): the netlist puts `S1_PWR` / `S2_PWR` on pad 1. Confirm pad 1 is + on the footprint.
  - **D1–D8:** cathodes are on pad 1. The flyback diodes D1/D2 must have their cathodes on +7V4.
  - **J5 / J6:** confirm the screw-terminal wire entry faces off the board edge.
- **LCSC part numbers** are in the schematic symbol fields.

---

## 8. Bring-up procedure

1. **Visual / continuity check:**
   - With no power applied, check for shorts between +3V3 and GND.
   - Check VBUS to GND and S1/S2_PWR to GND.
   - Confirm the USB VBUS net isn't connected to +3V3 (a fixed schematic error).
2. **USB only:**
   - Plug in USB-C and measure 3.3 V on C31/C32.
   - The CH340 should enumerate as a serial port.
3. **Flash firmware:** `idf.py flash monitor`. Auto-reset should work without pressing buttons. If it doesn't, hold SW1 (BOOT), then tap SW2 (RESET).
4. **Sensors:** The serial log should show `ICM-42688 ready` and `BMP280 ready`, and SD logging should start.
5. **Logic battery (J2):** Confirm the board runs from battery alone and BATT_VTEST reads correctly.
6. **Servo battery (J4):**
   - Measure the S1_PWR / S2_PWR rails; D3 and D5 should light.
   - Check the rail voltage matches your servos' rating **before** plugging servos in.
7. **Pyro (J3), with test loads only:**
   - D4 should light and PYRO_VTEST should read correctly.
   - With a test load on J5/J6, the continuity beeps should report it.
   - Simulate a flight on the bench to confirm both channels fire.

---

## 9. Known issues / TODO for next revision

- [ ] Move MAIN pyro off IO5 (strapping pin).
- [ ] Add a hardware arm switch or remove-before-flight jumper on the pyro rail.
- [ ] Silkscreen reference designators need a tidy-up pass.
- [ ] Optional: GND stitching vias around both servo bucks and along board edges.
- [ ] Optional: a test point on each rail (+3V3, VBUS, S1/S2_PWR, +7V4).
