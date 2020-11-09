CC=clang
CFLAGS=-Wall -Werror -g

default: server client

cp: server.c
	$(CC) $(CFLAGS) -o cp cp.c

ls: client.c
	$(CC) $(CFLAGS) -o ls ls.c

clean:
	rm -f *.o server client
