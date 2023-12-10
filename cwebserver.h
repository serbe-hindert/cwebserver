#ifndef CWEBSERVER_LIBRARY_H
#define CWEBSERVER_LIBRARY_H

#define PORT 8080
#define MAX_REQUEST_SIZE 1024

#define DEFAULT_NUMBER_RESERVERD_ROUTES 16

// HTTP Communication
struct Request {
    char *method;
    char *url;
    unsigned int queryParametersCount;
    char *queryParameters;
    char *bodyContentType;
    unsigned int bodyLength;
    char *body;
};

struct Response {
    int httpCode;
    char *contentType;
    char *content;
};

// Routing Table
struct RoutingTableEntry {
    char *method;
    char *path;
    struct Response *(*operation)(struct Request *request);
};

struct RoutingTable {
    unsigned int currentCount;
    unsigned int routesCount;
    struct RoutingTableEntry *entries;
};

/**
 * \brief Sets up the routing table used to store the paths.
 * \return [1 if allocating memory for the table was successful]
 */
_Bool setupRoutingTable();

/**
 * \brief Registers a method-and-path tuple and a function that should be called if that tuple is equal to an incoming request.
 * \param method HTTP Method [GET, POST, PUT, ...]
 * \param path URL Subpath [/abc, /abc/def, ...]
 * \param operation function that handles the request and returns a response to it
 * \return [1 if allocating memory for the entry was succesful]
 */
_Bool registerRoute(const char *method, const char *path, struct Response *(*operation)(struct Request *request));

/**
 * \brief Sets up the socket connection and runs the server, while listening to the socket
 * \details If you wish to stop the server prematurely, simply set the KEEP_SERVER_RUNNING variable to 0
 * \return [1 if the server ran normally and didnt encounter any errors]
 */
int runServer();

#endif //CWEBSERVER_LIBRARY_H
