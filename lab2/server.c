/*
 * server.c - a chat server (and monitor) that uses pipes and sockets
 */


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

// constants for pipe FDs
#define WFD 1
#define RFD 0

#define DEFAULT_PORT 7777



/*
 * monitor - provides a local chat window
 * @param srfd - server read file descriptor
 * @param swfd - server write file descriptor
 */
void monitor(int srfd, int swfd) {
	char buf[1024]; // initialize buf
	int rbytes = 1;
	int wbytes;
	
	// drop into server monitor loop
	while (rbytes > 0) {
		// read from server pipe and check for failure or EOF
		rbytes = read(srfd, buf, 1024);
		if (rbytes == -1) {
			perror("read");
			exit(1);
		} else if (rbytes == 0) {
			break;
		}
		
		// write to std output and check for failure
		wbytes = write(STDOUT_FILENO, buf, rbytes);
		if (wbytes == -1) {
			perror("write");
			exit(1);
		}
		
		// write > to std output and check for failure
		wbytes = write(STDOUT_FILENO, "\n> ", 4);
		if (wbytes == -1) {
			perror("write");
			exit(1);
		}
		
		// zero buffer
		memset(&buf, 0, 1024);
		
		// read from std input and check for failure or EOF
		rbytes = read(STDIN_FILENO, buf, 1024);
		if (rbytes == -1) {
			perror("read");
			exit(1);
		} else if (rbytes == 0) {
			break;
		}
		
		// write to server pipe and check for failure
		wbytes = write(swfd, buf, rbytes);
		if (wbytes == -1) {
			perror("write");
			exit(1);
		}
	}
}



/*
 * server - relays chat messages
 * @param mrfd - monitor read file descriptor
 * @param mwfd - monitor write file descriptor
 * @param portno - TCP port number to use for client connections
 */
void server(int mrfd, int mwfd, int portno) {
	char buf[1024];
	int val = 1;
	int rbytes = 1;
	int sfd, bind_check, listen_check, clientfd, wbytes;
	socklen_t addrlen;
	struct sockaddr_in address0, address1;
	
	// create socket
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1) {
		perror("socket");
		exit(1);
	}
	
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	
	// set address vals
	address0.sin_family = AF_INET;
	address0.sin_addr.s_addr = htonl(INADDR_ANY);
	address0.sin_port = htons(portno);

	// bind socket and check for failure
	addrlen = sizeof(address0);
	bind_check = bind(sfd, (struct sockaddr *) &address0, addrlen);
	if (bind_check == -1) {
		perror("bind");
		exit(1);
	}
	
	// check for listen failure
	listen_check = listen(sfd, 5);
	if (listen_check == -1) {
		perror("listen");
		exit(-1);
	}
	
	// accept client socket connection and check for failure
	addrlen = sizeof(address1);
	clientfd = accept(sfd, (struct sockaddr *) &address1, &addrlen);
	if (clientfd == -1) {
		perror("accept");
		exit(1);
	}

	// drop into loop relaying info
	while (rbytes > 0) {
		// read from client socket and check for failure or EOF
		rbytes = read(clientfd, buf, 1024);
		if (rbytes == -1) {
			perror("read");
			exit(1);
		} else if (rbytes == 0) {
			break;
		}

		// write to monitor pipe and check for failure
		wbytes = write(mwfd, buf, rbytes);
		if (wbytes == -1) {
			perror("write");
			exit(1);
		}
		
		// zero buffer
		memset(&buf, 0, 1024);

		// read from monitor pipe and check for failure or EOF
		rbytes = read(mrfd, buf, 1024);
		if (rbytes == -1) {
			perror("read");
			exit(1);
		} else if (rbytes == 0) {
			break;
		}

		// write to client socket and check for failure
		wbytes = write(clientfd, buf, rbytes);
		if (wbytes == -1) {
			perror("write");
			exit(1);
		}
		
		// zero buffer
		memset(&buf, 0, 1024);
	}
}



int main(int argc, char **argv) {
	char opt;
	int pfds0[2];
	int pfds1[2];
	pid_t pid;

	// parse command line args
	while ((opt = getopt(argc, argv, "p:h")) != -1) {
		switch(opt) {
			// portno provided
			case 'p': {
					  // initialize pipes
					  if (pipe(pfds0) == -1) {
						  perror("pipe");
						  exit(1);
					  }
					  if (pipe(pfds1) == -1) {
						  perror("pipe");
						  exit(1);
					  }
					  
					  // custom portno
					  int portno = atoi(optarg);

					  pid = fork();
					  if (pid == -1) {
						  perror("fork");
						  exit(1);
					  } else if (pid == 0) {
						  // child process
						  if (close(pfds1[WFD]) == -1 || close(pfds0[RFD]) == -1) {
							  perror("close");
							  exit(1);
						  }
						  monitor(pfds1[RFD], pfds0[WFD]);
						  if (close(pfds1[RFD]) == -1 || close(pfds0[WFD]) == -1) {
							  perror("close");
							  exit(1);
						  }
						  exit(0);
					  } else {
						  // parent process
						  if (close(pfds0[WFD]) == -1 || close(pfds1[RFD]) == -1) {
							  perror("close");
							  exit(1);
						  }
						  server(pfds0[RFD], pfds1[WFD], portno);
						  if (close(pfds0[RFD]) == -1 || close(pfds1[WFD]) == -1) {
							  perror("close");
							  exit(1);
						  }

						  wait(NULL);
					  	
					  }

					  break;
				  }
			
			// help case
			case 'h': {
					  printf("usage: ./server [-h] [-p port #]\n");
					  printf("\t-h - this help message\n");
					  printf("\t-p # - the port to use when connecting to the server\n");
					  break;
				  }
		}
	}
	
	// default case (no command line args)
	if (argc == 1) {
		
		// create pipes
		if (pipe(pfds0) == -1) {
			perror("pipe");
			exit(1);
		}
		if (pipe(pfds1) == -1) {
			perror("pipe");
			exit(1);
		}

		int def_portno = 7777;

		pid = fork();

		if (pid == -1) {
			perror("fork");
			exit(1);
		} else if (pid == 0) {
			// child process
			if (close(pfds0[WFD]) == -1 || close(pfds1[RFD]) == -1) {
				perror("close");
				exit(1);
			}
			monitor(pfds0[RFD], pfds1[WFD]);
			if (close(pfds0[RFD]) == -1 || close(pfds1[WFD]) == -1) {
				perror("close");
				exit(1);
			}

			exit(0);
		} else {
			// parent process
			if (close(pfds1[WFD]) == -1 || close(pfds0[RFD]) == -1) {
				perror("close");
				exit(1);
			}
			server(pfds1[RFD], pfds0[WFD], def_portno);
			if (close(pfds1[RFD]) == -1 || close(pfds0[WFD]) == -1) {
				perror("close");
				exit(1);
			}

			wait(NULL);
		}
	}

	return 0;
}
