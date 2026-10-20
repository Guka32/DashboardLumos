/*
 * sbus.c
 *
 *  Created on: Jan 6, 2022
 *      Author: abiel
 */

#include "SBUS/sbus.h"
#include <string.h>

extern SBUS_Data sbusData;
static uint8_t prevByte = 0xff;


HAL_StatusTypeDef SBUS_Init(SBUS_Data *data, UART_HandleTypeDef *huart){
	if(data == NULL || huart == NULL) return HAL_ERROR;

	//set buffers to 0 (mostly for ease of debugging)
	memset(data->channels, 0, sizeof(data->channels));
	memset(data->msgBuffer, 0, sizeof(data->msgBuffer));
	data->timestamp = 0;
	data->msgLen = 0;

		data->huart = huart;
	data->failsafe = true;
	prevByte = 0xff;
	return HAL_OK;
}

void SBUS_Update(void){
	uint8_t headerByte = 0U;
	HAL_StatusTypeDef ret;
	if(sbusData.huart == NULL) return;

	int32_t lock = osKernelLock();
	do {
		ret = HAL_UART_Receive(sbusData.huart, &headerByte, 1, 0);
		if(ret != HAL_OK) break;

		if(headerByte == SBUS_HEADER && prevByte == SBUS_FOOTER){
			sbusData.msgBuffer[0] = headerByte;
			ret = HAL_UART_Receive(sbusData.huart, &sbusData.msgBuffer[1], SBUS_MSG_LEN - 1U, 50);
			if(ret == HAL_OK){
				SBUS_Parse(&sbusData);
				// The last byte consumed is the SBUS footer; preserve it to sync the next frame.
				prevByte = sbusData.msgBuffer[SBUS_MSG_LEN - 1U];
			} else {
				/* A partial frame was consumed; resynchronize at the next footer. */
				prevByte = 0xff;
			}
		} else {
			prevByte = headerByte;
		}
	} while(ret == HAL_OK);

	osKernelRestoreLock(lock);
}



void SBUS_Parse(SBUS_Data *data){
	if(data == NULL) return;

	if(data->msgBuffer[0] != SBUS_HEADER || data->msgBuffer[SBUS_MSG_LEN - 1] != SBUS_FOOTER){
		//Data invalid!
		return;
	}

	uint16_t *ch_ = data->channels;
	uint8_t *buf_ = data->msgBuffer;

	//Parse message
	/* Each channel spans 11 bits packed across adjacent bytes.
	 * Parenthesise the full OR before masking: without the outer parens
	 * the mask applies only to the last shift term and zeros most of the value. */
	ch_[0]  = ((uint16_t)buf_[1]        | (uint16_t)buf_[2]  << 8)  & 0x07FFU;
	ch_[1]  = ((uint16_t)buf_[2]  >> 3  | (uint16_t)buf_[3]  << 5)  & 0x07FFU;
	ch_[2]  = ((uint16_t)buf_[3]  >> 6  | (uint16_t)buf_[4]  << 2
	                                     | (uint16_t)buf_[5]  << 10) & 0x07FFU;
	ch_[3]  = ((uint16_t)buf_[5]  >> 1  | (uint16_t)buf_[6]  << 7)  & 0x07FFU;
	ch_[4]  = ((uint16_t)buf_[6]  >> 4  | (uint16_t)buf_[7]  << 4)  & 0x07FFU;
	ch_[5]  = ((uint16_t)buf_[7]  >> 7  | (uint16_t)buf_[8]  << 1
	                                     | (uint16_t)buf_[9]  << 9)  & 0x07FFU;
	ch_[6]  = ((uint16_t)buf_[9]  >> 2  | (uint16_t)buf_[10] << 6)  & 0x07FFU;
	ch_[7]  = ((uint16_t)buf_[10] >> 5  | (uint16_t)buf_[11] << 3)  & 0x07FFU;
	ch_[8]  = ((uint16_t)buf_[12]       | (uint16_t)buf_[13] << 8)  & 0x07FFU;
	ch_[9]  = ((uint16_t)buf_[13] >> 3  | (uint16_t)buf_[14] << 5)  & 0x07FFU;
	ch_[10] = ((uint16_t)buf_[14] >> 6  | (uint16_t)buf_[15] << 2
	                                     | (uint16_t)buf_[16] << 10) & 0x07FFU;
	ch_[11] = ((uint16_t)buf_[16] >> 1  | (uint16_t)buf_[17] << 7)  & 0x07FFU;
	ch_[12] = ((uint16_t)buf_[17] >> 4  | (uint16_t)buf_[18] << 4)  & 0x07FFU;
	ch_[13] = ((uint16_t)buf_[18] >> 7  | (uint16_t)buf_[19] << 1
	                                     | (uint16_t)buf_[20] << 9)  & 0x07FFU;
	ch_[14] = ((uint16_t)buf_[20] >> 2  | (uint16_t)buf_[21] << 6)  & 0x07FFU;
	ch_[15] = ((uint16_t)buf_[21] >> 5  | (uint16_t)buf_[22] << 3)  & 0x07FFU;

	/* Always refresh the timestamp when a structurally valid frame is received
	 * (header + footer already checked above). The X8R can report the failsafe
	 * bit (bit 3 of byte 23) even while the RF link is active and channels are
	 * responding — gating the timestamp on that bit leaves sbus_valid permanently
	 * false. Radio loss is detected by frames stopping entirely: once no frame
	 * arrives for >1000 ms, sbus_dt exceeds the threshold in boat_main and
	 * sbus_valid goes false. */
	data->timestamp = HAL_GetTick();
}
