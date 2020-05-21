#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include "http.h"
#include "queue.h"

typedef struct worker_struct {
    ssize_t client_sockd;
    pthread_cond_t cond;
    pthread_t thread_fd;
} worker_struct_t;

typedef struct thread_runtime {
    int log_fd;
    size_t thread_count;
} thread_runtime_t;

void* dispatcher(void* runtime_info);
void* worker_thread(void* worker);

#endif
