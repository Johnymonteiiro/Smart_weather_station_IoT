#pragma once

struct SensorData
{
    float temp = 0.0f;
    float hum = 0.0f;
    float rain = 0;
};

// Instância global que será compartilhada entre os arquivos
extern SensorData globalSensorData;