/*
 * can.h
 */

#ifndef INC_CAN_CAN_H_
#define INC_CAN_CAN_H_

#include "can_bus.h"
#include <stdint.h>
#include "cmsis_os.h"

extern osMessageQueueId_t debugMessageQueue;
extern osMessageQueueId_t txMessageQueue;

osStatus_t handle_debug_msg_queue();

void send_can_debug_char(char c);

void send_can_debug_msg(const char *str, uint32_t len);

// Standard CAN message queue function
void queue_can_msg(uint32_t stdId, uint8_t dlc, const uint8_t* payload);

#endif /* INC_CAN_CAN_H_ */
