#pragma once
#include <stddef.h>
#include <stdint.h>

#define LOGBUF_MAX 100

typedef enum
{
    LOG_LVL_INFO = 0,
    LOG_LVL_WARN = 1,
    LOG_LVL_ERROR = 2
} log_level_t;

typedef struct
{
    uint32_t seq;      // sequência incremental
    uint32_t ts_ms;    // timestamp relativo em ms (uptime)
    log_level_t level; // nível
    char tag[16];      // componente
    char msg[96];      // mensagem
} log_entry_t;

// Inicializa o buffer (opcional)
void logbuf_init(void);

// Adiciona uma entrada de log ao buffer
void logbuf_add(log_level_t lvl, const char *tag, const char *msg);

// Serializa todas as entradas em JSON (array de objetos)
// Retorna o número de bytes escritos
int logbuf_to_json(char *out, size_t out_size);

// Exposição opcional para serialização em chunks
extern log_entry_t g_buf[LOGBUF_MAX];
extern uint32_t g_count;
extern uint32_t g_head;
