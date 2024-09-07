#include "hash_map.c"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int err() {
    printf("%s", strerror(errno));
    return 1;
}

int main() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        return err();
    }

    char *key = strdup("key");
    char *value = strdup("value");

    struct HashMap hash_map = hash_map_new();
    hash_map_set(&hash_map, key, value);
    char *value2 = hash_map_get(&hash_map, key);

    printf("value: %s", value2);
    return 0;

    printf("created %i socket\n", socket_fd);

    struct sockaddr_in sockaddr = {
        AF_INET,
        htons(3000),
        {inet_addr("127.0.0.1")},
    };

    if (bind(socket_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
        return err();
    }

    printf("bound to %d\n", socket_fd);

    if (listen(socket_fd, 128) == -1) {
        return err();
    }

    printf("listening\n");

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        int conn;
        if ((conn = accept(socket_fd, (struct sockaddr *)&client_addr,
                           &client_addr_size)) == -1) {
            return err();
        }

        struct Vector request = vector_new(sizeof(char));

        int bufsize = 256;
        char *buf = malloc(bufsize);

        int read_count;
        while ((read_count = read(conn, buf, bufsize)) != 0) {
            if (read_count == -1) {
                return err();
            }

            for (int i = 0; i < read_count; i++) {
                vector_push(&request, buf + i * sizeof(char));
            }

            int len = request.len;
            char *items = vector_items(&request);
            // check for end \r\n\r\n
            if (len >= 4 && items[len - 1] == '\n' && items[len - 2] == '\r' &&
                items[len - 3] == '\n' && items[len - 4] == '\r') {
                break;
            }
        }
        free(buf);

        for (int i = 0; i < request.len; i++) {
            printf("%c", ((char *)vector_items(&request))[i]);
        }

        char *response = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
        if (write(conn, response, strlen(response)) == -1) {
            return err();
        }

        close(conn);
    }

    return 0;
}
