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

#define BUFFER_SIZE 4096

struct httpRequest {
    /*
        Create some object 'struct' to keep track of all
        the components related to a HTTP message
        NOTE: There may be more member variables you would want to add
    */
    char method[5];         // PUT, HEAD, GET
    char filename[28];      // what is the file we are worried about
    char httpversion[9];    // HTTP/1.1
    ssize_t content_length; // example: 13
} dummyReq;

struct httpResponse {
    char method[5];                // PUT, HEAD, GET
    int status_code;               // 200, 404, etc.
    char status_code_message[100]; // OK, File not found, etc
} dummyRes;

int testingfunction(int a) {
    return a + 2;
}

/*
   Takes socket and returns parsed request in a data structure
   \param client_sockd - socket file descriptor
   \return  struct httpRequest parsed data
*/
struct httpRequest read_http_request(ssize_t client_sockd);

/*
   \ param request struct httpRequest holding parsed information from client
                          socket
*/
struct httpResponse process_request(const struct httpRequest request);

/*
    \brief 3. Construct some response based on the HTTP request you recieved
*/
void send_response(const struct httpResponse response, int client_sockd) ;
