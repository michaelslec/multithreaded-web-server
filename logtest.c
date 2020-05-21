#include <err.h> // warn
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdbool.h> // true, false
#include <stdio.h>
#include <stdlib.h> // atoi
#include <string.h> // memset
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h> // write
#include <errno.h> //perror
#include <pthread.h>

#define BUFFER_SIZE 4096

pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

int log_fd; // Log file descriptor
const char* SERVER_PATH = "../testfiles/server_out/%s";

struct httpResponse {
  int status_code;
  char response[BUFFER_SIZE];
};

struct httpObject {
    char method[5];         // PUT, HEAD, GET
    char filename[28];      // what is the file we are worried about
    char httpversion[9];    // HTTP/1.1
    ssize_t content_length; // example: 13
    //int status_code;
    uint8_t buffer[BUFFER_SIZE];
    //uint8_t body[BUFFER_SIZE];
    //char response[BUFFER_SIZE];
    int fd; // file descriptor of file being opened by GET function
};


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

/*
    \brief 1. Want to read in the HTTP message/ data coming in from socket
    \param client_sockd - socket file descriptor
    \param message - object we want to 'fill in' as we read in the HTTP message
*/
int read_http_response(ssize_t client_sockd, struct httpObject *message) {
    printf("Reading client response...\n");
    
    memset(message->buffer, '\0', BUFFER_SIZE);
    
    int res = recv(client_sockd, message->buffer, BUFFER_SIZE, 0);
    if(res < 0) {
        return 400;
    }
    
    // read in http header
    sscanf(message->buffer, "%s%*[ /]%s%*[ HTTP/]%s", message->method, message->filename, 
            message->httpversion);
    char con_len[10];
    sscanf(message->buffer, "%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%s",  con_len);
    int con_int = atoi(con_len);
    message->content_length = (ssize_t) con_int;

    return 1;
}

/*
   char* -> int
   takes filename nad produces true if value is valid
  NOTE: proper file name has only A-Za-z0-9-_ && is preceded by /
*/
int check_filename(char* filename) {
    // iterate over each char in filename
    // convert to ascii value
    // compare valid ascii values
    // return 0 if char not in whitelist
    // return 0 if filename longer than 27 char
        
    if(strlen(filename) > 27) {
        return 0;
    }
        
    for(int i = 0; filename[i] != '\0'; i++) {
        if((64 < filename[i] && filename[i]  <= 90) || (96 < filename[i] && filename[i] <= 122) 
            || (47 < filename[i] && filename[i] <= 58) || filename[i] == 45 || filename[i] == 95) {
            
        }
        else {
            return 0;
        }
    }

    return 1;
}

// httpObject* -> SIDE EFFECTS:
// Takes httpObject and tries to write to filename
void put_request(int client_fd, struct httpResponse *response, struct httpObject *message) {
    printf("Processing PUT request\n\n");

    memset(message->buffer, '\0', BUFFER_SIZE);

    char write_file[BUFFER_SIZE];
    sprintf(write_file, SERVER_PATH, message->filename);

    // PRAISE BE TO OPEN FLAGS RDWR CREATE && TRUNC
    int fd = open(write_file, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(fd < 0) {
        printf("errono, %d\n", errno);
        perror("put error");
        response->status_code = 400;
        return;
    }

    printf("writing...\n");

    int total_recv_length = 0;

    do {
        int recv_length = read(client_fd, message->buffer, BUFFER_SIZE);
        total_recv_length += recv_length;
        write(fd, message->buffer, recv_length);
        // printf("total_recv_length: %d\n", total_recv_length);
        //printf("body == %s\n", message->body);
    } while (total_recv_length < message->content_length);

    close(fd);
    
    response->status_code = 201;
}
/*
    httpObject* -> SIDE EFFECTS:
    Tries to open file then calls construct_http_response() with proper
    information
*/
void get_request(struct httpResponse *response, struct httpObject *message) {
    printf("Processing GET request\n\n");

    memset(message->buffer, '\0', BUFFER_SIZE);
    
    char write_file[BUFFER_SIZE];
    sprintf(write_file, SERVER_PATH, message->filename);

    if(access(write_file, F_OK)) {
        response->status_code = 404;
        return;
    }
    
    printf("Trying to open file: %s\n", write_file);

    int fd = open(write_file, O_RDONLY);

    if(fd < 0) {
        printf("errono, %d\n", errno);
        perror("put error");
        response->status_code = 403;
        return;
    }

    message->fd = fd;
    struct stat statbuf;
    int stat_info = stat(write_file, &statbuf);

    if(stat_info < 0) {
        response->status_code = 403;
        return;
    }
    message->content_length = statbuf.st_size;
    response->status_code = 200;
}
/*            
    httpObject* -> SIDE EFFECTS
    Simply generate proper response and pass to construct_http_response()
*/
void head_request(struct httpResponse *response, struct httpObject *message) {
    printf("Processing HEAD request\n\n");

    char write_file[BUFFER_SIZE];
    sprintf(write_file, SERVER_PATH, message->filename);

    struct stat statbuf;
    int stat_info = stat(write_file, &statbuf);

    if(stat_info < 0) {
        response->status_code = 404;
        return;
    }
    message->content_length = statbuf.st_size;
    
    response->status_code = 200;
}
/*
    httpObject* -> SIDE EFFECTS:
     1) check for non-supported requests (unsupported method, //).
     2) Route to proper method handler
*/
void process_request(int client_fd, struct httpResponse *response, struct httpObject *message) {
    if (!check_filename(message->filename)) {
        response->status_code = 400;
        return;
        
    }

    // Switch for methods
    if((strcmp(message->method, "PUT")) == 0) {
        put_request(client_fd, response, message);
    }
    else if((strcmp(message->method, "GET") == 0)) {
        get_request(response, message);
    }
    else if((strcmp(message->method, "HEAD") == 0)) {
        head_request(response, message);
    }
    else {
        return;
    }
}


void construct_http_response(struct httpResponse *response, struct httpObject *message) {
    printf("Status Code: %d\nFilename: %s\nContent-Length: %ld\n\n",
             response->status_code, message->filename, message->content_length);

    ssize_t temp_length = 0;
    if ((strcmp(message->method, "PUT")) != 0) {
    temp_length = message->content_length;
    }

    switch(response->status_code) {
    case 200: snprintf(response->response, BUFFER_SIZE, "HTTP/%s 200 OK\r\nContent-Length: %ld\r\n\r\n",
                   message->httpversion, temp_length);
              break;
    case 201: snprintf(response->response, BUFFER_SIZE, "HTTP/%s 201 Created\r\nContent-Length: %ld\r\n\r\n",
                   message->httpversion, temp_length);
              break;          
    case 400: snprintf(response->response, BUFFER_SIZE, "HTTP/%s 400 Bad Request\r\nContent-Length: %ld\r\n\r\n",
                   message->httpversion, temp_length);
              break;          
    case 403: snprintf(response->response, BUFFER_SIZE, "HTTP/%s 403 Forbidden\r\nContent-Length: %ld\r\n\r\n",
                   message->httpversion, temp_length);
              break;          
    case 404: snprintf(response->response, BUFFER_SIZE, "HTTP/%s 404 Not Found\r\nContent-Length: %ld\r\n\r\n",
                   message->httpversion, temp_length);
              break;          
    default: snprintf(response->response, BUFFER_SIZE, "HTTP/%s 500 Internal Server Error\r\nContent-Length: %ld\r\n\r\n",
                   message->httpversion, temp_length);      
    }
}

void send_http_response(int client_fd, struct httpResponse *response, struct httpObject *message){
    // send http header response to the client
    dprintf(client_fd, response->response, BUFFER_SIZE);

    // if get request send contents of file to client
    if((strcmp(message->method, "GET") == 0) && response->status_code == 200) {
        memset(message->buffer, '\0', BUFFER_SIZE);
        int total_writ_length = message->content_length;

        do {
            int writ_length = read(message->fd, message->buffer, BUFFER_SIZE);
            total_writ_length -= writ_length;
            write(client_fd, message->buffer, writ_length);
            //printf("total_writ_length: %d\n", total_writ_length);
            //printf("body == %s\n", message.body);
        } while (total_writ_length > 0);
        //printf("response: %s\n", response.response);
    }
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

    /*
      Create sockaddr_in with server information
    */

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
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
    int ret = setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &enable,
                       sizeof(enable));

    if (ret == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
    }

    /*
      Bind server address to socket that is open
    */
    ret = bind(server_sockd, (struct sockaddr *)&server_addr, addrlen);

    if (ret == -1) {
    warn("bind");
    return -1;
    }

    /*
      Listen for incoming connections
    */
    ret = listen(server_sockd, 5); // 5 should be enough, if not use SOMAXCONN

    if (ret < 0) {
        return EXIT_FAILURE;
    }

    /*
      Connecting with a client
    */
    struct sockaddr client_addr;
    socklen_t client_addrlen;

    struct httpObject message;
    struct httpResponse response;

    while (true) {
        printf("[+] server is waiting...\n");
        

        /*
         * 1. Accept Connection
         */
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);

        if (client_sockd == -1) {
          warn("accept");
          return -1;
        }
        /*
         * 2. Read HTTP Message
         */

        response.status_code = read_http_response(client_sockd, &message);

        /*
         * 3. Process Request
         */
        process_request(client_sockd, &response, &message);

        /*
         * 4. Construct Response
         */

        construct_http_response(&response, &message);

        /*
         * 5. Send Response
         */

        send_http_response(client_sockd, &response, &message);

        printf("Response Sent\n");
    }
    

    return EXIT_SUCCESS;
}

