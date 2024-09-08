#ifndef HTTP
#define HTTP

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
    bool is_closed;
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
        .is_closed = false,
        .fd = fd,
        .headers = hash_map_new(),
    };

    return request;
}

int request_parse_raw(struct Request* self) {
    if (self->is_closed) {
        printf("trying to parse closed socket\n");
        abort();
    }

    struct Vector raw = vector_new(sizeof(char));

    char buf[256];
    int read_count;
    while ((read_count = read(self->fd, buf, sizeof(buf))) != 0) {
        if (read_count == -1) {
            return err();
        }

        for (int i = 0; i < read_count; i++) {
            vector_push(&raw, buf + i * sizeof(char));
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

int request_send_response(struct Request* self) {
    self->is_closed = true;
    // todo: free here all shits

    char* response = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
    if (write(self->fd, response, strlen(response)) == -1) {
        close(self->fd);
        return err();
    }

    close(self->fd);

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

int request_accept(int fd, struct sockaddr_in client_addr) {
    struct Request request = request_new(fd);

    if (request_parse_raw(&request) != 0) {
        return err();
    }
    if (request_parse_method(&request) != 0) {
        return err();
    }
    if (request_parse_path(&request) != 0) {
        return err();
    }
    if (request_parse_headers(&request) != 0) {
        return err();
    }

    if (request_send_response(&request) != 0) {
        return err();
    }

    request_free(&request);

    return 0;
}

#endif
