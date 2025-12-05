#include "webserver.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "wifi.h"
#include "mqtt.h"
#include "status.h"
#include "logbuf.h"
#include "config.h"
#include "cJSON.h"
#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "WEB";

// Servidor de arquivos: lê de /spiffs conforme URI
static esp_err_t file_handler(httpd_req_t *req)
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

static esp_err_t status_handler(httpd_req_t *req)
{
    bool wifi_ok = wifi_is_connected();
    bool mqtt_ok = mqtt_is_connected();

    char ip_str[16] = "0.0.0.0";
    char gw_str[16] = "0.0.0.0";
    int rssi = -127; // valor padrão quando indisponível
    const char *mode = wifi_mode_is_ap() ? "AP" : "STA";
    esp_netif_t *netif = wifi_get_netif();
    if (netif)
    {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
        {
            snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", IP2STR(&ip_info.ip));
            snprintf(gw_str, sizeof(gw_str), "%d.%d.%d.%d", IP2STR(&ip_info.gw));
        }
    }
    // RSSI somente em modo STA
    if (!wifi_mode_is_ap())
    {
        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
        {
            rssi = ap.rssi; // dBm
        }
    }

    int64_t us = esp_timer_get_time();
    int64_t secs = us / 1000000LL;
    int days = (int)(secs / 86400);
    secs %= 86400;
    int hours = (int)(secs / 3600);
    secs %= 3600;
    int mins = (int)(secs / 60);
    int s = (int)(secs % 60);

    telemetry_t t = status_get_telemetry();

    uint32_t uptime_ms = (uint32_t)(us / 1000ULL);

    char json[400];
    int len = snprintf(json, sizeof(json),
                       "{\"wifi_connected\":%s,\"mode\":\"%s\",\"ip\":\"%s\",\"gw\":\"%s\",\"rssi\":%d,\"mqtt_connected\":%s,\"uptime\":\"%dd %dh %dm %ds\",\"uptime_ms\":%lu,\"temp\":%.2f,\"hum\":%.2f,\"rain_pct\":%d}",
                       wifi_ok ? "true" : "false", mode, ip_str, gw_str, rssi, mqtt_ok ? "true" : "false",
                       days, hours, mins, s, (unsigned long)uptime_ms,
                       t.temp, t.hum, t.rain_pct);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, len);
}

// --- Config API ---
static esp_err_t config_get_handler(httpd_req_t *req)
{
    const app_config_t *cfg = config_get();
    char json[768];
    int len = snprintf(json, sizeof(json),
                       "{\"ssid\":\"%s\",\"pass\":\"%s\",\"broker\":\"%s\",\"port\":%d,\"topic\":\"%s\",\"qos\":%d,\"user\":\"%s\",\"pass_mqtt\":\"%s\"}",
                       cfg->ssid, cfg->pass, cfg->broker, cfg->port, cfg->topic, cfg->qos, cfg->user, cfg->pass_mqtt);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json, len);
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    int total = req->content_len;
    if (total <= 0 || total > 2048)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid content len");
        return ESP_FAIL;
    }
    char *buf = (char *)malloc((size_t)total + 1);
    if (!buf)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
        return ESP_FAIL;
    }
    int received = 0;
    while (received < total)
    {
        int to_read = total - received;
        if (to_read > 512)
            to_read = 512;
        int r = httpd_req_recv(req, buf + received, to_read);
        if (r <= 0)
        {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Recv error");
            return ESP_FAIL;
        }
        received += r;
    }
    buf[received] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad JSON");
        return ESP_FAIL;
    }

    app_config_t cfg = {};
    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass = cJSON_GetObjectItem(root, "pass");
    cJSON *broker = cJSON_GetObjectItem(root, "broker");
    cJSON *port = cJSON_GetObjectItem(root, "port");
    cJSON *topic = cJSON_GetObjectItem(root, "topic");
    cJSON *qos = cJSON_GetObjectItem(root, "qos");
    cJSON *user = cJSON_GetObjectItem(root, "user");
    cJSON *pass_mqtt = cJSON_GetObjectItem(root, "pass_mqtt");

    if (ssid && cJSON_IsString(ssid))
        strlcpy(cfg.ssid, ssid->valuestring, sizeof(cfg.ssid));
    if (pass && cJSON_IsString(pass))
        strlcpy(cfg.pass, pass->valuestring, sizeof(cfg.pass));
    if (broker && cJSON_IsString(broker))
        strlcpy(cfg.broker, broker->valuestring, sizeof(cfg.broker));
    if (port && cJSON_IsNumber(port))
        cfg.port = port->valueint;
    else
        cfg.port = 1883;
    if (topic && cJSON_IsString(topic))
        strlcpy(cfg.topic, topic->valuestring, sizeof(cfg.topic));
    if (qos && cJSON_IsNumber(qos))
        cfg.qos = qos->valueint;
    else
        cfg.qos = 0;
    if (user && cJSON_IsString(user))
        strlcpy(cfg.user, user->valuestring, sizeof(cfg.user));
    if (pass_mqtt && cJSON_IsString(pass_mqtt))
        strlcpy(cfg.pass_mqtt, pass_mqtt->valuestring, sizeof(cfg.pass_mqtt));

    cJSON_Delete(root);

    if (config_save(&cfg))
    {
        logbuf_add(LOG_LVL_INFO, "CFG", "Configuracoes salvas");
        // Alterna para STA com as novas credenciais
        wifi_start_sta(cfg.ssid, cfg.pass);
        // Resposta para o front-end: tenta fornecer IP atual se já conectado
        const char *ip = wifi_get_ip_str();
        char next[64];
        snprintf(next, sizeof(next), "http://%s/", ip);
        char resp[128];
        int len = snprintf(resp, sizeof(resp), "{\"next_url\":\"%s\"}", next);
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, resp, len);
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Save failed");
        return ESP_FAIL;
    }
}

static esp_err_t config_clear_handler(httpd_req_t *req)
{
    if (config_clear())
    {
        logbuf_add(LOG_LVL_WARN, "CFG", "Configuracoes apagadas");
        // Volta para modo AP para reconfigurar
        wifi_start_ap("Admin_AP");
        const char *ap_url = "http://192.168.4.1/"; // IP padrão do AP do ESP32
        char resp[128];
        int len = snprintf(resp, sizeof(resp), "{\"ap_url\":\"%s\"}", ap_url);
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, resp, len);
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Clear failed");
        return ESP_FAIL;
    }
}

static esp_err_t logs_handler(httpd_req_t *req)
{
    // Serializa em chunks para reduzir uso de memória e evitar fragmentação
    httpd_resp_set_type(req, "application/json");

    // Início de array
    esp_err_t ret = httpd_resp_send_chunk(req, "[", 1);
    if (ret != ESP_OK)
        return ret;

    // Determina janela atual do buffer
    extern uint32_t g_count;    // declarado em logbuf.cpp
    extern uint32_t g_head;     // declarado em logbuf.cpp
    extern log_entry_t g_buf[]; // declarado em logbuf.cpp

    uint32_t valid = g_count < 100 ? g_count : 100; // LOGBUF_MAX = 100
    uint32_t start = (g_count >= 100) ? g_head : 0;

    char item[256];
    for (uint32_t i = 0; i < valid; ++i)
    {
        uint32_t idx = (start + i) % 100;
        log_entry_t *e = &g_buf[idx];
        // vírgula separadora
        if (i > 0)
        {
            ret = httpd_resp_send_chunk(req, ",", 1);
            if (ret != ESP_OK)
                return ret;
        }
        const char *lvl = (e->level == LOG_LVL_ERROR) ? "ERROR" : (e->level == LOG_LVL_WARN) ? "WARN"
                                                                                             : "INFO";
        int n = snprintf(item, sizeof(item),
                         "{\"seq\":%lu,\"ts_ms\":%lu,\"level\":\"%s\",\"tag\":\"%s\",\"msg\":\"%s\"}",
                         (unsigned long)e->seq, (unsigned long)e->ts_ms, lvl, e->tag, e->msg);
        if (n < 0)
            continue;
        ret = httpd_resp_send_chunk(req, item, (size_t)n);
        if (ret != ESP_OK)
            return ret;
    }

    // Fim do array e encerra chunked
    ret = httpd_resp_send_chunk(req, "]", 1);
    if (ret != ESP_OK)
        return ret;
    return httpd_resp_send_chunk(req, NULL, 0);
}

httpd_handle_t webserver_start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // Habilita wildcard para servir quaisquer arquivos via "/*"
    config.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t server = NULL;

    // Monta SPIFFS em /spiffs
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 8,
        .format_if_mount_failed = true,
    };
    esp_err_t sret = esp_vfs_spiffs_register(&conf);
    if (sret != ESP_OK)
    {
        ESP_LOGE(TAG, "Falha ao montar SPIFFS (%d)", (int)sret);
        logbuf_add(LOG_LVL_ERROR, TAG, "Falha ao montar SPIFFS");
    }
    else
    {
        ESP_LOGI(TAG, "SPIFFS montado em /spiffs");
        logbuf_add(LOG_LVL_INFO, TAG, "SPIFFS montado");
    }

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t status = {.uri = "/status", .method = HTTP_GET, .handler = status_handler, .user_ctx = NULL};
        httpd_uri_t logs = {.uri = "/logs", .method = HTTP_GET, .handler = logs_handler, .user_ctx = NULL};
        httpd_uri_t cfg_get = {.uri = "/api/config", .method = HTTP_GET, .handler = config_get_handler, .user_ctx = NULL};
        httpd_uri_t cfg_post = {.uri = "/api/config", .method = HTTP_POST, .handler = config_post_handler, .user_ctx = NULL};
        httpd_uri_t cfg_clear = {.uri = "/api/config/clear", .method = HTTP_POST, .handler = config_clear_handler, .user_ctx = NULL};
        // Registra endpoints primeiro para evitar captura pelo wildcard
        httpd_register_uri_handler(server, &status);
        httpd_register_uri_handler(server, &logs);
        httpd_register_uri_handler(server, &cfg_get);
        httpd_register_uri_handler(server, &cfg_post);
        httpd_register_uri_handler(server, &cfg_clear);
        // Wildcard para arquivos estáticos
        httpd_uri_t files = {.uri = "/*", .method = HTTP_GET, .handler = file_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &files);
        ESP_LOGI(TAG, "Webserver iniciado na porta %d", config.server_port);
    }
    else
    {
        ESP_LOGW(TAG, "Falha ao iniciar webserver");
    }
    return server;
}
