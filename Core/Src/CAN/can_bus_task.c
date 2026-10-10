/*
 * can_bus_task.c
 */

#include <string.h>
#include "CAN/can_bus.h"
#include "stm32f4xx_hal.h"
#include "main.h"
#include "CAN/can_bus_task.h"
#include "SBUS/sbus.h"
#include "CAN/can.h"
#include "CAN/can_bus_parser.h"

extern CAN_HandleTypeDef hcan2;
extern SBUS_Data sbusData;

osMessageQueueId_t debugMessageQueue;
osMessageQueueId_t txMessageQueue;

void can_init(){
	CAN_FilterTypeDef filter = {0};
	filter.FilterBank = 20;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterFIFOAssignment = CAN_RX_FIFO0;
	filter.FilterActivation = CAN_FILTER_ENABLE;
	filter.FilterIdHigh = 0;
	filter.FilterIdLow = 0;
	filter.FilterMaskIdHigh = 0;
	filter.FilterMaskIdLow = 0;
	filter.SlaveStartFilterBank = 14;

	HAL_StatusTypeDef ret = HAL_CAN_ConfigFilter(&hcan2, &filter);
	if(ret != HAL_OK) Error_Handler();
	if(HAL_CAN_Start(&hcan2) != HAL_OK) Error_Handler();
	if(HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
		Error_Handler();

	//Initialize queues
	//First char -> size
	debugMessageQueue = osMessageQueueNew(64, 8, NULL);
	if(debugMessageQueue == NULL)
		Error_Handler();

	txMessageQueue = osMessageQueueNew(64, sizeof(CAN_TX_QUEUE_OBJ), NULL);
	if(txMessageQueue == NULL)
		Error_Handler();
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){
	CAN_RxHeaderTypeDef rxHeader;
	uint8_t data[8];

	if(hcan == NULL || hcan->Instance != CAN2) return;
	while(HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0U){
		if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, data) != HAL_OK)
			break;
		can_parse_msg(&rxHeader, data);
	}
}

void can_tx_update(){
	uint32_t txMailbox;
	CAN_TX_QUEUE_OBJ txOut;

	while(osMessageQueueGet(txMessageQueue, &txOut, NULL, 0) == osOK){
		HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(&hcan2, &txOut.header, txOut.payload, &txMailbox);
		if(ret != HAL_OK){
			// Mailbox full, requeue message and stop trying for now
			osMessageQueuePut(txMessageQueue, &txOut, 0, 0);
			break;
		}
	}
}

void can_tx_task(void * params){
	for(;;){
		can_tx_update();

		osDelay(can_tx_task_delay);
	}
}
