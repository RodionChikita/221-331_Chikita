#include <tchar.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sgx_urts.h"
#include "Enclave_u.h"

#define ENCLAVE_FILE _T("Enclave.signed.dll")

int main(void)
{
    sgx_enclave_id_t eid = 0;
    sgx_status_t ret = SGX_SUCCESS;
    sgx_launch_token_t token = { 0 };
    int updated = 0;

    ret = sgx_create_enclave(ENCLAVE_FILE, SGX_DEBUG_FLAG, &token, &updated, &eid, NULL);
    if (ret != SGX_SUCCESS) {
        printf("App: error %#x -- failed to create enclave.\n", ret);
        return -1;
    }

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

    if (sgx_destroy_enclave(eid) != SGX_SUCCESS) {
        printf("App: warning -- failed to cleanly destroy enclave.\n");
        return -1;
    }

    printf("Exit. Enclave unloaded.\n");
    return 0;
}
