#include "http.h"

int log_fd = 0;
int thread_count = 0;

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

