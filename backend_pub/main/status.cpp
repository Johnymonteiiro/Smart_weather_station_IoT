#include "status.h"

static telemetry_t s_last = {0};

void status_set_telemetry(float temp, float hum, int rain_pct)
{
    s_last.temp = temp;
    s_last.hum = hum;
    s_last.rain_pct = rain_pct;
}

telemetry_t status_get_telemetry()
{
    return s_last;
}

