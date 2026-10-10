/*
 * sbus.c
 *
 *  Created on: Jan 6, 2022
 *      Author: abiel
 */

#include "SBUS/sbus.h"
#include <string.h>

static int min(int a, int b){
	return (a > b) ? b : a;
}

extern UART_HandleTypeDef huart5;
extern SBUS_Data sbusData;


HAL_StatusTypeDef SBUS_Init(SBUS_Data *data, UART_HandleTypeDef *huart){
	if(data == NULL || huart == NULL) return HAL_ERROR;

	//set buffers to 0 (mostly for ease of debugging)
	memset(data->channels, 0, sizeof(data->channels));
	memset(data->msgBuffer, 0, sizeof(data->msgBuffer));
	data->timestamp = 0;
	data->msgLen = 0;

	data->huart = huart;
	return HAL_OK;
}

static uint8_t prevByte = 0xff;

void SBUS_Update(){
	uint8_t headerByte;
	HAL_StatusTypeDef ret;
	int32_t lock = osKernelLock();
	do {
		ret = HAL_UART_Receive(&huart5, &headerByte, 1, 0);
		//if(ret != HAL_OK){
		//	HAL_UART_AbortReceive(&huart5);
		//	return;
		//}

		if(headerByte == SBUS_HEADER && prevByte == SBUS_FOOTER){
			//Start recv rest of data
			memset(sbusData.msgBuffer, 0xFF, sizeof(sbusData.msgBuffer));
			sbusData.msgBuffer[0] = headerByte;
			if(HAL_UART_Receive(&huart5, sbusData.msgBuffer, SBUS_MSG_LEN, 50) == HAL_OK){
				SBUS_Parse(&sbusData);
			}
		} else {
			prevByte = headerByte;
		}
	} while(ret == HAL_OK);

	osKernelRestoreLock(lock);
}


void SBUS_Parse(SBUS_Data *data){

	if(data->msgBuffer[0] != SBUS_HEADER || data->msgBuffer[SBUS_MSG_LEN - 1] != SBUS_FOOTER){
		//Data invalid!
		return;
	}

	uint16_t *ch_ = data->channels;
	uint8_t *buf_ = data->msgBuffer;

	//Parse message
	ch_[0]  = buf_[1]       | buf_[2]  << 8 & 0x07FF;
	ch_[1]  = buf_[2]  >> 3 | buf_[3]  << 5 & 0x07FF;
	ch_[2]  = buf_[3]  >> 6 | buf_[4]  << 2  |
								   buf_[5] << 10 & 0x07FF;
	ch_[3]  = buf_[5]  >> 1 | buf_[6]  << 7 & 0x07FF;
	ch_[4]  = buf_[6]  >> 4 | buf_[7]  << 4 & 0x07FF;
	ch_[5]  = buf_[7]  >> 7 | buf_[8]  << 1  |
								   buf_[9] << 9 & 0x07FF;
	ch_[6]  = buf_[9]  >> 2 | buf_[10] << 6 & 0x07FF;
	ch_[7]  = buf_[10] >> 5 | buf_[11] << 3 & 0x07FF;
	ch_[8]  = buf_[12]      | buf_[13] << 8 & 0x07FF;
	ch_[9]  = buf_[13] >> 3 | buf_[14] << 5 & 0x07FF;
	ch_[10] = buf_[14] >> 6 | buf_[15] << 2  |
								   buf_[16] << 10 & 0x07FF;
	ch_[11] = buf_[16] >> 1 | buf_[17] << 7 & 0x07FF;
	ch_[12] = buf_[17] >> 4 | buf_[18] << 4 & 0x07FF;
	ch_[13] = buf_[18] >> 7 | buf_[19] << 1  |
								   buf_[20] << 9 & 0x07FF;
	ch_[14] = buf_[20] >> 2 | buf_[21] << 6 & 0x07FF;
	ch_[15] = buf_[21] >> 5 | buf_[22] << 3 & 0x07FF;

	bool failsafe = buf_[SBUS_FLAGS_BYTE] & (1 << SBUS_FAILSAFE_BIT);

	if(!failsafe)
		data->timestamp = xTaskGetTickCount();
}
