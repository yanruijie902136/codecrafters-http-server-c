#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_request.h"
#include "http_response.h"
#include "map.h"
#include "socket_channel.h"
#include "xmalloc.h"

static int create_server_socket(void) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        err(EXIT_FAILURE, "socket");
    }

    const int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        err(EXIT_FAILURE, "setsockopt");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(4221);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        err(EXIT_FAILURE, "bind");
    }

    const int backlog = 5;
    if (listen(server_socket, backlog) < 0) {
        err(EXIT_FAILURE, "listen");
    }

    return server_socket;
}

static HTTPResponse *handle_echo_endpoint(const HTTPRequest *request) {
    const char *target = http_request_get_target(request);
    const char *body = &target[6];
    size_t content_length = strlen(body);

    Map *headers = map_create();
    map_put(headers, "Content-Type", "text/plain");
    char content_length_str[32];
    snprintf(content_length_str, sizeof(content_length_str), "%zu", content_length);
    map_put(headers, "Content-Length", content_length_str);

    return http_response_create(HTTP_STATUS_OK, headers, xstrdup(body), content_length);
}

static HTTPResponse *handle_request(const HTTPRequest *request) {
    const char *target = http_request_get_target(request);

    if (strncmp(target, "/echo/", 6) == 0) {
        return handle_echo_endpoint(request);
    }

    if (strcmp(target, "/") == 0) {
        return http_response_create(HTTP_STATUS_OK, NULL, NULL, 0);
    }
    return http_response_create(HTTP_STATUS_NOT_FOUND, NULL, NULL, 0);
}

static void *handle_client(void *arg) {
    SocketChannel *sc = arg;
    for ( ; ; ) {
        HTTPRequest *request = http_request_from_socket_channel(sc);
        HTTPResponse *response = handle_request(request);
        http_response_write_to_socket_channel(response, sc);

        http_request_destroy(request);
        http_response_destroy(response);
    }
}

int main(int argc, char *argv[]) {
    int server_socket = create_server_socket();
    for ( ; ; ) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            err(EXIT_FAILURE, "accept");
        }

        pthread_t tid;
        int error = pthread_create(&tid, NULL, handle_client, socket_channel_create(client_socket));
        if (error) {
            errx(EXIT_FAILURE, "cannot spawn thread");
        }
        pthread_detach(tid);
    }
}
