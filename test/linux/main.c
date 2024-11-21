#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#define SHAREDMEM_IMPLEMENTATION
#include "shared_data.h"

int main() {
    /* create a unique key for the shared memory */
    key_t key = shared_mem_create_key("shmfile", 65); /* use same key as the main executable */
    shared_mem_t* shm = shared_mem_init(key, 0b100000000);

    shared_mem_get(shm, sizeof(struct SharedData));  /* get shared memory */

    if (shm->id == SM_INVALID_ID) {
        perror("shmget failed");
        exit(1);
    }

    /* attach to shared memory */
    shared_mem_attach(shm);
    struct SharedData *data = shm->data;
    if (data == SM_INVALID_DATA) {
        perror("shmat failed");
        exit(1);
    }

    while (1) {
        /* put a lock before reading */
        shared_mutex_lock(&data->mutex); {
            printf("Counter from second process: %d\n", data->counter);

        } shared_mutex_unlock(&data->mutex);

        sleep(1);
    }

    /* unreachable */
    shared_mem_detach(shm);
    shared_mem_destroy(shm);
    return 0;
}
