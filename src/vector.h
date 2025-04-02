#ifndef CODECRAFTERS_HTTP_SERVER_VECTOR_H_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_VECTOR_H_INCLUDED

#include <stddef.h>

typedef struct Vector Vector;

Vector *vector_create(void);
void vector_destroy(Vector *vector, void (*element_destroy)(void *));
size_t vector_get_size(const Vector *vector);
const void *vector_get(const Vector *vector, size_t i);
void vector_push_back(Vector *vector, void *element);

#endif
