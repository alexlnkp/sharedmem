#ifndef   __SHARED_MEM_H__
#define   __SHARED_MEM_H__

#if defined(_WIN32) || defined(WIN32)
  #define IS_WINDOWS
  #include <windows.h>

  typedef HANDLE __mutex_handle;
  typedef HANDLE __shared_memory_id;

  #if !defined(__key_t_defined)
    typedef const char* key_t;
  #endif

#elif defined(__unix__) || defined(unix)
  #define IS_LINUX
  #include <sys/shm.h>
  #include <pthread.h>

  typedef pthread_mutex_t __mutex_handle;
  typedef int __shared_memory_id;

  #if !defined(__key_t_defined)
    typedef int key_t;
  #endif

#endif

typedef struct _st_shared_mem shared_mem_t;

enum shared_mem_modes {
    SM_MODE_QUERY       = 1 << 0,
    SM_MODE_WRITE       = 1 << 1,
    SM_MODE_READ        = 1 << 2,
    SM_MODE_EXECUTE     = 1 << 3,
    SM_MODE_EXTEND_SIZE = 1 << 4, /* wtf does this one even do?? */
    SM_MODE_XEXECUTE    = 1 << 5, /* execute explicit */
    SM_MODE_STANDARD    = 0x000F0000, /* standard rights for windows bullshit */
    SM_MODE_FULL_ACCESS = SM_MODE_STANDARD |
                          SM_MODE_QUERY    |
                          SM_MODE_WRITE    |
                          SM_MODE_READ     |
                          SM_MODE_EXECUTE  |
                          SM_MODE_EXTEND_SIZE
};

key_t shared_mem_create_key(const char* name, int id);

shared_mem_t *shared_mem_init(key_t key);
void shared_mem_get(shared_mem_t* shm, size_t size);
void shared_mem_create(shared_mem_t* shm, size_t size);
void shared_mem_remove(shared_mem_t* shm);

void shared_mem_attach(shared_mem_t* shm);
void shared_mem_detach(shared_mem_t* shm);

#ifdef    SHAREDMEM_IMPLEMENTATION

struct _st_shared_mem {
    __shared_memory_id id;
    key_t key;
    size_t size;
    void* data;
    int perm;
};

key_t shared_mem_create_key(const char* name, int id) {
    key_t key;

  #if defined(IS_WINDOWS)
    key = name; /* TODO: fix id being ignored by (possibly) catting it to name */
  #elif defined(IS_LINUX)
    key = ftok(name, id);
  #endif

    return key;
}

shared_mem_t *shared_mem_init(key_t key) {
    shared_mem_t *shm = malloc(sizeof(shared_mem_t));
    shm->key = key;
    shm->id = 0;
    shm->size = 0;

    return shm;
}

void shared_mem_get(shared_mem_t* shm, size_t size) {
    shm->size = size;

  #if defined(IS_LINUX)
    shm->id = shmget(shm->key, shm->size, 0666);
  #elif defined(IS_WINDOWS)
    shm->id = OpenFileMapping(
        SM_MODE_FULL_ACCESS, // Read access
        FALSE,        // Do not inherit the name
        shm->key      // Name of the mapping object
    );

  #endif
}

void shared_mem_create(shared_mem_t* shm, size_t size) {
    shm->size = size;
  #if defined(IS_LINUX)
    shm->id = shmget(shm->key, size, 0666 | IPC_CREAT);
  #elif defined(IS_WINDOWS)
    shm->id = CreateFileMapping(
        INVALID_HANDLE_VALUE, // Use paging file
        NULL,                 // Default security
        PAGE_READWRITE,      // Read/write access
        0,                   // Maximum object size (high-order DWORD)
        shm->size, // Maximum object size (low-order DWORD)
        shm->key // Name of the mapping object
    );
  #endif
}

void shared_mem_attach(shared_mem_t* shm) {
  #if defined(IS_LINUX)
    shm->data = shmat(shm->id, NULL, 0);
  #elif defined(IS_WINDOWS)
    shm->data = MapViewOfFile(
        shm->id,
        SM_MODE_FULL_ACCESS,
        0,
        0,
        shm->size
    );
  #endif
}

void shared_mem_detach(shared_mem_t* shm) {
  #if defined(IS_LINUX)
    shmdt(shm->data);
  #elif defined(IS_WINDOWS)
    UnmapViewOfFile(shm->data);
  #endif
}

void shared_mem_remove(shared_mem_t* shm) {
  #if defined(IS_LINUX)
    shmctl(shm->id, IPC_RMID, NULL); /* remove shared memory */
  #elif defined(IS_WINDOWS)
    CloseHandle(shm->id);
  #endif
}

#endif /* SHAREDMEM_IMPLEMENTATION */

#endif /* __SHARED_MEM_H__ */
