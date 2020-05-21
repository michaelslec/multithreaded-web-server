#include <pthread.h>

typedef struct worker_struct {
    int client_sockd;
    pthread_cond_t cond;
} worker_struct_t;

void* dispatcher(void);
void* worker_thread(void* worker);
