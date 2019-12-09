CCFLAGS= -g 

all: printlib routing_server node

routing_server:
	gcc server.c protocol.h protocol.c dijkstra.h dijkstra.c node.h util.h util.c print_lib/print_lib.h print_lib.o $(CCFLAGS) -o routing_server

node:
	gcc client.c protocol.h protocol.c util.h util.c print_lib/print_lib.h print_lib.o $(CCFLAGS) -o node

printlib:
	gcc -c print_lib/print_lib.c $(CCFLAGS)

run: all
	bash run_1.sh $(PORT)
	bash run_2.sh $(PORT)

clean:
	rm -f routing_server
	rm -f node
	rm -f print_lib.o
