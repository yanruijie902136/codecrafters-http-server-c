#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

int main(int argc, char *argv[]) {
    int server_socket = create_server_socket();
    for ( ; ; ) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            err(EXIT_FAILURE, "accept");
        }

        close(client_socket);
    }
}
