#include "httpserver.h"

void printRequest(const struct httpRequest req) {
    printf("HTTP REQUEST: \n");   
    printf("method == %s\n", req.method);
    printf("filename == %s\n", req.filename);
    printf("httpversion == %s\n", req.httpversion);
    printf("content_length == %ld\n", req.content_length);
}

struct httpRequest read_http_request(ssize_t client_sockd) {
    struct httpRequest req;
    char buffer[BUFFER_SIZE];

    // Receive client info
    int res = recv(client_sockd, buffer, BUFFER_SIZE, 0);
    if(res < 0) {
        // Return 400 in failure
        struct httpResponse res = {"", 400, ""};
        send_response(res, client_sockd);

        return dummyReq;
    }

    // parse response into httpRequest object
    sscanf(buffer, "%s%*[ /]%s%*[ HTTP/]%s", req.method, req.filename, 
            req.httpversion);
    char con_len[10];
    sscanf(buffer, "%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%s",  con_len);
    int con_int = atoi(con_len);
    req.content_length = (ssize_t) con_int;

    // LOGGING
    /* printRequest(req); */
    /* printf("http_request: buffer == %s\n", buffer); */
    
    return req;
}

struct httpResponse process_request(const struct httpRequest request) {
    printf("Processing Request\n");

    return dummyRes;
}

// TODO
void send_response(const struct httpResponse response, int client_sockd) {
    printf("Constructing Response\n");
}


int main(int argc, char** argv) {
    /*
        Create sockaddr_in with server information
    */
    char* port = "8080";
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
        // Remember errors happen

        /*
         * 2. Read HTTP Message
         */
        send_response(process_request(read_http_request(client_sockd)), client_sockd);
    }

    return EXIT_SUCCESS;
}

