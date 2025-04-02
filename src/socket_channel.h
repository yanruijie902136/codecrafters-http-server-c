#ifndef CODECRAFTERS_HTTP_SERVER_SOCKET_CHANNEL_H_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_SOCKET_CHANNEL_H_INCLUDED

#include <stddef.h>

typedef struct SocketChannel SocketChannel;

SocketChannel *socket_channel_create(int sockfd);
void socket_channel_destroy(SocketChannel *sc);
char *socket_channel_read_exact(SocketChannel *sc, size_t size);
char *socket_channel_read_until(SocketChannel *sc, const char *separator);
void socket_channel_write(SocketChannel *sc, const void *buffer, size_t size);

#endif
