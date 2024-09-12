#pragma once

#include <stdbool.h>
#include <unistd.h>

int read_until(int fd, void* buf, int buf_len, void* until, int until_size) {
    int count = read(fd, buf, buf_len);
    if (count == 0 || count == -1)
        return count;

    for (int i = until_size - 1; i < count; i += until_size) {
        bool found = false;

        for (int j = 0; j < until_size; j++) {
            char l = ((char*)buf)[i - j];
            char r = ((char*)until)[until_size - 1 - j];
            found = true;

            if (l != r) {
                found = false;
                break;
            }
        }

        if (found) {
            return 0;
        }
    }

    return count;
}
