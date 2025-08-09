#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#define LOG(msg, ...) \
    fprintf(stderr, "[%s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif // LOGGER_H
