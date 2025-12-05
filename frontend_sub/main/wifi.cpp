#include "wifi.h"
#include "esp_log.h"
#include <string.h>
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "WIFI_MGR";
static EventGroupHandle_t s_wifi_events;
static const int WIFI_STA_GOT_IP_BIT = BIT0;
static esp_netif_t *s_sta_netif = nullptr;
static esp_netif_t *s_ap_netif = nullptr;

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(s_wifi_events, WIFI_STA_GOT_IP_BIT);
    }
}

void WiFiManager::start(const AppConfig &config)
{
    esp_netif_init();
    esp_event_loop_create_default();
    s_sta_netif = esp_netif_create_default_wifi_sta();
    s_ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if (!s_wifi_events)
    {
        s_wifi_events = xEventGroupCreate();
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL);
    }

    if (strlen(config.ssid) > 0)
    {
        // MODO STATION (Cliente)
        wifi_config_t wifi_config = {};
        strncpy((char *)wifi_config.sta.ssid, config.ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, config.password, sizeof(wifi_config.sta.password));

        ESP_LOGI(TAG, "Iniciando modo STATION. SSID: %s", config.ssid);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_connect();
    }
    else
    {
        // MODO AP (Access Point)
        wifi_config_t wifi_ap_config = {};
        strncpy((char *)wifi_ap_config.ap.ssid, "Monitor_Wi-Fi", 32);
        wifi_ap_config.ap.ssid_len = strlen("Monitor_Wi-Fi");
        wifi_ap_config.ap.max_connection = 4;
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;

        ESP_LOGI(TAG, "Sem config valida. Iniciando modo AP: Monitor_Wi-Fi");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
}

bool WiFiManager::connectStaGetIp(const AppConfig &config, char *ip_out, size_t ip_len, bool keep_ap)
{
    if (!s_sta_netif || !s_ap_netif)
    {
        esp_netif_init();
        esp_event_loop_create_default();
        s_sta_netif = esp_netif_create_default_wifi_sta();
        s_ap_netif = esp_netif_create_default_wifi_ap();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    }
    if (!s_wifi_events)
    {
        s_wifi_events = xEventGroupCreate();
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL);
    }

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, config.ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, config.password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(keep_ap ? WIFI_MODE_APSTA : WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    xEventGroupClearBits(s_wifi_events, WIFI_STA_GOT_IP_BIT);
    EventBits_t bits = xEventGroupWaitBits(s_wifi_events, WIFI_STA_GOT_IP_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));
    if ((bits & WIFI_STA_GOT_IP_BIT) == 0)
    {
        return false;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(s_sta_netif, &ip_info) == ESP_OK)
    {
        snprintf(ip_out, ip_len, "%d.%d.%d.%d", IP2STR(&ip_info.ip));
        return true;
    }
    return false;
}

void WiFiManager::switchToAp()
{
    wifi_config_t wifi_ap_config = {};
    strncpy((char *)wifi_ap_config.ap.ssid, "Monitor_Wi-Fi", 32);
    wifi_ap_config.ap.ssid_len = strlen("Monitor_Wi-Fi");
    wifi_ap_config.ap.max_connection = 4;
    wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_LOGI(TAG, "Alternando para modo AP: Monitor_Wi-Fi");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
