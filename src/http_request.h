#ifndef CODECRAFTERS_HTTP_SERVER_HTTP_REQUEST_H_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_HTTP_REQUEST_H_INCLUDED

#include "map.h"
#include "socket_channel.h"

typedef struct HTTPRequest HTTPRequest;

HTTPRequest *http_request_from_socket_channel(SocketChannel *sc);
void http_request_destroy(HTTPRequest *request);
const char *http_request_get_method(const HTTPRequest *request);
const char *http_request_get_target(const HTTPRequest *request);
const Map *http_request_get_headers(const HTTPRequest *request);

#endif
