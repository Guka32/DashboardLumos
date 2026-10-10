/*
 * battery_monitor.c
 *
 *  Created on: Feb 17, 2022
 *      Author: abiel
 */

#include "Sensors/battery_monitor.h"
#include "stm32f4xx_hal.h"
#include "main.h"

extern ADC_HandleTypeDef hadc2;

void battery_monitor_init(){
	HAL_StatusTypeDef ret;
	ret = HAL_ADC_Start(&hadc2);
	if(ret != HAL_OK) Error_Handler();
}

float battery_monitor_poll(){
	float ret = HAL_ADC_GetValue(&hadc2);
	return ret * 0.010439 - 0.41648;
}

