#pragma once

typedef enum {
    ALERT_LED_OFF = 0,
    ALERT_LED_GREEN = 1,
    ALERT_LED_YELLOW = 2,
    ALERT_LED_RED = 3
} alert_color_t;

typedef enum {
    RAIN_NONE = 0,
    RAIN_MEDIUM = 1,
    RAIN_HEAVY = 2
} rain_alert_t;

typedef enum {
    TEMP_LOW = 0,
    TEMP_HIGH = 1
} temp_alert_t;

// Avaliações de chuva
rain_alert_t alert_level_rain(int rain_pct);
alert_color_t alert_color_for_rain(int rain_pct);

// Avaliações de temperatura (limiar 30°C)
temp_alert_t alert_level_temp(float temp_c);
alert_color_t alert_color_for_temp(float temp_c);

// Estado e logging
void alert_init(void);
void alert_eval_and_log(float temp_c, int rain_pct);
alert_color_t alert_get_rain_color(void);
alert_color_t alert_get_temp_color(void);

// Auxiliares de string
const char *alert_color_str(alert_color_t c);
const char *alert_rain_str(rain_alert_t a);
const char *alert_temp_str(temp_alert_t a);

