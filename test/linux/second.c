#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#define SHAREDMEM_IMPLEMENTATION
#include "sharedmem.h"

#include "shared_data.h"

/* yes those are global vars, fight me */
shared_mem_t* shm;
struct SharedData *data;
int shmid;

void cleanup() {
    if (data) {
        pthread_mutex_destroy(&data->mutex);
        shared_mem_detach(shm); /* detach from shared memory */
        shared_mem_remove(shm); /* remove shared memory */
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
    key_t key = shared_mem_create_key("shmfile", 65); /* create a unique key */
    shm = shared_mem_init(key, 0b110100000);

    shared_mem_create(shm, sizeof(struct SharedData)); /* create shared memory */

    /* attach to the shared memory */
    shared_mem_attach(shm);
    data = shm->data;

    /* init shared data */
    data->counter = 0;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&data->mutex, &attr);

    while (1) {
        /* put a lock before modifying */
        pthread_mutex_lock(&data->mutex); {
            data->counter++;
            printf("Counter: %d\n", data->counter);

        } pthread_mutex_unlock(&data->mutex);

        sleep(1);
    }

    /* unreachable */
    cleanup();
    return 0;
}
