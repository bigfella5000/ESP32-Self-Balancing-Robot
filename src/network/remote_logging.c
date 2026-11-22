#include "esp_log.h"
#include "lwip/sockets.h"

static int udp_log_sock = -1;
static struct sockaddr_in dest_addr;
static vprintf_like_t default_vprintf;

// The custom log interceptor
int udp_vprintf(const char *fmt, va_list args) {
    char buffer[256];
    // Format the log string
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    // Send it over UDP if the socket is ready
    if (len > 0 && udp_log_sock != -1) {
        sendto(udp_log_sock, buffer, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }
    
    // Pass it to the standard serial output
    return default_vprintf(fmt, args); 
}

// Call this AFTER wifi_init()
void init_udp_logging(const char* target_ip, int port) {
    udp_log_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, target_ip, &dest_addr.sin_addr.s_addr);

    // Override the default log handler
    default_vprintf = esp_log_set_vprintf(udp_vprintf);
    
    ESP_LOGI("UDP_LOG", "Network logging initialized to %s:%d", target_ip, port);
}