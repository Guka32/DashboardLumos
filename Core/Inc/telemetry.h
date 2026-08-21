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
    uint32_t gateway_uptime;
    float battery_voltage;        // Sniffed from Battery Control PCB (ID 0x200)
    float battery_current;        // Sniffed from Battery Control PCB (ID 0x200)
    uint8_t pump_active;          // Sniffed from Actuator Control PCB (ID 0x150)
    uint8_t actuator_active;      // Sniffed from Actuator Control PCB (ID 0x150)
    float local_battery_voltage;  // Read from local ADC
    
    // GPS 1 Data (Simulated for diagnostic testing)
    double gps1_lat;
    double gps1_lon;
    uint8_t gps1_fix;
    
    // GPS 2 Data (Simulated for diagnostic testing)
    double gps2_lat;
    double gps2_lon;
    uint8_t gps2_fix;
    
    // Keep track of node activity
    uint32_t main_pcb_last_seen;
    uint32_t actuator_pcb_last_seen;
    uint32_t battery_pcb_last_seen;
} SystemStatus;

extern volatile SystemStatus status;

/**
 * @brief Formats a standard NMEA sentence and appends the XOR checksum.
 * @param out_buf Buffer where the completed sentence will be stored.
 * @param max_len Maximum length of the output buffer.
 * @param msg_type NMEA sentence type (e.g. "PSTAT" or "PGPS").
 * @param format Format string for payload data.
 * @return Number of characters written, or -1 on overflow/error.
 */
int generate_nmea_sentence(char *out_buf, size_t max_len, const char *msg_type, const char *format, ...);

#endif /* INC_TELEMETRY_H_ */
