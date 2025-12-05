idf_component_register(SRCS "logbuf.cpp" "webserver.cpp" "mqtt.cpp" "wifi.cpp" "status.cpp" "config.cpp" "alert.cpp" "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver json mqtt esp_wifi esp_event esp_netif nvs_flash dht esp_http_server spiffs)
