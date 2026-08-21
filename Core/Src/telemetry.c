/*
 * telemetry.c
 *
 *
 */

#include "telemetry.h"
#include <stdio.h>

// Global status variable containing the aggregated sniffer readings
volatile SystemStatus status = {0};

int generate_telemetry_string(char *out_buf, size_t max_len) {
    // Cast the floating-point battery voltage into integers to bypass the STM32CubeIDE 
    // default limitation that prevents %f from working without linker flags.
    int volt_int = (int)status.battery_voltage;
    int volt_dec = (int)(status.battery_voltage * 100.0f) % 100;
    if (volt_dec < 0) {
        volt_dec = -volt_dec; // Handle negative decimals just in case
    }
    
    // Format the simple, human-readable vital metrics string
    int len = snprintf(out_buf, max_len, 
        "[HB: %lu] [Batt: %d.%02dV] [PWM: %s] [Act: %s] [Pump: %s]\r\n",
        status.gateway_uptime,
        volt_int, volt_dec,
        status.pwm_received ? "Yes" : "No",
        status.actuator_active ? "Yes" : "No",
        status.pump_active ? "Yes" : "No"
    );
    
    if (len < 0 || (size_t)len >= max_len) {
        return -1;
    }
    
    return len;
}
