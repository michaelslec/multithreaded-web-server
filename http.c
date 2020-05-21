#include "http.h"

void printRequest(const struct httpRequest req) {
    printf("HTTP REQUEST: \n");
    printf("method == %s\n", req.method);
    printf("filename == %s\n", req.filename);
    printf("httpversion == %s\n", req.httpversion);
    printf("content_length == %ld\n", req.content_length);
}

void printResponse(const struct httpResponse res) {
    printf("HTTP RESPONSE: \n");
    printf("method == %s\n", res.method);
    printf("status_code == %d\n", res.status_code);
    printf("status_code_message == %s\n", res.status_code_message);
    printf("content_length == %ld\n", res.content_length);
}


void put_request(struct httpRequest req, struct httpResponse* res) {
    /* printf("Processing PUT request...\n\n"); */
    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    strcpy(res->method, "PUT");

    int fd = open(req.filename,  O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(fd < 0) {
        perror("put error");
        res->status_code = 500;
        return;
    }

    /* printf("writing...\n"); */

    int total_recv_length = 0;
    do {
        int recv_length = read(req.origin_socket, buffer, BUFFER_SIZE);
        total_recv_length += recv_length;
        write(fd, buffer, recv_length);
    } while (total_recv_length < req.content_length);

    close(fd);

    res->status_code =  201;
}

void get_request(struct httpRequest req, struct httpResponse* res) {
    /* printf("Processing GET request\n\n"); */
    strcpy(res->method, "GET");

    if(access(req.filename, F_OK)) {
        res->status_code = 404;
        return;
    }

    int fd = open(req.filename, O_RDONLY);
    if (fd < 0) {
        res->status_code = 403;
        return;
    }

    struct stat statbuf;
    fstat(fd, &statbuf);
    res->content_length = statbuf.st_size;

    close(fd);

    res->status_code = 200;
}

void head_request(struct httpRequest req, struct httpResponse* res) {
    /* printf("Processing HEAD request\n\n"); */
    strcpy(res->method, "HEAD");

    struct stat statbuf;
    int stat_info = stat(req.filename, &statbuf);
    if(stat_info < 0) {
        res->status_code = 404;
        return;
    }
    res->content_length = statbuf.st_size;

    res->status_code = 200;
}

int check_filename(const char* filename) {
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

int check_method(const char* method){
    return strcmp(method, "PUT") == 0
        || strcmp(method, "GET") == 0
        || strcmp(method, "HEAD") == 0;
}

int check_http_version(const char* version) {
    return strcmp(version, "1.1") == 0;
}

int validate(const struct httpRequest req) {
    return check_filename(req.filename)
        && check_method(req.method)
        && check_http_version(req.httpversion);
}


struct httpRequest read_http_request(ssize_t client_sockd) {
    struct httpRequest req;
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);

    // Receive client info
    int res = recv(client_sockd, buffer, BUFFER_SIZE, 0);
    if(res < 0) {
        // Return 400 in failure by passing bad filename
        strcpy(req.filename, ".}");
        return req;
    }

    // parse response into httpRequest object
    sscanf(buffer, "%s%*[ /]%s%*[ HTTP/]%s", req.method, req.filename,
            req.httpversion);
    char con_len[10];
    sscanf(buffer, "%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%s",  con_len);
    int con_int = atoi(con_len);
    req.content_length = (ssize_t) con_int;
    req.origin_socket = client_sockd;

    // LOGGING
    /* printRequest(req); */
    /* printf("http_request: buffer == %s\n", buffer); */

    return req;
}

struct httpResponse process_request(const struct httpRequest request) {
    struct httpResponse res;
    if (!validate(request)) {
        res.status_code = 400;
        return res;
    }

    // Switch for methods
    if((strcmp(request.method, "PUT")) == 0) {
        put_request(request, &res);
    }
    else if((strcmp(request.method, "GET") == 0)) {
        get_request(request, &res);
    }
    else if((strcmp(request.method, "HEAD") == 0)) {
        head_request(request, &res);
    }

    calculate_status_code_message(&res);
    res.origin_socket = request.origin_socket;
    /* printResponse(res); */

    return res;
}

void calculate_status_code_message(struct httpResponse* res) {
    switch(res->status_code) {
        case 200:
            strcpy(res->status_code_message, "OK");
            break;
        case 201:
            strcpy(res->status_code_message, "Created");
            break;
        case 400:
            strcpy(res->status_code_message, "Bad Request");
            res->content_length = 0;
            break;
        case 403:
            strcpy(res->status_code_message, "Forbidden");
            res->content_length = 0;
            break;
        case 404:
            strcpy(res->status_code_message, "Not Found");
            res->content_length = 0;
            break;
        default:
            strcpy(res->status_code_message, "Internal Server Error");
            res->content_length = 0;
            break;
    }
}

// TODO
void send_response(const struct httpResponse response) {
    // if get request send contents of file to client
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);

    sprintf(buffer, "HTTP/1.1 %d %s\r\nContent-Length:"
            "%ld\r\n\r\n", response.status_code, response.status_code_message,
            response.content_length);

    write(response.origin_socket, buffer, BUFFER_SIZE);

    /* if((strcmp(message->method, "GET") == 0) && response->status_code == 200) { */
    /*     memset(buffer, '\0', BUFFER_SIZE); */
    /*     int total_writ_length = response->content_length; */
    /*  */
    /*     int fd = open(res.filename, O_RDONLY); */
    /*     if (fd < 0) { */
    /*         res->status_code = 403; */
    /*         return; */
    /*     } */
    /*     do { */
    /*         int writ_length = read(message->fd, message->buffer, BUFFER_SIZE); */
    /*         total_writ_length -= writ_length; */
    /*         write(client_fd, message->buffer, writ_length); */
    /*         //printf("total_writ_length: %d\n", total_writ_length); */
    /*         //printf("body == %s\n", message.body); */
    /*     } while (total_writ_length > 0); */
    /*     //printf("response: %s\n", response.response); */
    /*  */
    /*     close(fd); */
    /* } */
}

