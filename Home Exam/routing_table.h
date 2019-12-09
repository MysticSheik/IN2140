#ifndef __ROUTINGTABLE_H
#define __ROUTINGTABLE_H

#include <stdint.h>

struct routing_path {
    uint16_t dest;
    uint16_t next_hop;
};

struct routing_table {
    uint16_t num_pairs;
    struct routing_path** pairs;
};

#endif
