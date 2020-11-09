CC=clang
CFLAGS=-Wall -Werror -g

default: cp ls

cp: cp.c
	$(CC) $(CFLAGS) -o cp cp.c

ls: ls.c
	$(CC) $(CFLAGS) -o ls ls.c

clean:
	rm -f *.o hello cp ls
