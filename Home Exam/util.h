// Kilde: https://github.uio.no/IN2140/PlenarySessions/blob/master/week12/util.h

#ifndef __UTIL_H
#define __UTIL_H

#include <string.h>
#include <stdio.h>
#include <time.h>

#define LOGGER_BUF_SIZE 1024

#define CNRM "\x1b[0m"
#define CMAG "\x1b[35m"
#define CYEL "\x1b[33m"
#define CRED "\x1b[0;31m"
#define MAG "\x1b[1;35m"

#define LOGGER_LEVEL_DEBUG 2
#define LOGGER_LEVEL_INFO 1
#define LOGGER_LEVEL_OFF 0

#define LOGGER_RESPONSE "RESPONSE"
#define LOGGER_ERROR "ERROR"
#define LOGGER_DEBUG "DEBUG"
#define LOGGER_INFO "INFO"

#define LOG(MODE, FORMAT, ...) \
    do { \
        char buf[LOGGER_BUF_SIZE]; \
        sprintf(buf, "%s:%d in %s() - ", __FILE__, __LINE__, __func__); \
        snprintf(buf + strlen(buf), LOGGER_BUF_SIZE - strlen(buf), FORMAT, ## __VA_ARGS__); \
        logger(MODE, buf); \
    } while (0);

void logger(char* type, char* s);

#endif
