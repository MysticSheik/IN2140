#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "util.h"
#include "protocol.h"
#include "routing_table.h"
#include "print_lib/print_lib.h"

#define MAX_MESSAGE_LEN 1000

struct packet_data {
    uint16_t len;
    uint16_t dest;
    uint16_t src;
    char* message;
};

// Create Client TCP Socket
// ----------------------------------------------------
int create_client_tcp_socket(int port, char* address) {
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        LOG(LOGGER_ERROR, "Something went to hell in socket()!");
        perror("socket");
        return 0;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(address);

    LOG(LOGGER_DEBUG, "Connecting to server at %s, port %d", address, port);
    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in)) == -1) {
        LOG(LOGGER_ERROR, "Something went to hell in connect()!");
        perror("connect");
        return -2;
    }

    return sockfd;
}

// Create Client UDP Socket
// ------------------------------------------------------
int create_client_udp_socket(int port) {
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        LOG(LOGGER_ERROR, "Something went to hell in socket()");
        perror("socket");
        return 0;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind Socket
    LOG(LOGGER_DEBUG, "Binding UDP Socket to port %d", port);
    if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in)) < 0) {
        LOG(LOGGER_ERROR, "Error binding socket.");
        perror("bind");
        return -1;
    }

    return sockfd;
}

// Recieve Routing Table
// -----------------------------------------------------
struct routing_table* recv_routing_table(int socket_fd) {
    char* buffer;
    ssize_t bytes;
    struct routing_table_header header;

    buffer = (char*) malloc(ROUTING_TABLE_HEADER_SIZE);
    bytes = recv_n(socket_fd, buffer, ROUTING_TABLE_HEADER_SIZE, 0);
    // Error Checking

    memcpy(&(header.num_pairs), &(buffer[0]), sizeof(uint8_t));
    LOG(LOGGER_DEBUG, "Recieved num_pairs: %d", header.num_pairs);
    send_response(socket_fd, RESPONSE_OK);

    struct routing_table* table = (struct routing_table*) malloc(sizeof(struct routing_table));
    table->num_pairs = header.num_pairs;
    table->pairs = malloc(sizeof(struct routing_path*) * table->num_pairs);

    int i;
    for (i = 0; i < table->num_pairs; i++) {
        buffer = (char*) realloc(buffer, sizeof(uint16_t) * 2);
        bytes = recv_n(socket_fd, buffer, sizeof(uint16_t) * 2, 0);
        
        table->pairs[i] = (struct routing_path*) malloc(sizeof(struct routing_path));
        memcpy(&(table->pairs[i]->dest), &(buffer[0]), sizeof(uint16_t));
        memcpy(&(table->pairs[i]->next_hop), &(buffer[2]), sizeof(uint16_t));
        LOG(LOGGER_DEBUG, "Recieved pair: %d:%d", table->pairs[i]->dest, table->pairs[i]->next_hop);
        send_response(socket_fd, RESPONSE_OK);
    }

    free(buffer);
    return table;
}

// Create a Packet
// ------------------------------------------------------------------------------------
char* create_packet(int destination, int source, char* message) {
    struct packet_header* header;
    header = (struct packet_header*) malloc(sizeof(struct packet_header));

    int message_len = strlen(message) + 1;
    header->len = (uint16_t) htons(PACKET_HEADER_SIZE + message_len);
    header->dest = (uint16_t) htons(destination);
    header->src = (uint16_t) htons(source);

    char* packet = (char*) malloc(PACKET_HEADER_SIZE + message_len);
    memcpy(&(packet[0]), &(header->len), sizeof(uint16_t));
    memcpy(&(packet[2]), &(header->dest), sizeof(uint16_t));
    memcpy(&(packet[4]), &(header->src), sizeof(uint16_t));
    memcpy(&(packet[6]), message, message_len);

    LOG(LOGGER_DEBUG, "Created Packet! Len: %d Dest: %d Source: %d Message: %s", ntohs(header->len), ntohs(header->dest), ntohs(header->src), packet + 6);

    free(header);
    return packet;
}

// Send a Packet to Destination
// ------------------------------------------------------------------------------------
void send_packet(int socket_fd, uint16_t base_port, uint16_t destination, char* packet) {
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(base_port + destination);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    uint16_t packet_len;
    memcpy(&(packet_len), &(packet[0]), sizeof(uint16_t));
    packet_len = ntohs(packet_len);

    LOG(LOGGER_DEBUG, "Sending of length %d Packet to: %d", packet_len, destination);
    sendto(socket_fd, packet, packet_len, 0, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in));
}

// Inspect Packet
// -----------------------------------------------------------------------------------
struct packet_data* inspect_packet(char* packet, ssize_t packet_size) {
    
    // Decode Header
    struct packet_data* data = (struct packet_data*) malloc(sizeof(struct packet_data));
    memcpy(&(data->len), &(packet[0]), sizeof(uint16_t));
    memcpy(&(data->dest), &(packet[2]), sizeof(uint16_t));
    memcpy(&(data->src), &(packet[4]), sizeof(uint16_t));
    data->len = ntohs(data->len);
    data->dest = ntohs(data->dest);
    data->src = ntohs(data->src);

    // Validate Size
    if (data->len != packet_size) {
        LOG(LOGGER_ERROR, "Mismatch between received bytes and expected packet size");
        free(data);
        return NULL;
    }

    // Decode Message
    uint16_t message_len = data->len - PACKET_HEADER_SIZE;
    data->message = (char*) malloc(message_len);
    memcpy(data->message, &(packet[6]), message_len);

    LOG(LOGGER_DEBUG, "Decoded Packet: Len %d Dest %d Src %d Msg %s", data->len, data->dest, data->src, data->message);

    return data;
}

// Send Edges
// ------------------------------------------------------------------
int send_edges(int socket_fd, int src, int num_edges, char* edges[]) {
    uint8_t response;
    struct edge_header* header;
    header = (struct edge_header*) malloc(sizeof(struct edge_header));

    header->src = (uint16_t) src;
    header->num_edges = (uint8_t) num_edges;

    char* buffer = (char*) malloc(EDGE_HEADER_SIZE);
    memcpy(&(buffer[0]), &(header->src), sizeof(uint16_t));
    memcpy(&(buffer[2]), &(header->num_edges), sizeof(uint8_t));

    send(socket_fd, buffer, EDGE_HEADER_SIZE, 0);
    free(header);

    // Recieve Response
    if (recv_response(socket_fd) == RESPONSE_OK) {
        LOG(LOGGER_RESPONSE, "OK");
    }

    // Send All Edges
    int i;
    for (i = 0; i < num_edges; i++) {
        char* data = edges[i];
        char* token = strtok(data, ":");
        int adr = atoi(token);
        token = strtok(NULL, ":");
        int cost = atoi(token);

        LOG(LOGGER_DEBUG, "Sending Edge %d : %d", adr, cost);
        buffer = realloc(buffer, sizeof(uint16_t) * 2);
        memcpy(&(buffer[0]), &(adr), sizeof(uint16_t));
        memcpy(&(buffer[2]), &(cost), sizeof(uint16_t));

        send(socket_fd, buffer, sizeof(uint16_t) * 2, 0);

        response = recv_response(socket_fd);
        if (response == RESPONSE_OK) {
            LOG(LOGGER_RESPONSE, "OK");
        } else if (response == RESPONSE_WRONG_FORMAT) {
            LOG(LOGGER_RESPONSE, "WRONG_FORMAT");
        }
    }

    free(buffer);
    return 0;
}

// Routing Table Lookup
// -------------------------------------------------------------------
uint16_t routing_table_lookup(uint16_t destination, struct routing_table* table) {
    int i;
    for (i = 0; i < table->num_pairs; i++) {
        if (table->pairs[i]->dest == destination) {
            return table->pairs[i]->next_hop;
        }
    }
    return 0;
}

// Run Sender Node
// -------------------------------------------------------------------
int run_sender_node(int socket_fd, int port, int address, struct routing_table* table) {
    FILE *file;
    file = fopen("data.txt", "r");
    if (!file) {
        LOG(LOGGER_ERROR, "Failed to open data.txt!");
        perror("fopen");
        return -1;
    }

    char buffer[MAX_MESSAGE_LEN];
    char* token;
    while (fgets(buffer, MAX_MESSAGE_LEN, file) != NULL) {
        token = strtok(buffer, " ");
        int dest = atoi(token);
        token = strtok(NULL, "\n");

        // Create Packet
        char* packet = create_packet(dest, address, token);
        print_pkt(packet);

        // Handle Packet
        if (dest == address) {
            LOG(LOGGER_DEBUG, "Packet is already at destination - no forwarding required.");
            print_received_pkt(address, packet);
        } else {
            print_forwarded_pkt(address, packet);
            uint16_t next = routing_table_lookup(dest, table);
            send_packet(socket_fd, port, next, packet);
            // TODO: Response
        }

        free(packet);
    }

    fclose(file);
}

// Run Receiver Node
// -------------------------------------------------------
void run_receiver_node(int socket_fd, int port, int address, struct routing_table* table) {
    int running = 1;
    struct sockaddr_in addr;
    int addr_len = sizeof(struct sockaddr_in);

    while (running) {
        // Receive Packet
        char buffer[PACKET_HEADER_SIZE + MAX_MESSAGE_LEN];
        ssize_t recv_bytes = recvfrom(socket_fd, buffer, PACKET_HEADER_SIZE + MAX_MESSAGE_LEN, 0, (struct sockaddr*) &addr, &addr_len);

        struct packet_data* data = inspect_packet(buffer, recv_bytes);
        if (data == NULL) {
            // TODO: Send Response
        }

        // Handle Packet
        if (data->dest == address) {
            LOG(LOGGER_DEBUG, "Packet reached destination!");
            print_received_pkt(address, buffer);

            if (strcmp(data->message, "QUIT") == 0) {
                running = 0;
            }
        } else {
            print_forwarded_pkt(address, buffer);
            uint16_t next = routing_table_lookup(data->dest, table);
            send_packet(socket_fd, port, next, buffer);
        }

        free(data->message);
        free(data);
    }
}

// Main
// -------------------------------------------------------
int main(int argc, char* argv[]) {
    
    // Check Arguments
    if (argc < 3) {
        printf("Usage: [Port] [Address] [Edges] \n");
        return -1;
    }

    int port = atoi(argv[1]);
    int address = atoi(argv[2]);

    if (address < 1 || address > 1023) {
        perror("Invalid input! Address must be between 1 - 1023");
        exit(-1);
        return -1;
    }

    // Check if P+A is a valid port number
    int bindPort = port + address;
    if (bindPort < 1024 || bindPort > 62555) {
        perror("Invalid Bind Port!");
        exit(-1);
        return -1;
    }
    
    // Create UDP Socket
    int udp_socket = create_client_udp_socket(bindPort);
    if (udp_socket == -1) {
        perror("Failed to bind UDP socket!");
        exit(-1);
        return -1;
    }

    // Open TCP Connection to Server
    LOG(LOGGER_DEBUG, "Setting up connection to server...");
    int tcp_socket = create_client_tcp_socket(port, "127.0.0.1");
    if (tcp_socket == 0) { exit(EXIT_FAILURE); }
    if (tcp_socket == -2) {
        perror("Failed to create TCP socket!");
        exit(-2);
        return -2;
    }

    // Send Edge Data
    int num_edges = argc - 3;
    LOG(LOGGER_DEBUG, "Sending edge data to server...");
    send_edges(tcp_socket, address, num_edges, &argv[3]);
    LOG(LOGGER_DEBUG, "Finished sending edge data to server!");
    
    // Wait for Routing Table from Server
    LOG(LOGGER_DEBUG, "Waiting for Routing Table from Server...");
    struct routing_table* table = recv_routing_table(tcp_socket);
    close(tcp_socket);
    LOG(LOGGER_DEBUG, "Finished recieving routing table!");

    // Run Node
    if (address == 1) {
        sleep(1);
        run_sender_node(udp_socket, port, address, table);
    } else {
        run_receiver_node(udp_socket, port, address, table);
    }

    // Free memory and Close UDP Socket
    int i;
    for (i = 0; i < table->num_pairs; i++) {
        free(table->pairs[i]);
    }
    free(table->pairs);
    free(table);

    close(udp_socket);
    return 0;
}
