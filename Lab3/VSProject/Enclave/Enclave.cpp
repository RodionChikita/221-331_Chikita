/*
 *   Copyright(C) 2011-2018 Intel Corporation All Rights Reserved.
 *
 *   The source code, information  and  material ("Material") contained herein is
 *   owned  by Intel Corporation or its suppliers or licensors, and title to such
 *   Material remains  with Intel Corporation  or its suppliers or licensors. The
 *   Material  contains proprietary information  of  Intel or  its  suppliers and
 *   licensors. The  Material is protected by worldwide copyright laws and treaty
 *   provisions. No  part  of  the  Material  may  be  used,  copied, reproduced,
 *   modified, published, uploaded, posted, transmitted, distributed or disclosed
 *   in any way  without Intel's  prior  express written  permission. No  license
 *   under  any patent, copyright  or  other intellectual property rights  in the
 *   Material  is  granted  to  or  conferred  upon  you,  either  expressly,  by
 *   implication, inducement,  estoppel or  otherwise.  Any  license  under  such
 *   intellectual  property  rights must  be express  and  approved  by  Intel in
 *   writing.
 *
 *   *Third Party trademarks are the property of their respective owners.
 *
 *   Unless otherwise  agreed  by Intel  in writing, you may not remove  or alter
 *   this  notice or  any other notice embedded  in Materials by Intel or Intel's
 *   suppliers or licensors in any way.
 *
 */

#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

/* ===================== ЛР3: защищённое хранилище в анклаве ===================== */
#include <string.h>

/* Защищённая таблица данных. Имитирует массив учётных записей.
   Изменяемый буфер фиксированной ёмкости (не const), чтобы ecall_add_record()
   мог дописывать новые записи ВНУТРИ анклава. */
#define MAX_RECORDS 32
#define RECORD_MAX_LEN 128

static char g_records[MAX_RECORDS][RECORD_MAX_LEN] = {
    "github.com | rodion | S3cr3t!github",
    "gitlab.com | rodion | gl_p@ss_2026",
    "mospolytech.ru | chikita | study#2026",
    "mail.ru | r.chikita | m@ilPass_09",
    "yandex.ru | chikita.r | y@ndex!key",
};

static int g_record_count = 5;   /* число изначально заполненных строк выше */

/* Возвращает количество записей в таблице (включая добавленные позже). */
int ecall_get_count(void)
{
    return g_record_count;
}

/*
 * Добавляет новую запись в конец таблицы. Строка text пришла снаружи анклава
 * (untrusted), копируется во внутренний (trusted) буфер g_records — с этого
 * момента она защищена так же, как и исходные записи.
 * Возвращает индекс новой записи, либо -1, если таблица заполнена.
 */
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
