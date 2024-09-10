#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "err.c"
#include "http.c"
#include "route.c"
#include "vector.c"

struct HttpClient {
    // struct Route[]
    struct Vector* routes;
};

struct HttpClient http_client_new() {
    struct HttpClient client = {vector_new_heap(sizeof(struct Route))};
    return client;
}

void http_client_add(struct HttpClient* self, struct Route* route) {
    vector_push(self->routes, route);
}

int _http_client_accept(struct HttpClient* self, int fd,
                        struct sockaddr_in client_addr) {
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

    // todo find correct route for request
    struct Route* route = vector_index(self->routes, 0);
    struct Response response = response_new();

    route->handler(request, &response);
    // todo: send response back to client here
    response_free(&response);

    request_free(&request);

    return 0;
}

int http_client_listen(struct HttpClient* self, char* addr, int port) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        return err();
    }

    printf("created %i socket\n", socket_fd);

    struct sockaddr_in sockaddr = {
        AF_INET,
        htons(port),
        {inet_addr(addr)},
    };

    if (bind(socket_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1) {
        return err();
    }

    printf("bound to %d\n", socket_fd);

    if (listen(socket_fd, 128) == -1) {
        return err();
    }

    printf("listening on %s:%d\n", addr, port);

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        int conn;
        if ((conn = accept(socket_fd, (struct sockaddr*)&client_addr,
                           &client_addr_size)) == -1) {
            return err();
        }

        if (_http_client_accept(self, conn, client_addr) != 0) {
            return err();
        }
    }

    return 0;
}
