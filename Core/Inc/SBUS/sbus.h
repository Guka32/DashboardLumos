/* Nonblocking SBUS UART capture, frame parsing, and receiver health state. */

#pragma once

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include <stdbool.h>

#define SBUS_HEADER 0x0f
#define SBUS_FOOTER 0x00
#define SBUS_MSG_LEN 25

#define SBUS_FLAGS_BYTE 23
#define SBUS_FAILSAFE_BIT 3
#define SBUS_FRAME_LOST_BIT 2
#define SBUS_CONTROL_TIMEOUT_MS 100U
#define SBUS_HEALTHY_FRAMES_REQUIRED 3U
#define SBUS_FRAME_LOST_LIMIT 3U

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
	uint16_t controlChannels[16];
	uint8_t flags;
	bool failsafe;
	bool frameLost;
	uint32_t validFrameCount;
	volatile uint32_t rxErrorCount;
	volatile uint32_t rxOverflowCount;
	volatile uint32_t rxRearmErrorCount;
	uint8_t healthyFrameStreak;
	uint8_t frameLostStreak;
	uint32_t timestamp;
	uint32_t healthyTimestamp;
} SBUS_Data;

typedef struct {
	uint16_t channels[16];
	uint16_t controlChannels[16];
	uint8_t flags;
	bool failsafe;
	bool frameLost;
	uint32_t validFrameCount;
	uint32_t rxErrorCount;
	uint32_t rxOverflowCount;
	uint32_t rxRearmErrorCount;
	uint8_t healthyFrameStreak;
	uint8_t frameLostStreak;
	uint32_t timestamp;
	uint32_t healthyTimestamp;
} SBUS_Snapshot;

HAL_StatusTypeDef SBUS_Init(SBUS_Data *data, UART_HandleTypeDef *huart);

void SBUS_Update(void);

void SBUS_Parse(SBUS_Data *data);

bool SBUS_GetSnapshot(SBUS_Data *data, SBUS_Snapshot *snapshot);
bool SBUS_IsControlValid(const SBUS_Snapshot *snapshot, uint32_t now);

