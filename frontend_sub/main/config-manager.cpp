#include "config-manager.h"

static const char *TAG = "CONFIG_MGR";

void ConfigManager::init()
{
    // configuração da NVS para salvar os dados de configuração
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void ConfigManager::load(AppConfig &config)
{
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK)
    {
        size_t sz;

        // Wi-Fi
        sz = sizeof(config.ssid);
        nvs_get_str(handle, "ssid", config.ssid, &sz);
        sz = sizeof(config.password);
        nvs_get_str(handle, "pass", config.password, &sz);

        // MQTT
        sz = sizeof(config.mqtt_broker);
        nvs_get_str(handle, "broker", config.mqtt_broker, &sz);
        sz = sizeof(config.mqtt_topic);
        nvs_get_str(handle, "topic", config.mqtt_topic, &sz);
        sz = sizeof(config.mqtt_user);
        nvs_get_str(handle, "user", config.mqtt_user, &sz);
        sz = sizeof(config.mqtt_pass);
        nvs_get_str(handle, "mqpass", config.mqtt_pass, &sz);

        nvs_get_i32(handle, "port", &config.mqtt_port);
        nvs_get_i32(handle, "qos", &config.mqtt_qos);

        nvs_close(handle);
        ESP_LOGI(TAG, "Config carregada. SSID: %s, Broker: %s", config.ssid, config.mqtt_broker);
    }
    else
    {
        ESP_LOGW(TAG, "Nenhuma config salva na NVS.");
    }
}

void ConfigManager::save(const AppConfig &config)
{
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK)
    {
        nvs_set_str(handle, "ssid", config.ssid);
        nvs_set_str(handle, "pass", config.password);
        nvs_set_str(handle, "broker", config.mqtt_broker);
        nvs_set_str(handle, "topic", config.mqtt_topic);
        nvs_set_str(handle, "user", config.mqtt_user);
        nvs_set_str(handle, "mqpass", config.mqtt_pass);
        nvs_set_i32(handle, "port", config.mqtt_port);
        nvs_set_i32(handle, "qos", config.mqtt_qos);

        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGI(TAG, "Configuracoes salvas com sucesso.");
    }
}

void ConfigManager::clear()
{
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK)
    {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
        ESP_LOGW(TAG, "Todas as configuracoes foram apagadas da NVS.");
    }
}
