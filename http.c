#include "http.h"

void printRequest(const struct httpRequest req) {
    printf("HTTP REQUEST: \n");
    printf("method == %s\n", req.method);
    printf("filename == %s\n", req.filename);
    printf("httpversion == %s\n", req.httpversion);
    printf("content_length == %ld\n", req.content_length);
}

// TODO
int put_request(struct httpRequest req) {
    return 404;
}

// TODO
int get_request(struct httpRequest req) {
    return 404;
}

// TODO
int head_request(struct httpRequest req) {
    return 404;
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
        res.status_code = put_request(request);
    }
    else if((strcmp(request.method, "GET") == 0)) {
        res.status_code = get_request(request);
    }
    else if((strcmp(request.method, "HEAD") == 0)) {
        res.status_code = head_request(request);
    }

    return res;
}

// TODO
void send_response(const struct httpResponse response) {
    printf("Constructing Response\n");
}

