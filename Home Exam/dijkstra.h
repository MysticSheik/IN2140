#ifndef __DIJKSTRA_H
#define __DIJKSTRA_H

#include "node.h"

#define INFINITY 9999
#define MAX 10

int** dijkstra(int** graph, int n, int start, struct node** nodes);

#endif
