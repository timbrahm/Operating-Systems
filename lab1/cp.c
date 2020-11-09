#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char **argv) {
	
	// initialize a buffer to hold chars from file
	char buffer[1024];
	// initialize ints for read/write file, and a count to keep track of # chars read
	int file1, file2, count;
	
	// check if too few params
	if (argc < 3) {
		printf("Insufficient parameters, please provide in the form below:\n");
		printf("cp filename1 filename2\n");
		exit(-1);
	}

	// check if too many params
	if (argc > 3) {
		printf("Too many parameters, please provide in form below:\n");
		printf("cp filename1 filename2\n");
		exit(-1);
	}

	// attempt to open the file
	file1 = open(argv[1], O_RDONLY);
	
	// check if file has been successfully opened
	if (file1 == -1) {
		perror("open");
		exit(-1);
	}
	
	// initialize stat struct to write to
	struct stat ret;
	// initialize return value from stat func and a perissions var to use with open
	int rv, permissions;
	
	// attempt to call stat func and check if successful
	rv = stat(argv[1], &ret);
	if (rv == -1) {
		perror("stat");
		exit(-1);
	}
	
	// calculate permissions int based on stat ret data
	permissions = (ret.st_mode & S_IRUSR)|(ret.st_mode & S_IWUSR)|(ret.st_mode & S_IXUSR)|(ret.st_mode & S_IRGRP)|(ret.st_mode & S_IWGRP)|(ret.st_mode & S_IXGRP)|(ret.st_mode & S_IROTH)|(ret.st_mode & S_IWOTH)|(ret.st_mode & S_IXOTH);		

	// use stat to check if write file already exists
	struct stat exist_buffer;
	int exist = stat(argv[2], &exist_buffer);
	
	// if file exists, unlink it
	if (exist == 1) {
		// initialize return value for unlink func
		int status;

		// unlink file that we are copying to in order to overwrite that data and check if successful
		status = unlink(argv[2]);
		if (status == -1) {
			perror("unlink");
			exit(-1);
		}
	}
	
	// open write file, using permissions var as mode. Check if successful
	file2 = open(argv[2], O_CREAT|O_WRONLY, permissions);
	if (file2 == -1) {
		perror("open");
		exit(-1);
	}
	
	// read from read file into buffer until eof, after each 1024 chars write to write file from buffer
	while ((count = read(file1, buffer, 1024)) != 0) {
		write(file2, buffer, count);
	}
	

	return 0;
}
