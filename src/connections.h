#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "useful_stuff.h"

#define WIFI_SSID "ZyXELAAFD88"
#define WIFI_PASSWORD "3EKH74VVNXXM4"
#define WIFI_MAX_RETRY 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define UDP_PORT 1234

typedef struct {
    char gesture[32];
    float dx;
    float dy;
    SemaphoreHandle_t mutex;
} cv_data_t;

// Connects the ESP32-WROOM-32D to the WiFi
void wifi_init();

// Sets up a task to listen to the stream on the UDP socket where the python script is communicating
void udp_listener_task(void *pvParameters);

#endif