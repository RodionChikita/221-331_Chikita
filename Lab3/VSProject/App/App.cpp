#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <cstdlib>

#ifdef _MSC_VER
# include <Shlobj.h>
#else
# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX
#endif

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug;
} sgx_errlist_t;

static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
    {
        SGX_ERROR_NDEBUG_ENCLAVE,
        "The enclave is signed as product enclave, and can not be created as debuggable enclave.",
        NULL
    },
};

void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }

    if (idx == ttl)
    	printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

int initialize_enclave(void)
{
    char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;

#ifdef _MSC_VER

    if (S_OK != SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, token_path)) {
        strncpy_s(token_path, _countof(token_path), TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    } else {
        strncat_s(token_path, _countof(token_path), "\\" TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+2);
    }

    HANDLE token_handler = CreateFileA(token_path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, NULL, NULL);
    if (token_handler == INVALID_HANDLE_VALUE) {
        printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
    } else {

        DWORD read_num = 0;
        ReadFile(token_handler, token, sizeof(sgx_launch_token_t), &read_num, NULL);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {

            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }
#else

    const char *home_dir = getpwuid(getuid())->pw_dir;

    if (home_dir != NULL &&
        (strlen(home_dir)+strlen("/")+sizeof(TOKEN_FILENAME)+1) <= MAX_PATH) {

        strncpy(token_path, home_dir, strlen(home_dir));
        strncat(token_path, "/", strlen("/"));
        strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME)+1);
    } else {

        strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    }

    FILE *fp = fopen(token_path, "rb");
    if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
        printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
    }

    if (fp != NULL) {

        size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {

            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }
#endif

    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
#ifdef _MSC_VER
        if (token_handler != INVALID_HANDLE_VALUE)
            CloseHandle(token_handler);
#else
        if (fp != NULL) fclose(fp);
#endif
        return -1;
    }

#ifdef _MSC_VER
    if (updated == FALSE || token_handler == INVALID_HANDLE_VALUE) {

        if (token_handler != INVALID_HANDLE_VALUE)
            CloseHandle(token_handler);
        return 0;
    }

    FlushFileBuffers(token_handler);

    SetFilePointer(token_handler, 0, NULL, FILE_BEGIN);

    DWORD write_num = 0;
    WriteFile(token_handler, token, sizeof(sgx_launch_token_t), &write_num, NULL);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
    CloseHandle(token_handler);
#else
    if (updated == FALSE || fp == NULL) {

        if (fp != NULL) fclose(fp);
        return 0;
    }

    fp = freopen(token_path, "wb", fp);
    if (fp == NULL) return 0;
    size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
    fclose(fp);
#endif
    return 0;
}

void ocall_print_string(const char *str)
{

    printf("%s", str);
}

int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    if (initialize_enclave() < 0) {
        printf("App: failed to create enclave.\n");
        return -1;
    }

    sgx_status_t ret = SGX_SUCCESS;
    int count = 0;
    ret = ecall_get_count(global_eid, &count);
    if (ret != SGX_SUCCESS) {
        printf("App: ECALL ecall_get_count error (%#x)\n", ret);
        sgx_destroy_enclave(global_eid);
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

        size_t linelen = strlen(line);
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            line[--linelen] = '\0';
        }
        if (linelen == 0) {
            continue;
        }

        if (strncmp(line, "add ", 4) == 0) {

            const char* text = line + 4;
            int newIndex = -1;
            ret = ecall_add_record(global_eid, &newIndex, text);
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

        char record[256] = { 0 };
        int len = -1;
        ret = ecall_get_record(global_eid, &len, index, record, sizeof(record));
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

    if (sgx_destroy_enclave(global_eid) != SGX_SUCCESS) {
        printf("App: warning -- failed to cleanly destroy enclave.\n");
        return -1;
    }

    printf("Exit. Enclave unloaded.\n");
    return 0;
}
