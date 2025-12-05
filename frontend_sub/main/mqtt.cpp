#include "mqtt.h"
#include "SensorData.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h> // Necessário para memcpy

static const char *TAG = "MQTT_MGR";

void MqttManager::event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    MqttManager *self = (MqttManager *)handler_args;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Conectado ao Broker!");
        // Faz a subscrição automática usando o tópico e QoS da config
        esp_mqtt_client_subscribe(self->client, self->topic, self->qos);
        ESP_LOGI(TAG, "Inscrito no topico: %s com QoS: %d", self->topic, self->qos);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT Desconectado. Tentando reconectar automaticamente...");
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Mensagem recebida no topico %.*s", event->topic_len, event->topic);

        if (event->data_len > 0)
        {
            char *buffer = (char *)malloc(event->data_len + 1);
            if (buffer == NULL)
            {
                ESP_LOGE(TAG, "Falha de memoria ao processar payload MQTT");
                return;
            }

            // 2. Copia os dados brutos para o buffer e fecha a string
            memcpy(buffer, event->data, event->data_len);
            buffer[event->data_len] = '\0';

            // printf("Payload JSON recebido: %s\n", buffer); // Descomente para debug

            // 3. Faz o Parse do JSON
            cJSON *root = cJSON_Parse(buffer);
            if (root)
            {
                // 4. Busca os itens dentro do JSON com segurança
                cJSON *t = cJSON_GetObjectItem(root, "dht_temp");
                cJSON *h = cJSON_GetObjectItem(root, "dht_hum");
                cJSON *p = cJSON_GetObjectItem(root, "rain_pct");

                // 5. Atualiza a struct Global (Back do Back)
                // Verifica se o campo existe e é número antes de atribuir
                if (cJSON_IsNumber(t))
                    globalSensorData.temp = (float)t->valuedouble;
                if (cJSON_IsNumber(h))
                    globalSensorData.hum = (float)h->valuedouble;
                if (cJSON_IsNumber(p))
                    globalSensorData.rain = (float)p->valuedouble;

                ESP_LOGI(TAG, "Dados Atualizados -> Temp: %.2f | Hum: %.2f | Rain: %d",
                         globalSensorData.temp, globalSensorData.hum, globalSensorData.rain);

                cJSON_Delete(root); // Limpa o objeto JSON da memória RAM
            }
            else
            {
                ESP_LOGW(TAG, "JSON Inválido Recebido");
            }

            free(buffer); // IMPORTANTE: Libera a memória do buffer temporário
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Erro MQTT");
        break;
    default:
        break;
    }
}

void MqttManager::start(const AppConfig &config)
{
    if (strlen(config.mqtt_broker) == 0)
    {
        ESP_LOGW(TAG, "MQTT nao configurado. Pulando inicializacao.");
        return;
    }

    // Salva tópico e QoS localmente para usar no callback
    strncpy(this->topic, config.mqtt_topic, sizeof(this->topic));
    this->qos = config.mqtt_qos;

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = config.mqtt_broker;
    mqtt_cfg.broker.address.port = config.mqtt_port; // Porta (geralmente 1883)
    mqtt_cfg.session.keepalive = 60;                 // seconds
    mqtt_cfg.buffer.size = 2048;                     // bytes

    if (strlen(config.mqtt_user) > 0)
    {
        mqtt_cfg.credentials.username = config.mqtt_user;
        mqtt_cfg.credentials.authentication.password = config.mqtt_pass;
    }

    this->client = esp_mqtt_client_init(&mqtt_cfg);

    // Registra eventos passando 'this' para termos contexto da classe dentro do callback estático
    esp_mqtt_client_register_event(this->client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, event_handler, this);

    esp_mqtt_client_start(this->client);
    ESP_LOGI(TAG, "Cliente MQTT iniciado para: %s", config.mqtt_broker);
}
