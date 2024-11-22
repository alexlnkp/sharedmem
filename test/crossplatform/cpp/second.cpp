#include <iostream>

#include <signal.h>

#define SHAREDMEM_IMPLEMENTATION
#include "sharedmem.h"

#include "shared_data.hpp"

/* yes those are global vars, fight me */
shared_mem_t* shm;
struct SharedData *data;

void cleanup(void) {
    if (!data) return;

    shared_mutex_destroy(&data->mutex);
    shared_mem_detach(shm); /* detach from shared memory */
    shared_mem_remove(shm); /* remove shared memory */
    shared_mem_destroy(shm);
}

void signal_handler(int signum) {
    /**
     * TODO: figure out if there's a way to abstract all this,
     * as it's not really necessary on windows and might bring
     * issues when code works on windows without any tinkering
     * but needs signal handling on linux.
     */
    cleanup();
    exit(signum);
}

int main(void) {
    /* setup signal handling for mutex and shmdt */
    signal(SIGINT, signal_handler);

    /* create a unique key for the shared memory */
    shared_key_t key = shared_mem_create_key("shmfile", 65); /* create a unique key */
    shm = shared_mem_init(key, SM_DEFAULT_PERM);

    shared_mem_create(shm, sizeof(struct SharedData)); /* create shared memory */

    /* attach to the shared memory */
    shared_mem_attach(shm);
    data = static_cast<struct SharedData *>(shm->data);

    /* init shared data */
    data->counter = 0;
    shared_mutex_init(&data->mutex);

    while (1) {
        /* put a lock before modifying */
        shared_mutex_lock(&data->mutex); {
            data->counter++;
            std::cout << "Counter: " << data->counter << std::endl;

        } shared_mutex_unlock(&data->mutex);

        go_sleep(1);
    }

    /* unreachable */
    cleanup();
    return 0;
}
