#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

// Commands
void printRouter(unsigned char routerID, int *numRouters);
void setModel(unsigned char routerID, unsigned char *modelString, int *numRouters);
void addConnection(unsigned char routerID, unsigned char connection);
void setFlag(unsigned char routerID, unsigned char flag, unsigned char value, int *numRouters);
void delRouter(unsigned char routerID, int *numRouters);
int pathExists(unsigned char fromRouter, unsigned char toRouter, int *numRouters);

// Helper Functions
int dfs(unsigned char node, unsigned char target, unsigned char* visited);
#endif
