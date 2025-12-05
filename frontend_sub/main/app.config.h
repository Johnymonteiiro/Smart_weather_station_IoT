#pragma once
#include <string.h>
#include <stdint.h>

struct AppConfig
{
    char ssid[32] = "";
    char password[64] = "";

    // Configurações MQTT completas baseadas no Frontend
    char mqtt_broker[128] = ""; // URL completa
    int32_t mqtt_port = 1883;
    char mqtt_topic[64] = "esp/sensors";
    char mqtt_user[32] = "";
    char mqtt_pass[64] = "";
    int32_t mqtt_qos = 0;
};
