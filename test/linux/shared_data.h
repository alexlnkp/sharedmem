#ifndef   __SHARED_DATA_H__
#define   __SHARED_DATA_H__

#include "sharedmem.h"

struct SharedData {
    int counter; /* something that we'll share between programs */
    shared_mutex_t mutex; /* mutex for sync */
};

#endif /* __SHARED_DATA_H__ */
