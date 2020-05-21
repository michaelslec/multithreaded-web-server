#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h> // write
#include <string.h> // memset
#include <stdlib.h> // atoi
#include <stdbool.h> // true, false
#include <pthread.h> //threads
#include <string.h>
#include <err.h>

#define BUFFER_SIZE 4096

pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

int log_fd; // Log file descriptor

struct dispatcherStruct {
    int lflag;
    int thread_count;
    pthread_t thread_id;
    int log_fd;
};

struct worker{
    pthread_t worker_id;
    //int client_sockd;
    int log_fd;
    //pthread_cond_t condition_var;
    //char* message;
    ssize_t *poffset;
    uint8_t buffer[BUFFER_SIZE];
    ssize_t con_len;
    char method[5];
    char filename[28];
    int error;
    //uint8_t buffer[BUFFER_SIZE];
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

void logger(struct worker* w_thread, ssize_t offset) {
    char buff[70];
    if(w_thread->error == 400) {
        ssize_t incr = snprintf(buff, 70, "FAIL: %s /%s HTTP/1.1 --- response"
                "%d\n========\n", w_thread->method, w_thread->filename,
                w_thread->error);
        pwrite(w_thread->log_fd, buff, incr, offset);
    }
    else {
        ssize_t length = w_thread->con_len;
        ssize_t incr = snprintf(buff, 70, "%s /%s length %ld\n",
                w_thread->method,  w_thread->filename,  w_thread->con_len);
        pwrite(w_thread->log_fd, buff, incr, offset);
        offset += incr;
        int count = 0;
        char * c = &(w_thread->buffer);
        for (int i = 0; i < length; i++) {
            if ((i % 20) == 0) {
                if (i != 0) {
                    incr = snprintf(buff, 70, "\n");
                    pwrite(w_thread->log_fd, buff, incr, offset);
                    offset += incr;
                }
                incr = snprintf(buff, 70, "%07d", count);
                pwrite(w_thread->log_fd, buff, incr, offset);
                offset += incr;
                count +=2;
                incr = snprintf(buff, 70, "0");
                pwrite(w_thread->log_fd, buff, incr, offset);
                offset += incr;
            }
            incr = snprintf(buff, 70, " %02x", c[i]);
            pwrite(w_thread->log_fd, buff, incr, offset);
            offset += incr;
        }
        incr = snprintf(buff, 70, "\n========\n");
        pwrite(w_thread->log_fd, buff, incr, offset);
        offset += incr;
    }
}

void* handle_task(void* thread) {
    struct worker* w_thread = (struct worker*)thread;
    ssize_t total;
    if (w_thread->error == 400) {
        total = strlen(w_thread->method) + strlen(w_thread->filename) + 38;
    }
    else {
        ssize_t length = w_thread->con_len;
        total = strlen(w_thread->method) + strlen(w_thread->filename) + countdigits(length) + 11;
        total += (69*(length/20))+((length%20)*3+9) +9;
    }

    pthread_mutex_lock(&mutex_log);
    ssize_t offset = *(w_thread->poffset);
    *(w_thread->poffset) += total;
    logger(w_thread, offset);
    pthread_mutex_unlock(&mutex_log);

    return NULL;
}
void* dispatcher(void* dispatch){
    struct dispatcherStruct* dStruct = (struct dispatcherStruct *) dispatch;

    while(dStruct->lflag != 0){
        printf("lflag = %d\n", dStruct->lflag);
        sleep(1);
    }

    printf("dStruct: %d, %d\n", dStruct->lflag, dStruct->thread_count);

    return NULL;
}

int main(int argc, char** argv) {
    // GRAB OPTIONS
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

    int port_number = atoi(argv[optind]);
    printf("Nflag = %d, ", Nflag);
    printf("lflag = %d, ", lflag);
    printf("tcount = \"%d\"\n", thread_count);
    printf("logname = \"%s\"\n", logname);
    printf("port# = %d\n", port_number);
    printf("log_fd = %d\n", log_fd);

    

    return EXIT_SUCCESS;
}

