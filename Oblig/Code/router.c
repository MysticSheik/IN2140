#include <stdio.h>
#include <stdlib.h>

// Get Router - Gets router pointer from global array based on ID
// unsigned char routerID: ID of router to get
// int* numRouters: Number of routers
struct Router* getRouter(unsigned char routerID, int *numRouters) {
    int i;
    for (i = 0; i < *numRouters; i++) {
        if (routers[i] != NULL && (routers[i]->id == routerID)) {
            return routers[i];
        }
    }
    return NULL;
}
