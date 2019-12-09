#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "util.h"
#include "protocol.h"
#include "routing_table.h"
#include "node.h"
#include "dijkstra.h"

// Create Socket
// --------------------------------------------------
int create_socket(int port) {
    int sockfd;
    struct sockaddr_in servaddr;
    
    // Create Socket
    LOG(LOGGER_DEBUG, "Creating Socket");
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP Socket
    if (sockfd == -1) {
        LOG(LOGGER_ERROR, "Something went to hell in socket()!");
        perror("socket");
        exit(EXIT_FAILURE);
    }
    LOG(LOGGER_DEBUG, "Created Socket");

    // Specify Socket Information
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // Reuse Port Immediately after Previous User
    int enable = 0; // OFF - Set to 1 for easier debugging.
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    // Bind Socket to Port
    LOG(LOGGER_DEBUG, "Binding Socket to Port: %d", port);
    if (bind(sockfd, (const struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        LOG(LOGGER_ERROR, "Something went to hell in bind()!");
        perror("bind");
        exit(EXIT_FAILURE);
    }
    LOG(LOGGER_DEBUG, "Successfully Bound Socket!");

    return sockfd;
}

// Node Exists
// --------------------------------------------------
int node_exists(int address, unsigned char nodeIndex, struct node** nodes) {
    int cntr;
    for (cntr = 0; cntr < nodeIndex; cntr++) {
        if (nodes[cntr]->address == address) {
            return nodes[cntr]->id;
        }
    }
    return -1;
}

// Add Node
// ---------------------------------------------------
int add_node(int address, unsigned char nodeIndex, struct node** nodes) {
    nodes[nodeIndex] = malloc(sizeof(struct node));
    nodes[nodeIndex]->address = address;
    nodes[nodeIndex]->id = nodeIndex;
    LOG(LOGGER_DEBUG, "Added Node: %d with ID: %u", nodes[nodeIndex]->address, nodes[nodeIndex]->id);
    return nodes[nodeIndex]->id;
}

// Test Edges
// ---------------------------------------------------
void test_edges(int** graph, int numNodes) {
    int i;
    int j;
    for (i = 0; i < numNodes; i++) {
        for (j = 0; j < numNodes; j++) {
            if (graph[i][j] > 0) {
                if (graph[i][j] == graph[j][i]) {
                    LOG(LOGGER_DEBUG, "Connection %d - %d: OK", i, j);
                } else {
                    LOG(LOGGER_ERROR, "Connection %d - %d: NOT OK - Ignoring", i, j);
                    graph[i][j] = 0;
                }
            }
        }
    }
}

// Recieve Edge Data
// ----------------------------------------------------
int recieve_edge(int socket_fd, int** graph, unsigned char* index, struct node** nodes) {
    char* buffer;
    ssize_t bytes;
    struct edge_header header;

    buffer = (char*) malloc(EDGE_HEADER_SIZE);
    bytes = recv_n(socket_fd, buffer, EDGE_HEADER_SIZE, 0);
    if (bytes == -1) {
        LOG(LOGGER_ERROR, "Error in recv_n!");
        free(buffer);
        return -1;
    }

    memcpy(&(header.src), &(buffer[0]), sizeof(uint16_t));
    memcpy(&(header.num_edges), &(buffer[2]), sizeof(uint8_t));
    send_response(socket_fd, RESPONSE_OK);

    // Add Node to Graph
    int exists = node_exists(header.src, *index, nodes);
    if (exists < 0) {
        int nid = add_node(header.src, *index, nodes);
        (*index)++;
        nodes[nid]->fd = socket_fd;
    } else {
        nodes[exists]->fd = socket_fd;
    }

    int i;
    for (i = 0; i < header.num_edges; i++) {
        buffer = (char*) realloc(buffer, sizeof(uint16_t) * 2);
        bytes = recv_n(socket_fd, buffer, sizeof(uint16_t) * 2, 0);
        if (bytes == -1) {
            free(buffer);
            return -1;
        }

        uint16_t adr;
        uint16_t cost;
        memcpy(&(adr), &(buffer[0]), sizeof(uint16_t));
        memcpy(&(cost), &(buffer[2]), sizeof(uint16_t));

        if (node_exists(adr, *index, nodes) < 0) {
            add_node(adr, *index, nodes);
            (*index)++;
        }

        int id_a = node_exists(header.src, *index, nodes);
        int id_b = node_exists(adr, *index, nodes);
        graph[id_a][id_b] = cost;

        send_response(socket_fd, RESPONSE_OK);
    }

    free(buffer);
    return 0;
}

// Send Routing Table
// ----------------------------------------------------
void send_routing_table(int socket_fd, struct routing_table* table) {

    // Build Header
    struct routing_table_header* header;
    header = (struct routing_table_header*) malloc(sizeof(struct routing_table_header));
    header->num_pairs = table->num_pairs;

    // Send Header
    char* buffer = (char*) malloc(ROUTING_TABLE_HEADER_SIZE);
    memcpy(&(buffer[0]), &(header->num_pairs), sizeof(uint8_t));
    send(socket_fd, buffer, ROUTING_TABLE_HEADER_SIZE, 0);
    
    if (recv_response(socket_fd) == RESPONSE_OK) {
        LOG(LOGGER_RESPONSE, "OK");
    } else {
        LOG(LOGGER_RESPONSE, "UNKNOWN");
    }

    free(header);

    // Send Routing Pairs
    int i;
    for (i = 0; i < table->num_pairs; i++) {
        buffer = (char*) realloc(buffer, sizeof(uint16_t) * 2);
        memcpy(&(buffer[0]), &(table->pairs[i]->dest), sizeof(uint16_t));
        memcpy(&(buffer[2]), &(table->pairs[i]->next_hop), sizeof(uint16_t));

        send(socket_fd, buffer, sizeof(uint16_t) * 2, 0);

        // Response
        if (recv_response(socket_fd) == RESPONSE_OK) {
            LOG(LOGGER_RESPONSE, "OK");
        }
    }

    free(buffer);
}

// Creates Routing Table for Node
// ----------------------------------------------------
struct routing_table* create_routing_table(int n_id, int num_nodes, int** paths, struct node** nodes) {
    struct routing_table* table = (struct routing_table*) malloc(sizeof(struct routing_table));
    table->num_pairs = 0;
    table->pairs = malloc(sizeof(struct routing_path*) * num_nodes);

    int i, j;
    for (i = 0; i < num_nodes; i++) {
        uint16_t dest = nodes[i]->address;
        int node_id = nodes[i]->id; // May be removable

        for (j = 0; j < num_nodes; j++) {
            if (paths[node_id][j] == -1 || j + 1 >= num_nodes) {
                break;
            }

            int next = paths[node_id][j + 1];
            int cur = paths[node_id][j];

            if (next != -1 && nodes[n_id]->id == next) {
                uint16_t next_hop = nodes[cur]->address;
                table->pairs[table->num_pairs]  = (struct routing_path*) malloc(sizeof(struct routing_path));
                table->pairs[table->num_pairs]->dest = dest;
                table->pairs[table->num_pairs]->next_hop = next_hop;
                table->num_pairs++;
                LOG(LOGGER_DEBUG, "Added pair: %d:%d", dest, next_hop);
            }
        }
    }

    return table;
}

// Main
// ----------------------------------------------------
int main(int argc, char* argv[]) {
    // Check Arguments
    if (argc < 3) {
        printf("Usage: [P: TCP Port] [N: Number of Nodes]\n");
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int numNodes = atoi(argv[2]);

    int** adjMatrix = malloc(numNodes * sizeof(int*));
    int i, j;

    for (i = 0; i < numNodes; i++) {
        adjMatrix[i] = malloc(numNodes * sizeof(int));
    }

    for (i = 0; i < numNodes; i++) {
        for (j = 0; j < numNodes; j++) {
            adjMatrix[i][j] = 0;
        }
    }

    // Create TCP Socket
    int server_socket_fd = create_socket(port);
    LOG(LOGGER_DEBUG, "Trying to listen to Socket...");
    if (listen(server_socket_fd, 10) == -1) {
        LOG(LOGGER_ERROR, "Something went to hell in listen()!");
        perror("listen");
        exit(EXIT_FAILURE);
    }
    LOG(LOGGER_DEBUG, "Now listening on socket");

    struct node** nodes = malloc(sizeof(struct node*) * numNodes);
    unsigned char nodeIndex = 0;

    // Recive Address & Edge Data from Nodes
    for (i = 0; i < numNodes; i++) {
        int client_fd = accept(server_socket_fd, NULL, NULL);
        if (client_fd < 0) {
            LOG(LOGGER_ERROR, "Shit!");
        } else {
            // Recieve Edges
            LOG(LOGGER_DEBUG, "Accepted Connection! Recieving edge data...");
            recieve_edge(client_fd, adjMatrix, &nodeIndex, nodes);
        }
    }
    
    // Test every edge between two nodes
    test_edges(adjMatrix, numNodes);

    // Compute shortest path from Node 1 to every other node -> Dijkstra
    int start_node = node_exists(1, nodeIndex, nodes);
    int** paths = dijkstra(adjMatrix, numNodes, start_node, nodes);

    // Create Routing Tables
    LOG(LOGGER_DEBUG, "Sending Routing Tables...");
    for (i = 0; i < numNodes; i++) {
        LOG(LOGGER_DEBUG, "Creating Routing Table for: %d", nodes[i]->address);
        struct routing_table* table = create_routing_table(nodes[i]->id, numNodes, paths, nodes);
        send_routing_table(nodes[i]->fd, table);

        for (j = 0; j < table->num_pairs; j++) {
            free(table->pairs[j]);
        }
        free(table->pairs);
        free(table);
    }

    // Quit Server - Close Sockets, Free Memory
    for (i = 0; i < numNodes; i++) {
        free(paths[i]);
    }
    free(paths);

    close(server_socket_fd);

    for (i = 0; i < numNodes; i++) {
        free(adjMatrix[i]);
        free(nodes[i]);
    }
    free(adjMatrix);
    free(nodes);

    return 0;
}
