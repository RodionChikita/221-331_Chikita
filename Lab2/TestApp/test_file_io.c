/*
 * ЛР2. Тестовое приложение для проверки работы драйвер-фильтра прозрачного шифрования.
 *
 * Приложение намеренно использует только функции стандартной библиотеки Си
 * (fopen_s/fread/fwrite/fseek/fclose), которые внутри оборачивают системные вызовы
 * WinAPI ReadFile()/WriteFile(). Именно эти вызовы перехватывает минифильтр-драйвер.
 * Такие механизмы, как memory-mapped files (их использует, например, notepad.exe),
 * драйвером не обрабатываются — это допущение лабораторной работы.
 *
 * Условности лабораторной реализации:
 *   - весь файл читается/записывается за один вызов, без разбивки на буферы;
 *   - размер буфера (и файла) фиксирован константой BUFFER_SIZE и кратен 16 байтам
 *     (размер блока AES), поэтому паддинг не требуется.
 *
 * Сборка:  cl /W4 test_file_io.c           (из x64 Native Tools Command Prompt)
 *     или:  gcc -Wall -o test_file_io.exe test_file_io.c
 *
 * Использование:
 *   test_file_io.exe write <файл> <текст>   — записать текст в файл (драйвер шифрует)
 *   test_file_io.exe read  <файл>           — прочитать файл (драйвер расшифровывает)
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

/* Фиксированный размер файла/буфера, кратный размеру блока AES (16 байт). */
#define BUFFER_SIZE 64

/* Записать ровно BUFFER_SIZE байт в файл. Возвращает 0 при успехе. */
static int write_fixed(const char *path, const char *text)
{
    FILE *f = NULL;
    unsigned char buffer[BUFFER_SIZE];

    /* Заполняем буфер: сначала текст, затем нули-дополнение до BUFFER_SIZE. */
    memset(buffer, 0, sizeof(buffer));
    strncpy((char *)buffer, text, sizeof(buffer) - 1);

    if (fopen_s(&f, path, "wb") != 0 || f == NULL) {
        printf("[test] не удалось открыть '%s' для записи\n", path);
        return 1;
    }

    /* Запись всего буфера за один вызов — сработает PtPreOperationPassThrough (IRP_MJ_WRITE). */
    if (fwrite(buffer, 1, sizeof(buffer), f) != sizeof(buffer)) {
        printf("[test] ошибка записи\n");
        fclose(f);
        return 1;
    }

    fclose(f);
    printf("[test] записано %d байт в '%s'\n", BUFFER_SIZE, path);
    return 0;
}

/* Прочитать ровно BUFFER_SIZE байт из файла и вывести содержимое в двух видах. */
static int read_fixed(const char *path)
{
    FILE *f = NULL;
    unsigned char buffer[BUFFER_SIZE];
    size_t got;
    size_t i;

    if (fopen_s(&f, path, "rb") != 0 || f == NULL) {
        printf("[test] не удалось открыть '%s' для чтения\n", path);
        return 1;
    }

    fseek(f, 0, SEEK_SET);

    /* Чтение всего буфера за один вызов — сработает PtPostOperationPassThrough (IRP_MJ_READ). */
    got = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);

    printf("[test] прочитано %zu байт из '%s'\n", got, path);

    /* Текстовое представление (то, что видит пользователь). */
    printf("[test] как текст : \"");
    for (i = 0; i < got; ++i) {
        unsigned char c = buffer[i];
        putchar((c >= 32 && c < 127) ? c : '.');
    }
    printf("\"\n");

    /* Шестнадцатеричное представление первых 32 байт (видно, зашифровано ли на диске). */
    printf("[test] как hex   : ");
    for (i = 0; i < got && i < 32; ++i) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc >= 4 && strcmp(argv[1], "write") == 0) {
        return write_fixed(argv[2], argv[3]);
    }
    if (argc >= 3 && strcmp(argv[1], "read") == 0) {
        return read_fixed(argv[2]);
    }

    printf("Использование:\n");
    printf("  %s write <файл> <текст>   — записать (драйвер зашифрует на диске)\n", argv[0]);
    printf("  %s read  <файл>           — прочитать (драйвер расшифрует в буфер)\n", argv[0]);
    return 1;
}
