CC=clang
CFLAGS=-Wall -Werror -g

default: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f *.o hello server client
