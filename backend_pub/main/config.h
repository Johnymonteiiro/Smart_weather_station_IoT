#pragma once
#include <stdint.h>

typedef struct {
    char ssid[32];
    char pass[64];
    char broker[128];
    int  port;
    char topic[64];
    int  qos;
    char user[64];
    char pass_mqtt[64];
} app_config_t;

// Inicializa NVS e carrega configuração salva (se houver)
void config_init();
bool config_load(app_config_t *out);
bool config_save(const app_config_t *cfg);
bool config_clear();
bool config_has_sta();

// Obtém configuração atual em memória
const app_config_t* config_get();
void config_set(const app_config_t *cfg);

