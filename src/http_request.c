#include "http_request.h"
#include "map.h"
#include "socket_channel.h"
#include "xmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HTTPRequest {
    char *method;
    char *target;
    Map *headers;
};

static HTTPRequest *http_request_create(char *method, char *target, Map *headers) {
    HTTPRequest *request = xmalloc(sizeof(HTTPRequest));
    request->method = method;
    request->target = target;
    request->headers = headers;
    return request;
}

static char *strip(const char *str) {
    const char *lp = str;
    while (*lp == ' ') {
        lp++;
    }
    if (*lp == '\0') {
        return xstrdup("");
    }
    const char *rp = str + strlen(str) - 1;
    while (*rp == ' ') {
        rp--;
    }
    return xstrndup(lp, rp - lp + 1);
}

HTTPRequest *http_request_from_socket_channel(SocketChannel *sc) {
    char *res = socket_channel_read_until(sc, "\r\n");
    char *method = xstrdup(strtok(res, " "));
    char *target = xstrdup(strtok(NULL, " "));
    free(res);

    Map *headers = map_create();
    for ( ; ; ) {
        res = socket_channel_read_until(sc, "\r\n");
        if (*res == '\0') {
            free(res);
            break;
        }

        char *colonp = strchr(res, ':');
        *colonp = '\0';
        char *value = strip(colonp + 1);
        map_put(headers, res, value);
        free(value);
    }

    return http_request_create(method, target, headers);
}

void http_request_destroy(HTTPRequest *request) {
    free(request->method);
    free(request->target);
    map_destroy(request->headers);
    free(request);
}

const char *http_request_get_method(const HTTPRequest *request) {
    return request->method;
}

const char *http_request_get_target(const HTTPRequest *request) {
    return request->target;
}

const Map *http_request_get_headers(const HTTPRequest *request) {
    return request->headers;
}
