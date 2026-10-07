/*
 * ЛР3, Этапы 2–3. Клиентское приложение, интегрированное с анклавом Intel SGX.
 *
 * Хранилище данных и логика запроса вынесены в анклав (Enclave/Enclave.cpp).
 * Это приложение (untrusted часть) лишь:
 *   1) создаёт анклав из подписанной библиотеки *.signed.dll,
 *   2) по команде пользователя вызывает ECALL (получить запись по индексу,
 *      либо добавить новую запись) и печатает результат,
 *   3) выгружает анклав при завершении.
 *
 * Консольные команды (вводятся построчно):
 *   <число>        — получить запись с этим индексом (ecall_get_record)
 *   add <текст>    — добавить новую запись в таблицу внутри анклава
 *                     (ecall_add_record), в ответ придёт её индекс
 *   отрицательное число — выход
 *
 * ВАЖНО: добавленные записи живут только в памяти текущего запущенного
 * анклава (пока работает этот процесс). Между отдельными запусками App.exe
 * не сохраняются — persistent-хранение (sealing) не входит в базовое задание.
 *
 * Заголовок Enclave_u.h и мост Enclave_u.c генерируются sgx_edger8r из
 * Enclave.edl при импорте анклава в проект приложения (команда контекстного
 * меню Visual Studio "Import Enclave"). Собирать в конфигурации Simulation / x64.
 */
#include <tchar.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sgx_urts.h"       // функции управления анклавом (create/destroy)
#include "Enclave_u.h"      // автогенерируемые прокси ECALL (ecall_get_record и т.д.)

/* Имя подписанной библиотеки анклава, сгенерированной при сборке проекта Enclave. */
#define ENCLAVE_FILE _T("Enclave.signed.dll")

int main(void)
{
    sgx_enclave_id_t eid = 0;
    sgx_status_t ret = SGX_SUCCESS;
    sgx_launch_token_t token = { 0 };
    int updated = 0;

    /* 1. Активация (создание) анклава. */
    ret = sgx_create_enclave(ENCLAVE_FILE, SGX_DEBUG_FLAG, &token, &updated, &eid, NULL);
    if (ret != SGX_SUCCESS) {
        printf("App: error %#x -- failed to create enclave.\n", ret);
        return -1;
    }

    /* Узнаём число записей через ECALL. Результат ECALL возвращается через
       выходной параметр retval, сам вызов возвращает статус SGX. */
    int count = 0;
    ret = ecall_get_count(eid, &count);
    if (ret != SGX_SUCCESS) {
        printf("App: ECALL ecall_get_count error (%#x)\n", ret);
        sgx_destroy_enclave(eid);
        return -1;
    }

    printf("LR3 (SGX, Simulation). Records in protected table: %d\n", count);
    printf("Commands:\n");
    printf("  <N>          -- get record by index (e.g. 0)\n");
    printf("  add <text>   -- add a new record into the enclave, prints its index\n");
    printf("  negative N   -- exit\n");

    char line[512];
    while (true) {
        printf("cmd> ");
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        /* Убираем завершающий перевод строки. */
        size_t linelen = strlen(line);
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            line[--linelen] = '\0';
        }
        if (linelen == 0) {
            continue;
        }

        if (strncmp(line, "add ", 4) == 0) {
            /* 2a. Добавить запись: текст уходит в анклав, обратно приходит индекс. */
            const char* text = line + 4;
            int newIndex = -1;
            ret = ecall_add_record(eid, &newIndex, text);
            if (ret != SGX_SUCCESS) {
                printf("App: ECALL ecall_add_record error (%#x)\n", ret);
                break;
            }
            if (newIndex < 0) {
                printf("  [!] table is full, could not add\n");
            } else {
                printf("  + added at index %d (now inside the enclave)\n", newIndex);
            }
            continue;
        }

        int index = atoi(line);
        if (index < 0) {
            break;
        }

        /* 2b. Запрос записи у анклава. Открытый текст записи существует только
           в этом буфере и только после явного ECALL. */
        char record[256] = { 0 };
        int len = -1;
        ret = ecall_get_record(eid, &len, index, record, sizeof(record));
        if (ret != SGX_SUCCESS) {
            printf("App: ECALL ecall_get_record error (%#x)\n", ret);
            break;
        }

        if (len < 0) {
            printf("  [!] record %d not found\n", index);
        } else {
            printf("  -> %s\n", record);
        }
    }

    /* 3. Выгрузка анклава. */
    if (sgx_destroy_enclave(eid) != SGX_SUCCESS) {
        printf("App: warning -- failed to cleanly destroy enclave.\n");
        return -1;
    }

    printf("Exit. Enclave unloaded.\n");
    return 0;
}
