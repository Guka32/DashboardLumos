# Telemetry Gateway & CAN Diagnostics — System Summary

## Overview
- Board: STM32 Nucleo F446RE acting as a passive CAN sniffer and telemetry gateway.
- CAN bus: 125 kbps standard CAN (single wire pair, H/L) between vessel nodes.
- Radio link: Holybro telemetry radio bridged to the STM32 via `USART2` (PA2/PA3) at 115200 baud.
- Telemetry format: human-readable NMEA-style ASCII sentences (`$PSTAT`, `$PGPS`) with XOR checksum.

## Functional Flow
1. MCU runs in main loop; it periodically (200 ms) formats and transmits NMEA telemetry over UART.
2. MCU passively sniffs CAN frames in interrupt-driven RX; parsed values are stored in `status`.
3. Gateway emits its own CAN heartbeat (ID `0x010`) at 1 Hz containing a 4-byte uptime (`uint32_t`).
4. On sending the heartbeat the code toggles LD2 (PA5) to provide a visual 1 Hz blink.

## Standard CAN IDs (used in code)
- `0x010` — Telemetry Gateway heartbeat (4-byte uptime, little-endian uint32)
- `0x100` — Main PCB heartbeat (sniffed; recorded as `main_pcb_last_seen`)
- `0x150` — Actuator Control PCB (payload: at least 2 bytes; `pump_active`, `actuator_active`)
- `0x200` — Battery Control PCB (payload: 8 bytes interpreted as two floats: voltage, current)

Note: CAN IDs are standard (11-bit) in this implementation. Filters are set to mask `0` to accept all IDs (passive sniffing).

## UART Telemetry Sentences
- `$PSTAT`: gateway uptime, battery voltage/current, pump/actuator flags, local ADC battery
- `$PGPS`: two simulated GPS fixes (used for diagnostics and Grafana plotting)

Example NMEA generation is implemented in `telemetry.c` via `generate_nmea_sentence()` which appends the XOR checksum and CR/LF.

## Important Implementation Details
- CAN timing: configured for a 16 MHz HSI clock using Prescaler = 8, TimeSeg1 = 13 TQ, TimeSeg2 = 2 TQ, SJW = 1 TQ (125 kbps).
- CAN transmit safety: code checks `HAL_CAN_GetTxMailboxesFreeLevel()` before `HAL_CAN_AddTxMessage()`.
- Interrupts: `HAL_CAN_ActivateNotification(..., CAN_IT_RX_FIFO0_MSG_PENDING)` and `HAL_CAN_RxFifo0MsgPendingCallback()` parse incoming frames.
- LED: `PA5` is configured as push-pull output and toggled on each heartbeat send.

## Common Failure Modes & Quick Checks
- Bitrate mismatch — ensure PC `can0` / CANable is set to 125000 bps.
- Transceiver wiring — MCU `PA11` = CAN RX, `PA12` = CAN TX; ensure TX/RX and GND are connected to CAN adapter.
- SocketCAN state: on PC run:
```
sudo ip link set can0 down
sudo ip link set can0 up type can bitrate 125000
ip -details link show can0
candump can0
```
- If frames are not observed but MCU logs show heartbeat started: try MCU-only loopback mode (`CAN_MODE_LOOPBACK`) to verify HAL-level TX/RX.

## Diagnostics Added
- Startup UART debug prints report CAN start success/failure and per-TX errors (mailbox full or HAL add-TX failure).
- These messages appear on `USART2` at 115200 and help distinguish MCU-side vs bus/transceiver issues.

## Where to look in code
- Telemetry/NMEA: `Core/Src/telemetry.c` and `Core/Inc/telemetry.h`
- Main control loop, CAN init, heartbeat, LED toggle: `Core/Src/main.c`
- CAN HAL MSP (pin + clock setup): `Core/Src/stm32f4xx_hal_msp.c`

---
If you want, I can also update the CubeMX `.ioc` file to reflect the 125 kbps CAN timing so regenerations stay consistent.
