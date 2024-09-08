#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hash_map.c"
#include "vector.c"

int err() {
    printf("%s", strerror(errno));
    return 1;
}

int accept_connection(int conn, struct sockaddr_in sockaddr) {
    struct Vector request = vector_new(sizeof(char));

    char buf[256];

    int read_count;
    while ((read_count = read(conn, buf, sizeof(buf))) != 0) {
        if (read_count == -1) {
            return err();
        }

        for (int i = 0; i < read_count; i++) {
            char* item = buf + i * sizeof(char);
            vector_push(&request, item);
        }

        int len = request.len;
        char* items = vector_items(&request);
        // check for end \r\n\r\n
        if (len >= 4 && items[len - 1] == '\n' && items[len - 2] == '\r' &&
            items[len - 3] == '\n' && items[len - 4] == '\r') {
            break;
        }
    }

    char* request_as_str = strdup(request.items);

    struct HashMap req_headers = hash_map_new();

    bool first = true;
    for (char* token = strtok(request_as_str, "\r\n"); token != NULL;
         token = strtok(NULL, "\r\n")) {

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

        hash_map_set(&req_headers, key, value);

        free(token_og);
    }

    free(request_as_str);

    char* response = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
    if (write(conn, response, strlen(response)) == -1) {
        return err();
    }

    vector_free(&request);
    hash_map_free(&req_headers);

    close(conn);

    return 0;
}

int main() {
    char* addr = "127.0.0.1";
    int port = 3000;

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

        if (accept_connection(conn, client_addr) != 0) {
            return err();
        }
    }

    return 0;
}
