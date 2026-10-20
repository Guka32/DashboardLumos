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

extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart4;
extern SBUS_Data sbusData;

static bool boatArmed = false;

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
	SBUS_Init(&sbusData, &huart5);

	enum BoatState state = BoatState_Disabled;
	enum BoatState aux = 0;
	for(;;){
		SBUS_Update();
		uint32_t sbus_dt = HAL_GetTick() - sbusData.timestamp;
		bool sbus_valid = (sbusData.timestamp != 0U) && (sbus_dt <= 1000U);
		/* Failsafe is implicit: when the receiver flags failsafe, SBUS_Parse withholds
		 * the timestamp update, so sbus_dt exceeds 1000 ms and sbus_valid goes false. */

		// Original CH5 thresholds, applied only to fresh, non-failsafe SBUS data.
		if (!sbus_valid) state = BoatState_Disabled;
		else if (sbusData.channels[4] < 900U) state = BoatState_Disabled;
		else if (sbusData.channels[4] > 900U && sbusData.channels[4] < 1110U)
			state = BoatState_Teleoperated;
		else if (sbusData.channels[4] > 1100U) state = BoatState_Autonomous;
		else state = BoatState_Disabled;

		// update aux
		if(sbusData.channels[5] < 900) aux = 0;
		else if(sbusData.channels[5] > 900 && sbusData.channels[5] < 1110) aux = 1; /* was channels[4] — copy-paste error */
		else if(sbusData.channels[5] > 1100) aux = 2;

		// SBUS freshness and failsafe are included in sbus_valid above.

		//Arm if necessary
		if(!boatArmed && state != BoatState_Disabled){
			boat_arming_sequence();
		}
		char buffer[20];
		snprintf(buffer, 20, "%d, %d\r\n", sbusData.channels[0], sbusData.channels[4]);

		SEGGER_RTT_WriteString(0, buffer);

		//Update based on state
		switch(state){
		case BoatState_Autonomous:
			boat_autonomous_loop();
			break;
		case BoatState_Teleoperated:
			boat_teleoperated_loop();
			break;
		case BoatState_Disabled:
			boat_disabled_loop();
			break;
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

void boat_teleoperated_loop(){
	// Fresh CAN actuator commands can exercise servos and the enabled IBT-2
	// while the SBUS selector remains in manual mode.
	if (boat_actuator_command_fresh()) {
		boat_apply_can_actuators();
	} else {
		boat_actuator_failsafe();
	}

	// Take in motor control from SBUS, and write out to PWM
	float throttle = (sbusData.channels[1] - 1000) / 1500.0;
	float steer = (sbusData.channels[3] - 1000) / 1500.0;

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
	osDelay(1100);
	for(int i = 0; i < 8; i++)
		pwm_set(i, 0.0f);
	osDelay(1100);
	boatArmed = true;
}
