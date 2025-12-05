#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "mqtt.h"
#include "webserver.h"
#include "status.h"
#include "logbuf.h"
#include "config.h"
#include "alert.h"
#include <string.h>
#include <math.h>
#include "driver/gpio.h"

// --- NOVO: Biblioteca do ADC para o FC-37 ---
#include "driver/adc.h"

// SENSORES
#include "dht.h"

static const char *TAG = "MQTT_PUB";

// Sensor configuration
#define DHT_GPIO GPIO_NUM_21
#define DHT_TYPE DHT_TYPE_DHT11

// GPIO 34 corresponde ao ADC1_CHANNEL_6.
#define RAIN_ADC_CHANNEL ADC1_CHANNEL_6

// --- Pinos dos LEDs (preferência solicitada) ---
#define LED_RAIN_GREEN GPIO_NUM_23
#define LED_RAIN_YELLOW GPIO_NUM_22
#define LED_RAIN_RED GPIO_NUM_18
#define LED_TEMP_GREEN GPIO_NUM_19
#define LED_TEMP_RED GPIO_NUM_17

static void leds_init()
{
    gpio_config_t io = {};
    io.intr_type = GPIO_INTR_DISABLE;
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = (1ULL << LED_RAIN_GREEN) |
                      (1ULL << LED_RAIN_YELLOW) |
                      (1ULL << LED_RAIN_RED) |
                      (1ULL << LED_TEMP_GREEN) |
                      (1ULL << LED_TEMP_RED);
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io);
    // Desliga todos inicialmente
    gpio_set_level(LED_RAIN_GREEN, 0);
    gpio_set_level(LED_RAIN_YELLOW, 0);
    gpio_set_level(LED_RAIN_RED, 0);
    gpio_set_level(LED_TEMP_GREEN, 0);
    gpio_set_level(LED_TEMP_RED, 0);
}

static void set_rain_led(alert_color_t c)
{
    gpio_set_level(LED_RAIN_GREEN, c == ALERT_LED_GREEN);
    gpio_set_level(LED_RAIN_YELLOW, c == ALERT_LED_YELLOW);
    gpio_set_level(LED_RAIN_RED, c == ALERT_LED_RED);
}

static void set_temp_led(alert_color_t c)
{
    gpio_set_level(LED_TEMP_GREEN, c == ALERT_LED_GREEN);
    gpio_set_level(LED_TEMP_RED, c == ALERT_LED_RED);
}

extern "C" void app_main(void)
{
    logbuf_init();
    nvs_flash_init();
    logbuf_add(LOG_LVL_INFO, "SYS", "NVS inicializado");
    wifi_init();
    logbuf_add(LOG_LVL_INFO, "SYS", "Wi-Fi inicializado");

    // Configuração: AP para setup se não houver STA, senão inicia STA
    config_init();
    const app_config_t *cfg = config_get();
    if (config_has_sta())
    {
        wifi_start_sta(cfg->ssid, cfg->pass);
        // Aguarda IP antes de iniciar serviços
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    }
    else
    {
        wifi_start_ap("Admin_AP");
        // Em modo AP, não aguardamos conexão
        logbuf_add(LOG_LVL_INFO, "WIFI", "Modo AP para configuracao");
    }

    // Inicia WebServer
    webserver_start();
    logbuf_add(LOG_LVL_INFO, "WEB", "Webserver iniciado");
    const char *ip = wifi_mode_is_ap() ? "192.168.4.1" : wifi_get_ip_str();
    ESP_LOGI(TAG, "Acesse: http://%s/", ip);
    logbuf_add(LOG_LVL_INFO, "WEB", "Acesse via HTTP");

    // Inicializa alertas e LEDs
    alert_init();
    leds_init();

    // --- NOVO: Inicializa ADC para Sensor de Chuva ---
    // Configura resolução de 12 bits (0 a 4095)
    adc1_config_width(ADC_WIDTH_BIT_12);
    // Configura atenuação para ler a faixa completa de 0 a ~3.3V
    adc1_config_channel_atten(RAIN_ADC_CHANNEL, ADC_ATTEN_DB_11);
    logbuf_add(LOG_LVL_INFO, "ADC", "ADC FC-37 configurado");

    // Inicia MQTT somente se conectado e broker configurado
    esp_mqtt_client_handle_t client = nullptr;
    if (wifi_is_connected() && cfg->broker[0])
    {
        char url[160];
        snprintf(url, sizeof(url), "%s", cfg->broker);
        client = mqtt_start(url, cfg->port > 0 ? cfg->port : 1883);
        logbuf_add(LOG_LVL_INFO, "MQTT", "Cliente MQTT iniciado");
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));

        // 1. Leitura DHT
        float dht_temp = NAN, dht_hum = NAN;
        esp_err_t dht_res = dht_read_float_data(DHT_TYPE, DHT_GPIO, &dht_hum, &dht_temp);
        if (dht_res != ESP_OK)
        {
            ESP_LOGW(TAG, "Falha ao ler DHT: %d", (int)dht_res);
            logbuf_add(LOG_LVL_WARN, "DHT", "Falha ao ler DHT");
        }

        // 2. --- NOVO: Leitura FC-37 ---
        int rain_raw = adc1_get_raw(RAIN_ADC_CHANNEL);

        // Vamos converter para % de umidade (0% = seco, 100% = chuva forte)
        int rain_percent = 100 - ((rain_raw * 100) / 4095);

        // Clamp para garantir limites entre 0 e 100
        if (rain_percent < 0)
            rain_percent = 0;
        if (rain_percent > 100)
            rain_percent = 100;

        ESP_LOGI(TAG, "Chuva Raw: %d | Chuva Pct: %d%%", rain_raw, rain_percent);
        // Log informativo resumido do ciclo
        logbuf_add(LOG_LVL_INFO, "SENS", "Leitura sensores concluida");

        // 3. Monta payload JSON com os dados novos
        // Atualiza telemetria para Dashboard
        status_set_telemetry(dht_temp, dht_hum, rain_percent);

        // Avalia alertas e aciona LEDs
        alert_eval_and_log(dht_temp, rain_percent);
        set_rain_led(alert_get_rain_color());
        set_temp_led(alert_get_temp_color());

        char payload[150];
        int len = snprintf(payload, sizeof(payload),
                           "{\"dht_temp\":%.2f,\"dht_hum\":%.2f,\"rain_pct\":%d}",
                           dht_temp, dht_hum, rain_percent);

        if (len < 0)
        {
            ESP_LOGE(TAG, "Erro ao montar payload");
            logbuf_add(LOG_LVL_ERROR, "MQTT", "Erro ao montar payload");
            continue;
        }

        // Publica no tópico de sensores
        const char *topic = (cfg->topic[0]) ? cfg->topic : "esp/sensors";
        int qos = (cfg->qos >= 0 && cfg->qos <= 2) ? cfg->qos : 0;
        int msg_id = -1;
        if (client)
            msg_id = mqtt_publish(client, topic, payload, qos, 0);
        if (msg_id >= 0)
        {
            ESP_LOGI(TAG, "Payload publicado: %s", payload);
            logbuf_add(LOG_LVL_INFO, "MQTT", "Payload publicado");
        }
        else
        {
            ESP_LOGE(TAG, "Falha ao publicar MQTT");
            logbuf_add(LOG_LVL_ERROR, "MQTT", "Falha ao publicar");
        }
    }
}
