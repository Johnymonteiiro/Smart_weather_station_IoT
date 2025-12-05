#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

extern EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

void wifi_init();
bool wifi_is_connected();
esp_netif_t *wifi_get_netif();

// Controle de modos e utilitários
void wifi_start_ap(const char *ap_ssid);
void wifi_stop_ap();
void wifi_start_sta(const char *ssid, const char *pass);
bool wifi_mode_is_ap();
const char *wifi_get_ip_str();

#endif // WIFI_H
