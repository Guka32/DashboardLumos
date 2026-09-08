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
    float battery_voltage;        // 2. Battery Voltage (0x200)
    uint8_t actuator_active;      // 3. Actuators On/Off (0x150, Byte 0)
    float heading;                // 4. Heading (0x301)
    float latitude;               // 5. Latitude (0x300, Bytes 0-3)
    float longitude;              // 6. Longitude (0x300, Bytes 4-7)
    
    // Keep track of node activity
    uint32_t actuator_pcb_last_seen;
    uint32_t battery_pcb_last_seen;
    uint32_t nav_pcb_last_seen;
} SystemStatus;

extern volatile SystemStatus status;

/**
 * @brief Formats the telemetry string for UART output (CSV format).
 * @param out_buf Buffer where the completed sentence will be stored.
 * @param max_len Maximum length of the output buffer.
 * @return Number of characters written, or -1 on overflow/error.
 */
int generate_telemetry_string(char *out_buf, size_t max_len);

#endif /* INC_TELEMETRY_H_ */
