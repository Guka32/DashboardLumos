/*
 * telemetry.c
 *
 *  Created on: Aug 17, 2026
 *      Author: Antigravity
 */

#include "telemetry.h"
#include <stdio.h>
#include <stdarg.h>

// Global status variable containing the aggregated sniffer readings
volatile SystemStatus status = {0};

int generate_nmea_sentence(char *out_buf, size_t max_len, const char *msg_type, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    // Write start of sentence and type
    int len = snprintf(out_buf, max_len, "$%s,", msg_type);
    if (len < 0 || (size_t)len >= max_len) {
        va_end(args);
        return -1;
    }
    
    // Write payload content
    int payload_len = vsnprintf(out_buf + len, max_len - len, format, args);
    va_end(args);
    
    if (payload_len < 0 || (size_t)(len + payload_len) >= max_len) {
        return -1;
    }
    len += payload_len;
    
    // Calculate NMEA XOR checksum (skipping the leading '$' character)
    uint8_t checksum = 0;
    for (int i = 1; i < len; i++) {
        checksum ^= (uint8_t)out_buf[i];
    }
    
    // Append checksum and CRLF (\r\n)
    int suffix_len = snprintf(out_buf + len, max_len - len, "*%02X\r\n", checksum);
    if (suffix_len < 0 || (size_t)(len + suffix_len) >= max_len) {
        return -1;
    }
    
    return len + suffix_len;
}
