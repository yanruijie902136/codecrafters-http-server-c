#include "socket_channel.h"
#include "xmalloc.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct SocketChannel {
    int sockfd;
    char *buffer;
    size_t buffer_size;
    char *current;
    char *end;
};

SocketChannel *socket_channel_create(int sockfd) {
    SocketChannel *sc = xmalloc(sizeof(SocketChannel));
    sc->sockfd = sockfd;
    sc->buffer_size = 16;
    sc->buffer = xmalloc(sc->buffer_size);
    sc->current = sc->end = sc->buffer;
    return sc;
}

void socket_channel_destroy(SocketChannel *sc) {
    close(sc->sockfd);
    free(sc->buffer);
    free(sc);
}

static char *find_separator(char *current, char *end, const char *separator) {
    size_t n = strlen(separator);
    while (current + n <= end) {
        if (memcmp(current, separator, n) == 0) {
            return current;
        }
        current++;
    }
    return end;
}

static size_t socket_channel_read_new_data(SocketChannel *sc) {
    size_t len = sc->end - sc->current;
    memmove(sc->buffer, sc->current, len);

    if (sc->end == sc->buffer + sc->buffer_size) {
        sc->buffer_size *= 2;
        sc->buffer = xrealloc(sc->buffer, sc->buffer_size);
    }

    sc->current = sc->buffer;
    sc->end = sc->buffer + len;

    size_t left = sc->buffer + sc->buffer_size - sc->end;
    ssize_t nr = read(sc->sockfd, sc->end, left);
    if (nr < 0) {
        err(EXIT_FAILURE, "read from socket %d", sc->sockfd);
    }
    sc->end += nr;
    return nr;
}

char *socket_channel_read_exact(SocketChannel *sc, size_t size) {
    while (sc->current + size > sc->end) {
        if (socket_channel_read_new_data(sc) == 0) {
            return NULL;
        }
    }
    char *res = xstrndup(sc->current, size);
    sc->current += size;
    return res;
}

char *socket_channel_read_until(SocketChannel *sc, const char *separator) {
    for ( ; ; ) {
        char *p = find_separator(sc->current, sc->end, separator);
        if (p != sc->end) {
            char *res = xstrndup(sc->current, p - sc->current);
            sc->current = p + strlen(separator);
            return res;
        }

        size_t nr = socket_channel_read_new_data(sc);
        if (nr == 0) {
            return NULL;
        }
    }
}

void socket_channel_write(SocketChannel *sc, const void *buffer, size_t size) {
    const char *p = buffer;
    while (size > 0) {
        ssize_t nw = write(sc->sockfd, p, size);
        if (nw < 0) {
            err(EXIT_FAILURE, "write to socket %d", sc->sockfd);
        }
        p += nw;
        size -= nw;
    }
}
