#include <stdio.h>
#include <stdlib.h>

#include "dijkstra.h"
#include "print_lib/print_lib.h"

#define INFINITY 9999
#define MAX 10

int** dijkstra(int** graph, int n, int start, struct node** nodes) {
    int num_nodes = n;
    int cost[num_nodes][num_nodes];
    int distance[num_nodes];
    int pred[num_nodes];
    int visited[num_nodes];
    int count;
    int minDistance;
    int next;
    int i;
    int j;

    // Initialize Cost Table
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (graph[i][j] == 0) {
                cost[i][j] = INFINITY;
            } else {
                cost[i][j] = graph[i][j];
            }
        }
    }

    // Initialize Data
    for (i = 0; i < n; i++) {
        distance[i] = cost[start][i];
        pred[i] = start;
        visited[i] = 0;
    }

    distance[start] = 0;
    visited[start] = 1;
    count = 1;

    while (count < n - 1) {
        minDistance = INFINITY;

        for (i = 0; i < n; i++)
            if (distance[i] < minDistance && !visited[i]) {
                minDistance = distance[i];
                next = i;
            }

            visited[next] = 1;
            for (i = 0; i < n; i++) {
                if (!visited[i]) {
                    if (minDistance + cost[next][i] < distance[i]) {
                        distance[i] = minDistance + cost[next][i];
                        pred[i] = next;
                    }
                }
            }
        count++;
    }

    int** paths = malloc(n * sizeof(int*));

    // Create Paths
    for (i = 0; i < n; i++) {

        paths[i] = malloc(n * sizeof(int));
        int x;
        for (x = 0; x < n; x++) {
            paths[i][x] = -1;
        }

        if (i != start) {
            int index = 0;

            j = i;
            paths[i][index] = j;
            index++;
            do {
                j = pred[j];
                paths[i][index] = j;
                index++;
            } while (j != start);
        } else {
            paths[i][0] = start;
        }
    }

    // Print Weighted Edges
    int from;
    int to;
    int pathlen;
    for (from = 0; from < n; from++) {
        for (to = 0; to < n; to++) {
            pathlen = -1;

            int counter = to;
            if (counter == from) {
                pathlen = distance[from];
            }

            do {
                counter = pred[counter];
                if (counter == from) {
                    pathlen = distance[from];
                    break;
                }
            } while (counter != start);

            print_weighted_edge(nodes[from]->address, nodes[to]->address, pathlen);
        }
    }

    return paths;
}
