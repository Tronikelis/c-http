#pragma once

#include <errno.h>
#include <stdio.h>
#include <string.h>

int err() {
    printf("%s\n", strerror(errno));
    return 1;
}
