#include "http_response.h"
#include "map.h"
#include "socket_channel.h"
#include "vector.h"
#include "xmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HTTPResponse {
    HTTPStatusCode status_code;
    Map *headers;
    char *body;
    size_t body_size;
};

HTTPResponse *http_response_create(HTTPStatusCode status_code, Map *headers, char *body, size_t body_size) {
    HTTPResponse *response = xmalloc(sizeof(HTTPResponse));
    response->status_code = status_code;
    response->headers = headers;
    response->body = body;
    response->body_size = body_size;
    return response;
}

void http_response_destroy(HTTPResponse *response) {
    map_destroy(response->headers);
    free(response->body);
    free(response);
}

static const char *status_code_to_reason_phrase(HTTPStatusCode status_code) {
    switch (status_code) {
    case HTTP_STATUS_OK:
        return "OK";
    case HTTP_STATUS_NOT_FOUND:
        return "Not Found";
    }
}

void http_response_write_to_socket_channel(const HTTPResponse *response, SocketChannel *sc) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "HTTP/1.1 %d %s\r\n",
        response->status_code, status_code_to_reason_phrase(response->status_code));
    socket_channel_write(sc, buffer, strlen(buffer));

    if (response->headers != NULL) {
        Vector *items = map_get_items(response->headers);
        for (size_t i = 0; i < vector_get_size(items); i++) {
            const KVPair *kvpair = vector_get(items, i);
            snprintf(buffer, sizeof(buffer), "%s: %s\r\n", kvpair->key, kvpair->value);
            socket_channel_write(sc, buffer, strlen(buffer));
        }
        vector_destroy(items, kvpair_destroy);
    }
    socket_channel_write(sc, "\r\n", 2);

    if (response->body != NULL) {
        socket_channel_write(sc, response->body, response->body_size);
    }
}
