/*
 * boat_main.c
 *
 *  Created on: Jan 29, 2022
 *      Author: abiel
 */

#include "Boat/boat_main.h"
#include "SBUS/sbus.h"
#include "PWM/pwm_out.h"
#include "CAN/can_bus.h"
#include "CAN/can.h"
#include "SEGGER_RTT.h"
#include "main.h"
#include <stdbool.h>
#include <stdio.h>
#include "telemetry.h"

extern UART_HandleTypeDef huart4;
extern SBUS_Data sbusData;

static bool boatArmed = false;
static bool boatArmingInProgress = false;
static uint8_t boatArmingPhase = 0U;
static uint32_t boatArmingPhaseTick = 0U;
static bool sbusRearmRequired = true;

static bool boat_arming_update(void);
static void boat_arming_cancel(void);

static const osThreadAttr_t boatTask_attributes = {
  .name = "boatTask",
  .stack_size = 128 * 8,
  .priority = (osPriority_t) osPriorityNormal,
};


void createTasks_boat() {
	osThreadNew(mainTask_boat, NULL, &boatTask_attributes);
}

uint8_t op_mode_i = 0;

static void boat_actuator_failsafe(void){
	pwm_set_ibt2_motor(0.0f);
	pwm_set_ball_enable(0);
	pwm_set(PWM_CH_SERVO_TURRET_X, 0.0f);
	pwm_set(PWM_CH_SERVO_TURRET_Y, 0.0f);
	pwm_set(PWM_CH_SERVO_UNSTICK, 0.0f);
}

static bool boat_actuator_command_fresh(void){
	return can_rx_data.actuatorCommandValid &&
		(HAL_GetTick() - can_rx_data.actuatorCmdTick <= 500);
}

static void boat_apply_can_actuators(void){
	pwm_set(PWM_CH_SERVO_TURRET_X, can_rx_data.turret_x_angle);
	pwm_set(PWM_CH_SERVO_TURRET_Y, can_rx_data.turret_y_angle);
	pwm_set(PWM_CH_SERVO_UNSTICK, can_rx_data.unstick_servo_pos);

	if (can_rx_data.ball_motor_enable) {
		pwm_set_ibt2_motor(can_rx_data.ball_motor_speed);
		pwm_set_ball_enable(1);
	} else {
		pwm_set_ibt2_motor(0.0f);
		pwm_set_ball_enable(0);
	}
}

static void boat_update_mode_led(enum BoatState state, bool sbus_valid){
	// PA15: solid on with no valid SBUS or in stop, off in manual, blinking in auto.
	if (!sbus_valid || state == BoatState_Disabled) {
		HAL_GPIO_WritePin(DEBUG_1_GPIO_Port, DEBUG_1_Pin, GPIO_PIN_SET);
	} else if (state == BoatState_Teleoperated) {
		HAL_GPIO_WritePin(DEBUG_1_GPIO_Port, DEBUG_1_Pin, GPIO_PIN_RESET);
	} else if (state == BoatState_Autonomous) {
		HAL_GPIO_WritePin(DEBUG_1_GPIO_Port, DEBUG_1_Pin,
			((HAL_GetTick() / 250U) & 1U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(DEBUG_1_GPIO_Port, DEBUG_1_Pin, GPIO_PIN_SET);
	}
}

/*
 * Handle mode switching based on sbus data
 */
void mainTask_boat(void * params) {
	pwm_init();

	enum BoatState state = BoatState_Disabled;
	uint8_t aux = 0U;
	for(;;){
		SBUS_Update();
		SBUS_Snapshot sbusSnapshot = {0};
		bool snapshot_ok = SBUS_GetSnapshot(&sbusData, &sbusSnapshot);
		uint32_t now = HAL_GetTick();
		bool sbus_valid = snapshot_ok && SBUS_IsControlValid(&sbusSnapshot, now);

		// Restore the prior CH5 thresholds. Stop is below 900, Manual is
		// 900-1109, and Auto is above 1100 (Manual takes precedence in overlap).
		uint16_t mode_ch5 = sbusSnapshot.controlChannels[4];
		bool stop_selected = (mode_ch5 < 900U);
		bool mode_known = false;
		enum BoatState requested_state = BoatState_Disabled;
		if (mode_ch5 > 900U && mode_ch5 < 1110U) {
			requested_state = BoatState_Teleoperated;
			mode_known = true;
		} else if (mode_ch5 > 1100U) {
			requested_state = BoatState_Autonomous;
			mode_known = true;
		} else if (stop_selected) {
			mode_known = true;
		}

		if (!sbus_valid || !mode_known) {
			/* Any receiver failsafe, stale input, or unknown selector value
			 * latches the boat in Disabled until a healthy Stop selection is seen.
			 */
			sbusRearmRequired = true;
			state = BoatState_Disabled;
			boatArmed = false;
			boat_arming_cancel();
		} else if (sbusRearmRequired) {
			state = BoatState_Disabled;
			boatArmed = false;
			boat_arming_cancel();
			if (stop_selected) sbusRearmRequired = false;
		} else {
			state = requested_state;
		}

		// update aux
		if (sbus_valid && !sbusRearmRequired) {
			if (sbusSnapshot.controlChannels[5] < 900U) aux = 0U;
			else if (sbusSnapshot.controlChannels[5] > 900U && sbusSnapshot.controlChannels[5] < 1110U) aux = 1U;
			else if (sbusSnapshot.controlChannels[5] > 1100U) aux = 2U;
		} else {
			aux = 0U;
		}

		// Begin/advance arming without blocking the task; SBUS bytes continue
		// entering the RX ring while neutral pulses are held.
		if (state != BoatState_Disabled && !boatArmed) {
			if (!boatArmingInProgress) {
				boat_arming_sequence();
			}
			boat_arming_update();
		} else if (state == BoatState_Disabled) {
			boatArmed = false;
			boat_arming_cancel();
		}
		char buffer[20];
		snprintf(buffer, 20, "%u, %u\r\n",
			(unsigned int)sbusSnapshot.channels[0],
			(unsigned int)sbusSnapshot.channels[4]);

		SEGGER_RTT_WriteString(0, buffer);

		/* Do not run a control loop until the nonblocking arming interval ends. */
		if (state == BoatState_Disabled || !boatArmed) {
			boat_disabled_loop();
		} else {
			switch(state){
			case BoatState_Autonomous:
				boat_autonomous_loop();
				break;
			case BoatState_Teleoperated:
				boat_teleoperated_loop(sbusSnapshot.controlChannels);
				break;
			case BoatState_Disabled:
			default:
				boat_disabled_loop();
				break;
			}
		}

		boat_update_mode_led(state, sbus_valid);

		// Broadcast telemetry over CAN at 10Hz (every 100ms since loop is 10ms)
		if(op_mode_i > 10){
			// Gateway Heartbeat on CAN_ID_GATEWAY_HB (0x010)
			uint32_t tick = HAL_GetTick();
			uint8_t hb_payload[8] = {0};
			hb_payload[0] = (uint8_t)(tick & 0xFF);
			hb_payload[1] = (uint8_t)((tick >> 8) & 0xFF);
			hb_payload[2] = (uint8_t)((tick >> 16) & 0xFF);
			hb_payload[3] = (uint8_t)((tick >> 24) & 0xFF);
			hb_payload[4] = (uint8_t)state;
			hb_payload[5] = (uint8_t)aux;
			hb_payload[6] = boatArmed ? 1 : 0;
			queue_can_msg(CAN_ID_GATEWAY_HB, 8, hb_payload);

			uint16_t pwm_0_3[4] = {pwm_get_raw(0), pwm_get_raw(1), pwm_get_raw(2), pwm_get_raw(3)};
			queue_can_msg(CAN_ID_MAIN_PWM_0_3, 8, (uint8_t*)pwm_0_3);
            
			uint16_t pwm_4_7[4] = {pwm_get_raw(4), pwm_get_raw(5), pwm_get_raw(6), pwm_get_raw(7)};
			queue_can_msg(CAN_ID_MAIN_PWM_4_7, 8, (uint8_t*)pwm_4_7);
            
			op_mode_i = 0;
		}
		op_mode_i++;

		// Send CSV Telemetry string over UART4 at 5Hz (every 200ms)
		static uint32_t last_telemetry_tick = 0;
		if (HAL_GetTick() - last_telemetry_tick >= 200) {
			last_telemetry_tick = HAL_GetTick();
			status.gateway_uptime = last_telemetry_tick;
			
			char tx_buf[128];
			int len = generate_telemetry_string(tx_buf, sizeof(tx_buf));
			if (len > 0) {
				HAL_UART_Transmit(&huart4, (uint8_t*)tx_buf, len, 100);
			}
		}

		//TODO handle status lights
		osDelay(10);
	}
}

void boat_autonomous_loop(){

	// Take in motor setpoints from CAN and write them out to PWM
	uint32_t dt = HAL_GetTick() - can_rx_data.jetsonHBTick;
	if(!can_rx_data.jetsonHeartbeatValid || dt > 500) {
		boat_actuator_failsafe();
		for(int i = 0; i < 8; i++)
			pwm_set(i, 0);
		return;
	}

	// Two propulsion ESC outputs (CAN motor setpoints 0 and 1).
	pwm_set(PWM_CH_ESC_LEFT, can_rx_data.motorSetpoints[0]);
	pwm_set(PWM_CH_ESC_RIGHT, can_rx_data.motorSetpoints[1]);

	// Servo and IBT-2 commands arrive in CAN_ID_ACTUATOR (0x150).
	if (boat_actuator_command_fresh()) {
		boat_apply_can_actuators();
	} else {
		boat_actuator_failsafe();
	}
}

void boat_teleoperated_loop(const uint16_t channels[16]){
	// Fresh CAN actuator commands can exercise servos and the enabled IBT-2
	// while the SBUS selector remains in manual mode.
	if (boat_actuator_command_fresh()) {
		boat_apply_can_actuators();
	} else {
		boat_actuator_failsafe();
	}

	// Right stick: SBUS CH2 is vertical throttle; CH3 is horizontal steering.
	float throttle = (channels[1] - 1000) / 1500.0;
	float steer = (channels[2] - 1000) / 1500.0;

	float leftMotor, rightMotor;
	leftMotor = -throttle + steer;
	rightMotor = -throttle - steer;

	pwm_set(PWM_CH_ESC_LEFT, leftMotor);
	pwm_set(PWM_CH_ESC_RIGHT, -rightMotor);
}

void boat_disabled_loop(){
	boat_actuator_failsafe();

	// Stop propulsion outputs and zero out PWM channels.
	for(int i = 0; i < 8; i++)
		pwm_set(i, 0);
	return;
}

void boat_arming_sequence(){
	pwm_set_ball_enable(0);
	pwm_set_ibt2_motor(0.0f);
	for(int i = 0; i < 8; i++)
		pwm_set(i, 0.0f); // Keep 1.5 ms neutral pulses during arming.
	boatArmingInProgress = true;
	boatArmingPhase = 1U;
	boatArmingPhaseTick = HAL_GetTick();
}

static bool boat_arming_update(void){
	if (!boatArmingInProgress) return boatArmed;
	if ((HAL_GetTick() - boatArmingPhaseTick) < 1100U) return false;

	if (boatArmingPhase == 1U) {
		for (int i = 0; i < 8; i++) pwm_set(i, 0.0f);
		boatArmingPhase = 2U;
		boatArmingPhaseTick = HAL_GetTick();
		return false;
	}

	boatArmingInProgress = false;
	boatArmingPhase = 0U;
	boatArmed = true;
	return true;
}

static void boat_arming_cancel(void){
	boatArmingInProgress = false;
	boatArmingPhase = 0U;
}
