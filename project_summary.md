# F446RE USV Project Summary

Updated: 2026-10-19

## Project identity and scope

- Active firmware project: C:\Users\gutie\STM32CubeIDE\workspace_2.1\F446RE_USV.
- This project was renamed from earlier working copies called FullF103. The original STM32F405 Vanttec Embedded project was used as a reference.
- Keep changes in F446RE_USV. Earlier confusion came from editing or discussing Vanttec_STM_Common, Vanttec_F446RE, FullF103, and the active F446RE_USV copy interchangeably.
- Hardware target: Nucleo-64 STM32F446RE, external CAN transceiver, no PCA9685. Direct timer PWM is used.
- A past CubeIDE rename caused a Run/Debug launch error about a missing program file. If it returns, verify the launch configuration points to Debug/F446RE_USV.elf and that the ELF exists.
- User reported the latest SBUS/LED work was built and run by another agent/user. This handoff author did not build or flash the current image.

## Intended functions and pin map

| Function | Pin/peripheral |
|---|---|
| SBUS input | PD2, UART5_RX, AF8 |
| ESC left | PC6, TIM3_CH1 |
| ESC right | PC7, TIM3_CH2 |
| Turret X servo | PB0, TIM3_CH3 |
| Turret Y servo | PB1, TIM3_CH4 |
| Unstick servo | PB6, TIM4_CH1 |
| IBT-2 PWM legs | PA2, TIM9_CH1; PA3, TIM9_CH2 |
| IBT-2 enable pins | PC0 and PC1 (software-controlled GPIO) |
| Debug mode output | PA15, DEBUG_1 |
| CAN | CAN2 with external transceiver |

PWM API maps normalized -1..+1 to approximately 1100..1900 us; 0 maps to a neutral 1500 us pulse. The three servo outputs and two ESC outputs are 50 Hz. The IBT-2 legs are 20 kHz. The implementation is in Core/Src/PWM/pwm_out.c.

## Clock and peripheral configuration

Core/Src/main.c selects HSI as SYSCLK at 16 MHz. AHB, APB1, and APB2 dividers are all 1, so PCLK1/PCLK2 are 16 MHz.

- UART5 is 100000 baud, 9-bit word length with even parity (8 data bits plus parity), 2 stop bits, RX-only, oversampling by 8. This is SBUS 100 kbaud 8E2.
- CAN2 timing is prescaler 8, BS1 13 TQ, BS2 2 TQ, SJW 1 TQ. Total is 16 TQ; at PCLK1=16 MHz this is 125 kbit/s.
- TIM3/TIM4: PSC=15, ARR=19999; 16 MHz / 16 = 1 MHz timer counter, 20,000 counts => 50 Hz.
- TIM9: PSC=0, ARR=799 => 20 kHz.
- TIM14 is the HAL 1 ms timebase. FreeRTOS tick is configured to 1 kHz. TIM13 is a runtime-statistics counter, not the SBUS receiver timer.

SystemClock_Config first selects HSI, then separately attempts an 8 MHz HSE bypass + PLL (M=8, N=192, P=2, Q=4) for USB's 48 MHz clock. It does not switch SYSCLK to PLL. If HSE/PLL setup fails, USB initialization is skipped; UART5, CAN, and PWM remain on the HSI-derived clocks. HSE bypass matches an external clock fed to PH0, if that clock and the board solder-bridge configuration are present. The prior CubeMX HSE warning is therefore not by itself an explanation for SBUS/CAN/PWM behavior.

## SBUS wiring and receive path

- The user reported returning to the original SN74LVC1G240DBVT inverter on the PCB, with common ground and a clean approximately 3.3-3.6 V inverted signal at the MCU side. Earlier experiments used SN74HC00N/CD4093 NAND gates and HD74LS04P; those were replaced for the reported current setup.
- The configured receive pin is PD2/UART5_RX. PC5 was considered temporarily, but PD2 is the current configured UART pin.
- The receiver is an FrSky X8R driven by a Taranis transmitter. The user says channels 1-4 are stick inputs and channel 5 selects Stop/Manual/Auto. In C arrays, channels are zero-indexed: CH5 is channels[4].
- SBUS_Update polls UART5 with HAL_UART_Receive; there is no SBUS DMA. It hunts for header 0x0F after footer 0x00, reads the rest of the 25-byte frame, parses it, and preserves the footer for next-frame synchronization.
- Structurally valid frames require header 0x0F and footer 0x00. Flags are at byte 23; expected footer is byte 24.

### Recent SBUS code changes and caveats

- Timestamp assignment was changed to HAL_GetTick(), matching the freshness calculation in boat_main.c. xTaskGetTickCount() is a valid RTOS clock, but comparing it to HAL_GetTick() mixes clocks with different origins.
- The latest SBUS_Parse in F446RE_USV refreshes timestamp for every structurally valid frame. It currently does NOT assign data->failsafe from flags byte 23.
- SBUS_Init sets sbusData.failsafe=true. CAN ID 0x107 transmits this field, so with the current parser it will remain at its startup value (normally 1). A 0x107 payload of 01 is not the boat mode.
- boat_main.c now defines sbus_valid only as timestamp != 0 and age <= 1000 ms. It no longer gates state on the failsafe bit. Thus radio loss is recognized only if structurally valid frames stop for more than 1000 ms. If the X8R continues sending failsafe frames, this code can still treat SBUS as valid. Decide and implement the intended failsafe policy before operational use.
- Earlier debugger capture had msgBuffer[23]=0x7C. Bit 3 of 0x7C is set. The user also reported that mode/LED behavior improved after another agent changed parser/failsafe handling. Verify the raw flags byte and receiver behavior; do not assume the old capture was aligned correctly.
- Current parser expressions parenthesize/mask the full 11-bit channel expression. A code comment says the old precedence corrupted values. The old expressions applied a mask to the final shifted term, but the other terms appear naturally constrained to their non-overlapping bit ranges; independently validate the current parse against a known SBUS frame or the receiver monitor rather than assuming the parentheses alone fixed the channel readings.

## Mode selection, state codes, and LED

The enum in Core/Inc/Boat/boat_main.h is:
- BoatState_Autonomous = 0
- BoatState_Teleoperated (Manual) = 1
- BoatState_Disabled (Stop) = 2

Core/Src/Boat/boat_main.c selects state from fresh CH5 data:
- CH5 < 900: Disabled (2)
- CH5 > 900 and < 1110: Manual (1)
- CH5 > 1100: Autonomous (0)
- Otherwise: Disabled (2)

Because Manual is tested first, 901-1109 selects Manual; exactly 900 is Disabled and 1110 or higher selects Auto. The conditions are asymmetric around the original endpoints.

The user previously measured CH5 (channels[4]) approximately as Stop=1147, Manual=123, Auto=891. Under current thresholds those would select Auto, Disabled, Disabled. Temporary thresholds around those measured values were tried and then reverted at the user's request. The values may have come from earlier parsing/debug conditions; recapture the current CH5 raw value for every switch position before changing thresholds.

The main loop distinguishes modes by calling separate functions:
- Manual calls boat_teleoperated_loop(), which uses SBUS channels[1] and channels[3] for propulsion. Fresh CAN actuator commands control the other actuators.
- Auto calls boat_autonomous_loop(), which requires a valid Jetson heartbeat no older than 500 ms; absent/stale heartbeat stops outputs. Auto can be selected without Jetson, but propulsion will remain stopped.
- Disabled calls boat_disabled_loop() and sets the standard PWM channels to neutral (normalized 0 => 1500 us).

PA15 DEBUG_1 was changed to: set high when SBUS is invalid or mode is Disabled; low in Manual; toggle every 250 ms in Auto. Physical LED polarity/wiring may invert visible behavior. The original F405 code only used its GPIO to indicate Disabled versus any active mode; separate Manual/Auto indication is a newer behavior.

## CAN protocol and interpretation of observed dump

CAN ID 0x010 heartbeat payload is constructed in boat_main.c:
- bytes 0-3: HAL_GetTick(), little-endian
- byte 4: enum state (Auto=00, Manual=01, Disabled=02)
- byte 5: aux selector (0, 1, or 2 from channels[5])
- byte 6: boatArmed flag (0 or 1)
- byte 7: zero

Therefore the recent observation of 0x010 byte 4 changing 00, 02, 00 shows Auto and Disabled states; it does not show Manual because 01 is absent. The 01 in byte 6 is the armed flag, not Manual. boatArmed is set true after the arming sequence and is not cleared on Stop, so Disabled state with byte 6=01 is possible.

Other IDs:
- 0x101/0x102 report PWM compare values for channels 0-3 and 4-7. Repeated DC 05 values are little-endian 1500 counts/us, the neutral pulse.
- 0x103-0x106 broadcast raw SBUS channel values as little-endian uint16 words, four channels per frame.
- 0x107 is intended to broadcast SBUS failsafe as one byte, but is stale/incorrect with the current parser as described above.
- 0x108 is a 4-byte voltage value; 0x112 is a one-byte ping/status.

For the supplied dump:
- 0x010 frames had payload bytes 4 equal to 00, 00, 02, 02, then 00. There was no Manual state 01.
- The 0x104 data was 7B 04 15 00 7C 00 7C 00. As little-endian uint16 values this is CH5=0x047B=1147, then 21, 124, 124. CH5=1147 maps to Auto (00) under the current thresholds, consistent with nearby 00 heartbeat if the samples are close in time.
- 0x107 [1] 01 means the current failsafe field is 1; with current code this likely reflects its init value, not a live decoded flag.
- 0x101/0x102 DC 05 repeats are neutral PWM outputs.

The original F405 project had the same enum values, threshold-based state selection, and separate Manual/Auto control loops. The original heartbeat also put state in byte 4. Its status LED was not a Manual-vs-Auto indicator.

## Other known behavior and issues

- CAN startup previously appeared to wait for CAN ready/INAK and required a connected bus during some tests. Later CAN dumps demonstrate traffic was working. If startup hangs again, inspect the CAN clock/bit timing, CAN2's CAN1 clock dependency, transceiver power/standby, wiring, termination, and bus state.
- Initial user CAN test requests involved a transceiver, one CAN at 125 kbit/s, and no CRC because only small control/status payloads are sent. CAN's own frame CRC remains part of the CAN protocol.
- The code currently sends no CRC field from the application; standard CAN hardware generates/checks its link-layer CRC.
- One aux condition was a copy-paste bug in the original/current project history: it checked channels[4] while evaluating channels[5]. Current F446RE code was corrected to check channels[5] for both comparisons.
- No Jetson connection means Auto propulsion is expected to fail safe; it does not explain lack of Manual SBUS propulsion.

## Immediate handoff actions

1. Build and flash the exact F446RE_USV project; verify the .elf timestamp/path matches the source being inspected.
2. Capture a full CAN line while deliberately selecting each switch position. Interpret 0x010 byte 4 specifically. Expected code values are Stop=02, Manual=01, Auto=00.
3. At the same time observe live channels[4] in debugger/RTT and compare it with 0x104's first uint16. Reconcile the transmitter position labels with raw CH5 and the threshold ranges.
4. Confirm a Manual test produces state 01. If state stays 02, CH5 is outside the Manual range or SBUS is stale. If state becomes 00 in physical Stop, thresholds/mapping are wrong or the image differs from source.
5. Fix the failsafe member update and safe mode policy. Do not leave CAN 0x107 reporting a stale init value or silently ignore receiver failsafe without a deliberate safety decision.
6. Verify live UART frame header/footer/flags at PD2 and check RC-loss behavior.
7. Measure ESC/servo PWM frequency and pulse width and verify PA15's electrical level.
8. Check HSE only if USB is needed; the HSI 16 MHz path drives current SBUS, CAN, and PWM setup.
