idf_component_register(SRCS "main.cpp" "mqtt.cpp" "wifi.cpp" "web-server.cpp" "config-manager.cpp" "SensorData.cpp" "alerts.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_http_server nvs_flash esp_netif esp_wifi spiffs json mqtt)
