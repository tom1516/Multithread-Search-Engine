CC = gcc
CFLAGS = -g -O2 -Wall -Wundef -lpthread
OBJECTS = 

all: client server

client: client.c utils.o
	$(CC) $(CFLAGS) -o client client.c utils.o

server: server.c utils.o
	$(CC) $(CFLAGS) -lpthread -o server server.c utils.o

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o client server

