#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "util.h"

// Response Codes
#define RESPONSE_OK 1
#define RESPONSE_WRONG_FORMAT 2
#define RESPONSE_LOST_PACKET 3

// Header Sizes
#define RESPONSE_HEADER_SIZE sizeof(uint8_t)
#define PACKET_HEADER_SIZE (sizeof(uint16_t) * 3)
#define EDGE_HEADER_SIZE (sizeof(uint16_t) + sizeof(uint8_t))
#define ROUTING_TABLE_HEADER_SIZE sizeof(uint8_t)

// Response Code Header
struct response_header {
    uint8_t code;
};

// Packet Header
struct packet_header {
    uint16_t len;
    uint16_t dest;
    uint16_t src;
};

// Edge Header
struct edge_header {
    uint16_t src;
    uint8_t num_edges;
};

// Routing Table Header
struct routing_table_header {
    uint8_t num_pairs;
};

ssize_t recv_n(int socket_fd, void* buffer, size_t size, int flags);
uint8_t recv_response(int socket_fd);
void send_response(int socket_fd, uint8_t response);

#endif
