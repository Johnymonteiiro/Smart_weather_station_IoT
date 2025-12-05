#include "config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static app_config_t g_cfg = {};
static bool g_has_sta = false;

void config_init() {
    // nvs_flash_init já é chamado no main, mas garantimos aqui
    nvs_flash_init();
    app_config_t tmp = {};
    g_has_sta = config_load(&tmp);
    if (g_has_sta) g_cfg = tmp;
}

bool config_load(app_config_t *out) {
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    nvs_handle_t h;
    esp_err_t err = nvs_open("appcfg", NVS_READONLY, &h);
    if (err != ESP_OK) return false;
    size_t len;

    len = sizeof(out->ssid); nvs_get_str(h, "ssid", out->ssid, &len);
    len = sizeof(out->pass); nvs_get_str(h, "pass", out->pass, &len);
    len = sizeof(out->broker); nvs_get_str(h, "broker", out->broker, &len);
    int32_t port=0; nvs_get_i32(h, "port", &port); out->port = port ? port : 1883;
    len = sizeof(out->topic); nvs_get_str(h, "topic", out->topic, &len);
    int32_t qos=0; nvs_get_i32(h, "qos", &qos); out->qos = qos;
    len = sizeof(out->user); nvs_get_str(h, "user", out->user, &len);
    len = sizeof(out->pass_mqtt); nvs_get_str(h, "pass_mqtt", out->pass_mqtt, &len);

    nvs_close(h);

    bool has_sta = out->ssid[0] != '\0' && out->pass[0] != '\0';
    return has_sta;
}

bool config_save(const app_config_t *cfg) {
    if (!cfg) return false;
    nvs_handle_t h;
    esp_err_t err = nvs_open("appcfg", NVS_READWRITE, &h);
    if (err != ESP_OK) return false;
    nvs_set_str(h, "ssid", cfg->ssid);
    nvs_set_str(h, "pass", cfg->pass);
    nvs_set_str(h, "broker", cfg->broker);
    nvs_set_i32(h, "port", cfg->port);
    nvs_set_str(h, "topic", cfg->topic);
    nvs_set_i32(h, "qos", cfg->qos);
    nvs_set_str(h, "user", cfg->user);
    nvs_set_str(h, "pass_mqtt", cfg->pass_mqtt);
    err = nvs_commit(h);
    nvs_close(h);
    if (err == ESP_OK) { g_cfg = *cfg; g_has_sta = cfg->ssid[0] && cfg->pass[0]; }
    return err == ESP_OK;
}

bool config_clear() {
    nvs_handle_t h;
    esp_err_t err = nvs_open("appcfg", NVS_READWRITE, &h);
    if (err != ESP_OK) return false;
    nvs_erase_all(h);
    err = nvs_commit(h);
    nvs_close(h);
    memset(&g_cfg, 0, sizeof(g_cfg));
    g_has_sta = false;
    return err == ESP_OK;
}

bool config_has_sta() { return g_has_sta; }

const app_config_t* config_get() { return &g_cfg; }

void config_set(const app_config_t *cfg) {
    if (cfg) { g_cfg = *cfg; g_has_sta = cfg->ssid[0] && cfg->pass[0]; }
}

