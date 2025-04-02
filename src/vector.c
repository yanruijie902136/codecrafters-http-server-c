#include "vector.h"
#include "xmalloc.h"

#include <assert.h>
#include <stdlib.h>

struct Vector {
    void **elements;
    size_t capacity;
    size_t size;
};

Vector *vector_create(void) {
    Vector *vector = xmalloc(sizeof(Vector));
    vector->capacity = 10;
    vector->elements = xmalloc(sizeof(void *) * vector->capacity);
    vector->size = 0;
    return vector;
}

void vector_destroy(Vector *vector, void (*element_destroy)(void *)) {
    for (size_t i = 0; i < vector->size; i++) {
        element_destroy(vector->elements[i]);
    }
    free(vector->elements);
    free(vector);
}

size_t vector_get_size(const Vector *vector) {
    return vector->size;
}

const void *vector_get(const Vector *vector, size_t i) {
    assert(i < vector->size);
    return vector->elements[i];
}

void vector_push_back(Vector *vector, void *element) {
    if (vector->size == vector->capacity) {
        vector->capacity *= 2;
        vector->elements = xrealloc(vector->elements, sizeof(void *) * vector->capacity);
    }
    vector->elements[vector->size++] = element;
}

bool vector_contains(const Vector *vector, const void *element, int (*element_compare)(const void *, const void *)) {
    for (size_t i = 0; i < vector->size; i++) {
        if (element_compare(element, vector->elements[i]) == 0) {
            return true;
        }
    }
    return false;
}
