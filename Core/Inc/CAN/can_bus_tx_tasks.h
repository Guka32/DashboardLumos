/*
 * can_bus_tx_tasks.h
 *
 *  Created on: Jan 15, 2022
 *      Author: abiel
 */

#ifndef INC_CAN_CAN_BUS_TX_TASKS_H_
#define INC_CAN_CAN_BUS_TX_TASKS_H_

//Delay between data updates
#define HEARTBEAT_DELAY 500
#define DEBUG_MSG_DELAY 10
#define SBUS_DELAY 1000

void start_can_tx_tasks();

#endif /* INC_CAN_CAN_BUS_TX_TASKS_H_ */
