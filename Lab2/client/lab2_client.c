#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define LAB2_FILE_SIZE 256

static void print_error(const char *operation)
{
    fprintf(stderr, "%s failed (Win32 error %lu).\n", operation, GetLastError());
}

static int read_file(const char *path)
{
    HANDLE file;
    BYTE buffer[LAB2_FILE_SIZE + 1] = {0};
    DWORD bytes_read = 0;

    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        print_error("CreateFile(read)");
        return 1;
    }
    if (!ReadFile(file, buffer, LAB2_FILE_SIZE, &bytes_read, NULL)) {
        print_error("ReadFile");
        CloseHandle(file);
        return 1;
    }
    CloseHandle(file);
    buffer[bytes_read] = '\0';
    printf("Read %lu bytes:\n%s\n", bytes_read, (const char *)buffer);
    return 0;
}

static int write_file(const char *path, const char *text)
{
    HANDLE file;
    BYTE buffer[LAB2_FILE_SIZE] = {0};
    DWORD bytes_written = 0;
    size_t text_length = strlen(text);

    if (text_length >= LAB2_FILE_SIZE) {
        fprintf(stderr, "Text must be shorter than %d bytes.\n", LAB2_FILE_SIZE);
        return 1;
    }
    memcpy(buffer, text, text_length);
    file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        print_error("CreateFile(write)");
        return 1;
    }
    if (!WriteFile(file, buffer, LAB2_FILE_SIZE, &bytes_written, NULL) ||
        bytes_written != LAB2_FILE_SIZE) {
        print_error("WriteFile");
        CloseHandle(file);
        return 1;
    }
    if (!FlushFileBuffers(file)) {
        print_error("FlushFileBuffers");
        CloseHandle(file);
        return 1;
    }
    CloseHandle(file);
    printf("Written %lu bytes to %s.\n", bytes_written, path);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "read") == 0) return read_file(argv[2]);
    if (argc == 4 && strcmp(argv[1], "write") == 0) return write_file(argv[2], argv[3]);
    fprintf(stderr, "Usage:\n  %s read <file.lab2ext>\n  %s write <file.lab2ext> <text>\n", argv[0], argv[0]);
    return 2;
}
