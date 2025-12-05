#pragma once
#include "app.config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

class ConfigManager
{
public:
    static void init();
    static void load(AppConfig &config);
    static void save(const AppConfig &config);
    static void clear();
};
