/*
 * can_bus_tx_tasks.c
 *
 *  Created on: Jan 15, 2022
 *      Author: abiel
 */

#include "CAN/can_bus_tx_tasks.h"
#include "cmsis_os.h"
#include "SBUS/sbus.h"
#include "CAN/can.h"
#include "Sensors/battery_monitor.h"
#include <stdio.h>

extern CAN_HandleTypeDef hcan2;

static const osThreadAttr_t sbusTask_attributes = {
  .name = "sbusTxCan",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t debugMsgTask_attributes = {
  .name = "debugMsgTx",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t pingTask_attributes = {
  .name = "pingMsgTx",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t batteryTask_attributes = {
  .name = "batteryMonitorTx",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t debugVarsTask_attributes = {
	.name = "debugVars",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityNormal,
};

static const osThreadAttr_t canReset_attributes = {
	.name = "canReset",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityNormal,
};

static void can_reset_bus_off(void *param){
	for(;;){
		SET_BIT(hcan2.Instance->MCR, CAN_MCR_ABOM);
		osDelay(100);
	}
}

extern SBUS_Data sbusData;
static void can_sbus_tx_task(void * param){
	for(;;){
		SBUS_Snapshot sbusSnapshot;
		if (!SBUS_GetSnapshot(&sbusData, &sbusSnapshot)) {
			osDelay(SBUS_DELAY);
			continue;
		}

		// Broadcast SBUS data in chunks of 4 channels (8 bytes)
		// Frame 1: Ch 0-3 (0x103)
		queue_can_msg(0x103, 8, (uint8_t*)&sbusSnapshot.channels[0]);
		// Frame 2: Ch 4-7 (0x104)
		queue_can_msg(0x104, 8, (uint8_t*)&sbusSnapshot.channels[4]);
		// Frame 3: Ch 8-11 (0x105)
		queue_can_msg(0x105, 8, (uint8_t*)&sbusSnapshot.channels[8]);
		// Frame 4: Ch 12-15 (0x106)
		queue_can_msg(0x106, 8, (uint8_t*)&sbusSnapshot.channels[12]);
		
		// Frame 5: Failsafe (0x107)
		uint8_t failsafe_payload = sbusSnapshot.failsafe ? 1U : 0U;
		queue_can_msg(0x107, 1, &failsafe_payload);

		osDelay(SBUS_DELAY);
	}
}

static void can_debug_msg_tx_task(void *param){
	for(;;){
		while(handle_debug_msg_queue() == osOK);
		osDelay(DEBUG_MSG_DELAY);
	}
}

static void can_send_ping_task(void * param){
	for(;;){
		// Broadcast standard ping frame on ID 0x112
		uint8_t ping_payload[1] = {0};
		queue_can_msg(0x112, 1, ping_payload);
		osDelay(1000);
	}
}

#include "ver_data.h"
char debug_buffer[128];
char runtime_stats_buffer[1024];
static void debug_vars_task(void *param){
	for(;;){
		int len = snprintf(debug_buffer, sizeof(debug_buffer), "[DEBUG] Version: %s\n", GIT_VERSION);
		send_can_debug_msg(debug_buffer, len);
		len = snprintf(debug_buffer, sizeof(debug_buffer), "[DEBUG] Date: %s\n", COMPILE_DATE);
		send_can_debug_msg(debug_buffer, len);
		len = snprintf(debug_buffer, sizeof(debug_buffer), "[DEBUG] User: %s\n", COMPILE_USER);
		send_can_debug_msg(debug_buffer, len);
		osDelay(1000 * 60); //send every minute
	}
}

static void can_battery_monitor_task(void * param){
	battery_monitor_init();
	for(;;){
		float voltage = battery_monitor_poll();
		// Sending local battery monitor voltage as 0x108
		queue_can_msg(0x108, 4, (uint8_t*)&voltage);
		osDelay(1000);
	}
}

void start_can_tx_tasks(){
	// Note: Heartbeat and ping removed (Status heartbeat is now driven from boat_main.c at 10Hz)
	osThreadNew(can_sbus_tx_task, NULL, &sbusTask_attributes);
	osThreadNew(can_debug_msg_tx_task, NULL, &debugMsgTask_attributes);
    // Ping task restored with standard CAN ID 0x112
    osThreadNew(can_send_ping_task, NULL, &pingTask_attributes);
	osThreadNew(can_battery_monitor_task, NULL, &batteryTask_attributes);
	osThreadNew(debug_vars_task, NULL, &debugVarsTask_attributes);
	osThreadNew(can_reset_bus_off, NULL, &canReset_attributes);
}
