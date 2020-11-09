#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WRITE_FD 1
#define READ_FD 0

void listener(int fd);
void talker(int fd);

void listener(int fd) {
  char buf[1024];
  int rbytes;

  rbytes = read(fd, buf, sizeof(buf));
  while (rbytes > 0) {
    write(STDOUT_FILENO, "r: ", 3);
    write(STDOUT_FILENO, buf, rbytes);
    rbytes = read(fd, buf, sizeof(buf));
  }
}

void talker(int fd) {
  char buf[1024];
  int rbytes;

  while ((rbytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
    write(fd, buf, rbytes);
  }
}

int main(int argc, char **argv) {
  int fds[2];
  pid_t pid;

  if (pipe(fds) == -1) {
    perror("pipe");
    exit(1);
  }

  pid = fork();

  if (pid == -1) {
    perror("fork");
    exit(1);
  } else if (pid == 0) {
    // child process
    printf("child pid: %d\n", getpid());
    close(fds[WRITE_FD]);
    listener(fds[READ_FD]);
  } else {
    // parent process
    printf("parent pid: %d\n", getpid());
    close(fds[READ_FD]);
    talker(fds[WRITE_FD]);
  }
  exit(0);
}
