#ifndef CODECRAFTERS_HTTP_SERVER_MAP_H_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_MAP_H_INCLUDED

#include "vector.h"

typedef struct Map Map;

Map *map_create(void);
void map_destroy(Map *map);
void map_put(Map *map, const char *key, const char *value);
const char *map_get(Map *map, const char *key);

typedef struct {
    char *key;
    char *value;
} KVPair;

void kvpair_destroy(void *kvpair);

Vector *map_get_items(const Map *map);

#endif
