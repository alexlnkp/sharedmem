#ifndef   __SHARED_DATA_H__
#define   __SHARED_DATA_H__

#if defined(_WIN32)
  #include <windows.h>
  #define go_sleep(x) Sleep((x) * 1000)
#elif defined(__unix__)
  #include <unistd.h>
  #define go_sleep(x) sleep((x))
#endif

#include "sharedmem.h"

struct SharedData {
    int counter; /* something that we'll share between programs */
    shared_mutex_t mutex; /* mutex for sync */
};

#endif /* __SHARED_DATA_H__ */
