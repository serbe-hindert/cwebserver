# cwebserver
webserver implementation in C FOR WINDOWS ONLY

## How to link

Link the library and the required ws2_32 windows websockets.

```
...

add_library(cwebserver STATIC lib/cwebserver.c)

...

target_link_libraries(servertest PRIVATE cwebserver ${CMAKE_CURRENT_SOURCE_DIR}lib/libcwebserver.a)
target_link_libraries(servertest PRIVATE ws2_32)
```

Full example:
```
cmake_minimum_required(VERSION 3.27)
project(servertest C)

add_library(cwebserver STATIC lib/cwebserver.c)

set(CMAKE_C_STANDARD 23)

add_executable(servertest main.c)
target_link_libraries(servertest PRIVATE cwebserver ${CMAKE_CURRENT_SOURCE_DIR}lib/libcwebserver.a)
target_link_libraries(servertest PRIVATE ws2_32)

```

## How to use
Three steps: setup the routing table, register all routes and run the server

Example:

```
#include "lib/cwebserver.h"

struct Response *abc(struct Request *request) {
    return memcpy(malloc(sizeof(struct Response)), &(struct Response){
        .httpCode = 200,
        .contentType = "text/plain",
        .content = "abc !!!"
    }, sizeof(struct Response));
}

int main() {
    setupRoutingTable();
    registerRoute("GET", "/abc", abc);
    
    runServer();
    return 0;
}
```
