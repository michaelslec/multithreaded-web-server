#include "http.h"

void printRequest(const struct httpRequest req) {
    printf("HTTP REQUEST: \n");
    printf("method == %s\n", req.method);
    printf("filename == %s\n", req.filename);
    printf("httpversion == %s\n", req.httpversion);
    printf("content_length == %ld\n", req.content_length);
}

int put_request(struct httpRequest req) {
    /* printf("Processing PUT request...\n\n"); */
    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);

    int fd = open(req.filename,  O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(fd < 0) {
        perror("put error");
        return 500;
    }

    /* printf("writing...\n"); */

    int total_recv_length = 0;
    do {
        int recv_length = read(req.origin_socket, buffer, BUFFER_SIZE);
        total_recv_length += recv_length;
        write(fd, buffer, recv_length);
    } while (total_recv_length < req.content_length);

    close(fd);
    
    return 201;
}

int get_request(struct httpRequest req) {
    /* printf("Processing GET request\n\n"); */

    if(access(req.filename, F_OK)) {
        return 404;
    }

    int fd = open(req.filename, O_RDONLY);
    if (fd < 0) {
        return 403;
    }

    close(fd);

    return 200;
}

int head_request(struct httpRequest req) {
    /* printf("Processing HEAD request\n\n"); */

    struct stat statbuf;
    int stat_info = stat(req.filename, &statbuf);

    if(stat_info < 0) {
        return 404;
    }

    return 200;
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

// TODO
struct httpResponse process_request(const struct httpRequest request) {
    struct httpResponse res;
    if (!validate(request)) {
        res.status_code = 400;
        return res;
    }

    // Switch for methods
    if((strcmp(request.method, "PUT")) == 0) {
        strcpy(res.method, "PUT");
        res.status_code = put_request(request);
    }
    else if((strcmp(request.method, "GET") == 0)) {
        strcpy(res.method, "GET");
        res.status_code = get_request(request);
    }
    else if((strcmp(request.method, "HEAD") == 0)) {
        strcpy(res.method, "HEAD");
        res.status_code = head_request(request);
    }

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
            break;
        case 403:
            strcpy(res->status_code_message, "Forbidden");
            break;
        case 404: 
            strcpy(res->status_code_message, "Not Found");
            break;
        default:
            strcpy(res->status_code_message, "Internal Server Error");
            break;
    }
}

// TODO
void send_response(const struct httpResponse response) {
    printf("Constructing Response\n");
}

