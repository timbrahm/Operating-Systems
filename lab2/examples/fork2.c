#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define WRITE_FD 1
#define READ_FD  0

char pidstr[256];

void listener(int fd);
void talker(int fd);


void listener(int fd) {
  char buf[1024];
  int rbytes;

  do {
    rbytes = read(fd, buf, sizeof(buf));
    write(STDOUT_FILENO, pidstr, strlen(pidstr));
    write(STDOUT_FILENO, ": ", 3);
    write(STDOUT_FILENO, buf, rbytes);
  } while (rbytes > 0);
}

void talker(int fd) {
  char buf[1024];
  int rbytes;

  while ((rbytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
    write(fd, buf, rbytes);
  }
}

int main(void) {
  int pfds[2], ret;

  ret = pipe(pfds);

  pid_t p = fork();

  sprintf(pidstr, "%d: ", getpid());

  if (p == -1) {
    perror("fork");
    exit(1);
  } else if (p == 0) {
    // child - reader/listener
    printf("%s: child pipe[READ]: %d pipe[WRITE]: %d\n", pidstr, pfds[READ_FD], pfds[WRITE_FD]);

    close(pfds[WRITE_FD]);
    listener(pfds[READ_FD]);
    close(pfds[READ_FD]);

  } else {
    // parent - writer/talker
    printf("%s: parent pipe[READ]: %d pipe[WRITE]: %d\n", pidstr, pfds[READ_FD], pfds[WRITE_FD]);

    close(pfds[READ_FD]);
    talker(pfds[WRITE_FD]);
    close(pfds[WRITE_FD]);

    wait(NULL); // wait for child process to terminate
  }
}
