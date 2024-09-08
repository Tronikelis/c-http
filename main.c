#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "err.c"
#include "http.c"

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

        if (request_accept(conn, client_addr) != 0) {
            return err();
        }
    }

    return 0;
}
