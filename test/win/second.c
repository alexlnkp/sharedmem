#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#define SHAREDMEM_IMPLEMENTATION
#include "shared_data.h"

int main() {
    /* create named file mapping */
    key_t key = shared_mem_create_key("SharedMemoryExample", 22);
    shared_mem_t *shm = shared_mem_init(key);

    shared_mem_create(shm, sizeof(struct SharedData));

    if (shm->id == NULL) {
        printf("Could not create file mapping object (%d).\n", GetLastError());
        return 1;
    }

    /* map view of file into address space */
    shared_mem_attach(shm);

    struct SharedData* data = shm->data;

    if (data == NULL) {
        printf("Could not map view of file (%d).\n", GetLastError());
        shared_mem_detach(shm);
        return 1;
    }

    /* init data that will be shared */
    data->counter = 0;
    data->mutex = CreateMutex(NULL, FALSE, "SharedMutex");

    while (1) {
        WaitForSingleObject(data->mutex, INFINITE); /* lock mutex */
        data->counter++;
        printf("Counter: %d\n", data->counter);
        ReleaseMutex(data->mutex); /* unlock mutex */
        Sleep(1000); /* sleep for 1 second */
    }

    /* unreachable */
    shared_mem_remove(shm);
    shared_mem_detach(shm);
    return 0;
}
