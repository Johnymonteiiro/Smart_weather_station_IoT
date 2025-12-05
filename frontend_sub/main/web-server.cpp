#include "web-server.h"
#include "config-manager.h"
#include "wifi.h"
#include "cJSON.h"
#include "esp_log.h"
#include "SensorData.h"
#include "alerts.h"
#include <string>

static const char *TAG = "WEB_SERVER";

void WebServer::mountSpiffs()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true};
    if (esp_vfs_spiffs_register(&conf) != ESP_OK)
    {
        ESP_LOGE(TAG, "Falha ao montar SPIFFS");
    }
}

esp_err_t WebServer::apiDataHandler(httpd_req_t *req)
{

    float temp = globalSensorData.temp;
    float hum = globalSensorData.hum;
    float rain = globalSensorData.rain;
    Alerts alerts = AlertManager::evaluate(temp, rain);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temp", temp);
    cJSON_AddNumberToObject(root, "hum", hum);
    cJSON_AddNumberToObject(root, "rain", rain);
    cJSON *alertsObj = cJSON_CreateObject();
    cJSON_AddStringToObject(alertsObj, "temp", AlertManager::tempLabel(alerts.temp));
    cJSON_AddStringToObject(alertsObj, "rain", AlertManager::rainLabel(alerts.rain));
    cJSON_AddItemToObject(root, "alerts", alertsObj);

    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

// --- PROCESSA O FORMULÁRIO DE CONFIG DO FRONTEND ---
esp_err_t WebServer::apiConfigHandler(httpd_req_t *req)
{
    char buf[512]; // Aumentado para caber todos os campos
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0)
        return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root)
        return ESP_FAIL;

    AppConfig newConfig;

    // Parsing seguro dos campos JSON vindos do Frontend
    cJSON *item;

    item = cJSON_GetObjectItem(root, "ssid");
    if (cJSON_IsString(item))
        strncpy(newConfig.ssid, item->valuestring, sizeof(newConfig.ssid));

    item = cJSON_GetObjectItem(root, "pass");
    if (cJSON_IsString(item))
        strncpy(newConfig.password, item->valuestring, sizeof(newConfig.password));

    item = cJSON_GetObjectItem(root, "broker");
    if (cJSON_IsString(item))
        strncpy(newConfig.mqtt_broker, item->valuestring, sizeof(newConfig.mqtt_broker));

    item = cJSON_GetObjectItem(root, "topic");
    if (cJSON_IsString(item))
        strncpy(newConfig.mqtt_topic, item->valuestring, sizeof(newConfig.mqtt_topic));

    item = cJSON_GetObjectItem(root, "user");
    if (cJSON_IsString(item))
        strncpy(newConfig.mqtt_user, item->valuestring, sizeof(newConfig.mqtt_user));

    item = cJSON_GetObjectItem(root, "pass_mqtt"); // ou "password" dependendo do front
    if (cJSON_IsString(item))
        strncpy(newConfig.mqtt_pass, item->valuestring, sizeof(newConfig.mqtt_pass));

    // Parse de inteiros/strings numericas
    item = cJSON_GetObjectItem(root, "port");
    if (cJSON_IsString(item))
        newConfig.mqtt_port = atoi(item->valuestring);
    else if (cJSON_IsNumber(item))
        newConfig.mqtt_port = item->valueint;

    item = cJSON_GetObjectItem(root, "qos");
    if (cJSON_IsString(item))
        newConfig.mqtt_qos = atoi(item->valuestring);
    else if (cJSON_IsNumber(item))
        newConfig.mqtt_qos = item->valueint;

    ConfigManager::save(newConfig);

    char ip_buf[32] = {0};
    bool got_ip = WiFiManager::connectStaGetIp(newConfig, ip_buf, sizeof(ip_buf), true);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "message", "Configuracao salva");
    if (got_ip)
    {
        char next_url[64];
        snprintf(next_url, sizeof(next_url), "http://%s/", ip_buf);
        cJSON_AddStringToObject(resp, "next_url", next_url);
    }
    const char *json_str = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free((void *)json_str);
    cJSON_Delete(resp);
    cJSON_Delete(root);

    vTaskDelay(pdMS_TO_TICKS(800));
    esp_restart();
    return ESP_OK;
}

// --- RETORNA CONFIGURACAO SALVA ---
esp_err_t WebServer::apiGetConfigHandler(httpd_req_t *req)
{
    AppConfig cfg;
    ConfigManager::load(cfg);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ssid", cfg.ssid);
    cJSON_AddStringToObject(root, "pass", cfg.password);
    cJSON_AddStringToObject(root, "broker", cfg.mqtt_broker);
    cJSON_AddNumberToObject(root, "port", cfg.mqtt_port);
    cJSON_AddStringToObject(root, "topic", cfg.mqtt_topic);
    cJSON_AddNumberToObject(root, "qos", cfg.mqtt_qos);
    cJSON_AddStringToObject(root, "user", cfg.mqtt_user);
    cJSON_AddStringToObject(root, "pass_mqtt", cfg.mqtt_pass);

    const char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free((void *)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

// --- LIMPA CONFIGURACAO NA NVS ---
esp_err_t WebServer::apiConfigClearHandler(httpd_req_t *req)
{
    ConfigManager::clear();
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "message", "Memoria limpa");
    cJSON_AddStringToObject(resp, "ssid", "Monitor_Wi-Fi");
    cJSON_AddStringToObject(resp, "ap_url", "http://192.168.4.1/");
    const char *json_str = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    free((void *)json_str);
    cJSON_Delete(resp);

    vTaskDelay(pdMS_TO_TICKS(800));
    WiFiManager::switchToAp();
    return ESP_OK;
}

esp_err_t WebServer::fileHandler(httpd_req_t *req)
{
    std::string path = "/spiffs";
    if (strcmp(req->uri, "/") == 0)
        path += "/index.html";
    else
        path += req->uri;

    FILE *fd = fopen(path.c_str(), "r");
    if (!fd)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // MIME Types básicos
    if (path.find(".css") != std::string::npos)
        httpd_resp_set_type(req, "text/css");
    else if (path.find(".js") != std::string::npos)
        httpd_resp_set_type(req, "application/javascript");
    else
        httpd_resp_set_type(req, "text/html");

    char chunk[1024];
    size_t chunksize;
    while ((chunksize = fread(chunk, 1, sizeof(chunk), fd)) > 0)
    {
        httpd_resp_send_chunk(req, chunk, chunksize);
    }
    httpd_resp_send_chunk(req, NULL, 0);
    fclose(fd);
    return ESP_OK;
}

void WebServer::start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 8192; // Aumentado para segurança

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t api_data = {"/api/dados", HTTP_GET, apiDataHandler, NULL};
        httpd_register_uri_handler(server, &api_data);

        httpd_uri_t api_config_post = {"/api/config", HTTP_POST, apiConfigHandler, NULL};
        httpd_register_uri_handler(server, &api_config_post);

        httpd_uri_t api_config_get = {"/api/config", HTTP_GET, apiGetConfigHandler, NULL};
        httpd_register_uri_handler(server, &api_config_get);

        httpd_uri_t api_config_clear = {"/api/config/clear", HTTP_POST, apiConfigClearHandler, NULL};
        httpd_register_uri_handler(server, &api_config_clear);

        httpd_uri_t file_serve = {"/*", HTTP_GET, fileHandler, NULL};
        httpd_register_uri_handler(server, &file_serve);

        ESP_LOGI(TAG, "Servidor Web Online");
    }
}
