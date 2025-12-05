#include "alert.h"
#include "logbuf.h"

static const float kTempHighThresholdC = 30.0f;
static const int kRainMediumMinPct = 1;    // 0 = sem chuva
static const int kRainHeavyMinPct = 61;    // >=61% considerado forte

static alert_color_t s_rain_color = ALERT_LED_GREEN;
static alert_color_t s_temp_color = ALERT_LED_GREEN;
static rain_alert_t s_rain_level = RAIN_NONE;
static temp_alert_t s_temp_level = TEMP_LOW;

rain_alert_t alert_level_rain(int rain_pct)
{
    if (rain_pct <= 0) return RAIN_NONE;
    if (rain_pct >= kRainHeavyMinPct) return RAIN_HEAVY;
    return RAIN_MEDIUM;
}

alert_color_t alert_color_for_rain(int rain_pct)
{
    switch (alert_level_rain(rain_pct))
    {
    case RAIN_NONE:   return ALERT_LED_GREEN;
    case RAIN_MEDIUM: return ALERT_LED_YELLOW;
    case RAIN_HEAVY:  return ALERT_LED_RED;
    default:          return ALERT_LED_OFF;
    }
}

temp_alert_t alert_level_temp(float temp_c)
{
    return (temp_c > kTempHighThresholdC) ? TEMP_HIGH : TEMP_LOW;
}

alert_color_t alert_color_for_temp(float temp_c)
{
    return (alert_level_temp(temp_c) == TEMP_HIGH) ? ALERT_LED_RED : ALERT_LED_GREEN;
}

void alert_init(void)
{
    s_rain_color = ALERT_LED_GREEN;
    s_temp_color = ALERT_LED_GREEN;
    s_rain_level = RAIN_NONE;
    s_temp_level = TEMP_LOW;
}

void alert_eval_and_log(float temp_c, int rain_pct)
{
    rain_alert_t r_new = alert_level_rain(rain_pct);
    alert_color_t rc_new = alert_color_for_rain(rain_pct);
    temp_alert_t t_new = alert_level_temp(temp_c);
    alert_color_t tc_new = alert_color_for_temp(temp_c);

    if (r_new != s_rain_level)
    {
        s_rain_level = r_new;
        s_rain_color = rc_new;
        const char *lvl = (r_new == RAIN_NONE) ? "SEM CHUVA" : (r_new == RAIN_MEDIUM) ? "CHUVA MEDIA" : "CHUVA FORTE";
        logbuf_add(LOG_LVL_INFO, "ALERT", lvl);
    }

    if (t_new != s_temp_level)
    {
        s_temp_level = t_new;
        s_temp_color = tc_new;
        const char *lvl = (t_new == TEMP_HIGH) ? "TEMPERATURA ALTA" : "TEMPERATURA BAIXA";
        logbuf_add(LOG_LVL_INFO, "ALERT", lvl);
    }
}

alert_color_t alert_get_rain_color(void) { return s_rain_color; }
alert_color_t alert_get_temp_color(void) { return s_temp_color; }

const char *alert_color_str(alert_color_t c)
{
    switch (c)
    {
    case ALERT_LED_GREEN: return "GREEN";
    case ALERT_LED_YELLOW: return "YELLOW";
    case ALERT_LED_RED: return "RED";
    case ALERT_LED_OFF: return "OFF";
    default: return "OFF";
    }
}

const char *alert_rain_str(rain_alert_t a)
{
    switch (a)
    {
    case RAIN_NONE: return "SEM_CHUVA";
    case RAIN_MEDIUM: return "CHUVA_MEDIA";
    case RAIN_HEAVY: return "CHUVA_FORTE";
    default: return "SEM_CHUVA";
    }
}

const char *alert_temp_str(temp_alert_t a)
{
    switch (a)
    {
    case TEMP_LOW: return "TEMP_BAIXA";
    case TEMP_HIGH: return "TEMP_ALTA";
    default: return "TEMP_BAIXA";
    }
}

