#ifndef __NODE_H
#define __NODE_H

#include <stdint.h>

struct node {
    int fd;
    uint16_t address;
    unsigned char id;
};

#endif
