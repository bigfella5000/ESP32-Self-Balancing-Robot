#ifndef REMOTE_LOGGING_H
#define REMOTE_LOGGING_H

#include "esp_log.h"

int udp_vprintf(const char *fmt, va_list args);
void init_udp_logging(const char* target_ip, int port);

#endif