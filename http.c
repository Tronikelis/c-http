#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "err.c"
#include "hash_map.c"
#include "vector.c"

enum Method {
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
    HEAD,
};

struct Request {
    int fd;
    char* raw;

    struct HashMap headers;
    char* body;
    enum Method method;
    char* path;
};

struct Request request_new(int fd) {
    struct Request request = {
        .method = -1,
        .body = NULL,
        .raw = NULL,
        .path = NULL,
        .fd = fd,
        .headers = hash_map_new(),
    };

    return request;
}

int request_parse_raw(struct Request* self) {
    struct Vector raw = vector_new(sizeof(char));

    char buf[256];
    int read_count;
    while ((read_count = read(self->fd, buf, sizeof(buf))) != 0) {
        if (read_count == -1) {
            return err();
        }

        for (int i = 0; i < read_count; i++) {
            vector_push(&raw, &buf[i]);
        }

        char* end = "\r\n\r\n";
        char* items = raw.items;
        int len = raw.len;

        if (raw.len >= 4 && items[len - 1] == '\n' && items[len - 2] == '\r' &&
            items[len - 3] == '\n' && items[len - 4] == '\r') {
            break;
        }
    }

    // convert items vec into valid string
    self->raw = malloc(raw.len + 1);
    memcpy(self->raw, raw.items, raw.len);
    self->raw[raw.len] = '\0';

    vector_free(&raw);

    return 0;
}

int request_parse_path(struct Request* self) {
    if (self->raw == NULL) {
        if (request_parse_raw(self) != 0) {
            return err();
        }
    }

    char* raw_owned = strdup(self->raw);
    char* raw_owned_og = raw_owned;

    strsep(&raw_owned, " "); // skip method
    char* path = strsep(&raw_owned, " ");

    self->path = strdup(path);
    free(raw_owned_og);

    return 0;
}

int request_parse_headers(struct Request* self) {
    if (self->raw == NULL) {
        if (request_parse_raw(self) != 0) {
            return err();
        }
    }

    struct HashMap headers = hash_map_new();
    char* raw_owned = strdup(self->raw);
    char* raw_owned_og = raw_owned;

    bool first = true;
    for (char* token = strtok(raw_owned, "\r\n"); token != NULL;
         token = strtok(NULL, "\r\n")) {

        // stop when hitting http meta end \r\n\r\n (body is after)
        if (strlen(token) == 0) {
            break;
        }

        if (first) {
            first = false;
            continue;
        }

        token = strdup(token);
        char* token_og = token;

        char* key = strdup(strsep(&token, ":"));
        char* value = strdup(strsep(&token, ":"));

        char* value_og = value;
        value++;
        value = strdup(value);
        free(value_og);

        hash_map_set(&headers, key, value);

        free(token_og);
    }

    self->headers = headers;
    free(raw_owned_og);

    return 0;
}

int request_parse_method(struct Request* self) {
    if (self->raw == NULL) {
        if (request_parse_raw(self) != 0) {
            return err();
        }
    }

    char* raw_owned = strdup(self->raw);
    char* raw_owned_og = raw_owned;

    char* method = strsep(&raw_owned, " ");

    if (strcmp(method, "GET")) {
        self->method = GET;
    } else if (strcmp(method, "POST")) {
        self->method = POST;
    } else if (strcmp(method, "PUT")) {
        self->method = PUT;
    } else if (strcmp(method, "PATCH")) {
        self->method = PATCH;
    } else if (strcmp(method, "DELETE")) {
        self->method = DELETE;
    } else if (strcmp(method, "HEAD")) {
        self->method = HEAD;
    } else {
        printf("unknown method: %s\n", method);
        free(raw_owned);
        return 1;
    }

    free(raw_owned_og);
    return 0;
}

void request_free(struct Request* self) {
    if (self->raw != NULL)
        free(self->raw);
    if (self->path != NULL)
        free(self->path);
    if (self->body != NULL)
        free(self->body);

    hash_map_free(&self->headers);
}

enum HttpStatus {
    OK = 200,
    ServerError = 500,
    BadRequest = 400,
};

struct Response {
    char* body;
    enum HttpStatus http_status;
    struct HashMap headers;
};

struct Response response_new() {
    struct Response response = {
        .body = NULL,
        .http_status = OK,
        .headers = hash_map_new(),
    };

    return response;
}

void response_free(struct Response* self) {
    if (self->body != NULL)
        free(self->body);

    hash_map_free(&self->headers);
}

void strcat_realloc(char** dest, char* src) {
    int new_strlen = strlen(src) + strlen(*dest);
    *dest = realloc(*dest, new_strlen + 1);
    strcat(*dest, src);
}

char* response_gen(struct Response* self) {
    char* version = "HTTP/1.1 ";
    char* status;

    switch (self->http_status) {
    case OK:
        status = "200 OK";
        break;
    case BadRequest:
        status = "400 Bad Request";
        break;
    case ServerError:
        status = "500 Server Error";
        break;
    default:
        status = "999 WTF";
    }

    char* first_line = strdup("");
    strcat_realloc(&first_line, version);
    strcat_realloc(&first_line, status);

    struct Vector lines = vector_new(sizeof(char*));
    vector_push(&lines, &first_line);

    for (int i = 0; i < self->headers.vec->len; i++) {
        struct Vector* container = vector_index(self->headers.vec, i);

        for (int j = 0; j < container->len; j++) {
            struct HashItem* header_item = vector_index(container, j);

            char* header = strdup("");

            strcat_realloc(&header, header_item->key);
            strcat_realloc(&header, ": ");
            strcat_realloc(&header, header_item->value);

            vector_push(&lines, &header);
        }
    }

    char* response = strdup("");

    for (int i = 0; i < lines.len; i++) {
        char** line_ptr = vector_index(&lines, i);
        strcat_realloc(&response, *line_ptr);
        strcat_realloc(&response, "\r\n");
        free(*line_ptr);
    }

    strcat_realloc(&response, "\r\n");

    return response;
}
