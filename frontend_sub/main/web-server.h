#pragma once
#include "esp_http_server.h"
#include "app.config.h"
#include "esp_spiffs.h"

class WebServer
{
private:
    httpd_handle_t server = nullptr;

    static esp_err_t apiDataHandler(httpd_req_t *req);
    static esp_err_t apiConfigHandler(httpd_req_t *req);
    static esp_err_t apiGetConfigHandler(httpd_req_t *req);
    static esp_err_t apiConfigClearHandler(httpd_req_t *req);
    static esp_err_t fileHandler(httpd_req_t *req);

public:
    void start();
    static void mountSpiffs();
};
