#include "map.h"
#include "xmalloc.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct AANode AANode;
struct AANode {
    char *key;
    char *value;
    size_t level;
    AANode *lch;
    AANode *rch;
};

static AANode *aa_node_create(const char *key, const char *value) {
    AANode *node = xmalloc(sizeof(AANode));
    node->key = xstrdup(key);
    node->value = xstrdup(value);
    node->level = 1;
    node->lch = node->rch = NULL;
    return node;
}

static void aa_node_destroy(AANode *node) {
    if (node == NULL) {
        return;
    }
    aa_node_destroy(node->lch);
    aa_node_destroy(node->rch);
    free(node->key);
    free(node->value);
    free(node);
}

static void aa_skew(AANode **rootp) {
    AANode *root = *rootp;
    if (root == NULL || root->lch == NULL || root->level != root->lch->level) {
        return;
    }
    AANode *lch = root->lch;
    root->lch = lch->rch;
    lch->rch = root;
    *rootp = lch;
}

static void aa_split(AANode **rootp) {
    AANode *root = *rootp;
    if (root == NULL || root->rch == NULL || root->rch->rch == NULL || root->level != root->rch->rch->level) {
        return;
    }
    AANode *rch = root->rch;
    root->rch = rch->lch;
    rch->lch = root;
    rch->level++;
    *rootp = rch;
}

static void aa_insert(AANode **rootp, const char *key, const char *value) {
    AANode *root = *rootp;
    if (root == NULL) {
        *rootp = aa_node_create(key, value);
        return;
    }

    int c = strcmp(key, root->key);
    if (c < 0) {
        aa_insert(&root->lch, key, value);
    } else if (c > 0) {
        aa_insert(&root->rch, key, value);
    } else {
        free(root->value);
        root->value = xstrdup(value);
    }

    aa_skew(&root);
    aa_split(&root);
    *rootp = root;
}

static const AANode *aa_search(const AANode *root, const char *key) {
    if (root == NULL) {
        return NULL;
    }
    int c = strcmp(key, root->key);
    if (c == 0) {
        return root;
    }
    return aa_search(c < 0 ? root->lch : root->rch, key);
}

struct Map {
    AANode *root;
};

Map *map_create(void) {
    Map *map = xmalloc(sizeof(Map));
    map->root = NULL;
    return map;
}

void map_destroy(Map *map) {
    aa_node_destroy(map->root);
    free(map);
}

void map_put(Map *map, const char *key, const char *value) {
    aa_insert(&map->root, key, value);
}

const char *map_get(Map *map, const char *key) {
    const AANode *node = aa_search(map->root, key);
    return node == NULL ? NULL : node->value;
}
