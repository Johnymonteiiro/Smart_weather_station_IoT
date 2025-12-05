#pragma once
#include "app.config.h"
#include "esp_wifi.h"
#include "esp_event.h"

class WiFiManager
{
public:
    static void start(const AppConfig &config);
    static bool connectStaGetIp(const AppConfig &config, char *ip_out, size_t ip_len, bool keep_ap);
    static void switchToAp();
};
