/*
 * telemetry.c
 *
 */

#include "telemetry.h"
#include <stdio.h>

// Global status variable containing the aggregated sniffer readings
volatile SystemStatus status = {0};

// Helper function to safely cast floats to integers (including negatives like -100.30)
// This avoids needing the -u _printf_float compiler flag.
static void float_to_parts(float val, int precision_multiplier, int *int_part, int *dec_part, char *sign) {
    if (val < 0) {
        *sign = '-';
        val = -val;
    } else {
        *sign = '\0';
    }
    *int_part = (int)val;
    *dec_part = (int)(val * precision_multiplier) % precision_multiplier;
}

int generate_telemetry_string(char *out_buf, size_t max_len) {
    int v_int, v_dec, h_int, h_dec, lat_int, lat_dec, lon_int, lon_dec;
    char v_sign, h_sign, lat_sign, lon_sign;
    
    // Convert floats to integer parts (multiplier determines decimal places)
    float_to_parts(status.battery_voltage, 100, &v_int, &v_dec, &v_sign);     // 2 decimals
    float_to_parts(status.heading, 10, &h_int, &h_dec, &h_sign);              // 1 decimal
    float_to_parts(status.latitude, 100000, &lat_int, &lat_dec, &lat_sign);   // 5 decimals
    float_to_parts(status.longitude, 100000, &lon_int, &lon_dec, &lon_sign);  // 5 decimals
    
    // Format a simple comma-separated string (CSV) that the Python script can read easily:
    // Format: Heartbeat,BatteryV,Actuators,Heading,Lat,Lon
    int len = snprintf(out_buf, max_len, 
        "%lu,%c%d.%02d,%u,%c%d.%01d,%c%d.%05d,%c%d.%05d\n",
        status.gateway_uptime,
        v_sign, v_int, v_dec,
        status.actuator_active,
        h_sign, h_int, h_dec,
        lat_sign, lat_int, lat_dec,
        lon_sign, lon_int, lon_dec
    );
    
    if (len < 0 || (size_t)len >= max_len) {
        return -1;
    }
    
    return len;
}
