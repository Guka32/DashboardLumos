/*
 * telemetry.h
 *
 *  Created on: Aug 17, 2026
 *      Author: Antigravity
 */

#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t gateway_uptime;      // 1. Heartbeat
    float battery_voltage;        // 2. Battery Voltage (Sniffed from Battery PCB ID 0x200)
    uint8_t pwm_received;         // 3. PWM Received (Sniffed from Main PCB ID 0x100)
    uint8_t actuator_active;      // 4. Actuator Active (Sniffed from Actuator PCB ID 0x150)
    uint8_t pump_active;          // 5. Pump Active (Sniffed from Actuator PCB ID 0x150)
    
    // Keep track of node activity
    uint32_t main_pcb_last_seen;
    uint32_t actuator_pcb_last_seen;
    uint32_t battery_pcb_last_seen;
} SystemStatus;

extern volatile SystemStatus status;

/**
 * @brief Formats the simplified vital metrics string.
 * @param out_buf Buffer where the completed sentence will be stored.
 * @param max_len Maximum length of the output buffer.
 * @return Number of characters written, or -1 on overflow/error.
 */
int generate_telemetry_string(char *out_buf, size_t max_len);

#endif /* INC_TELEMETRY_H_ */
