#include "connections.h"

static const char *TAG = "connections";
static EventGroupHandle_t wifi_event_group;
static int retry_count = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Retrying WiFi connection (%d/%d)...", retry_count, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "WiFi connection failed after %d attempts", WIFI_MAX_RETRY);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init() {
    wifi_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to WiFi...");

    // Block here until connected or failed
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully");
    } else {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
    }
}

void udp_listener_task(void *pvParameters) {
    cv_data_t *cv = (cv_data_t*)pvParameters;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in listen_addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(UDP_PORT),
    };

    bind(sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
    ESP_LOGI(TAG, "UDP listening on port %d", UDP_PORT);

    char buf[128];
    struct sockaddr_in src_addr;
    socklen_t src_len = sizeof(src_addr);

    while (1) {
        int len = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&src_addr, &src_len);
        if (len > 0) {
            buf[len] = '\0';

            char new_gesture[32];
            float new_dx, new_dy;
            if (sscanf(buf, "%31[^,],%f,%f", &new_gesture, &new_dx, &new_dy) == 3) {
                xSemaphoreTake(cv -> mutex, portMAX_DELAY);
                strncpy(cv -> gesture, new_gesture, sizeof(cv -> gesture) - 1);
                cv -> gesture[sizeof(cv -> gesture) - 1] = '\0';
                cv -> dx = new_dx;
                cv -> dy = new_dy;
                xSemaphoreGive(cv -> mutex);
                ESP_LOGI(TAG, "gesture=%s dx=%.2f dy=%.2f", new_gesture, new_dx, new_dy);
            }
            else {
                ESP_LOGW(TAG, "Failed to parse UDP packet: %s", buf);
            }
        }
    }
}