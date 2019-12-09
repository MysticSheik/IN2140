#include "protocol.h"

// Recieves Bytes in a loop to ensure that everything is recieved
// (Based on https://github.uio.no/IN2140/PlenarySessions/blob/master/week12/protocol.c)
ssize_t recv_n(int socket_fd, void* buffer, size_t size, int flags) {
    char* char_buffer = buffer;
    ssize_t bytes = 0;
    ssize_t rec_bytes;

    while (bytes < size) {
        rec_bytes = recv(socket_fd, char_buffer + bytes, size - bytes, flags);

        if (rec_bytes < 0) {
            perror("recv");
            return -1;
        }

        if (rec_bytes == 0) {
            break;
        }

        bytes += rec_bytes;
    }

    return bytes;
}

// Recieve Response Code
// --------------------------------------------------
uint8_t recv_response(int socket_fd) {
    int flags = 0;
    char* buffer = (char*) malloc(RESPONSE_HEADER_SIZE);
    size_t bytes = recv_n(socket_fd, buffer, RESPONSE_HEADER_SIZE, flags);
    // Error Checking
    
    uint8_t response = (uint8_t) (buffer[0]);
    free(buffer);
    return response;
}

// Send Response Code
// --------------------------------------------------
void send_response(int socket_fd, uint8_t response) {
    int flags = 0;
    struct response_header* header;
    header = (struct response_header*) malloc(sizeof(struct response_header));
    header->code = response;

    char* buffer = (char*) malloc(RESPONSE_HEADER_SIZE);
    memcpy(&(buffer[0]), &(header->code), sizeof(uint8_t));
    send(socket_fd, buffer, RESPONSE_HEADER_SIZE, flags);
    // Error Checking

    free(buffer);
    free(header);
}
