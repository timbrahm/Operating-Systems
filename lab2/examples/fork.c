#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WRITE_FD 1
#define READ_FD  0

void listener(int fd) {
  char buf[1024];
  int rbytes;

  rbytes = read(fd, buf, sizeof(buf));
  while (rbytes > 0) {
    write(STDOUT_FILENO, "listener: ", 11);
    write(STDOUT_FILENO, buf, rbytes);
    rbytes = read(fd, buf, sizeof(buf));
  }
}


void talker(int fd) {
  char buf[1024];
  int rbytes;

  while ((rbytes == read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
    write(fd, buf, rbytes); // write to pipe
  }
}



int main(int argc, char **argv) {
  pid_t pid;
  int fds[2];

  if (pipe(fds) == -1) {
    perror("pipe problem");
    exit(1);
  }

  pid = fork();
  if (pid == -1) {
    perror("fork error");
    exit(1);
  } else if (pid == 0) {
    // child process
    close(fds[WRITE_FD]);
    listener(fds[READ_FD]);
    close(fds[READ_FD]);
  } else {
    // parent process
    close(fds[READ_FD]);
    talker(fds[WRITE_FD]);
    close(fds[WRITE_FD]);
  }

  return 0;
}
