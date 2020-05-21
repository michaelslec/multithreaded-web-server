#include "http.h"
#include "threads.h"

int log_fd = 0;
size_t thread_count = 0;

pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  queue_cond = PTHREAD_COND_INITIALIZER;

void* dispatcher(void* runtime_info)
{
    thread_runtime_t* runtime = (thread_runtime_t*) runtime_info;
    worker_struct_t workers[runtime->thread_count];

    // create the worker pool threads
    for (int i = 0; i < runtime->thread_count; i++) {
        printf("creating thread %d\n", i);
        workers[i].client_sockd = -1;
        workers[i].cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

        int error = pthread_create(&workers[i].thread_fd, NULL, worker_thread,
                (void *) &workers[i]);
        if (error != 0) {
            write(STDERR_FILENO, "Error creating worker thread thread\n", 50);
            exit(EXIT_FAILURE);
        }
    }

    // take client requests from queue, parse request, and send off for processing to next available thread
    while (true) {
        // Wait for main to signal that socket has been queue'd
        int *psock_fd;
        pthread_mutex_lock(&mutex_queue);
        if ((psock_fd = dequeue()) == NULL) {// take next available client
            pthread_cond_wait(&queue_cond, &mutex_queue);
            psock_fd = dequeue();
        }
        pthread_mutex_unlock(&mutex_queue);

        // check if pcleint has a fd to process
        // iterate through pool of thread to find waiting thread
        // unlock mutex and signal thread to work on processed client request

        int loop_reset_sleep = 0;


        /*
         * while we haven't passed of the client_socketd
         *   if worker with thread_fd of -1
         *      give client_sockd to worker
         *      exit while
         */

        if (psock_fd != NULL) {
            int i = 0;
            while (true) {
                if (workers[i].thread_fd == -1) {
                    workers[i].thread_fd = *psock_fd;
                    free(psock_fd);//TODO correct place to free this?

                    int error = pthread_cond_signal(&workers[i].cond);
                    if (error != 0) {
                        write(STDERR_FILENO, "Error signaling thread\n", 50);
                        exit(EXIT_FAILURE);
                    }

                    break;
                }
                ++i; // Increment loop
                // reset i to 0 to iterate from beginning or worker array
                if (i == runtime->thread_count)
                    i = 0;
            }
        }
    }

    return NULL;
}

void* worker_thread(void* worker)
{
    while (true) {
        pthread_mutex_lock(&wait_lock);
        worker_struct_t* todo = (worker_struct_t*) worker;
        if (todo->thread_fd < 0) {
            if (pthread_cond_wait(&todo->cond, &wait_lock)) {
                perror("worker_thread");
            }
        }
        pthread_mutex_unlock(&wait_lock);

        printf("Game time, baby: socket: %ld\n", todo->client_sockd);

        todo->thread_fd = -1;
    }
}


int main(int argc, char** argv) {
    // check for correct number of program arguments
    if (argc == 1) {
        write(STDERR_FILENO, "No port number provided\n", 25);
        exit(EXIT_FAILURE);
    }
    // parse arguments for flags and port number
    extern char *optarg;
    extern int optind;
    int c;
    int Nflag = 0, lflag = 0;
    char *logname, *tcount;

    while ((c = getopt(argc, argv, "N:l:")) != -1) {
        switch (c) {
            case 'N':
                Nflag = 1;
                tcount = optarg;
                break;
            case 'l':
                lflag = 1;
                logname = optarg;
                break;
            case '?':
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (Nflag == 1) {
        thread_count = atoi(tcount);
        if (thread_count < 1) {
            write(STDERR_FILENO, "N must be 1 or greater\n", 25);
            exit(EXIT_FAILURE);
        }
    } else {
        thread_count = 4;
    }

    if (lflag == 0) {
        logname = "";
    }
    else {
        //create log file if flagged
        log_fd = open(logname, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    }

    /*
      Create sockaddr_in with server information
    */
    char *port = argv[optind];
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t addrlen = sizeof(server_addr);

    /*
        Create server socket
    */
    int server_sockd = socket(AF_INET, SOCK_STREAM, 0);

    // Need to check if server_sockd < 0, meaning an error
    if (server_sockd < 0) {
        perror("socket");
    }

    /*
        Configure server socket
    */
    int enable = 1;

    /*
        This allows you to avoid: 'Bind: Address Already in Use' error
    */
    int ret = setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    /*
        Bind server address to socket that is open
    */
    ret = bind(server_sockd, (struct sockaddr *) &server_addr, addrlen);

    /*
        Listen for incoming connections
    */
    ret = listen(server_sockd, 5); // 5 should be enough, if not use SOMAXCONN

    if (ret < 0) {
        return EXIT_FAILURE;
    }

    /*
     * Create dispatch thread
     */

    thread_runtime_t runtime = {log_fd, thread_count};
    pthread_t dispatch_fd;
    int error = pthread_create(&dispatch_fd, NULL, dispatcher, (void*)&runtime);
    if (error < 0) {
        perror("Dispatcher thread");
    }

    /*
        Connecting with a client
    */
    struct sockaddr client_addr;
    socklen_t client_addrlen;

    while (true) {
        printf("[+] server is waiting...\n");

        /*
         * 1. Accept Connection
         */
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);
        int *pclient = malloc(sizeof(int));// TODO ensure this is free() somewhere later on
        *pclient = client_sockd;
        printf("Queuing up socket_fd: %d\n", client_sockd);

        pthread_mutex_lock(&mutex_queue);
        pthread_cond_signal(&queue_cond);
        enqueue(pclient);
        pthread_mutex_unlock(&mutex_queue);
        // Remember errors happen

        /* char* buffer = "HTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\n"; */
        /* write(client_sockd, buffer, strlen(buffer)); */
        /* write(client_sockd, "int main() {}\n", 14); */

        /*
         * 2. Read HTTP Message
         */
        send_response(process_request(read_http_request(client_sockd)));

        close(client_sockd);
    }

    return EXIT_SUCCESS;
}

