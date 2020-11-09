#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char **argv) {
	char opt; // initialize getopt val
	// parse command line args
	while ((opt = getopt(argc, argv, "p:h")) != -1) {
		switch(opt) {
			// portno provided
			case 'p': {
				char buf[1024]; // read line buffer
				int rbytes, wbytes, sfd, con_check;
				int portno = atoi(optarg);
				socklen_t addrlen;
				struct sockaddr_in serv_address;
				
				// create socket fd and check for failure
				sfd = socket(AF_INET, SOCK_STREAM, 0);
				if (sfd == -1) {
					perror("socket");
					exit(1);
				}
				
				// set struct vals
				serv_address.sin_family = AF_INET;
				serv_address.sin_port = htons(portno);
				serv_address.sin_addr.s_addr = htonl(INADDR_ANY);
				
				// connect to server and check for failure
				addrlen = sizeof(serv_address);
				con_check = connect(sfd, (struct sockaddr *) &serv_address, addrlen);
				if (con_check == -1) {
					perror("connect");
					exit(1);
				}
				printf("connecting to server...\n");
				
				// drop into loop to read client input
				for (;;) {
					// write to standard out and check for failure
					wbytes = write(STDOUT_FILENO, "\n> ", 4);
					if (wbytes == 0) {
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
					
					// write to socket and check for failure
					wbytes = write(sfd, buf, rbytes);
					if (wbytes == -1) {
						perror("write");
						exit(1);
					}
					
					// zero buffer
					memset(&buf, 0, 1024);
					
					// read from socket and check for failure or EOF
					rbytes = read(sfd, buf, 1024);
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
				}

				printf("Hanging up\n");
				
				break;
			}
			
			// help arg
			case 'h': {
				printf("usage: ./client [-h] [-p port #]\n");
				printf("\t-h - this help message\n");
				printf("\t-p # - the port to connect to\n");
				break;
			}
		}
	}
	
	return 0;
}
