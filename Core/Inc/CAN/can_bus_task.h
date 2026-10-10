/*
 * can_bus_task.h
 *
 *  Created on: Jan 9, 2022
 *      Author: abiel
 */

#ifndef INC_CAN_CAN_BUS_TASK_H_
#define INC_CAN_CAN_BUS_TASK_H_

#include "can_bus.h"
#include <stdint.h>
#include "cmsis_os.h"

static const uint32_t can_tx_task_delay = 5;

/**
 * Updates the tx_data struct which is sent over CAN
 */
void can_init();

void can_tx_update();

void can_tx_task(void *);

#endif /* INC_CAN_CAN_BUS_TASK_H_ */
