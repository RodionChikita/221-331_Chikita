/*
 * ЛР3, Этапы 2–3. Клиентское приложение, интегрированное с анклавом Intel SGX.
 *
 * Хранилище данных и логика запроса вынесены в анклав (Enclave/Enclave.cpp).
 * Это приложение (untrusted часть) лишь:
 *   1) создаёт анклав из подписанной библиотеки *.signed.dll,
 *   2) по введённому пользователем индексу вызывает ECALL ecall_get_record(),
 *   3) печатает полученную запись,
 *   4) выгружает анклав при завершении.
 *
 * Заголовок Enclave_u.h и мост Enclave_u.c генерируются sgx_edger8r из
 * Enclave.edl при импорте анклава в проект приложения (команда контекстного
 * меню Visual Studio "Import Enclave"). Собирать в конфигурации Simulation / x64.
 */
#include <tchar.h>
#include <cstdio>

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
        printf("App: ошибка %#x — не удалось создать анклав.\n", ret);
        return -1;
    }

    /* Узнаём число записей через ECALL. Результат ECALL возвращается через
       выходной параметр retval, сам вызов возвращает статус SGX. */
    int count = 0;
    ret = ecall_get_count(eid, &count);
    if (ret != SGX_SUCCESS) {
        printf("App: ошибка ECALL ecall_get_count (%#x)\n", ret);
        sgx_destroy_enclave(eid);
        return -1;
    }

    printf("ЛР3 (SGX, Simulation). Записей в защищённой таблице: %d\n", count);
    printf("Введите номер записи (или отрицательное число для выхода):\n");

    int index = 0;
    while (true) {
        printf("index> ");
        if (scanf_s("%d", &index) != 1) {
            break;
        }
        if (index < 0) {
            break;
        }

        /* 2. Запрос записи у анклава. Открытый текст записи существует только
           в этом буфере и только после явного ECALL. */
        char record[256] = { 0 };
        int len = -1;
        ret = ecall_get_record(eid, &len, index, record, sizeof(record));
        if (ret != SGX_SUCCESS) {
            printf("App: ошибка ECALL ecall_get_record (%#x)\n", ret);
            break;
        }

        if (len < 0) {
            printf("  [!] запись с номером %d не найдена (0..%d)\n", index, count - 1);
        } else {
            printf("  -> %s\n", record);
        }
    }

    /* 3. Выгрузка анклава. */
    if (sgx_destroy_enclave(eid) != SGX_SUCCESS) {
        printf("App: предупреждение — не удалось корректно выгрузить анклав.\n");
        return -1;
    }

    printf("Выход. Анклав выгружен.\n");
    return 0;
}
