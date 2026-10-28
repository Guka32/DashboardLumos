/* SBUS UART capture, frame assembly, and channel decoding. */

#include "SBUS/sbus.h"
#include <string.h>

#define SBUS_RX_RING_SIZE 128U
#define SBUS_RX_RING_MASK (SBUS_RX_RING_SIZE - 1U)
#define SBUS_DMA_BLOCK_LEN SBUS_MSG_LEN

extern SBUS_Data sbusData;

static volatile uint8_t rxRing[SBUS_RX_RING_SIZE];
static volatile uint16_t rxHead;
static volatile uint16_t rxTail;
static volatile uint32_t rxRecoveryEpoch;
static uint32_t parserRecoveryEpoch;
static uint8_t rxDmaBuffer[SBUS_DMA_BLOCK_LEN];
static uint8_t frameBuffer[SBUS_MSG_LEN];
static uint8_t frameLength;

static void sbus_enqueue_byte(uint8_t byte)
{
	uint16_t head = rxHead;
	uint16_t next = (uint16_t)((head + 1U) & SBUS_RX_RING_MASK);
	if (next == rxTail) {
		sbusData.rxOverflowCount++;
		__DMB();
		rxRecoveryEpoch++;
	} else {
		rxRing[head] = byte;
		__DMB();
		rxHead = next;
	}
}

static HAL_StatusTypeDef sbus_start_uart_receive(UART_HandleTypeDef *huart)
{
	HAL_StatusTypeDef status = HAL_UART_Receive_DMA(huart, rxDmaBuffer,
		(uint16_t)sizeof(rxDmaBuffer));
	if (status != HAL_OK) {
		sbusData.rxRearmErrorCount++;
		__DMB();
		rxRecoveryEpoch++;
	}
	return status;
}

HAL_StatusTypeDef SBUS_Init(SBUS_Data *data, UART_HandleTypeDef *huart)
{
	if (data == NULL || huart == NULL) return HAL_ERROR;

	memset(data, 0, sizeof(*data));
	data->huart = huart;
	data->failsafe = true;	/* Fail closed until healthy frames arrive. */

	rxHead = 0U;
	rxTail = 0U;
	rxRecoveryEpoch = 0U;
	parserRecoveryEpoch = 0U;
	memset(rxDmaBuffer, 0, sizeof(rxDmaBuffer));
	frameLength = 0U;

	return sbus_start_uart_receive(huart);
}

/* DMA callback: transfer the captured block into the parser ring, then rearm. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart != sbusData.huart) return;

	for (uint16_t i = 0U; i < SBUS_DMA_BLOCK_LEN; i++) {
		sbus_enqueue_byte(rxDmaBuffer[i]);
	}

	sbus_start_uart_receive(huart);
}

/* A UART error invalidates the partial frame; reception is restarted if HAL
 * ended the interrupted transfer. The task will resynchronize at a new header.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart != sbusData.huart) return;

	sbusData.rxErrorCount++;
	if ((huart->ErrorCode & HAL_UART_ERROR_ORE) != 0U) {
		__HAL_UART_CLEAR_OREFLAG(huart);
	}
	__DMB();
	rxRecoveryEpoch++;
	if (huart->RxState == HAL_UART_STATE_READY) {
		sbus_start_uart_receive(huart);
	}
}

static void sbus_process_byte(SBUS_Data *data, uint8_t byte)
{
	if (frameLength == 0U) {
		if (byte == SBUS_HEADER) {
			frameBuffer[0] = byte;
			frameLength = 1U;
		}
		return;
	}

	frameBuffer[frameLength++] = byte;
	if (frameLength < SBUS_MSG_LEN) return;

	if (frameBuffer[0] == SBUS_HEADER &&
	    frameBuffer[SBUS_MSG_LEN - 1U] == SBUS_FOOTER) {
		memcpy(data->msgBuffer, frameBuffer, SBUS_MSG_LEN);
		SBUS_Parse(data);
		frameLength = 0U;
		return;
	}

	/* Keep any possible header already seen inside the candidate, so a bad or
	 * dropped byte does not force the parser to wait for a full new frame.
	 */
	uint8_t nextHeader = 1U;
	while (nextHeader < SBUS_MSG_LEN &&
	       frameBuffer[nextHeader] != SBUS_HEADER) {
		nextHeader++;
	}
	if (nextHeader == SBUS_MSG_LEN) {
		frameLength = 0U;
	} else {
		frameLength = (uint8_t)(SBUS_MSG_LEN - nextHeader);
		memmove(frameBuffer, &frameBuffer[nextHeader], frameLength);
	}
}

/* Drain all bytes currently captured by the ISR. This function never waits for
 * UART data, and failsafe flags do not stop frame reception or parsing.
 */
void SBUS_Update(void)
{
	/* Recover if HAL left the receiver idle after an error or failed rearm. */
	if (sbusData.huart != NULL &&
	    sbusData.huart->RxState == HAL_UART_STATE_READY) {
		sbus_start_uart_receive(sbusData.huart);
	}

	for (;;) {
		uint32_t recoveryEpoch = rxRecoveryEpoch;
		if (recoveryEpoch != parserRecoveryEpoch) {
			parserRecoveryEpoch = recoveryEpoch;
			frameLength = 0U;
			/* A transport discontinuity is a control fault until three clean
			 * frames arrive. Keep receiver failsafe reporting independent.
			 */
			int32_t lock = osKernelLock();
			sbusData.healthyFrameStreak = 0U;
			osKernelRestoreLock(lock);
			/* Drop queued bytes from before the error/overflow and resume at the
			 * next complete stream segment.
			 */
			rxTail = rxHead;
			continue;
		}

		uint16_t tail = rxTail;
		uint16_t head = rxHead;
		if (tail == head) break;

		uint8_t byte = rxRing[tail];
		__DMB();
		rxTail = (uint16_t)((tail + 1U) & SBUS_RX_RING_MASK);
		sbus_process_byte(&sbusData, byte);
	}
}

void SBUS_Parse(SBUS_Data *data)
{
	if (data == NULL || data->msgBuffer[0] != SBUS_HEADER ||
	    data->msgBuffer[SBUS_MSG_LEN - 1U] != SBUS_FOOTER) {
		return;
	}

	const uint8_t *buf = data->msgBuffer;
	uint16_t channels[16];
	channels[0]  = ((uint16_t)buf[1]        | (uint16_t)buf[2]  << 8)  & 0x07FFU;
	channels[1]  = ((uint16_t)buf[2]  >> 3  | (uint16_t)buf[3]  << 5)  & 0x07FFU;
	channels[2]  = ((uint16_t)buf[3]  >> 6  | (uint16_t)buf[4]  << 2
	                                      | (uint16_t)buf[5]  << 10) & 0x07FFU;
	channels[3]  = ((uint16_t)buf[5]  >> 1  | (uint16_t)buf[6]  << 7)  & 0x07FFU;
	channels[4]  = ((uint16_t)buf[6]  >> 4  | (uint16_t)buf[7]  << 4)  & 0x07FFU;
	channels[5]  = ((uint16_t)buf[7]  >> 7  | (uint16_t)buf[8]  << 1
	                                      | (uint16_t)buf[9]  << 9)  & 0x07FFU;
	channels[6]  = ((uint16_t)buf[9]  >> 2  | (uint16_t)buf[10] << 6)  & 0x07FFU;
	channels[7]  = ((uint16_t)buf[10] >> 5  | (uint16_t)buf[11] << 3)  & 0x07FFU;
	channels[8]  = ((uint16_t)buf[12]       | (uint16_t)buf[13] << 8)  & 0x07FFU;
	channels[9]  = ((uint16_t)buf[13] >> 3  | (uint16_t)buf[14] << 5)  & 0x07FFU;
	channels[10] = ((uint16_t)buf[14] >> 6  | (uint16_t)buf[15] << 2
	                                      | (uint16_t)buf[16] << 10) & 0x07FFU;
	channels[11] = ((uint16_t)buf[16] >> 1  | (uint16_t)buf[17] << 7)  & 0x07FFU;
	channels[12] = ((uint16_t)buf[17] >> 4  | (uint16_t)buf[18] << 4)  & 0x07FFU;
	channels[13] = ((uint16_t)buf[18] >> 7  | (uint16_t)buf[19] << 1
	                                      | (uint16_t)buf[20] << 9)  & 0x07FFU;
	channels[14] = ((uint16_t)buf[20] >> 2  | (uint16_t)buf[21] << 6)  & 0x07FFU;
	channels[15] = ((uint16_t)buf[21] >> 5  | (uint16_t)buf[22] << 3)  & 0x07FFU;

	uint8_t flags = buf[SBUS_FLAGS_BYTE];
	bool failsafe = (flags & (1U << SBUS_FAILSAFE_BIT)) != 0U;
	bool frameLost = (flags & (1U << SBUS_FRAME_LOST_BIT)) != 0U;
	uint32_t now = HAL_GetTick();

	/* Publish a whole frame under a short scheduler lock. UART interrupts remain
	 * enabled, so bytes continue entering the ring while tasks cannot see a
	 * partially updated channel set.
	 */
	int32_t lock = osKernelLock();
	/* Keep raw channels visible for diagnostics/CAN even on receiver failsafe.
	 * The boat consumes a separate copy that only healthy frames can update.
	 */
	memcpy(data->channels, channels, sizeof(channels));
	if (!failsafe && !frameLost) {
		memcpy(data->controlChannels, channels, sizeof(channels));
	}
	data->flags = flags;
	data->failsafe = failsafe;
	data->frameLost = frameLost;
	data->timestamp = now;
	data->validFrameCount++;

	if (frameLost) {
		if (data->frameLostStreak < 0xFFU) data->frameLostStreak++;
	} else {
		data->frameLostStreak = 0U;
	}

	if (failsafe) {
		data->healthyFrameStreak = 0U;
	} else if (!frameLost) {
		if (data->healthyFrameStreak < SBUS_HEALTHY_FRAMES_REQUIRED) {
			data->healthyFrameStreak++;
		}
		data->healthyTimestamp = now;
	}
	osKernelRestoreLock(lock);
}

bool SBUS_GetSnapshot(SBUS_Data *data, SBUS_Snapshot *snapshot)
{
	if (data == NULL || snapshot == NULL) return false;

	int32_t lock = osKernelLock();
	memcpy(snapshot->channels, data->channels, sizeof(snapshot->channels));
	memcpy(snapshot->controlChannels, data->controlChannels, sizeof(snapshot->controlChannels));
	snapshot->flags = data->flags;
	snapshot->failsafe = data->failsafe;
	snapshot->frameLost = data->frameLost;
	snapshot->timestamp = data->timestamp;
	snapshot->healthyTimestamp = data->healthyTimestamp;
	snapshot->validFrameCount = data->validFrameCount;
	snapshot->rxErrorCount = data->rxErrorCount;
	snapshot->rxOverflowCount = data->rxOverflowCount;
	snapshot->rxRearmErrorCount = data->rxRearmErrorCount;
	snapshot->healthyFrameStreak = data->healthyFrameStreak;
	snapshot->frameLostStreak = data->frameLostStreak;
	osKernelRestoreLock(lock);
	return true;
}

bool SBUS_IsControlValid(const SBUS_Snapshot *snapshot, uint32_t now)
{
	if (snapshot == NULL || snapshot->validFrameCount == 0U ||
	    snapshot->failsafe ||
	    snapshot->healthyFrameStreak < SBUS_HEALTHY_FRAMES_REQUIRED ||
	    snapshot->frameLostStreak >= SBUS_FRAME_LOST_LIMIT) {
		return false;
	}

	return (now - snapshot->healthyTimestamp) <= SBUS_CONTROL_TIMEOUT_MS;
}
