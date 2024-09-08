#ifndef VECTOR
#define VECTOR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Vector {
    int len;
    int capacity;
    int item_size;
    void* items;
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

void* vector_index(struct Vector* self, int index) {
    if (index >= self->len || index < 0) {
        printf("out of bounds index\n");
        abort();
    }

    return self->items + index * self->item_size;
}

struct Vector* vector_new_heap(int item_size) {
    struct Vector* vector = malloc(sizeof(struct Vector));

    *vector = vector_new(item_size);

    return vector;
}

void* vector_items(struct Vector* self) { return self->items; }

void _vector_realloc(struct Vector* self) {
    self->items = realloc(self->items, self->capacity * self->item_size);
}

void _vector_realloc_if_needed(struct Vector* self) {
    if (self->capacity / 2 >= self->len) {
        self->capacity = self->len;
        _vector_realloc(self);
    }
}

void* _vector_len_offset(struct Vector* self) {
    return self->items + self->item_size * self->len;
}

// copies item memory
void vector_push(struct Vector* self, void* item) {
    if (self->capacity == 0) {
        self->capacity = 1;
        self->len = 1;

        void* copy = malloc(self->item_size);
        memcpy(copy, item, self->item_size);

        self->items = copy;

        return;
    }

    void* offset = _vector_len_offset(self);
    self->len++;

    // need bigger array
    if (self->len > self->capacity) {
        self->capacity *= 2;
        _vector_realloc(self);
        // vector realloc changes self.items pointer
        offset = _vector_len_offset(self) - self->item_size;
    }

    memcpy(offset, item, self->item_size);
}

void vector_free(struct Vector* self) {
    self->len = 0;
    self->capacity = 0;
    free(self->items);
    self->items = NULL;
}

void vector_remove(struct Vector* self, int index) {
    if (index != self->len - 1) {
        for (int i = index + 1; i < self->len; i++) {
            void* next = vector_index(self, i);
            void* prev = vector_index(self, i - 1);
            memcpy(prev, next, self->item_size);
        }
    }

    self->len--;

    _vector_realloc_if_needed(self);
}

void vector_pop(struct Vector* self) { vector_remove(self, self->len - 1); }

#endif
