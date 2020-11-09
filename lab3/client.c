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
#include <netdb.h>


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



int main(int argc, char **argv) {
	char opt;
	int portno = 7777;
	int host_test = 0;
	char *host_name;
	char buf[1024];
	int rbytes = 1;
	int wbytes, activity, sfd, con_check;
	
	socklen_t addrlen;
	struct sockaddr_in serv_address;
	struct hostent *host;

	fd_set rfds;
	int max_d;

	while ((opt = getopt(argc, argv, "h:p:")) != -1) {
		switch(opt) {
			case 'h':
				host_test = 1;
				size_t host_size = sizeof(optarg);
				host_name = (char *) malloc(host_size);
				strcpy(host_name, optarg);  //copy user provided hostname
				break;

			case 'p':
				portno = atoi(optarg);
				break;	
		}
	}
	
	//create socket and check for failure
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1) {
		perror("socket");
		exit(1);
	}
	
	//set address data based on hostname if provided
	serv_address.sin_family = AF_INET;
	serv_address.sin_port = htons(portno);
	
	if (host_test == 1) {
		host = gethostbyname(host_name);
		if (!host) {
			perror("gethostbyname");
			exit(1);
		}
	} else {
		host = gethostbyname("senna.rhodes.edu");
		if (!host) {
			perror("gethostbyname");
			exit(1);
		}
	}
	memcpy(&serv_address.sin_addr.s_addr, host->h_addr, host->h_length);
	
	//connect to server and check for failure
	addrlen = sizeof(serv_address);
	con_check = connect(sfd, (struct sockaddr *) &serv_address, addrlen);
	if (con_check == -1) {
		perror("connect");
		exit(1);
	}
	printf("connected to server...\n");
	
	//enter select loop
	nonblock(STDIN_FILENO);
	nonblock(sfd);
	while (rbytes > 0) {
		memset(&buf, 0, 1024);

		FD_ZERO(&rfds);
		
		//add std input and socket fd into fd set
		FD_SET(STDIN_FILENO, &rfds);
		max_d = STDIN_FILENO;
		FD_SET(sfd, &rfds);
		if (sfd > max_d) {
			max_d = sfd;
		}
		
		//monitor select return and failure checking
		activity = select(max_d + 1, &rfds, NULL, NULL, NULL);
		if ((activity == -1) && (errno != EINTR)) {
			perror("select");
			exit(1);
		}
		
		//if std input ready, read and write to socket, or disconnect
		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			rbytes = read(STDIN_FILENO, buf, 1024);
			if (rbytes == -1) {
				perror("read");
				exit(1);
			} else if (rbytes == 0) {
				//disconnect
				break;
			} else {
				wbytes = write(sfd, buf, rbytes);
				if (wbytes == -1) {
					perror("write");
					exit(1);
				}
			}
		}
		
		//if socket ready, read and write to std output, or disconnect if server shutdown
		if (FD_ISSET(sfd, &rfds)) {
			rbytes = read(sfd, buf, 1024);
			if (rbytes == -1) {
				perror("read");
				exit(1);
			} else if (rbytes == 0) {
				//server shutdown
				break;
			} else {
				wbytes = write(STDOUT_FILENO, buf, rbytes);
				if (wbytes == -1) {
					perror("write");
					exit(1);
				}
			}
		}
	}
	
	//close socket fd
	if (close(sfd) == -1) {
		perror("close");
		exit(1);
	}

	printf("hanging up\n");	

	return 0;
}
