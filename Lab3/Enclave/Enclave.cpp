/*
 * ЛР3. Реализация доверенной части (анклав Intel SGX).
 *
 * Здесь объявлено защищённое хранилище данных (таблица учётных записей) и
 * реализованы ECALL-функции, описанные в Enclave.edl. Таблица располагается
 * в памяти анклава, которая шифруется аппаратно (Memory Encryption Engine) и
 * недоступна ни ОС, ни другим процессам, ни при снятии дампа памяти.
 *
 * Условность лабораторной работы: данные объявлены статически прямо в коде, из-за
 * чего видны при реверс-инжиниринге *.signed.dll. В production защищаемые данные
 * должны передаваться в анклав в зашифрованном виде и расшифровываться только
 * внутри него (см. доп. задание SealUnseal и вопрос №7 к защите).
 */
#include "Enclave_t.h"   // автогенерируется sgx_edger8r из Enclave.edl
#include <string.h>

/* Защищённая таблица данных. Имитирует массив учётных записей. */
static const char* const g_records[] = {
    "github.com | rodion | S3cr3t!github",
    "gitlab.com | rodion | gl_p@ss_2026",
    "mospolytech.ru | chikita | study#2026",
    "mail.ru | r.chikita | m@ilPass_09",
    "yandex.ru | chikita.r | y@ndex!key",
};

static const int g_record_count = (int)(sizeof(g_records) / sizeof(g_records[0]));

/* Возвращает количество записей в таблице. */
int ecall_get_count(void)
{
    return g_record_count;
}

/*
 * Копирует строку с индексом index во внешний буфер out_buf размера buf_len.
 * Возвращает длину строки или -1, если индекс вне диапазона. Данные покидают
 * анклав только по явному запросу приложения и только для конкретной записи.
 */
int ecall_get_record(int index, char* out_buf, size_t buf_len)
{
    if (index < 0 || index >= g_record_count || out_buf == NULL || buf_len == 0) {
        return -1;
    }

    const char* src = g_records[index];
    size_t len = strlen(src);

    /* Обрезаем по размеру буфера приложения, оставляя место под '\0'. */
    if (len >= buf_len) {
        len = buf_len - 1;
    }

    memcpy(out_buf, src, len);
    out_buf[len] = '\0';
    return (int)len;
}
