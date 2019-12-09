#include "util.h"

// Kilde: https://github.uio.no/IN2140/PlenarySessions/blob/master/week12/util.c
int logger_level = LOGGER_LEVEL_DEBUG;

void logger(char* type, char* s) {
    if ((strcmp(type, LOGGER_DEBUG) == 0 && logger_level > 1) ||
        (strcmp(type, LOGGER_INFO) == 0 && logger_level > 0) ||
        (strcmp(type, LOGGER_ERROR) == 0) ||
        (strcmp(type, LOGGER_RESPONSE) == 0)) {
        
        char* color;
        if (strcmp(type, LOGGER_DEBUG) == 0) {
            color = CMAG;
        } else if (strcmp(type, LOGGER_INFO) == 0) {
            color = CYEL;
        } else  if (strcmp(type, LOGGER_ERROR) == 0) {
            color = CRED;
        } else {
            color = MAG;
        }

        time_t rawtime;
        struct tm* timeinfo;
        struct timespec spec;

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char* datetime = asctime(timeinfo);
        datetime[strlen(datetime) - 6] = 0;
        datetime = &datetime[strlen(datetime) - 8];

        clock_gettime(CLOCK_REALTIME, &spec);


        printf("[%s.%09ld][%s%-5s%s] %s\n", datetime, spec.tv_nsec, color, type, CNRM, s);
    }
}
