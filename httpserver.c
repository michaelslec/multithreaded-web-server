#include "queue.h"//linked list implementation
#include <ctype.h>//getopt
#include <err.h>  // warn
#include <errno.h>//perror
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>//threads
#include <stdbool.h>// true, false
#include <stdio.h>
#include <stdlib.h>// atoi
#include <string.h>// memset
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>// write

// orginal buffer size - can be used by dispatcher
//#define BUFFER_SIZE 4096

// reduced worker buffer that I'm using for dispatcher as well for simplicity
#define BUFFER_SIZE 2048

// with help from:
// https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html
// http://www.cs.kent.edu/~ruttan/sysprog/lectures/multi-thread/thread-pool-server.c
// Michael Covarr's section videos/demo.c files

pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_variable_queue = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_file = PTHREAD_MUTEX_INITIALIZER;// TODO determine if you want this globally or have every worker have a copy
                                                       // in their struct
//pthread_cond_t condition_variable_file = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutex_pool;
//pthread_cond_t condition_variable_pool;

int data_ready;
int log_fd;
extern int errno;

void enqueue(int *client_sockd);
int *dequeue();

struct httpResponse {
    int status_code;
    char response[BUFFER_SIZE];
};

struct httpObject {
    char method[5];        // PUT, HEAD, GET
    char filename[28];     // what is the file we are worried about
    char httpversion[9];   // HTTP/1.1
    ssize_t content_length;// example: 13
    uint8_t buffer[BUFFER_SIZE];
    int fd;// file descriptor of file being opened by GET function
};

struct dispatcherStruct {
    int lflag;
    int thread_count;
    pthread_t thread_id;
};

struct workerStruct {
    int thread_fd;
    int thread_number;
    int lflag;
    struct httpResponse *response;
    struct httpObject *message;
    pthread_t thread_id;
    pthread_cond_t condition_variable;
    pthread_mutex_t *lock;//using global mutex TODO
    ssize_t *poffset;
    //ssize_t incr;
    ssize_t i;
    int count;
};

int countdigits(int num) {
    int count = 0;

    do
    {
        count++;
        num /= 10;
    } while(num != 0);

    return count;

}

void logger(struct workerStruct* wStruct, ssize_t* offset) {
    printf("log_fd = %d\n", log_fd);
    char buff[70];
    char chars[BUFFER_SIZE];
    ssize_t incr;
    struct httpObject *message = wStruct->message;
    strcpy(chars, message->buffer);
    if(wStruct->response->status_code == 400) {
        incr = snprintf(buff, 70, "FAIL: %s /%s HTTP/1.1 --- response %d\n========\n", message->method, message->filename,  wStruct->response->status_code);
        pwrite(log_fd, buff, incr, *offset);
    }
    else {
        if(wStruct->i == 0){
            printf("log init\n");
            incr = snprintf(buff, 70, "%s /%s length %ld\n", message->method,  message->filename,  message->content_length);
            pwrite(log_fd, buff, incr, *offset);
            offset += incr;
            wStruct->count = 0;
        }
        incr = snprintf(buff, 70, "%s /%s length %ld\n", message->method,  message->filename,  message->content_length);
        pwrite(log_fd, buff, incr, *offset);
        offset += incr;
        wStruct->count = 0;
        //////
        int count = wStruct->count;
        printf("i = %ld\n", wStruct->i);
        printf("i + buf = %ld\n", (BUFFER_SIZE +  wStruct->i));

        char * c = &chars;

        for (ssize_t i = wStruct->i; i < message->content_length - wStruct->i; i++) {
            printf("%c", c[i]);
            //printf("made it to for loop\n");
            if ((i % 20) == 0) {
                if (i != 0) {
                    incr = snprintf(buff, 70, "\n");
                    pwrite(log_fd, buff, incr, *offset);
                    *offset += incr;
                }
                incr = snprintf(buff, 70, "%07d", count);
                pwrite(log_fd, buff, incr, *offset);
                *offset += incr;
                count +=2;
                incr = snprintf(buff, 70, "0");
                pwrite(log_fd, buff, incr, *offset);
                *offset += incr;
            }
            incr = snprintf(buff, 70, " %02x", c[i]);
            pwrite(log_fd, buff, incr, *offset);
            *offset += incr;
        }
        incr = snprintf(buff, 70, "\n========\n");
        pwrite(log_fd, buff, incr, *offset);
        *offset += incr;
    }
    printf("\n");
}

int read_http_response(struct workerStruct *wStruct) {
    int client_sockd = wStruct->thread_fd;
    struct httpObject *message = wStruct->message;

    printf("Worker %d reading client response...\n", wStruct->thread_number);

    memset(message->buffer, '\0', BUFFER_SIZE);

    int res = recv(client_sockd, message->buffer, BUFFER_SIZE, 0);
    if (res < 0) {
        return 400;
    }

    // read in http header
    sscanf(message->buffer, "%s%*[ /]%s%*[ HTTP/]%s", message->method, message->filename,
           message->httpversion);
    char con_len[10];
    sscanf(message->buffer, "%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%s", con_len);
    int con_int = atoi(con_len);
    message->content_length = (ssize_t) con_int;

    return 1;
}

int check_filename(char *filename) {
    /*
      iterate over each char in filename, convert to ascii value
      compare valid ascii values, return 0 if char not in whitelist
      return 0 if filename longer than 27 char
    */

    if (strlen(filename) > 27) {
        return 0;
    }

    for (int i = 0; filename[i] != '\0'; i++) {
        if ((64 < filename[i] && filename[i] <= 90) || (96 < filename[i] && filename[i] <= 122) || (47 < filename[i] && filename[i] <= 58) || filename[i] == 45 || filename[i] == 95) {

        } else {
            return 0;
        }
    }

    return 1;
}

// TODO: Change to int, struct workerStruct
void put_request(struct workerStruct *wStruct) {
    int client_sockd = wStruct->thread_fd;
    struct httpResponse *response = wStruct->response;
    struct httpObject *message = wStruct->message;
    memset(message->buffer, '\0', BUFFER_SIZE);

    //TODO SERIOUSLY FIGURE OUT THIS PERMISSIONS SHITS!!
    //int fd = open(message->filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    //pthread_mutex_lock(&mutex_pool);
    int fd = open(message->filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    //int fd = open(message->filename,  O_WRONLY | O_CREAT, S_IWUSR | O_TRUNC);
    if (fd < 0) {
        printf("errono, %d\n", errno);
        perror("put error");
        response->status_code = 400;
        return;
    }

    //    printf("writing to %s...\n", message->filename);

    int total_recv_length = 0;
    /////////////////////////////////////////// begin log offset calc
    ssize_t offset;
    if(wStruct->lflag) {
        ssize_t length = message->content_length;
        ssize_t total = strlen(message->method) + strlen(message->filename) + countdigits(length) + 11;
        total += (69*(length/20))+((length%20)*3+9) + 9;
        printf("total length = %ld\n", total);
        wStruct->i = 0;
        pthread_mutex_lock(&mutex_log);
        offset = *(wStruct->poffset);
        *(wStruct->poffset) += total;
        pthread_mutex_unlock(&mutex_log);
    }
    /////////////////////////////////////////// end log offset calc

    do {
        int recv_length = read(client_sockd, message->buffer, BUFFER_SIZE);
        total_recv_length += recv_length;
        write(fd, message->buffer, recv_length);
        //////////////////////////////////// begin log
        if(wStruct->lflag) {
            logger(wStruct, &offset);
        }
        //////////////////////////////////// end log

        // printf("total_recv_length: %d\n", total_recv_length);
        //        write(STDOUT_FILENO, "WRITING\n", 8);
    } while (total_recv_length < message->content_length);
    //printf("After write loop\n");

    //    char* asdf = "put_request: done writing!\n";
    //    write(STDOUT_FILENO, asdf, strlen(asdf));


    close(fd);
    //if ((pthread_mutex_unlock(&mutex_pool)) != 0) {
    //    printf("mutex_unlock_threads error: %s\n", strerror(errno));
    //}

    response->status_code = 201;
}

void get_request(struct workerStruct *wStruct) {
    struct httpResponse *response = wStruct->response;
    struct httpObject *message = wStruct->message;
    memset(message->buffer, '\0', BUFFER_SIZE);

    if (access(message->filename, F_OK)) {
        response->status_code = 404;
        return;
    }

    //int fd = open(message->filename, O_RDWR);
    int fd = open(message->filename, O_RDONLY);
    if (fd < 0) {
        printf("errono, %d\n", errno);
        perror("put error");
        response->status_code = 403;
        return;
    }
    message->fd = fd;
    struct stat statbuf;
    int stat_info = stat(message->filename, &statbuf);

    if (stat_info < 0) {
        response->status_code = 403;
        return;
    }
    message->content_length = statbuf.st_size;
    response->status_code = 200;
}

void head_request(struct workerStruct *wStruct) {
    struct httpResponse *response = wStruct->response;
    struct httpObject *message = wStruct->message;
    struct stat statbuf;
    int stat_info = stat(message->filename, &statbuf);

    if (stat_info < 0) {
        response->status_code = 404;
        return;
    }
    message->content_length = statbuf.st_size;

    response->status_code = 200;
}

void process_request(struct workerStruct *wStruct) {
    struct httpObject *message = wStruct->message;
    struct httpResponse *response = wStruct->response;

    if (!check_filename(message->filename)) {
        response->status_code = 400;
        return;
    }
    //printf("process_request: HANDLING FILE: %s\n", wStruct->message->filename);

    pthread_mutex_lock(&mutex_file);
    printf("Worker %d inside of lock\n", wStruct->thread_number);

    // Switch for methods
    if ((strcmp(message->method, "PUT")) == 0) {
        printf("Worker %d processing PUT request\n\n", wStruct->thread_number);
        put_request(wStruct);
    } else if ((strcmp(message->method, "GET") == 0)) {
        printf("Worker %d processing GET request\n\n", wStruct->thread_number);
        get_request(wStruct);
    } else if ((strcmp(message->method, "HEAD") == 0)) {
        printf("Worker %d processing HEAD request\n\n", wStruct->thread_number);
        head_request(wStruct);
    } else {
        return;
    }

    pthread_mutex_unlock(&mutex_file);
    printf("Worker %d outside of lock\n", wStruct->thread_number);

}

void construct_http_response(struct workerStruct *wStruct) {
    struct httpObject *message = wStruct->message;
    struct httpResponse *response = wStruct->response;

    printf("Status Code: %d\nFilename: %s\nContent-Length: %ld\n\n",
           response->status_code, message->filename, message->content_length);

    ssize_t temp_length = 0;
    if ((strcmp(message->method, "PUT")) != 0) {
        temp_length = message->content_length;
    }

    switch (response->status_code) {
        case 200:
            snprintf(response->response, BUFFER_SIZE, "HTTP/%s 200 OK\r\nContent-Length: %ld\r\n\r\n",
                     message->httpversion, temp_length);
            break;
        case 201:
            snprintf(response->response, BUFFER_SIZE, "HTTP/%s 201 Created\r\nContent-Length: %ld\r\n\r\n",
                     message->httpversion, temp_length);
            break;
        case 400:
            snprintf(response->response, BUFFER_SIZE, "HTTP/%s 400 Bad Request\r\nContent-Length: %ld\r\n\r\n",
                     message->httpversion, temp_length);
            break;
        case 403:
            snprintf(response->response, BUFFER_SIZE, "HTTP/%s 403 Forbidden\r\nContent-Length: %ld\r\n\r\n",
                     message->httpversion, temp_length);
            break;
        case 404:
            snprintf(response->response, BUFFER_SIZE, "HTTP/%s 404 Not Found\r\nContent-Length: %ld\r\n\r\n",
                     message->httpversion, temp_length);
            break;
        default:
            snprintf(response->response, BUFFER_SIZE, "HTTP/%s 500 Internal Server Error\r\nContent-Length: %ld\r\n\r\n",
                     message->httpversion, temp_length);
    }
}

void send_http_response(struct workerStruct *wStruct) {
    int client_sockd = wStruct->thread_fd;
    //int lflag = wStruct->lflag;

    struct httpObject *message = wStruct->message;
    struct httpResponse *response = wStruct->response;

    printf("Worker %d sending response\n", wStruct->thread_number);
    // send http header response to the client
    dprintf(client_sockd, response->response, BUFFER_SIZE);

    //TODO write to log if lfag == 1
    /*
    if(lflag == 1) {
        //write to log using
        log_fd = 1;
    }
    */

    // if get request send contents of file to client
    //TODO determine if keeping this part here or moving
    //Also test for if lock is cool here
    if ((strcmp(message->method, "GET") == 0) && response->status_code == 200) {
        memset(message->buffer, '\0', BUFFER_SIZE);
        int total_writ_length = message->content_length;
        //pthread_mutex_lock(&mutex_file); //TODO determine if keeping this part here or moving
        do {
            int writ_length = read(message->fd, message->buffer, BUFFER_SIZE);
            total_writ_length -= writ_length;
            write(client_sockd, message->buffer, writ_length);
            //printf("total_writ_length: %d\n", total_writ_length);
            //printf("body == %s\n", message.body);
        } while (total_writ_length > 0);
        //printf("response: %s\n", response.response);
        //pthread_mutex_unlock(&mutex_file);
    }
}

// TODO possibly get rid of the mutex here until it is used for locking files and not the thread itself
// TODO determine best way to implement mutex and if having only global mutex is fine or having 2nd mutex for file only
void *worker_thread(void *workerStruct) {
    struct workerStruct *wStruct = (struct workerStruct *) workerStruct;

    while (true) {
        printf("Worker thread %d ready to work\n", wStruct->thread_number);
        //pthread_mutex_lock(wStruct->lock);
        // wait for dispatcher to assign a client_sockd
        while (wStruct->thread_fd < 0) {
            //printf("Worker %d thread_fd = %d\n", wStruct->thread_number, wStruct->thread_fd);
            if ((pthread_cond_wait(&wStruct->condition_variable, wStruct->lock)) != 0) {
                printf("errno = %d", errno);
                printf("cond_wait_threads error: %s\n", strerror(errno));
            }
            printf("Worker %d once woken socket is = %d\n", wStruct->thread_number, wStruct->thread_fd);
        }

        //printf("Worker thread %d handling request \n", wStruct->thread_number);

        wStruct->response->status_code = read_http_response(wStruct);
        //printf("Worker thread %d handling file %s\n", wStruct->thread_number, wStruct->message->filename);

        process_request(wStruct);

        //printf("worker_thread: HANDLING FILE: %s\n", wStruct->message->filename);


        construct_http_response(wStruct);
        send_http_response(wStruct);
        wStruct->thread_fd = -1;
        printf("Worker thread %d completed request\n", wStruct->thread_number);
    }
}

//TODO figure out wierd error from this implementation
/*
error: variable-sized object may not be initialized
     struct workerStruct *arr_wStruct[thread_count] = malloc(sizeof(struct workerStruct) * thread_count);
*/
/*
struct workerStruct * init_pool(struct dispatcherStruct dStruct) {
    printf("%s\n", "Creating worker pool");
    int thread_count = dStruct.thread_count;

    struct workerStruct *arr_wStruct[thread_count] = malloc(sizeof(struct workerStruct) * thread_count);

    // create the worker pool threads
    for (int i = 0; i < dStruct.thread_count; i++) {
        printf("creating thread %d\n", i);
        arr_wStruct[i]->thread_fd = -1;
        arr_wStruct[i]->thread_number = i;
        arr_wStruct[i]->lflag = dStruct.lflag;
        //arr_wStruct[i]->log_fd = dStruct.log_fd;
        struct httpObject message;
        struct httpResponse response;
        arr_wStruct[i]->message = &message;
        arr_wStruct[i]->response = &response;
        //pthread_cond_init(&arr_wStruct[i].condition_variable, NULL); // worker specific condition variable
                                                                       // TODO determine if this or global works
        arr_wStruct[i]->condition_variable = &condition_variable_pool;
        arr_wStruct[i]->lock = &mutex_pool;
        int error = pthread_create(&arr_wStruct[i]->thread_id, NULL, worker_thread, (void*) &arr_wStruct[i]);
        if (error != 0) {
            write(STDERR_FILENO, "Error creating worker thread thread\n", 50);
            exit(EXIT_FAILURE);
        }
    }

    return arr_wStruct;
}
*/

// handle the incoming client requests and disburse them to waiting threads
void *dispatcher(void *dispatcher) {
    struct dispatcherStruct *dStruct = (struct dispatcherStruct *) dispatcher;
    int lflag = dStruct->lflag;
    int thread_count = dStruct->thread_count;

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    printf("%s\n", "Creating worker pool");

    //TODO figure out how return malloc struct array
    //struct workerStruct arr_wStruct = init_pool(*dStruct);

    struct workerStruct arr_wStruct[thread_count];
    ssize_t offset = 0;

    // create the worker pool threads
    for (int i = 0; i < thread_count; i++) {
        printf("creating thread %d\n", i);
        arr_wStruct[i].thread_fd = -1;
        arr_wStruct[i].thread_number = i;
        arr_wStruct[i].lflag = lflag;
        struct httpObject message;
        struct httpResponse response;
        arr_wStruct[i].message = &message;
        arr_wStruct[i].response = &response;
        arr_wStruct[i].poffset = &offset;

        //pthread_cond_init(arr_wStruct[i].condition_variable, NULL); // worker specific condition variable
        arr_wStruct[i].condition_variable = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
        // TODO determine if this or global works
        //arr_wStruct[i].condition_variable = &condition_variable_pool;
        arr_wStruct[i].lock = &mutex;
        int error = pthread_create(&arr_wStruct[i].thread_id, NULL, worker_thread, (void *) &arr_wStruct[i]);
        if (error != 0) {
            write(STDERR_FILENO, "Error creating worker thread thread\n", 50);
            exit(EXIT_FAILURE);
        }
    }

    // take client requests from queue, parse request, and send off for processing to next available thread
    while (true) {
        int *pclient;
        pthread_mutex_lock(&mutex_queue);
        if ((pclient = dequeue()) == NULL) {// take next available client
            pthread_cond_wait(&condition_variable_queue, &mutex_queue);
            pclient = dequeue();
            //printf("Dequeueing client_sockd = %d\n", *pclient);
        }
        pthread_mutex_unlock(&mutex_queue);

        // check if pcleint has a fd to process
        // iterate through pool of thread to find waiting thread
        // unlock mutex and signal thread to work on processed client request

        int loop_reset_sleep = 0;

        if (pclient != NULL) {
            int found_waiting_thread = 0, i = 0;
            while (found_waiting_thread == 0) {
                if (arr_wStruct[i].thread_fd == -1) {
                    arr_wStruct[i].thread_fd = *pclient;
                    printf("Waking worker thread %d to begin work on fd = %d\n",
                           i, arr_wStruct[i].thread_fd);
                    free(pclient);//TODO correct place to free this?

                    int error = pthread_cond_signal(&arr_wStruct[i].condition_variable);
                    if (error != 0) {
                        write(STDERR_FILENO, "Error signaling thread\n", 50);
                        exit(EXIT_FAILURE);
                    }
                    //printf("signal error = %d\n", error);
                    found_waiting_thread = 1;
                }

                //break out of loop once waiting thread is delegated
                if (found_waiting_thread == 1) {
                    break;
                }

                // reset i to 0 to iterate from beginning or worker array
                if (++i == thread_count) {
                    printf("reseting dispatcher loop\n");
                    if(loop_reset_sleep == 1) {
                        sleep(3);
                        loop_reset_sleep = 0;
                    }
                    else{
                        loop_reset_sleep = 1;
                    }
                    i = 0;
                }
            }
            //pthread_mutex_unlock(&mutex);
        }
    }
}

int main(int argc, char **argv) {
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
    int thread_count = 0;

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
        log_fd = open(logname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }

    //char buff[70];
    //ssize_t incr = snprintf(buff, 70, "FAIL: %s /%s HTTP/1.1 --- response %d\n========\n", "PUT", "nananme",  400);
    //pwrite(log_fd, buff, incr, 0);
    int port_number = atoi(argv[optind]);
    printf("Nflag = %d, ", Nflag);
    printf("lflag = %d, ", lflag);
    printf("tcount = \"%d\"\n", thread_count);
    printf("logname = \"%s\"\n", logname);
    printf("port# = %d\n", port_number);
    printf("log_fd = %d\n", log_fd);


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
        return EXIT_FAILURE;
    }

    printf("server socket = %d\n", server_sockd);

    /*
      Configure server socket
    */
    int enable = 1;

    /*
      This allows you to avoid: 'Bind: Address Already in Use' error
    */
    int ret = setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &enable,
                         sizeof(enable));
    /*
      Bind server address to socket that is open
    */
    ret = bind(server_sockd, (struct sockaddr *) &server_addr, addrlen);

    /*
      Listen for incoming connections
    */
    ret = listen(server_sockd, 5);// set to 20 queued up client requests
                                  // unknown behavior if more than this

    if (ret < 0) {
        return EXIT_FAILURE;
    }

    /*
        Connecting with a client
    */
    struct sockaddr client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    //Create dispatcher thread
    struct dispatcherStruct dStruct;
    dStruct.lflag = lflag;
    dStruct.thread_count = thread_count;
    int error = pthread_create(&dStruct.thread_id, NULL, dispatcher, (void *) &dStruct);
    if (error != 0) {
        write(STDERR_FILENO, "Error creating dispatcher thread\n", 25);
        exit(EXIT_FAILURE);
    }
    /*
    if ((pthread_mutex_init(&mutex_pool, NULL) != 0)) {
        printf("mutex_init_threads error: %s\n", strerror(errno));
    }
    */
    /*
    if ((pthread_cond_init(&condition_variable_pool, NULL) != 0)) {
        printf("cv_init_threads error: %s\n", strerror(errno));
    }
    */

    //set data_ready flag as 1 for first worker thread
    data_ready = 1;

    while (true) {
        printf("[+] server is waiting...\n");

        /*
         * 1. Accept Connection
         */
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);
        if (client_sockd == -1) {
            warn("accept\n");
            return -1;
        }
        int *pclient = malloc(sizeof(int));// TODO ensure this is free() somewhere later on
        *pclient = client_sockd;

        // uses mutex lock to make sure only one thread is reading from
        // queue at a time
        pthread_mutex_lock(&mutex_queue);
        pthread_cond_signal(&condition_variable_queue);
        //printf("%s\n", "Queueing client_sockd");
        enqueue(pclient);
        pthread_mutex_unlock(&mutex_queue);

        //pthread_create(&dispatcher, NULL, handle_request, pclient)


        //response.status_code = read_http_response(client_sockd, &message);

        //process_request(client_sockd, &response, &message);

        //construct_http_response(&response, &message);

        //send_http_response(client_sockd, &response, &message);
    }

    return 0;
}
