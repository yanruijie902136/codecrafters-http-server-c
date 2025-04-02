#include "xmalloc.h"

#include <err.h>
#include <stdlib.h>
#include <string.h>

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        err(EXIT_FAILURE, "malloc");
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    ptr = realloc(ptr, size);
    if (ptr == NULL) {
        err(EXIT_FAILURE, "realloc");
    }
    return ptr;
}

char *xstrdup(const char *str) {
    char *s = strdup(str);
    if (s == NULL) {
        err(EXIT_FAILURE, "strdup");
    }
    return s;
}

char *xstrndup(const char *str, size_t n) {
    char *s = strndup(str, n);
    if (s == NULL) {
        err(EXIT_FAILURE, "strndup");
    }
    return s;
}
