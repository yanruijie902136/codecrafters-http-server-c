#ifndef CODECRAFTERS_HTTP_SERVER_MAP_H_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_MAP_H_INCLUDED

typedef struct Map Map;

Map *map_create(void);
void map_destroy(Map *map);
void map_put(Map *map, const char *key, const char *value);
const char *map_get(Map *map, const char *key);

#endif
