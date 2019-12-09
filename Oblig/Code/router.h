#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

// 337 Bytes
struct Router {
    unsigned char id;
    unsigned char flag;
    unsigned char mLength;
    unsigned char numConnections;
    struct Router* connections[10];
    unsigned char model[253];
};

struct Router** routers;

struct Router* getRouter(unsigned char routerID, int *numRouters);
#endif
