#include "wifi.h"
#include "esp_log.h"
#include "logbuf.h"
#include <string.h>

static const char *TAG = "WIFI";

EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *s_wifi_netif = nullptr;
static esp_netif_t *s_wifi_ap_netif = nullptr;
static bool s_is_ap = false;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "Wi-Fi desconectado, tentando reconectar...");
        logbuf_add(LOG_LVL_WARN, TAG, "Wi-Fi desconectado, reconectando");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "Wi-Fi obteve IP");
        logbuf_add(LOG_LVL_INFO, TAG, "Wi-Fi conectado (IP obtido)");
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    s_wifi_event_group = xEventGroupCreate();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    // Não inicia automaticamente; main decide AP ou STA
}

void wifi_start_sta(const char *ssid, const char *pass)
{
    s_is_ap = false;
    if (!s_wifi_netif)
        s_wifi_netif = esp_netif_create_default_wifi_sta();
    wifi_config_t wifi_config = {};
    if (ssid)
        strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    if (pass)
        strlcpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_connect();
}

void wifi_start_ap(const char *ap_ssid)
{
    s_is_ap = true;
    if (!s_wifi_ap_netif)
        s_wifi_ap_netif = esp_netif_create_default_wifi_ap();
    wifi_config_t ap = {};
    if (ap_ssid)
        strlcpy((char *)ap.ap.ssid, ap_ssid, sizeof(ap.ap.ssid));
    ap.ap.ssid_len = strlen((char *)ap.ap.ssid);
    ap.ap.channel = 1;
    ap.ap.max_connection = 4;
    ap.ap.authmode = WIFI_AUTH_OPEN; // Sem senha para setup
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap);
    esp_wifi_start();
    // Mantém bit de conectado desligado em AP
}

void wifi_stop_ap()
{
    if (s_is_ap)
    {
        esp_wifi_stop();
        s_is_ap = false;
    }
}

bool wifi_is_connected()
{
    return !s_is_ap && ((xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0);
}

esp_netif_t *wifi_get_netif()
{
    return s_is_ap ? s_wifi_ap_netif : s_wifi_netif;
}

bool wifi_mode_is_ap() { return s_is_ap; }

const char *wifi_get_ip_str()
{
    static char ip_str[16];
    ip_str[0] = '\0';
    esp_netif_t *netif = wifi_get_netif();
    if (netif)
    {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
            snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", IP2STR(&ip_info.ip));
    }
    return ip_str[0] ? ip_str : "0.0.0.0";
}
