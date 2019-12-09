#include <stdio.h>
#include <stdlib.h>

// Print Router - prints data of a router to the terminal
// unsigned char routerID: 	ID of router to print
// int* numRouters: 		Number of routers
void printRouter(unsigned char routerID, int *numRouters) {
    if (getRouter(routerID, numRouters) == NULL) {
        printf("Router: %u does not exist!\n", routerID);
        return;
    }

    struct Router* router = getRouter(routerID, numRouters);
    printf("----------\nRouter: %u\n%s\n", router->id, router->model);
    printf("\nAktiv: %u\n", router->flag & (1 << 0));
    printf("Trådløs: %u\n", router->flag & (1 << 1));
    printf("5GHz: %u\n", router->flag & (1 << 2));
    printf("Ubrukt: %u\n", (router->flag & 0x8));
    printf("Endringsnummer: %u\n", (router->flag & (0x15 << 4)));
    printf("\nConnections:\n");
    int i;
    for(i = 0; i < router->numConnections; i++) {
         printf("Router %u - %s\n", router->connections[i]->id, router->connections[i]->model);
    } 
    printf("-----------\n");
}

// Set Model - changes model name of a given router
// unsigned char routerID: 	ID of router to change model name of
// unsigned char* modelString: 	New model name
// int* numRouters:		Number of routers
void setModel(unsigned char routerID, unsigned char* modelString, int *numRouters) {
    if (getRouter(routerID, numRouters) == NULL) {
        printf("Router: %u does not exist!\n", routerID);
        return;
    }

    struct Router* router = getRouter(routerID, numRouters);
    strcpy(router->model, modelString);
    router->mLength = strlen(modelString) + 1;
}

// Add Connection - adds a connection between two routers
// unsigned char routerID:	ID of router to make a connection from
// unsigned char connection:	ID of router to make a connection to
void addConnection(unsigned char routerID, unsigned char connection) {
    if (routers[routerID] == NULL) {
        printf("Router: %u does not exist!\n", routerID);
        return;
    }

    if (routers[connection] == NULL) {
        printf("Router: %u does not exist!\n", connection);
        return;
    }

    struct Router* r1 = routers[routerID];
    struct Router* r2 = routers[connection];
    r1->connections[r1->numConnections] = r2;
    r1->numConnections++;
}

// Set Flag - sets the value of a flag of a router
// unsigned char routerID:	ID of router to set flag of
// unsigned char flag:		Flag to change value of
// unsigned char value:		Value to assign to flag
// int* numRouters:		Number of routers
void setFlag(unsigned char routerID, unsigned char flag, unsigned char value, int* numRouters) {
     if (getRouter(routerID, numRouters) == NULL) {
         printf("Router: %u does not exist!\n", routerID);
         return;
     }

     if (flag == 3) {
          printf("Invalid flag!");
          return;
     }

     if (flag <= 2 && value > 1) {
          printf("Invalid flag value - flags 0, 1 and 2 can only be 0 or 1!");
          return;
     }

     if (flag == 4 && value > 15) {
          printf("Invalid flag value!");
          return;
     }

     struct Router* router = getRouter(routerID, numRouters);

     // Set Flag Value
     if (flag == 0) {
         if (value == 0) {
             router->flag &= value << flag;
         } else {
             router->flag |= value << flag;
         }
     } else if (flag == 1) {
         if (value == 0) {
             router->flag &= value << flag;
         } else {
             router->flag |= value << flag;
         }
     } else if (flag == 2) {
         if (value == 0) {
             router->flag &= value << flag;
         } else {
             router->flag |= value << flag;
         }
     } else if (flag == 4) {
         if (value == 0) {
             router->flag &= value << flag;
         } else {
             router->flag |= value << flag;
         }
     }

}

// Delete Router - deletes a router and all connections to it
// unsigned char routerID:	ID of router to delete
// int* numRouters:		Number of routers
void delRouter(unsigned char routerID, int *numRouters) {
    if (getRouter(routerID, numRouters) == NULL) {
        printf("Router: %u does not exist!\n", routerID);
        return;
    }

    struct Router* router = getRouter(routerID, numRouters);
 
    //Remove connection pointers
    int i;
    for(i = 0; i < *numRouters; i++) {
        int index;
        for(index = 0; index < routers[i]->numConnections; index++) {
            if (routers[i]->connections[index]->id == router->id) {
                int j;
                for(j = index + 1; j < routers[i]->numConnections; j++) {
                    routers[i]->connections[index] = routers[i]->connections[j];
                    index++;
                }
                routers[i]->connections[index] = NULL;
                routers[i]->numConnections--;
            }
        }
    }

    // Free Allocated Memory - 337 Bytes
    free(routers[routerID]);
    routers[routerID] = NULL;

    // Align Routers
    for(i = 0; i < *numRouters; i++) {
         if (routers[i] == NULL) {
             int index;
             for(index = i + 1; index < *numRouters; index++) {
                 routers[i] = routers[index];
                 i++;
             }
             routers[i] = NULL;
         }
    }
    (*numRouters)--;
}

// Depth First Search - performs a depth first search to find a target node
// unsigned char node:		ID of node to search from
// unsigned char target:	ID of node to find
// unsigned char* visited:	Array to keep track over visited nodes
int dfs(unsigned char node, unsigned char target, unsigned char* visited) {
    if (visited[node] == 1) {
         return 0;
    }    

    if (node == target) {
         return 1;
    }
    visited[node] = 1;

    int i;
    for (i = 0; i < routers[node]->numConnections; i++) {
        if (dfs(routers[node]->connections[i]->id, target, visited) == 1) {
             return 1;
        }
    }
    return 0;
}

// Path Exists - checks if a path exists between two routers: return 1 if true, 0 if false
// unsigned char fromRouter:	ID of router to search from
// unsigned char toRouter:	ID of router to find a path to
// int* numRouters:		Number of routers
int pathExists(unsigned char fromRouter, unsigned char toRouter, int *numRouters) {
    if (getRouter(fromRouter, numRouters) == NULL) {
        printf("Router: %u does not exist!\n", fromRouter);
        return 0;
    }

    if (getRouter(toRouter, numRouters) == NULL) {
        printf("Router: %u does not exist!\n", toRouter);
        return 0;
    }

    // Depth First Search
    unsigned char visited[*numRouters];
    int i;
    for(i = 0; i < *numRouters; i++) {
        visited[i] = 0;
    }
    return dfs(fromRouter, toRouter, visited);
}
