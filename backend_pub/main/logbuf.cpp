#include "logbuf.h"
#include "esp_timer.h"
#include <string.h>

log_entry_t g_buf[LOGBUF_MAX];
uint32_t g_count = 0; // total adicionados (para seq)
uint32_t g_head = 0;  // próxima posição para escrever

void logbuf_init(void)
{
    g_count = 0;
    g_head = 0;
    memset(g_buf, 0, sizeof(g_buf));
}

void logbuf_add(log_level_t lvl, const char *tag, const char *msg)
{
    log_entry_t e;
    e.seq = ++g_count;
    e.ts_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    e.level = lvl;
    // Copia segura de tag e msg
    strncpy(e.tag, tag ? tag : "-", sizeof(e.tag) - 1);
    e.tag[sizeof(e.tag) - 1] = '\0';
    strncpy(e.msg, msg ? msg : "-", sizeof(e.msg) - 1);
    e.msg[sizeof(e.msg) - 1] = '\0';

    g_buf[g_head] = e;
    g_head = (g_head + 1) % LOGBUF_MAX;
}

static const char *lvl_str(log_level_t l)
{
    switch (l)
    {
    case LOG_LVL_INFO:
        return "INFO";
    case LOG_LVL_WARN:
        return "WARN";
    case LOG_LVL_ERROR:
        return "ERROR";
    default:
        return "INFO";
    }
}

int logbuf_to_json(char *out, size_t out_size)
{
    // Serializa em um array JSON dos últimos registros (ordem cronológica)
    // Para simplificar, evitamos cJSON e usamos snprintf manualmente.
    if (!out || out_size < 2)
        return 0;
    size_t written = 0;
    out[written++] = '[';

    // Determina quantos itens válidos existem
    uint32_t valid = g_count < LOGBUF_MAX ? g_count : LOGBUF_MAX;
    // Calcula índice do primeiro item cronológico
    uint32_t start = (g_count >= LOGBUF_MAX) ? g_head : 0;

    for (uint32_t i = 0; i < valid; ++i)
    {
        uint32_t idx = (start + i) % LOGBUF_MAX;
        log_entry_t *e = &g_buf[idx];
        char item[256];
        int n = snprintf(item, sizeof(item),
                         "{\"seq\":%lu,\"ts_ms\":%lu,\"level\":\"%s\",\"tag\":\"%s\",\"msg\":\"%s\"}",
                         (unsigned long)e->seq, (unsigned long)e->ts_ms, lvl_str(e->level), e->tag, e->msg);
        // Adiciona vírgula se não for o primeiro
        if (i > 0)
        {
            if (written + 1 >= out_size)
                break;
            out[written++] = ',';
        }
        if (written + (size_t)n >= out_size)
        {
            // Sem espaço: fecha e retorna parcial
            break;
        }
        memcpy(out + written, item, (size_t)n);
        written += (size_t)n;
    }

    if (written + 1 <= out_size)
    {
        out[written++] = ']';
    }
    // Garante terminador (não contado como bytes retornados)
    if (written < out_size)
        out[written] = '\0';
    return (int)written;
}
