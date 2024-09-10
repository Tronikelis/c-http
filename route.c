#pragma once

#include "http.c"

struct Route {
    enum Method method;
    char* path;
    void (*handler)(struct Request request);
};
