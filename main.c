#include "http.c"
#include "http_client.c"
#include "vector.c"

void get_root_handler(struct Request request, struct Response* response) {}

int main() {
    struct HttpClient client = http_client_new();

    struct Vector routes = vector_new(sizeof(struct Route));

    struct Route get_root = {
        .path = "/",
        .method = GET,
        .handler = &get_root_handler,
    };
    vector_push(&routes, &get_root);

    for (int i = 0; i < routes.len; i++) {
        http_client_add(&client, vector_index(&routes, i));
    }

    if (http_client_listen(&client, "127.0.0.1", 3000) != 0) {
        return err();
    }
}
