#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 64

static int write_fixed(const char *path, const char *text)
{
    FILE *f = NULL;
    unsigned char buffer[BUFFER_SIZE];

    memset(buffer, 0, sizeof(buffer));
    strncpy((char *)buffer, text, sizeof(buffer) - 1);

    if (fopen_s(&f, path, "wb") != 0 || f == NULL) {
        printf("[test] failed to open '%s' for writing\n", path);
        return 1;
    }

    if (fwrite(buffer, 1, sizeof(buffer), f) != sizeof(buffer)) {
        printf("[test] write error\n");
        fclose(f);
        return 1;
    }

    fclose(f);
    printf("[test] wrote %d bytes to '%s'\n", BUFFER_SIZE, path);
    return 0;
}

static int read_fixed(const char *path)
{
    FILE *f = NULL;
    unsigned char buffer[BUFFER_SIZE];
    size_t got;
    size_t i;

    if (fopen_s(&f, path, "rb") != 0 || f == NULL) {
        printf("[test] failed to open '%s' for reading\n", path);
        return 1;
    }

    fseek(f, 0, SEEK_SET);

    got = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);

    printf("[test] read %zu bytes from '%s'\n", got, path);

    printf("[test] as text : \"");
    for (i = 0; i < got; ++i) {
        unsigned char c = buffer[i];
        putchar((c >= 32 && c < 127) ? c : '.');
    }
    printf("\"\n");

    printf("[test] as hex  : ");
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

    printf("Usage:\n");
    printf("  %s write <file> <text>   -- write (driver encrypts on disk)\n", argv[0]);
    printf("  %s read  <file>          -- read (driver decrypts into buffer)\n", argv[0]);
    return 1;
}
