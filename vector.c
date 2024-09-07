#include <stdlib.h>
#include <string.h>

struct Vector {
    int len;
    int capacity;
    int item_size;
    void *items;
};

struct Vector vector_new(int item_size) {
    struct Vector vector = {
        .len = 0,
        .capacity = 0,
        .item_size = item_size,
        .items = NULL,
    };
    return vector;
}

void *vector_index(struct Vector *self, int index) {
    return self->items + index * self->item_size;
}

struct Vector *vector_new_heap(int item_size) {
    struct Vector *vector = malloc(sizeof(struct Vector));

    *vector = vector_new(item_size);

    return vector;
}

void *vector_items(struct Vector *self) { return self->items; }

void _vector_realloc(struct Vector *self) {
    self->items = realloc(self->items, self->capacity * self->item_size);
}

// copies item memory
void vector_push(struct Vector *self, void *item) {
    if (self->capacity == 0) {
        self->capacity = 1;
        self->len = 1;

        void *copy = malloc(self->item_size);
        memcpy(copy, item, self->item_size);

        self->items = copy;

        return;
    }

    void *offset = self->items + self->item_size * self->len;
    self->len++;

    // need bigger array
    if (self->len > self->capacity) {
        self->capacity *= 2;
        _vector_realloc(self);
    }

    memcpy(offset, item, self->item_size);
}

// don't forget to free items before this call
void vector_clear(struct Vector *self) {
    self->len = 0;
    self->capacity = 0;
    self->items = NULL;
}

void vector_pop(struct Vector *self) {
    if (self->len == 0) {
        return;
    }

    self->len--;

    if (self->capacity / 2 >= self->len) {
        self->capacity = self->len;
        _vector_realloc(self);
    }
}
