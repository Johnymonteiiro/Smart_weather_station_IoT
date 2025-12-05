#ifndef MQTT_H
#define MQTT_H

#include "mqtt_client.h"

esp_mqtt_client_handle_t mqtt_start(const char *uri, int port);
int mqtt_publish(esp_mqtt_client_handle_t client, const char *topic, const char *payload, int qos, int retain);
bool mqtt_is_connected();

#endif // MQTT_H
