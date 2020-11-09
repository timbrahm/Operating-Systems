/*
 * server.c - a chat server (and monitor) that uses pipes and sockets
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#define MAX_CLIENTS 10

// constants for pipe FDs
#define WFD 1
#define RFD 0


/**
 * nonblock - a function that makes a file descriptor non-blocking
 * @param fd file descriptor
 */
void nonblock(int fd) {
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
		perror("fcntl (get):");
		exit(1);
	}
	if (fcntl(fd, F_SETFL, flags | FNDELAY) == -1) {
		perror("fcntl (set):");
		exit(1);
	}

}



/*
 * monitor - provides a local chat window
 * @param srfd - server read file descriptor
 * @param swfd - server write file descriptor
 */
void monitor(int srfd, int swfd) {
	char buf[1024];
	int rbytes = 1;
	int wbytes, activity;
	fd_set rfds;
	int max_d;

	//server monitor loop
	while (rbytes > 0) {
		memset(&buf, 0, 1024);

		FD_ZERO(&rfds);
		
		//add std input and server read fd to fd set
		FD_SET(STDIN_FILENO, &rfds);
		max_d = STDIN_FILENO;
		FD_SET(srfd, &rfds);
		if (srfd > max_d) {
			max_d = srfd;
		}
		
		//monitor select return value and check for failure
		activity = select(max_d + 1, &rfds, NULL, NULL, NULL);
		if ((activity == -1) && (errno != EINTR)) {
			perror("select");
			exit(1);
		}
		
		//if std input is ready, read and write to server write fd, unless monitor disconnects
		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			rbytes = read(STDIN_FILENO, buf, 1024);
			if (rbytes == -1) {
				perror("read");
				exit(1);
			} else if (rbytes == 0) {
				//monitor disconnects
				break;
			} else {
				wbytes = write(swfd, buf, rbytes);
				if (wbytes == -1) {
					perror("write");
					exit(1);
				}
			}
		}
		
		//if server read fd is ready, read and write to std output
		if (FD_ISSET(srfd, &rfds)) {
			rbytes = read(srfd, buf, 1024);
			if (rbytes == -1) {
				perror("read");
				exit(1);
			} else {
				wbytes = write(STDOUT_FILENO, buf, rbytes);
				if (wbytes == -1) {
					perror("write");
					exit(1);
				}
			}
		}
	}
	
	//make sure to call exit with child
	exit(0);
}



/*
 * server - relays chat messages
 * @param mrfd - monitor read file descriptor
 * @param mwfd - monitor write file descriptor
 * @param portno - TCP/IP port number
 */
void server(int mrfd, int mwfd, int portno) {
	char buf[1024];
	int val = 1;
	int rbytes = 1;
	int sfd, new_socket, bind_check, listen_check, wbytes, activity, sd;
	int client_sockets[10];
	int max_clients = 10;

	fd_set rfds;
	int max_d;
	struct timeval tv;

	socklen_t addrlen;
	struct sockaddr_in address;
	
	//initialize client sockets as 0
	for (int i = 0; i < max_clients; i++) {
		client_sockets[i] = 0;
	}

	// create socket and check for failure
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
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(portno);

	// bind socket and check for failure
	addrlen = sizeof(address);
	bind_check = bind(sfd, (struct sockaddr *) &address, addrlen);
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

	addrlen = sizeof(address);

	while(1) {
		memset(&buf, 0, 1024);

		FD_ZERO(&rfds);
		
		//add socket fd and monitor read fd to fd set
		FD_SET(sfd, &rfds);
		max_d = sfd;
		FD_SET(mrfd, &rfds);
		if (mrfd > max_d) {
			max_d = mrfd;
		}
		
		//add any active client sockets to fd set and update max if needed
		for (int i = 0; i < max_clients; i++) {
			sd = client_sockets[i];
			if (sd > 0) {
				FD_SET(sd, &rfds);
			}
			if (sd > max_d) {
				max_d = sd;
			}
		}
		
		//set timeout values for select
		tv.tv_sec = 0;
		tv.tv_usec = 100000;

		//monitor select return value and check for failure
		activity = select(max_d + 1, &rfds, NULL, NULL, &tv);
		if ((activity == -1) && (errno != EINTR)) {
			perror("select");
			exit(1);
		}
		
		//if socket fd is ready, a new client is trying to connect
		if (FD_ISSET(sfd, &rfds)) {
			//create new socket and check for failure
			new_socket = accept(sfd, (struct sockaddr *) &address, &addrlen);
			if (new_socket == -1) {
				perror("accpet");
				exit(1);
			}
			
			//set as nonblocking fd
			nonblock(new_socket);
			
			//add it to next inactive in client socket list
			for (int i = 0; i < max_clients; i++) {
				if (client_sockets[i] == 0) {
					client_sockets[i] = new_socket;
					break;
				}
			}
		}
		
		//if monitor read fd is ready, read and write to all clients, unless monitor has disconnected
		if (FD_ISSET(mrfd, &rfds)) {
			rbytes = read(mrfd, buf, 1024);
			if (rbytes == -1) {
				perror("read");
				exit(1);
			} else if (rbytes == 0) {
				//monitor disconnect
				break;
			} else {
				for (int i = 0; i < max_clients; i++) {
					sd = client_sockets[i];
					if (sd != 0) {
						wbytes = write(sd, buf, rbytes);
						if (wbytes == -1) {
							perror("write");
							exit(1);
						}
					}
				}
			}
		}
		
		//if a client socket is ready, read from it and write to monitor write fd as well as all other clients, unless it is disconnecting
		for (int i = 0; i < max_clients; i++) {
			sd = client_sockets[i];
			if (FD_ISSET(sd, &rfds)) {
				rbytes = read(sd, buf, 1024);
				if (rbytes == -1) {
					perror("read");
					exit(1);
				} else if (rbytes == 0) {
					//client closed connection
					if (close(sd) == -1) {
						perror("close");
						exit(1);
					}
					client_sockets[i] = 0;
				} else {
					wbytes = write(mwfd, buf, rbytes);
					if (wbytes == -1) {
						perror("write");
						exit(1);
					}

					for (int j = 0; j < max_clients; j++) {
						if (client_sockets[j] != sd && client_sockets[j] != 0) {
							wbytes = write(client_sockets[j], buf, rbytes);
							if (wbytes == -1) {
								perror("write");
								exit(1);
							}
						}
					}
				}
			}
		}
	}
	
	//close master socket fd
	if (close(sfd) == -1) {
		perror("close");
		exit(1);
	}	
}



int main(int argc, char **argv) {
	char opt;
	int mfds[2];
	int sfds[2];
	pid_t pid;
	int portno = 7777;

	// parse command line args
	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch(opt) {
			case 'p':
				portno = atoi(optarg);
				break;
		}
	}
	
	//create pipies and check for failure
	if (pipe(mfds) == -1) {
		perror("pipe");
		exit(1);
	}
	if (pipe(sfds) == -1) {
		perror("pipe");
		exit(1);
	}

	//set fds as nonblocking
	nonblock(mfds[RFD]);
	nonblock(mfds[WFD]);
	nonblock(sfds[RFD]);
	nonblock(sfds[WFD]);
	nonblock(STDIN_FILENO);
	
	//create child
	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	} else if (pid == 0) {
		// child process
		if (close(mfds[WFD]) == -1 || close(sfds[RFD]) == -1) {
			perror("close");
			exit(1);
		}

		monitor(mfds[RFD], sfds[WFD]);

		if (close(mfds[RFD]) == -1 || close(sfds[WFD]) == -1) {
			perror("close");
			exit(1);
		}
	} else {
		//parent process
		if (close(sfds[WFD]) == -1 || close(mfds[RFD]) == -1) {
			perror("close");
			exit(1);
		}

		server(sfds[RFD], mfds[WFD], portno);

		if (close(sfds[RFD]) == -1 || close(mfds[WFD]) == -1) {
			perror("close");
			exit(1);
		}

		wait(NULL);
	}

	return 0;
}
