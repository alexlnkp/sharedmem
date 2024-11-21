#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <pthread.h>

#define SHAREDMEM_IMPLEMENTATION
#include "sharedmem.h"

#include "shared_data.h"

/* yes those are global vars, fight me */
shared_mem_t* shm;
struct SharedData *data;
int shmid;

void cleanup() {
    if (data) {
        shared_mutex_destroy(&data->mutex);
        shared_mem_detach(shm); /* detach from shared memory */
        shared_mem_remove(shm); /* remove shared memory */
        shared_mem_destroy(shm);
    }
}

void signal_handler(int signum) {
    cleanup();
    exit(signum);
}

int main() {
    /* setup signal handling for mutex and shmdt */
    signal(SIGINT, signal_handler);

    /* create a unique key for the shared memory */
    shared_key_t key = shared_mem_create_key("shmfile", 65); /* create a unique key */
    shm = shared_mem_init(key, 0b110100000);

    shared_mem_create(shm, sizeof(struct SharedData)); /* create shared memory */

    /* attach to the shared memory */
    shared_mem_attach(shm);
    data = shm->data;

    /* init shared data */
    data->counter = 0;
    shared_mutex_init(&data->mutex);

    while (1) {
        /* put a lock before modifying */
        shared_mutex_lock(&data->mutex); {
            data->counter++;
            printf("Counter: %d\n", data->counter);

        } shared_mutex_unlock(&data->mutex);

        sleep(1);
    }

    /* unreachable */
    cleanup();
    return 0;
}
