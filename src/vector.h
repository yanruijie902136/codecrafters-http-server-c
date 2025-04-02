#ifndef CODECRAFTERS_HTTP_SERVER_VECTOR_H_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_VECTOR_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

typedef struct Vector Vector;

Vector *vector_create(void);
void vector_destroy(Vector *vector, void (*element_destroy)(void *));
size_t vector_get_size(const Vector *vector);
const void *vector_get(const Vector *vector, size_t i);
void vector_push_back(Vector *vector, void *element);
bool vector_contains(const Vector *vector, const void *element, int (*compare)(const void *, const void *));

#endif
