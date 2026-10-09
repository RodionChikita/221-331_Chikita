#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstring>

static const char* const g_records[] = {
    "github.com | rodion | S3cr3t!github",
    "gitlab.com | rodion | gl_p@ss_2026",
    "mospolytech.ru | chikita | study#2026",
    "mail.ru | r.chikita | m@ilPass_09",
    "yandex.ru | chikita.r | y@ndex!key",
};
static const int g_count = (int)(sizeof(g_records) / sizeof(g_records[0]));

static void print_record(int index)
{
    if (index < 0 || index >= g_count) {
        printf("  [!] запись с номером %d не найдена (0..%d)\n", index, g_count - 1);
        return;
    }
    printf("  -> %s\n", g_records[index]);
}

int main(void)
{
    printf("ЛР3 (этап 1, без SGX). Записей в таблице: %d\n", g_count);
    printf("Введите номер записи (или отрицательное число для выхода):\n");

    int index = 0;
    while (true) {
        printf("index> ");
        if (scanf("%d", &index) != 1) {
            break;
        }
        if (index < 0) {
            break;
        }
        print_record(index);
    }
    printf("Выход.\n");
    return 0;
}
