/* MIT NON-AI License

Copyright (c) 2024, Alex Murkoff

Permission is hereby granted, free of charge, to any person obtaining a copy of the software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions.

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

In addition, the following restrictions apply:

1. The Software and any modifications made to it may not be used for the purpose of training or improving machine learning algorithms,
including but not limited to artificial intelligence, natural language processing, or data mining. This condition applies to any derivatives,
modifications, or updates based on the Software code. Any usage of the Software in an AI-training dataset is considered a breach of this License.

2. The Software may not be included in any dataset used for training or improving machine learning algorithms,
including but not limited to artificial intelligence, natural language processing, or data mining.

3. Any person or organization found to be in violation of these restrictions will be subject to legal action and may be held liable
for any damages resulting from such use.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

/**********************************
 *************** HEADER BEGINS HERE
 *********
 ****/

#ifndef   __SHARED_MEM_H__
#define   __SHARED_MEM_H__

#if defined(_WIN32) || defined(WIN32)
  /* maybe better to replace this to #define IS_WINDOWS 1
   * so that i can do #if IS_WINDOWS instead of #ifdef IS_WINDOWS */
  #define IS_WINDOWS 1
  #include <windows.h>

  typedef HANDLE __mutex_handle;
  typedef HANDLE __shared_memory_id;

  #if !defined(__key_t_defined)
    typedef const char* key_t;
  #endif

  #define SM_DEFAULT_PERM SM_MODE_FULL_ACCESS
  #define SM_INVALID_ID NULL
  #define SM_INVALID_DATA NULL

#elif defined(__unix__) || defined(unix)
  /* maybe better to replace this to #define IS_LINUX 1
   * so that i can do #if IS_LINUX instead of #ifdef IS_LINUX */
  #define IS_LINUX 1
  #include <sys/shm.h>
  #include <pthread.h>
  #include <stdlib.h>

  typedef pthread_mutex_t __mutex_handle;
  typedef int __shared_memory_id;

  /* this is pretty much unneded */
  #if !defined(__key_t_defined)
    typedef int key_t;
  #endif

  #define SM_DEFAULT_PERM 0b110000000 /* -rw------- */
  #define SM_INVALID_ID -1
  #define SM_INVALID_DATA (void*)-1

#endif

typedef struct _st_shared_mem shared_mem_t;
typedef __mutex_handle shared_mutex_t;

/* TODO: make generic modes that will THEN be transformed to linux or windows modes */
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

shared_mem_t *shared_mem_init(key_t key, int permissions);
void shared_mem_get(shared_mem_t* shm, size_t size);
void shared_mem_create(shared_mem_t* shm, size_t size);
void shared_mem_remove(shared_mem_t* shm);

void shared_mem_attach(shared_mem_t* shm);
void shared_mem_detach(shared_mem_t* shm);
void shared_mem_destroy(shared_mem_t* shm);

void shared_mutex_init(shared_mutex_t *mutex);
void shared_mutex_destroy(shared_mutex_t *mutex);
void shared_mutex_lock(shared_mutex_t *mutex);
void shared_mutex_unlock(shared_mutex_t *mutex);

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

  #if IS_WINDOWS
    key = name; /* TODO: fix id being ignored by (possibly) catting it to name */
  #elif IS_LINUX
    key = ftok(name, id);
  #endif

    return key;
}

shared_mem_t *shared_mem_init(key_t key, int permissions) {
    shared_mem_t *shm = malloc(sizeof(shared_mem_t));
    shm->key = key;
    shm->id = 0;
    shm->size = 0;

    shm->perm = permissions;

    return shm;
}

void shared_mem_get(shared_mem_t* shm, size_t size) {
    shm->size = size; /* setting size for windows (it will use it later) */

    /* TODO: fix a bug where if you restart the sender on linux, the reader doesn't notice
     * and still prints the last value from counter that was there on exit
     */

  #if IS_LINUX
    shm->id = shmget(shm->key, shm->size, shm->perm);
  #elif IS_WINDOWS
    shm->id = OpenFileMapping(shm->perm, FALSE, shm->key);
  #endif
}

void shared_mem_create(shared_mem_t* shm, size_t size) {
    shm->size = size;

  #if IS_LINUX
    shm->id = shmget(shm->key, size, shm->perm | IPC_CREAT);
  #elif IS_WINDOWS
    /* convert perms from FILE_MAP_* to PAGE_* */
    int pageProtection = 0x00;

    if (shm->perm & SM_MODE_WRITE) { pageProtection |= PAGE_READWRITE; }

    if ((shm->perm & SM_MODE_READ) && !(shm->perm & SM_MODE_WRITE)) { pageProtection |= PAGE_READONLY; }

    shm->id = CreateFileMapping(
        INVALID_HANDLE_VALUE, /* Use paging file */
        NULL,                 /* Default security */
        pageProtection,
        0,                    /* Maximum object size (high-order DWORD) */
        shm->size,            /* Maximum object size (low-order DWORD) */
        shm->key              /* Name of the mapping object */
    );

  #endif
}

void shared_mem_attach(shared_mem_t* shm) {
  #if IS_LINUX
    shm->data = shmat(shm->id, NULL, 0);
  #elif IS_WINDOWS
    shm->data = MapViewOfFile(shm->id, shm->perm, 0, 0, shm->size);
  #endif
}

void shared_mem_detach(shared_mem_t* shm) {
  #if IS_LINUX
    shmdt(shm->data);
  #elif IS_WINDOWS
    UnmapViewOfFile(shm->data);
  #endif
}

void shared_mem_remove(shared_mem_t* shm) {
  #if IS_LINUX
    shmctl(shm->id, IPC_RMID, NULL); /* remove shared memory */
  #elif IS_WINDOWS
    CloseHandle(shm->id);
  #endif
}

void shared_mem_destroy(shared_mem_t* shm) {
    shm->key = 0;
    shm->id = 0;
    shm->size = 0;
    shm->perm = 0;
    free(shm);
}

void shared_mutex_init(shared_mutex_t *mutex) {
  #if IS_LINUX
    /* maybe allowing user to change this could be better */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr); /* could be bad, but works so far */
  #elif IS_WINDOWS
    *mutex = CreateMutex(NULL, FALSE, NULL);
  #endif
}

void shared_mutex_destroy(shared_mutex_t *mutex) {
  #if IS_LINUX
    pthread_mutex_destroy(mutex);
  #elif IS_WINDOWS
    CloseHandle(*mutex);
  #endif
}

void shared_mutex_lock(shared_mutex_t *mutex) {
  #if IS_LINUX
    pthread_mutex_lock(mutex);
  #elif IS_WINDOWS
    WaitForSingleObject(*mutex, INFINITE);
  #endif
}

void shared_mutex_unlock(shared_mutex_t *mutex) {
  #if IS_LINUX
    pthread_mutex_unlock(mutex);
  #elif IS_WINDOWS
    ReleaseMutex(*mutex);
  #endif
}

#endif /* SHAREDMEM_IMPLEMENTATION */

#endif /* __SHARED_MEM_H__ */
