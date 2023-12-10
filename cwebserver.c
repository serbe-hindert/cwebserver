#include "cwebserver.h"

struct Request *translatePlainToRequest(char *plain, _Bool *hasQueryParamters, _Bool *hasBody);
void handleRequest(SOCKET client);
struct Response *route(struct Request *request);

_Bool KEEP_SERVER_ALIVE;
static struct RoutingTable *ROUTING_TABLE;

_Bool setupRoutingTable() {
    ROUTING_TABLE = malloc(sizeof(struct RoutingTable));
    ROUTING_TABLE->currentCount = 0;
    ROUTING_TABLE->routesCount = DEFAULT_NUMBER_RESERVERD_ROUTES;
    ROUTING_TABLE->entries = malloc(DEFAULT_NUMBER_RESERVERD_ROUTES * sizeof(struct RoutingTableEntry));
    return !(ROUTING_TABLE == NULL || ROUTING_TABLE->entries == NULL);
}

_Bool registerRoute(const char *method, const char *path, struct Response *(*operation)(struct Request *request)) {
    if (ROUTING_TABLE->currentCount == ROUTING_TABLE->routesCount) {
        ROUTING_TABLE->routesCount *= 2;
        ROUTING_TABLE->entries = realloc(ROUTING_TABLE->entries, ROUTING_TABLE->routesCount * sizeof(struct RoutingTableEntry));
        if (ROUTING_TABLE->entries == NULL) {
            return 0;
        }
    }

    ROUTING_TABLE->entries[ROUTING_TABLE->currentCount].method = strdup(method);
    ROUTING_TABLE->entries[ROUTING_TABLE->currentCount].path = strdup(path);
    ROUTING_TABLE->entries[ROUTING_TABLE->currentCount].operation = operation;

    // Check if allocation worked
    if (ROUTING_TABLE->entries[ROUTING_TABLE->currentCount].method == NULL || ROUTING_TABLE->entries[ROUTING_TABLE->currentCount].path == NULL) {
        free(ROUTING_TABLE->entries[ROUTING_TABLE->currentCount].method);
        free(ROUTING_TABLE->entries[ROUTING_TABLE->currentCount].path);
        return 0;
    }

    ROUTING_TABLE->currentCount++;
    return 1;
}

struct Request *translatePlainToRequest(char *plain, _Bool *hasQueryParamters, _Bool *hasBody) {
    struct Request *request = malloc(sizeof(struct Request));

    // METHOD
    int currentInterestLength = strchr(plain, ' ') - plain;
    // allocate one extra for the null terminator
    request->method = malloc(currentInterestLength + 1);
    strncpy(request->method, plain, currentInterestLength);
    request->method[currentInterestLength] = '\0';

    // URL + QUERY PARAMETERS
    const char *ofCurrentInterestPointer = plain + currentInterestLength + 1;
    const int entireUrlLength = strchr(ofCurrentInterestPointer, ' ') - ofCurrentInterestPointer;
    currentInterestLength = entireUrlLength;
    const char *queryParametersOffset = strchr(ofCurrentInterestPointer, '?');


    // if query parameters exist
    if (queryParametersOffset != NULL && queryParametersOffset < ofCurrentInterestPointer + entireUrlLength) {
        *hasQueryParamters = 1;
        // shorten the url length
        const int queryParameterLength = entireUrlLength - (queryParametersOffset - ofCurrentInterestPointer);
        currentInterestLength -= queryParameterLength;

        request->queryParameters = malloc(queryParameterLength + 1);
        request->queryParametersCount = 1;

        for (int i = 1; i < entireUrlLength - currentInterestLength; i++) {
            if (queryParametersOffset[i] == '&') {
                request->queryParameters[i - 1] = '\0';
                request->queryParametersCount++;
            } else if (queryParametersOffset[i] == '+') {
                request->queryParameters[i - 1] = ' ';
            } else {
                request->queryParameters[i - 1] = queryParametersOffset[i];
            }
        }
        request->queryParameters[entireUrlLength - currentInterestLength] = '\0'; // Add null terminator
    }
    request->url = malloc(currentInterestLength + 1);
    strncpy(request->url, ofCurrentInterestPointer, currentInterestLength);
    request->url[currentInterestLength] = '\0'; // Add null terminator

    // CONTENT
    // forward pointer to Content-Type:
    ofCurrentInterestPointer = strstr(ofCurrentInterestPointer, "Content-Type:");
    if (ofCurrentInterestPointer != NULL) {
        *hasBody = 1;
        ofCurrentInterestPointer += 14;
        currentInterestLength = strchr(ofCurrentInterestPointer, '\n') - ofCurrentInterestPointer;
        request->bodyContentType = malloc(currentInterestLength + 1);
        strncpy(request->bodyContentType, ofCurrentInterestPointer, currentInterestLength);
        // forward pointer to Content-Length
        ofCurrentInterestPointer = strstr(ofCurrentInterestPointer, "Content-Length:");
        if (ofCurrentInterestPointer != NULL) {
            ofCurrentInterestPointer = strchr(ofCurrentInterestPointer, '\n');
            request->bodyLength = strlen(ofCurrentInterestPointer);
            request->body = malloc(request->bodyLength * sizeof(char));
            strncpy(request->body, ofCurrentInterestPointer, request->bodyLength);
        }
    }

    return request;
}

void handleRequest(SOCKET client) {
    char plain[MAX_REQUEST_SIZE] = {0};
    _Bool hasQueryParameters, hasBody = 0;

    recv(client, plain, sizeof(plain) - 1, 0);

    struct Request *request = translatePlainToRequest(plain, &hasQueryParameters, &hasBody);

    struct Response *response = route(request);

    free(request->method);
    free(request->url);
    if (hasQueryParameters) {
        free(request->queryParameters);
        request->queryParametersCount = 0;
    }
    if (hasBody) {
        free(request->bodyContentType);
        request->bodyLength = 0;
        free(request->body);
    }
    free(request);

    // 38 staticly counted
    const int responseLength = 38 + strlen(response->contentType) + strlen(response->content);
    char *textResponse = malloc(responseLength);
    snprintf(textResponse, responseLength, "HTTP/1.1 %d\r\nContent-Type: %s\r\n\r\n%s", response->httpCode, response->contentType, response->content);

    free(response->contentType);
    free(response->content);
    free(response);

    send(client, textResponse, strlen(textResponse), 0);

    free(textResponse);

    closesocket(client);
}

struct Response *route(struct Request *request) {
    for (int i = 0; i < ROUTING_TABLE->currentCount; i++) {
        if (strcmp(ROUTING_TABLE->entries[i].method, request->method) == 0 && strcmp(ROUTING_TABLE->entries[i].path, request->url) == 0) {
            return ROUTING_TABLE->entries[i].operation(request);
        }
    }

    struct Response *response = malloc(sizeof(struct Response));
    struct Response def = {
        .httpCode = 400,
        .contentType = strdup("text/plain"),
        .content = strdup("Not Found")
    };
    memcpy(response, &def, sizeof(struct Response));
    return response;
}

int runServer() {
    WSADATA wsa;
    SOCKET server, client;
    struct sockaddr_in serverAddress, clientAddress;
    int addressSize = sizeof(struct sockaddr_in);

    // init winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        perror("ERROR init Winsock");
        return EXIT_FAILURE;
    }

    // create server socket
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        perror("ERROR creating Server-Sockets");
        WSACleanup();
        return EXIT_FAILURE;
    }

    // configure the server address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    // bind server socket to the address and the port specified
    if (bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        perror("ERROR binding Server-Sockets");
        closesocket(server);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // listen, final error check in initialisation
    if (listen(server, 10) == SOCKET_ERROR) {
        perror("ERROR proving existance of Server-Socket");
        closesocket(server);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Keep the server alive
    KEEP_SERVER_ALIVE = 1;

    printf("Webserver runs at http://localhost:%d\n", PORT);

    do {
        // wait for client...
        client = accept(server, (struct sockaddr*)&clientAddress, &addressSize);
        if (client == INVALID_SOCKET) {
            perror("ERROR on client address");
            continue;
        }
        handleRequest(client);
        // handle possible client requests
    } while (KEEP_SERVER_ALIVE);

    // destroy the routing table
    for (int i = 0; i < ROUTING_TABLE->currentCount; i++) {
        free(ROUTING_TABLE->entries[i].method);
        free(ROUTING_TABLE->entries[i].path);
    }
    free(ROUTING_TABLE->entries);
    free(ROUTING_TABLE);

    return EXIT_SUCCESS;
}