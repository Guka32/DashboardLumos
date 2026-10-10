/*
 * can_bus_parser.c
 */

#include "CAN/can_bus.h"
#include "CAN/can_bus_parser.h"
#include "telemetry.h"

// Note: g_sendPing removed because PING_ID was deprecated in the new standard HAL architecture.

void can_parse_msg(CAN_RxHeaderTypeDef *header, uint8_t *data){
	if(header == NULL || data == NULL) return;

	if (header->StdId == CAN_ID_JETSON_HB) {
		can_rx_data.jetsonHBTick = HAL_GetTick();
		can_rx_data.jetsonHeartbeatValid = 1;
	}
	else if (header->StdId == CAN_ID_JETSON_MOTORS_01 && header->DLC >= 8) {
		float *f = (float*)data;
		can_rx_data.motorSetpoints[0] = f[0];
		can_rx_data.motorSetpoints[1] = f[1];
	}
	else if (header->StdId == CAN_ID_JETSON_MOTORS_23 && header->DLC >= 8) {
		float *f = (float*)data;
		can_rx_data.motorSetpoints[2] = f[0];
		can_rx_data.motorSetpoints[3] = f[1];
	}
	else if (header->StdId == CAN_ID_JETSON_MOTORS_45 && header->DLC >= 8) {
		float *f = (float*)data;
		can_rx_data.motorSetpoints[4] = f[0];
		can_rx_data.motorSetpoints[5] = f[1];
	}
	else if (header->StdId == CAN_ID_JETSON_MOTORS_67 && header->DLC >= 8) {
		float *f = (float*)data;
		can_rx_data.motorSetpoints[6] = f[0];
		can_rx_data.motorSetpoints[7] = f[1];
	}
	else if (header->StdId == CAN_ID_BATTERY && header->DLC >= 4) {
		float *f = (float*)data;
		status.battery_voltage = f[0];
		status.battery_pcb_last_seen = HAL_GetTick();
	}
	else if (header->StdId == CAN_ID_ACTUATOR && header->DLC >= 6) {
		status.actuator_active = data[0];
		status.actuator_pcb_last_seen = HAL_GetTick();

		// Bit 0 = ball motor enable. The remaining legacy enable bit is ignored.
		can_rx_data.ball_motor_enable = (data[0] & 0x01) ? 1 : 0;
		// Byte 1: signed ball motor speed. Byte 2 remains reserved for compatibility.
		int8_t b_speed = (int8_t)data[1];
		can_rx_data.ball_motor_speed = b_speed / 100.0f;

		int8_t turret_x = (int8_t)data[3];
		can_rx_data.turret_x_angle = turret_x / 100.0f;

		int8_t turret_y = (int8_t)data[4];
		can_rx_data.turret_y_angle = turret_y / 100.0f;

		int8_t unstick = (int8_t)data[5];
		can_rx_data.unstick_servo_pos = unstick / 100.0f;
		can_rx_data.actuatorCmdTick = HAL_GetTick();
		can_rx_data.actuatorCommandValid = 1;
	}
	else if (header->StdId == CAN_ID_GPS_LATLON && header->DLC >= 8) {
		float *f = (float*)data;
		status.latitude = f[0];
		status.longitude = f[1];
		status.nav_pcb_last_seen = HAL_GetTick();
	}
	else if (header->StdId == CAN_ID_GPS_HEADING && header->DLC >= 4) {
		float *f = (float*)data;
		status.heading = f[0];
		status.nav_pcb_last_seen = HAL_GetTick();
	}
}
