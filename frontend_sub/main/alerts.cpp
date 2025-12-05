#include "alerts.h"

Alerts AlertManager::evaluate(float temperature, float rainPct)
{
    Alerts a;
    if (temperature < 25.0f) a.temp = TEMP_NORMAL;
    else if (temperature < 30.0f) a.temp = TEMP_MEDIA;
    else a.temp = TEMP_ALTA;

    if (rainPct < 10.0f) a.rain = RAIN_SEM;
    else if (rainPct < 50.0f) a.rain = RAIN_MEDIA;
    else a.rain = RAIN_FORTE;
    return a;
}

const char* AlertManager::tempLabel(TemperatureAlert a)
{
    switch (a) {
        case TEMP_NORMAL: return "normal";
        case TEMP_MEDIA: return "media";
        case TEMP_ALTA: return "alta";
        default: return "normal";
    }
}

const char* AlertManager::rainLabel(RainAlert a)
{
    switch (a) {
        case RAIN_SEM: return "sem_chuva";
        case RAIN_MEDIA: return "chuva_media";
        case RAIN_FORTE: return "chuva_forte";
        default: return "sem_chuva";
    }
}

