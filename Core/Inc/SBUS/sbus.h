/*
 * Handles SBUS interface on an (already initialized) UART with DMA
 */

#pragma once

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include <stdbool.h>

#define SBUS_HEADER 0x0f
#define SBUS_FOOTER 0x00
#define SBUS_MSG_LEN 25

#define SBUS_FLAGS_BYTE 23
#define SBUS_FAILSAFE_BIT 3

enum SBUS_STATE {
	SBUS_STATE_HEADER,
	SBUS_STATE_FOOTER,
	SBUS_STATE_END
};

typedef struct {
	UART_HandleTypeDef *huart;
	uint8_t msgBuffer[SBUS_MSG_LEN];

	enum SBUS_STATE parseState;
	uint16_t msgLen;

	uint16_t channels[16];
	bool failsafe;
	uint32_t timestamp;
} SBUS_Data;

HAL_StatusTypeDef SBUS_Init(SBUS_Data *data, UART_HandleTypeDef *huart);

/*
 * MUST BE ADDED TO HAL_UART_RxCpltCallback in order to handle DMA
 */
void SBUS_RxCallback(SBUS_Data *data, UART_HandleTypeDef *huart);

void SBUS_Update();

void SBUS_Parse(SBUS_Data *data);

void UART5_IRQHandler(void);
