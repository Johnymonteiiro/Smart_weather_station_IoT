#pragma once

enum TemperatureAlert
{
    TEMP_NORMAL,
    TEMP_MEDIA,
    TEMP_ALTA
};
enum RainAlert
{
    RAIN_SEM,
    RAIN_MEDIA,
    RAIN_FORTE
};

struct Alerts
{
    TemperatureAlert temp;
    RainAlert rain;
};

class AlertManager
{
public:
    static Alerts evaluate(float temperature, float rainPct);
    static const char *tempLabel(TemperatureAlert a);
    static const char *rainLabel(RainAlert a);
};
