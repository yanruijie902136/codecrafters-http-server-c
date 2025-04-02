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
#include "socket_channel.h"

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

static void *handle_client(void *arg) {
    SocketChannel *sc = arg;
    for ( ; ; ) {
        HTTPRequest *request = http_request_from_socket_channel(sc);
        fprintf(stderr, "method = %s\n", http_request_get_method(request));
        const char *target = http_request_get_target(request);
        fprintf(stderr, "target = %s\n", target);

        const char *response = "HTTP/1.1 200 OK\r\n\r\n";
        if (strcmp(target, "/") != 0) {
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
        socket_channel_write(sc, response, strlen(response));

        http_request_destroy(request);
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
