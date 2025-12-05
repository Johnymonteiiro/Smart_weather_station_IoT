#include "config-manager.h"
#include "wifi.h"
#include "web-server.h"
#include "mqtt.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando Sistema IoT...");

    // 1. Inicializa serviços base
    ConfigManager::init();
    WebServer::mountSpiffs();

    // 2. Carrega Configurações
    AppConfig config;
    ConfigManager::load(config);

    // 3. Inicia Wi-Fi
    WiFiManager::start(config);

    // 4. Inicia MQTT (se houver broker configurado)
    // Instanciamos aqui para manter o cliente vivo durante a execução
    static MqttManager mqtt;
    mqtt.start(config);

    // 5. Inicia Web Server
    static WebServer server;
    server.start();

    ESP_LOGI(TAG, "Sistema rodando!");
}