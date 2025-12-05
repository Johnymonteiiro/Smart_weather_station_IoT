#pragma once
#include "app.config.h"
#include "mqtt_client.h"

class MqttManager
{
private:
    esp_mqtt_client_handle_t client = nullptr;
    char topic[64];
    int qos;

    static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

public:
    void start(const AppConfig &config);
};