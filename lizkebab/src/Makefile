LD_FLAGS=-lpthread

all : server client

client : client.o 
	$(CC) -o client client.o

server : server.o 
	$(CC) -o server server.o $(LD_FLAGS)

client.o : client.c
	$(CC) -c client.c

server.o : server.c
	$(CC) -c server.c

clean :
	rm client server client.o server.o
