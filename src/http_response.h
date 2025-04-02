#ifndef CODECRAFTERS_HTTP_SERVER_HTTP_RESPONSE_H_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_HTTP_RESPONSE_H_INCLUDED

#include "map.h"
#include "socket_channel.h"

typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_NOT_FOUND = 404,
} HTTPStatusCode;

typedef struct HTTPResponse HTTPResponse;

HTTPResponse *http_response_create(HTTPStatusCode status_code, Map *headers, char *body);
void http_response_destroy(HTTPResponse *response);
void http_response_write_to_socket_channel(const HTTPResponse *response, SocketChannel *sc);

#endif
