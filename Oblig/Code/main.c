#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "router.h"
#include "router.c"
#include "commands.h"
#include "commands.c"

// Function Definitions
int readRouterFile(char *filename);
int readCommandFile(char *filename, int *numRouters);
int writeFile(char *filename, int *numRouters);
int freeMemory(int *numRouters);

// Main - main method of the program
// int argc:	Argument Count
// char* argv:	Argument Vector
int main(int argc, char *argv[]) {
    // Check number of arguments
    if (argc < 3) {
        printf("Usage: [ruterFil] [kommandoFil]\n");
        return -1;
    }

    int n = readRouterFile(argv[1]);
    readCommandFile(argv[2], &n);
    writeFile(argv[1], &n);
    freeMemory(&n);
    return 0;
}

// Read Command File - reads a command file and executes commands
// char* filename:	Name of commands file to read
// int* numRouters:	Number of routers
int readCommandFile(char *filename, int *numRouters) {
    // Open File
    FILE *file;
    file = fopen(filename, "r");

    if (!file) {
        perror("Noe gikk til helvete i fopen()");
        return -1;
    }

    // Read Lines, parse and execute commands
    char buffer[512];
    char *argument;
    while (fgets(buffer, 512, file) != NULL) {
         argument = strtok(buffer, " ");

         // Print
         if (strcmp(argument, "print") == 0) {
             argument = strtok(NULL, "\n");
             printRouter((unsigned char) atoi(argument), numRouters);

         // Sett Modell
         } else if (strcmp(argument, "sett_modell") == 0) {
             argument = strtok(NULL, " ");
             unsigned char rID = (unsigned char) atoi(argument);
             argument = strtok(NULL, "\n");
             setModel(rID, argument, numRouters);

         // Sett Flag
         } else if (strcmp(argument, "sett_flag") == 0) {
             argument = strtok(NULL, " ");
             unsigned char rID = (unsigned char) atoi(argument);
             argument = strtok(NULL, " ");
             unsigned char flag = (unsigned char) atoi(argument);
             argument = strtok(NULL, "\n");
             unsigned char value = (unsigned char) atoi(argument);
             setFlag(rID, flag, value, numRouters);

         // Finnes Rute
         } else if (strcmp(argument, "finnes_rute") == 0) {
             argument = strtok(NULL, " ");
             unsigned char fromRouter = (unsigned char) atoi(argument);
             argument = strtok(NULL, "\n");
             unsigned char toRouter = (unsigned char) atoi(argument);
             if (pathExists(fromRouter, toRouter, numRouters) == 1) {
                 printf("Path Exists between %u and %u\n", fromRouter, toRouter);
             } else {
                 printf("Path does not exists between %u and %u\n", fromRouter, toRouter);
             }

         // Legg til Kobling
         } else if (strcmp(argument, "legg_til_kobling") == 0) {
             argument = strtok(NULL, " ");
             unsigned char rID = (unsigned char) atoi(argument);
             argument = strtok(NULL, "\n");
             unsigned char connection = (unsigned char) atoi(argument);
             addConnection(rID, connection);

         // Slett Router
         } else if (strcmp(argument, "slett_router") == 0) {
             argument = strtok(NULL, "\n");
             unsigned char rID = (unsigned char) atoi(argument);
             delRouter(rID, numRouters);
         }
    }

    fclose(file);
    return 0;
}

// Read Router File - reads a router file
// char* filename:	Name of router file to read
int readRouterFile(char *filename) {
    // Open File
    FILE *file;
    file = fopen(filename, "rb");

    if (!file) {
        perror("Noe gikk til helvete i fopen()");
        return -1;
    }
    
    // Read N
    unsigned char bytes[4];
    int n = 0;
    fread(bytes, 1, 4, file);
    n += bytes[0] | (bytes[1]<<8) | (bytes[2]<<16) | (bytes[3]<<24); // Little-endian
    fseek(file, 1, SEEK_CUR); // Skip newline

    // Allocate N * 8 Bytes
    routers = malloc(sizeof(struct Router*) * n);

    // Read Router Data
    int i;
    for(i = 0; i < n; i++) {
        unsigned char buffer[3];
        unsigned char m[253];
        fread(buffer, 1, 3, file);
        fread(m, 1, buffer[2], file);
        m[buffer[2] - 1] = '\0';

        // Allocate 337 Bytes
        routers[i] = malloc(sizeof(struct Router));
        routers[i]->id = buffer[0];
        routers[i]->flag = buffer[1];
        routers[i]->mLength = buffer[2];
        routers[i]->numConnections = 0;
        strcpy(routers[i]->model, m);
    }

    // Read Connections
    unsigned char buffer[3]; // ID - ID - \n
    while (fread(buffer, 1, 3, file) == 3) {
        struct Router* r1 = getRouter(buffer[0], &n);
        struct Router* r2 = getRouter(buffer[1], &n);
        r1->connections[r1->numConnections] = r2;
        r1->numConnections++;
    }

    // Close File
    fclose(file);
    return n;
}

// Write File - writes router data to file
// char* filename:	Name of file to write to
// int* numRouters:	Number of routers
int writeFile(char *filename, int *numRouters) {
    // Open File
    FILE *file;
    file = fopen(filename, "wb");

    if (!file) {
        perror("Noe gikk til helvete i fopen()");
        return -1;
    }

    fwrite((void*) numRouters, 4, 1, file);
    fwrite("\n", sizeof(char), 1, file);

    int i;
    for (i = 0; i < *numRouters; i++) {
        if (routers[i] != NULL) {
            struct Router* r = routers[i];
            fwrite(&r->id, sizeof(unsigned char), 1, file);
            fwrite(&r->flag, sizeof(unsigned char), 1, file);
            fwrite(&r->mLength, sizeof(unsigned char), 1, file);
            fwrite(&r->model, routers[i]->mLength - sizeof(char), 1, file);
            fwrite("\n", sizeof(char), 1, file);
        }
    }

    // Write Connections
    for (i = 0; i < *numRouters; i++) {
        if (routers[i] != NULL) {
            int j;
            struct Router* r = routers[i];
            for (j = 0; j < r->numConnections; j++) {
                fwrite(&r->id, sizeof(unsigned char), 1, file);
                fwrite(&r->connections[j]->id, sizeof(unsigned char), 1, file);
                fwrite("\n", sizeof(char), 1, file);
            }
        }
    }

    fclose(file);
    return 0;
}

// Free Memory - free allocated memory to avoid leaks
// int* numRouters:	Number of routers
int freeMemory(int *numRouters) {
    int i;
    for(i = 0; i < *numRouters; i++) {
        struct Router* r = routers[i];
        if (r != NULL) {
            free(r); // Free 337 Bytes
            r = NULL;
        }
    }
    free(routers); // Free N * 8 Bytes
    return 0;
}
