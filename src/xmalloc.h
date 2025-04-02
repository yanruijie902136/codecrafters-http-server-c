#ifndef CODECRAFTERS_HTTP_SERVER_XMALLOC_H_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_XMALLOC_H_INCLUDED

#include <stddef.h>

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *str);
char *xstrndup(const char *str, size_t n);

#endif
