#include <stdarg.h>
#include <stdio.h>

#include "Enclave.h"
#include "Enclave_t.h"

void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

#include <string.h>

#define MAX_RECORDS 32
#define RECORD_MAX_LEN 128

static char g_records[MAX_RECORDS][RECORD_MAX_LEN] = {
    "github.com | rodion | S3cr3t!github",
    "gitlab.com | rodion | gl_p@ss_2026",
    "mospolytech.ru | chikita | study#2026",
    "mail.ru | r.chikita | m@ilPass_09",
    "yandex.ru | chikita.r | y@ndex!key",
};

static int g_record_count = 5;

int ecall_get_count(void)
{
    return g_record_count;
}

int ecall_add_record(const char* text)
{
    if (g_record_count >= MAX_RECORDS || text == NULL) {
        return -1;
    }

    size_t len = strlen(text);
    if (len >= RECORD_MAX_LEN) {
        len = RECORD_MAX_LEN - 1;
    }

    memcpy(g_records[g_record_count], text, len);
    g_records[g_record_count][len] = '\0';

    return g_record_count++;
}

int ecall_get_record(int index, char* out_buf, size_t buf_len)
{
    if (index < 0 || index >= g_record_count || out_buf == NULL || buf_len == 0) {
        return -1;
    }

    const char* src = g_records[index];
    size_t len = strlen(src);

    if (len >= buf_len) {
        len = buf_len - 1;
    }

    memcpy(out_buf, src, len);
    out_buf[len] = '\0';
    return (int)len;
}
