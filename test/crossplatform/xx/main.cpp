#include <iostream>
#include <cstring>
#include <cerrno>

#define SHAREDMEM_IMPLEMENTATION
#include "sharedmem.h"

#include "shared_data.hpp"

int main(void) {
    int __exit_code = 0;

    /* create a unique key for the shared memory */
    shared_key_t key = shared_mem_create_key("shmfile", 65); /* use same key as the main executable */
    shared_mem_t* shm = shared_mem_init(key, SM_PERM_READ | SM_PERM_WRITE);

    shared_mem_get(shm, sizeof(struct SharedData));  /* get shared memory */

    if (shm->id == SM_INVALID_ID) {
        std::cerr << "shared_mem_get failed: " << strerror(errno) << std::endl;
        __exit_code = 2; goto getfail;
    }

{
    /* attach to shared memory */
    shared_mem_attach(shm);
    struct SharedData *data = static_cast<struct SharedData *>(shm->data);

    if (data == SM_INVALID_DATA) {
        std::cerr << "shared_mem_attach failed: " << strerror(errno) << std::endl;
        __exit_code = 1; goto attachfail;
    }

    while (1) {
        /* put a lock before reading */
        shared_mutex_lock(&data->mutex); {
            std::cout << "Counter from second process: " << data->counter << std::endl;

        } shared_mutex_unlock(&data->mutex);

        go_sleep(1);
    }
}

attachfail:
    shared_mem_detach(shm);

getfail:
    shared_mem_destroy(shm);

    return 0;
}