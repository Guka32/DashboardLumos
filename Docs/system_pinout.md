# STM32 Nucleo-F446RE system pinout

This document describes the firmware pin assignments on the NUCLEO-F446RE (STM32F446RE). Connector references use the board's CN7/CN10 Morpho headers and the CN5/CN6 Arduino headers. The map reflects the firmware and CubeMX configuration in this repository; it does not describe wiring on an external carrier board.

## ESC outputs

The firmware has two logical propulsion commands, left and right. Each command is output on two MCU pins so one command can drive two ESC signal inputs on that side.

| Function | Nucleo header pin | MCU signal | Timer output | Behavior |
|---|---|---|---|---|
| Left ESC 1 (primary) | CN10-4 | PC6 | TIM3_CH1 | Left command |
| Left ESC 2 (mirror) | CN10-25 | PB10 | TIM2_CH3 | Same left pulse width |
| Right ESC 1 (primary) | CN10-19 | PC7 | TIM3_CH2 | Right command |
| Right ESC 2 (mirror) | CN10-23 | PA8 | TIM1_CH1 | Same right pulse width |

All four outputs use a nominal 50 Hz, 20 ms period. The normalized command is clamped to `[-1.0, +1.0]` and maps to a high pulse from 1100 us to 1900 us; `0.0` is neutral at 1500 us. The mirrors are updated in the same PWM setter as the primary, so manual, autonomous, arming, and failsafe writes keep the pair's pulse widths equal. The left pair uses TIM3/TIM2 and the right pair uses TIM3/TIM1, all with the same timer input, prescaler, and period. Their pulse widths and repetition rates match; their rising edges are not guaranteed to be phase-aligned.

There are four propulsion ESC outputs in this mapping. The request mentions five ESCs but describes two left and two right thrusters. The fifth ESC has no separately defined command or pin in the current firmware; do not connect it until its intended function (left, right, or independent) is decided.

**ESC wiring:** connect each signal input to its listed pin and connect ESC signal ground to Nucleo ground. Do not power motors from the Nucleo. Check the ESC input's logic-level requirements and handle BEC +5 V leads according to the ESC manufacturer's instructions; do not tie multiple BEC outputs together by default.

## Remaining actuator outputs

| Function | Nucleo header pin | MCU signal | Timer / GPIO | Notes |
|---|---|---|---|---|
| Turret X servo | CN7-34 | PB0 | TIM3_CH3 | 50 Hz, normalized `[-1,1]` to 1100–1900 us |
| Turret Y servo | CN10-24 | PB1 | TIM3_CH4 | 50 Hz, normalized `[-1,1]` to 1100–1900 us |
| Unstick servo | CN10-17 | PB6 | TIM4_CH1 | 50 Hz, normalized `[-1,1]` to 1100–1900 us |
| Reserved PWM 5 | CN7-21 | PB7 | TIM4_CH2 | 50 Hz, neutral unless explicitly commanded |
| Reserved PWM 6 | CN10-3 | PB8 | TIM4_CH3 | 50 Hz, neutral unless explicitly commanded |
| Reserved PWM 7 | CN10-5 | PB9 | TIM4_CH4 | 50 Hz, neutral unless explicitly commanded |
| IBT-2 right input | CN10-35 | PA2 | TIM9_CH1 | 20 kHz PWM leg; direction selected by signed command |
| IBT-2 left input | CN10-37 | PA3 | TIM9_CH2 | 20 kHz PWM leg; direction selected by signed command |
| IBT-2 R_EN | CN7-38 | PC0 | GPIO output | Both enables are low at boot; enabled together by actuator command |
| IBT-2 L_EN | CN7-36 | PC1 | GPIO output | Both enables are low at boot; enabled together by actuator command |
| Pump enable | CN10-27 | PB4 | GPIO output | Initialized low; API exists in `pwm_out.c` |

## CAN and communications

| Function | Nucleo header pin | MCU signal | Configuration / connection |
|---|---|---|---|
| CAN2 RX | CN10-16 | PB12 | 125 kbit/s; connect to a CAN transceiver RXD, not directly to CANH/CANL |
| CAN2 TX | CN10-30 | PB13 | Connect to a CAN transceiver TXD; use bus CANH/CANL and appropriate termination |
| SBUS receiver input | CN7-4 | PD2 / UART5_RX | 100 kbaud, 8 data bits, even parity, 2 stop bits; RX only |
| Telemetry UART TX | CN7-28 | PA0 / UART4_TX | 115200 baud, 8N1; application transmits telemetry here |
| Telemetry UART RX | CN7-30 | PA1 / UART4_RX | 115200 baud, 8N1; configured but not used by the current application |
| USART1 TX | CN10-21 | PA9 / USART1_TX | 115200 baud, 8N1; configured, no application use found |
| USART1 RX | CN10-33 | PA10 / USART1_RX | 115200 baud, 8N1; configured, no application use found |
| SPI1 SCK | CN10-11 | PA5 | SPI1 configured; no transfer call found in application code |
| SPI1 MISO | CN10-13 | PA6 | SPI1 configured; no transfer call found in application code |
| SPI1 MOSI | CN10-15 | PA7 | SPI1 configured; no transfer call found in application code |
| SPI3 SCK | CN7-1 | PC10 | SPI3 initialized; no transfer call found in application code |
| SPI3 MOSI | CN10-29 | PB5 | SPI3 initialized; no transfer call found in application code |
| USB FS DM | MCU pin PA11 | PA11 | USB device D−; use the board USB connector |
| USB FS DP | MCU pin PA12 | PA12 | USB device D+; use the board USB connector |

The CAN firmware uses standard 11-bit identifiers. Main IDs relevant to bench tests:

| ID | Direction | Meaning |
|---|---|---|
| `0x01E` | Jetson → STM32 | Jetson heartbeat; autonomous motor output is forced neutral if it becomes stale (500 ms) |
| `0x120` | Jetson → STM32 | Two little-endian `float32` motor commands: left, right |
| `0x121`–`0x123` | Jetson → STM32 | Additional motor pairs; not used for the two propulsion outputs |
| `0x150` | actuator PCB/host → STM32 | IBT-2 and three servo actuator command fields |
| `0x010` | STM32 → CAN bus | Gateway heartbeat/status |
| `0x101`, `0x102` | STM32 → CAN bus | Raw timer compare counts for PWM channels 0–3 and 4–7 |
| `0x103`–`0x106` | STM32 → CAN bus | SBUS channel groups, four 16-bit channels per frame |
| `0x107` | STM32 → CAN bus | SBUS failsafe flag |
| `0x108` | STM32 → CAN bus | Local battery ADC reading (`float32`) |
| `0x111`, `0x112` | STM32 → CAN bus | Debug text and 1 Hz ping |
| `0x200` | battery PCB → STM32 | Battery voltage (`float32`) |
| `0x300` | navigation PCB → STM32 | Latitude and longitude (`float32`, `float32`) |
| `0x301` | navigation PCB → STM32 | Heading (`float32`) |

The `0x010` status payload is uptime in bytes 0–3 (little-endian `uint32`), mode in byte 4, auxiliary selector in byte 5, armed flag in byte 6, and a reserved zero byte 7. PWM feedback IDs `0x101`/`0x102` report channels 0–7; the two added mirror outputs are not included in those feedback frames.

For autonomous bench checks, the SBUS selector must be in Auto and the controller must be armed. Send `0x01E` heartbeat frames and the two `float32` values in `0x120`. The firmware returns propulsion outputs to neutral when the Jetson heartbeat is older than 500 ms. There is no separate stale timer for the `0x120` motor setpoints: while the heartbeat remains fresh, the last received motor values remain active. Confirm neutral first with ESCs unpowered or motors mechanically safe, and verify pulse widths with an oscilloscope or logic analyzer before connecting propellers.

Example low-command bench loop (left `+0.10`, right `+0.20`; approximately 1540 us and 1580 us):

```sh
while true; do
  cansend can0 01E#01
  cansend can0 120#CDCCCC3DCDCC4C3E
  sleep 0.1
done
```

The loop only works after Auto is selected and armed. Stop it with Ctrl-C; loss of the heartbeat should return all propulsion PWM outputs to 1500 us after the 500 ms heartbeat timeout. For a commanded neutral while the loop is running, use `cansend can0 120#0000000000000000`.

## Analog input and indicators

| Function | Nucleo header pin | MCU signal | Notes |
|---|---|---|---|
| Local battery voltage sense | CN10-34 | PC4 / ADC2_IN14 | Firmware converts ADC counts using `raw * 0.010439 - 0.41648`; input must stay within ADC limits and requires the intended external divider |
| Mode/status indicator | CN7-17 | PA15 / DEBUG_1 | Solid in Stop/invalid SBUS, off in Manual, blinking in Auto |
| Debug output 2 | CN10-22 | PB2 / DEBUG_2 | Configured push-pull output |
| Debug output 3 | CN10-1 | PC9 / DEBUG_3 | Configured push-pull output |
| Debug output 5 | CN7-32 | PA4 / DEBUG_5 | Configured push-pull output |
| Debug output 6 | CN10-26 | PB15 / DEBUG_6 | Configured push-pull output |
| External clock input | MCU pin PH0 / OSC_IN | PH0 | Optional HSE bypass clock input used for USB's 48 MHz clock; system clock remains on HSI |

PA13/PA14 are reserved for ST-LINK SWD programming/debugging; PB3 is configured as the SWO trace pin. TIM13 is used internally and has no external pin assignment in this firmware. The Nucleo's LD2 is on PA5, which the project also configures as SPI1_SCK.

## Pin allocation added for the ESC mirrors

PB10 (TIM2_CH3, CN10-25) and PA8 (TIM1_CH1, CN10-23) were not allocated in the existing `.ioc` configuration. Their timer inputs are 16 MHz; both timers use prescaler 15 and period 19999 to match the existing 50 Hz PWM outputs. The F446RE package does not bond PB11; on this board CN10-18 is not a usable PB11 signal, so it is deliberately left alone. These mirror assignments avoid reusing pins already assigned to CAN, SBUS/UART, SPI, servos, or debug GPIO.

Board connector mapping is from ST's [NUCLEO-F446RE datasheet](https://www.st.com/resource/en/datasheet/stm32f446re.pdf) and [Nucleo-64 user manual UM1724](https://www.st.com/resource/en/user_manual/um1724-.pdf). Check the silkscreen and board revision before wiring.
