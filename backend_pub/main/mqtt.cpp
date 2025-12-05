#include "mqtt.h"
#include "esp_event.h"
#include "esp_log.h"
#include "logbuf.h"
#include "config.h"

static const char *TAG = "MQTT";

static volatile bool s_mqtt_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        logbuf_add(LOG_LVL_INFO, TAG, "MQTT conectado ao broker");
        s_mqtt_connected = true;
        esp_mqtt_client_publish(client, "esp/test", "hello", 0, 1, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT desconectado");
        logbuf_add(LOG_LVL_WARN, TAG, "MQTT desconectado");
        s_mqtt_connected = false;
        break;
    case MQTT_EVENT_ERROR:
    {
        ESP_LOGE(TAG, "MQTT erro");
        logbuf_add(LOG_LVL_ERROR, TAG, "Erro no cliente MQTT");
        s_mqtt_connected = false;
        if (event->error_handle)
        {
            esp_mqtt_error_codes_t *err = event->error_handle;
            ESP_LOGE(TAG, "tipo=%d, tls_last_esp_err=%d, tls_stack_err=%d, transport_errno=%d, conn_ret=%d",
                     err->error_type, err->esp_tls_last_esp_err, err->esp_tls_stack_err, err->esp_transport_sock_errno, err->connect_return_code);
        }
        break;
    }
    default:
        break;
    }
}

esp_mqtt_client_handle_t mqtt_start(const char *uri, int port)
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = uri;
    mqtt_cfg.broker.address.port = port;
    mqtt_cfg.session.keepalive = 60; // seconds
    mqtt_cfg.buffer.size = 2048;     // bytes

    // Credenciais do broker (opcionais) puxadas da memória
    const app_config_t *cfg = config_get();
    if (cfg)
    {
        if (cfg->user[0])
            mqtt_cfg.credentials.username = cfg->user;
        if (cfg->pass_mqtt[0])
            mqtt_cfg.credentials.authentication.password = cfg->pass_mqtt;
    }

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    return client;
}

int mqtt_publish(esp_mqtt_client_handle_t client, const char *topic, const char *payload, int qos, int retain)
{
    return esp_mqtt_client_publish(client, topic, payload, 0, qos, retain);
}

bool mqtt_is_connected()
{
    return s_mqtt_connected;
}
