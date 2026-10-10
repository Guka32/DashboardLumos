/*
 * heartbeat_led_task.c
 *
 *	Created on: may 30th, 2022
 *		Author: Ian
 *
 */

#include "heartbeat_led_task.h"
#include "cmsis_os.h"

void heartbeat_task(void){
	//HAL_GPIO_TogglePin(DEBUG_1_GPIO_Port, DEBUG_1_Pin);
	osDelay(HEARTBEAT_DELAY);
}


