# Goals

Write a web server that takes a port when the binary is executed:

    EXAMPLE: ./httpserver 8080

This web server will handle GET, PUT, HEAD requests. The requests should be formatted as such (Note: All return carriages are in Windows format \r\n):

    PUT:

        PUT /filepath HTTP/1.1

        Content-Length: x

        DATA

    GET:

        Get /filepath HTTP/1.1

    HEAD:

        HEAD /filepath HTTP/1.1

Response codes:

    200: OK

    201: Created

    400: Bad Request

    403: Forbidden

    404: Not Found

    500: Internal Server Error

File path will up 27 of the following characters: (a-zA-Z0-9_-)

Responses will look like the following:

    PUT:

        HTTP/1.1 STATUS_CODE STATUS_MESSAGE\r\nContent-Length: 0\r\n\r\n

    GET:

        HTTP/1.1 STATUS_CODE STATUS_MESSAGE\r\nContent-length: x\r\n\r\n


    Lorem ipsum blah blah blah


    HEAD:


        HTTP/1.1 STATUS_CODE STATUS_MESSAGE\r\nContent-Length: 0\r\n\r\n


# Design outline:



1. Run httpserver with specified port
2. Httpserver opens socket and listens on port
3. One receiving connection, httpserver will pass the socket fd to read_http_request
    1. Read_http_request will read from socket and parse into a struct httpObject
    2. Return httpObject
4. Pass httpObject to process_request
    3. Validate filepath, HTTP version, HTTP method
    4. Dispatches to HTTP method handler (GET/PUT/HEAD)
5. Within one of these functions, we will perform appropriate actions and send a response
6. Once file has been handled by server, have server send back a response to the client (have them as a STATIC CONST CHAR canned string at the top of code)

    (such as: static const char error_403[] = "403 Forbidden HTTP/1.1\r\nContent-Length: 0\r\n\r\n";)

    6a. 200 (OK) - response succeeded

    6b. 201 (Created) - response succeeded and a new resource has been created (use for PUT requests)

    6c. 400 (Bad Request) - server could not understand request because of bad client request (bad syntax)

    6d. 403 (Forbidden) - client does not have access rights to the content

    6e. 404 (Not Found) - server can not find requested resource (likely client sends a GET request for file not in server)

    6f. 500 (Internal Server Error) - server has reached an unknown situation it doesn't know how to handle

7. Server will end the client socket (use close() on the accept() fd?) and go back to listen state until receives new client request


# Constants:



*   Server address: localhost
*   HTTP Methods: GET, PUT, HEAD
*   Response Messages (see step 4)


# Data Definitions:



*   httpObject
    *   **method char[5]**
    *   **filename char[28]**
    *   **httpversion char[9]**
    *   **content_length ssize_t**
    *   **status_code int**
    *   **buffer uint[4096]**
    *   Interp. A structure that contains the request from a connecting client.
    *   Where method is HTTP method,
    *   filename a path for information,
    *   httpversion is the HTTP protocol version,
    *   content_length is the size in bytes of information stored in buffer,
    *   status_code is HTTP response (see step 4)
    *   Buffer is content of filename to be written OR having been read from
        filename OR to be displayed to screen


# API:
struct httpRequest {
    /* Create some object 'struct' to keep track of all
        the components related to a HTTP message
        NOTE: There may be more member variables you would want to add
    */
    char method[5];         // PUT, HEAD, GET
    char filename[28];      // what is the file we are worried about
    char httpversion[9];    // HTTP/1.1
    ssize_t content_length; // example: 13
    int origin_socket;      // originating socket file descriptor
};

struct httpResponse {
    char method[5];                // PUT, HEAD, GET
    int status_code;               // 200, 404, etc.
    char status_code_message[100]; // OK, File not found, etc
    ssize_t content_length;        // example: 13
    int origin_socket;      // originating socket file descriptor
    char filename[28];      // what is the file we are worried about
};

/*
   Takes socket and returns parsed request in a data structure
   \param client_sockd - socket file descriptor
   \return struct httpRequest parsed data
*/
struct httpRequest read_http_request(ssize_t client_sockd);

/*
   Validates HTTP request & creates a response
   \param request struct httpRequest holding parsed information from client
                                     socket
   \return struct httpResponse holding response data
*/
struct httpResponse process_request(const struct httpRequest request);

/*
   Sends proper HTTP response to client_sockd
   \param response holds response info
   \param client_sockd holds socket file descriptor of client with request
*/
void send_response(const struct httpResponse response);

/*
   Helper function: Prints out values of httpRequest struct
   \param req HTTP request object
*/
void printRequest(const struct httpRequest req);

/*
   Helper function: Prints out values of httpResponse struct
   \param req HTTP response object
*/
void printResponse(const struct httpResponse res);

/*
   Validates filename, HTTP version, and method
   \param req HTTP request object
   \return bool true if valid request
*/
int validate(const struct httpRequest req);

/*
   returns true if HTTP requested filename is a valid request
   \param filename requested file
*/
int check_filename(const char* filename);

/*
   returns true if version == 1.1
   \param version HTTP version
*/
int check_http_version(const char* version);

/*
   returns true if method is PUT,HEAD, OR GET
   \param method string representing HTTP method
*/
int check_method(const char* method);

/*
 * \param req the request made by the client
 * \param res the response object to store values
 */
void put_request(struct httpRequest req, struct httpResponse* res);

/*
 * \param req the request made by the client
 * \param res the response object to store values
 */
void get_request(struct httpRequest req, struct httpResponse* res);

/*
 * \param req the request made by the client
 * \param res the response object to store values
 */
void head_request(struct httpRequest req, struct httpResponse* res);

/*
 * Inserts proper message based off status code into given httpReponse obj
 \param res the HTTP resopnse
 */
void calculate_status_code_message(struct httpResponse* res);

# MULTITHREADING:

## FLOW
    * Dispatcher thread will wait conditionally on queue to be updated from
      main
    * This queue will contain socket file descriptors of clients
    * dispatcher will then dequeue and send sockfd to worker thread
    * worker thread will do  read, process, and send call chain

## CRITICAL AREAS
    * FILE I/O:
        - Within method handler functions
        - send_response()
    * ENQUEUE & DEQUEUE
        - main & dispatcher

## AVOIDING BUSY WORK
    * conditional signal for enqueueing and dequeueing
    * conditional signal when sending work to worker

## API

// Sends work from queue to awaiting threads
void * dispatcher();

// worker_struct_t -> Side effects
// Takes worker_struct_t, does work with socket and keeps track of conditional
void* workerthread(void* worker_struct);


# NOTES:

To practice parsing header, use dog with a file.txt containing a hypothetical header before trying to implement in httpserver

To test server using curl:

    GET:

     curl -s http://localhost:8080/ --request-target file

     -s is silent

     (file is name of what getting got)

    PUT:

     curl -v -T tosend.txt http://localhost:8080/ --request-target file

     -T is target of file to upload

     (tosend.txt is name of file uploaded)

    HEAD: curl -i -X HEAD http://localhost:8080/

     -X is custom request



    NOTE: -v for verbose is wanted



"curl -T tosend.txt localhost:8080/file.txt" will generate

    "PUT /file.txt HTTP/1.1\r\nContent-Length: %d\r\n\r\n" + file-data

    where %d is the length of the file in bytes, file-data is effectively "cat tosend.txt"

    and will send this string to the server. You can receive the full string by

    using recv() as described in the server starter code.

    You need to actually write the code that parses this http request and

    attempts to performs the specified task (which is a GET or PUT).

