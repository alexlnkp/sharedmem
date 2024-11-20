#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#define SHAREDMEM_IMPLEMENTATION
#include "shared_data.h"

int main() {
    key_t key = shared_mem_create_key("SharedMemoryExample", 22);
    shared_mem_t* shm = shared_mem_init(key);

    shared_mem_get(shm, sizeof(struct SharedData));

    if (shm->id == NULL) {
        printf("Could not open file mapping object (%d).\n", GetLastError());
        return 1;
    }

    shared_mem_attach(shm);
    struct SharedData* data = shm->data;

    if (data == NULL) {
        printf("Could not map view of file (%d).\n", GetLastError());
        shared_mem_detach(shm);
        return 1;
    }

    while (1) {
        WaitForSingleObject(data->mutex, INFINITE); /* lock mutex */
        printf("Counter from second process: %d\n", data->counter);
        ReleaseMutex(data->mutex); /* unlock mutex */
        Sleep(1000); /* sleep for a second */
    }

    shared_mem_detach(shm);
    shared_mem_remove(shm);
    return 0;
}
