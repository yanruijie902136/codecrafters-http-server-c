#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zconf.h>
#include <zlib.h>

#include "http_request.h"
#include "http_response.h"
#include "map.h"
#include "socket_channel.h"
#include "vector.h"
#include "xmalloc.h"

static int root_dirfd;

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

static char *size_t_to_str(size_t n) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%zu", n);
    return xstrdup(buffer);
}

static Vector *split_string(const char *str, const char *delimiters) {
    Vector *tokens = vector_create();
    char *s = xstrdup(str);
    char *last;
    char *token = strtok_r(s, delimiters, &last);
    while (token != NULL) {
        vector_push_back(tokens, xstrdup(token));
        token = strtok_r(NULL, delimiters, &last);
    }
    free(s);
    return tokens;
}

static int string_compare(const void *s1, const void *s2) {
    return strcmp(s1, s2);
}

static void gzip_compress(char *dest, size_t *dest_len, const char *source, size_t source_len) {
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = (uInt)source_len;
    zs.next_in = (Bytef *)source;
    zs.avail_out = (uInt)(*dest_len);
    zs.next_out = (Bytef *)dest;
    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    deflate(&zs, Z_FINISH);
    deflateEnd(&zs);
    *dest_len = zs.total_out;
}

static HTTPResponse *handle_echo_endpoint(const HTTPRequest *request) {
    const char *target = http_request_get_target(request);
    const char *message = &target[6];

    Map *response_headers = map_create();
    map_put(response_headers, "Content-Type", "text/plain");

    bool has_gzip = false;
    const char *accept_encoding = map_get(http_request_get_headers(request), "Accept-Encoding");
    if (accept_encoding != NULL) {
        Vector *compression_schemes = split_string(accept_encoding, ", ");
        if (vector_contains(compression_schemes, "gzip", string_compare)) {
            has_gzip = true;
        }
        vector_destroy(compression_schemes, free);
    }

    char *body;
    size_t body_size;
    if (has_gzip) {
        map_put(response_headers, "Content-Encoding", "gzip");
        body_size = 8192;
        body = xmalloc(body_size);
        gzip_compress(body, &body_size, message, strlen(message));
    } else {
        body = xstrdup(message);
        body_size = strlen(message);
    }

    char *s = size_t_to_str(body_size);
    map_put(response_headers, "Content-Length", s);
    free(s);

    return http_response_create(HTTP_STATUS_OK, response_headers, body);
}

static HTTPResponse *handle_files_endpoint_get(const HTTPRequest *request) {
    const char *target = http_request_get_target(request);
    const char *filename = &target[7];

    int fd = openat(root_dirfd, filename, O_RDONLY);
    if (fd < 0) {
        return http_response_create(HTTP_STATUS_NOT_FOUND, NULL, NULL);
    }

    struct stat statbuf;
    fstat(fd, &statbuf);
    size_t content_length = statbuf.st_size;

    char *body = xmalloc(content_length);
    char *p = body;
    for (size_t left = content_length; left > 0; ) {
        ssize_t nr = read(fd, p, left);
        if (nr < 0) {
            err(EXIT_FAILURE, "%s", filename);
        }
        p += nr;
        left -= nr;
    }

    close(fd);

    Map *headers = map_create();
    map_put(headers, "Content-Type", "application/octet-stream");
    char *content_length_str = size_t_to_str(content_length);
    map_put(headers, "Content-Length", content_length_str);
    free(content_length_str);

    const char *connection = map_get(http_request_get_headers(request), "Connection");
    if (connection != NULL) {
        map_put(headers, "Connection", connection);
    }

    return http_response_create(HTTP_STATUS_OK, headers, body);
}

static HTTPResponse *handle_files_endpoint_post(const HTTPRequest *request) {
    const char *target = http_request_get_target(request);
    const char *filename = &target[7];

    int fd = openat(root_dirfd, filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        err(EXIT_FAILURE, "%s", filename);
    }

    const Map *request_headers = http_request_get_headers(request);
    const char *content_length_str = map_get(request_headers, "Content-Length");
    size_t left = strtoumax(content_length_str, NULL, 10);
    const char *p = http_request_get_body(request);
    while (left > 0) {
        ssize_t nw = write(fd, p, left);
        if (nw < 0) {
            err(EXIT_FAILURE, "%s", filename);
        }
        p += nw;
        left -= nw;
    }

    close(fd);

    Map *response_headers = map_create();
    const char *connection = map_get(http_request_get_headers(request), "Connection");
    if (connection != NULL) {
        map_put(response_headers, "Connection", connection);
    }

    return http_response_create(HTTP_STATUS_CREATED, NULL, NULL);
}

static HTTPResponse *handle_files_endpoint(const HTTPRequest *request) {
    const char *method = http_request_get_method(request);
    if (strcmp(method, "POST") == 0) {
        return handle_files_endpoint_post(request);
    }
    return handle_files_endpoint_get(request);
}

static HTTPResponse *handle_user_agent_endpoint(const HTTPRequest *request) {
    const Map *request_headers = http_request_get_headers(request);
    const char *user_agent = map_get(request_headers, "User-Agent");
    size_t content_length = strlen(user_agent);

    Map *response_headers = map_create();
    map_put(response_headers, "Content-Type", "text/plain");
    char *content_length_str = size_t_to_str(content_length);
    map_put(response_headers, "Content-Length", content_length_str);
    free(content_length_str);

    const char *connection = map_get(http_request_get_headers(request), "Connection");
    if (connection != NULL) {
        map_put(response_headers, "Connection", connection);
    }

    return http_response_create(HTTP_STATUS_OK, response_headers, xstrdup(user_agent));
}

static HTTPResponse *handle_request(const HTTPRequest *request) {
    const char *target = http_request_get_target(request);

    if (strncmp(target, "/echo/", 6) == 0) {
        return handle_echo_endpoint(request);
    }

    if (strncmp(target, "/files/", 7) == 0) {
        return handle_files_endpoint(request);
    }

    if (strcmp(target, "/user-agent") == 0) {
        return handle_user_agent_endpoint(request);
    }

    Map *response_headers = map_create();
    const char *connection = map_get(http_request_get_headers(request), "Connection");
    if (connection != NULL) {
        map_put(response_headers, "Connection", connection);
    }

    if (strcmp(target, "/") == 0) {
        return http_response_create(HTTP_STATUS_OK, response_headers, NULL);
    }
    return http_response_create(HTTP_STATUS_NOT_FOUND, response_headers, NULL);
}

static void *handle_client(void *arg) {
    SocketChannel *sc = arg;

    for ( ; ; ) {
        HTTPRequest *request = http_request_from_socket_channel(sc);
        if (request == NULL) {
            break;
        }

        const Map *headers = http_request_get_headers(request);
        const char *connection = map_get(headers, "Connection");
        bool close_connection = false;
        if (connection != NULL && strcmp(connection, "close") == 0) {
            close_connection = true;
        }

        HTTPResponse *response = handle_request(request);
        http_response_write_to_socket_channel(response, sc);

        http_request_destroy(request);
        http_response_destroy(response);

        if (close_connection) {
            break;
        }
    }

    socket_channel_destroy(sc);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc == 3) {
        assert(strcmp(argv[1], "--directory") == 0);
        root_dirfd = dirfd(opendir(argv[2]));
    } else {
        root_dirfd = AT_FDCWD;
    }

    int server_socket = create_server_socket();
    for ( ; ; ) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            err(EXIT_FAILURE, "accept");
        }

        pthread_t tid;
        SocketChannel *sc = socket_channel_create(client_socket);
        int error = pthread_create(&tid, NULL, handle_client, sc);
        if (error) {
            errx(EXIT_FAILURE, "cannot spawn thread");
        }
        pthread_detach(tid);
    }
}
