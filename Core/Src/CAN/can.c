/*
 * can.c
 */

#include "CAN/can.h"
#include "stm32f4xx_hal.h"
#include <string.h>

extern CAN_HandleTypeDef hcan2;

osStatus_t handle_debug_msg_queue(){
	uint8_t msg_buf[8];
	uint32_t txMailbox;

	osStatus_t ret = osMessageQueueGet(debugMessageQueue, msg_buf, NULL, 0);
	if(ret != osOK) return ret;

	CAN_TxHeaderTypeDef txHeader;
	txHeader.IDE = CAN_ID_STD;
	txHeader.StdId = CAN_ID_MAIN_DEBUG;
	txHeader.RTR = CAN_RTR_DATA;
	txHeader.DLC = msg_buf[0];

	HAL_CAN_AddTxMessage(&hcan2, &txHeader, msg_buf + 1, &txMailbox);
	return ret;
}

static char debug_buf[7];
static uint8_t debug_buf_len = 0;
void send_can_debug_char(char c){
	debug_buf[debug_buf_len] = c;
	debug_buf_len++;
	if(debug_buf_len >= sizeof(debug_buf) || c == '\n'){
		send_can_debug_msg(debug_buf, debug_buf_len);
		debug_buf_len = 0;
	}
}

void send_can_debug_msg(const char *str, uint32_t len){
	uint8_t buf[8];
	int i;

	for(i = 0; i < len - len % 7; i += 7){
		memcpy(buf + 1, str + i, 7);
		buf[0] = 7;
		osMessageQueuePut(debugMessageQueue, buf, 1, 10);
	}

	memcpy(buf + 1, str + i, len % 7);
	buf[0] = len % 7;
	osMessageQueuePut(debugMessageQueue, buf, 1, 10);
}

void queue_can_msg(uint32_t stdId, uint8_t dlc, const uint8_t* payload){
	CAN_TX_QUEUE_OBJ out;
    memset(&out, 0, sizeof(out));
	out.header.StdId = stdId;
	out.header.IDE = CAN_ID_STD;
	out.header.RTR = CAN_RTR_DATA;
	out.header.DLC = dlc;
	out.header.TransmitGlobalTime = DISABLE;
	
	if(payload != NULL && dlc > 0) {
		memcpy(out.payload, payload, dlc);
	}
	
	osMessageQueuePut(txMessageQueue, &out, 0, 10);
}
