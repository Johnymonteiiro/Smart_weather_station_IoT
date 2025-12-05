#ifndef STATUS_H
#define STATUS_H

typedef struct {
    float temp;
    float hum;
    int rain_pct;
} telemetry_t;

void status_set_telemetry(float temp, float hum, int rain_pct);
telemetry_t status_get_telemetry();

#endif // STATUS_H

