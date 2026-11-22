#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"

static void ota_update_task(void *pvParameter) {
    char* url = (char*)pvParameter;
    ESP_LOGI("OTA", "Starting OTA Download from: %s", url);
    
    esp_http_client_config_t client_config = {
        .url = url,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &client_config,
    };

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI("OTA", "OTA update successful! Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE("OTA", "OTA update failed! Error: %s", esp_err_to_name(ret));
    }
    
    free(url);
    vTaskDelete(NULL);
}

void ota_trigger_task(void *pvParameter) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(4321),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };
    bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    char rx_buffer[32];
    while (1) {
        struct sockaddr_in source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        
        if (len > 0) {
            rx_buffer[len] = '\0';
            if (strncmp(rx_buffer, "UPDATE", 6) == 0) {
                // Dynamically extract the IP of the computer that clicked "Upload"
                char *host_ip = inet_ntoa(source_addr.sin_addr);
                
                char* url = malloc(64);
                snprintf(url, 64, "http://%s:8000/firmware.bin", host_ip);
                
                xTaskCreatePinnedToCore(ota_update_task, "ota_update", 8192, (void*)url, 5, NULL, 1);
            }
        }
    }
}