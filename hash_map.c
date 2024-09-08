#ifndef HASH_MAP
#define HASH_MAP

#include <stdio.h>
#include <string.h>

#include "vector.c"

struct HashItem {
    char* key;
    void* value;
};

struct HashMap {
    struct Vector* vec;
};

int _hash_string(char* key) {
    int hash = 0;
    // the worst hash function in existence xd
    for (int i = 0; i < strlen(key); i++) {
        hash += key[i];
    }
    return hash;
}

struct Vector* _hash_map_node(struct HashMap* self, char* key) {
    int index = _hash_string(key) % self->vec->len;

    struct Vector* node = vector_index(self->vec, index);

    return node;
}

// check for NULL
struct HashItem* _hash_map_get(struct HashMap* self, char* key) {
    struct Vector* node = _hash_map_node(self, key);

    for (int i = 0; i < node->len; i++) {
        struct HashItem* item = vector_index(node, i);

        if (strcmp(item->key, key) == 0) {
            return item;
        }
    }

    return NULL;
}

struct HashMap hash_map_new() {
    // hashmap won't increase in capacity for now as that is complicated
    int capacity = 10;
    struct HashMap hash_map = {
        .vec = NULL,
    };

    struct Vector* root = vector_new_heap(sizeof(struct Vector));

    for (int i = 0; i < capacity; i++) {
        struct Vector* node = vector_new_heap(sizeof(struct HashItem));
        vector_push(root, node);
        free(node);
    }

    hash_map.vec = root;

    return hash_map;
}

void hash_map_set(struct HashMap* self, char* key, void* value) {
    struct HashItem* hash_item = _hash_map_get(self, key);
    struct Vector* node = _hash_map_node(self, key);

    if (hash_item != NULL) {
        hash_item->value = value;
        return;
    }

    hash_item = malloc(sizeof(struct HashItem));
    hash_item->value = value;
    hash_item->key = key;

    vector_push(node, hash_item);

    free(hash_item);
}

// check for NULL
void* hash_map_get(struct HashMap* self, char* key) {
    struct HashItem* item = _hash_map_get(self, key);
    if (item == NULL) {
        return NULL;
    }

    return item->value;
}

void hash_map_remove(struct HashMap* self, char* key) {
    struct Vector* node = _hash_map_node(self, key);

    for (int i = 0; i < node->len; i++) {
        struct HashItem* item = vector_index(node, i);

        if (strcmp(item->key, key) == 0) {
            vector_remove(node, i);
            return;
        }
    }
}

#endif
