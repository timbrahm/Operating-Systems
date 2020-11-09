#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

int main(int argc, char **argv) {

	// initialize DIR pointer for opendir	
	DIR *dir;
	// initialize 2 dirent structs for dir parsing
	struct dirent *file_pre, *file;
	
	// check base ls case
	if (argc == 1) {
		// open cur dir and check for success
		dir = opendir(".");
		
		if (dir == NULL) {
			perror("opendir");
			exit(-1);
		}
		
		// parse dir, print file's d_name until NULL
		while ((file = readdir(dir)) != NULL) {
                	if (file->d_name[0] != '.') {
                        	printf("%s  ", file->d_name);
                	}
        	}
        	printf("\n");
	}
	// check for ls -l case when argc == 2
	else if (argc == 2) {
		if (strcmp(argv[1], "-l") == 0) {
			// open cur dir and check for success
			dir = opendir(".");

			if (dir == NULL) {
				perror("opendir");
				exit(-1);
			}
			
			// intiialize 3 ints to keep track of longest str/int in each ls -l column
			int max_username = 0, max_grpname = 0, max_filesize  = 0;
			
			// loop through files in dir first time to calculate lengths needed for whitespace in ls -l columns
			while ((file_pre = readdir(dir)) != NULL) {
                                if (file_pre->d_name[0] != '.') {
					// initialize stat buf and return value
                                        struct stat buf;
                                        int rv;

					// write stat value to buf and return value for error checking
                                        rv = stat(file_pre->d_name, &buf);
					// check for stat success
                                        if (rv == -1) {
                                                perror("stat");
                                                exit(-1);
                                        }
					
					// initialize passwd and group structs to get username and group name 
					struct passwd *pw;
                                        struct group *grp;
					
					// get user info and check for success
                                        pw = getpwuid(buf.st_uid);
                                        if (pw == NULL) {
                                                perror("getpwuid");
                                                exit(-1);
                                        }

					// get group info and check for success
                                        grp = getgrgid(buf.st_gid);
                                        if (grp == NULL) {
                                                perror("getgrgid");
                                                exit(-1);
                                        }
					
					
					// update max username as the longest username in dir
                                        if (max_username < strlen(pw->pw_name)) {
                                                max_username = strlen(pw->pw_name);
                                        }

					// update max group as the longest group name in dir
                                        if (max_grpname < strlen(grp->gr_name)) {
                                                max_grpname = strlen(grp->gr_name);
                                        }
					
					// initialize int and temp buffer to calc length of file size
					int digits = 0;
                                        int temp = buf.st_size;
					// find num of digits
                                        while (temp != 0) {
                                                temp = temp / 10;
                                                ++digits;
                                        }

					// update max filesize as the longest num in dir
                                        if (max_filesize < digits) {
                                                max_filesize = digits;
                                        }
				}
			}
			
			closedir(dir);
			
			// open dir again to actully print file info, with proper whitespace calculated from prev loop
			dir = opendir(".");

			// check for opendir success
                        if (dir == NULL) {
                                perror("opendir");
                                exit(-1);
                        }

			// loop through files in dir
			while ((file = readdir(dir)) != NULL) {
				if (file->d_name[0] != '.') {
					// initialize stat buffer and return value for file info
					struct stat buf;
        				int rv;
					
					// get file info and check for success
        				rv = stat(file->d_name, &buf);
        				if (rv == -1) {
               					perror("stat");
                				exit(-1);
        				}
					
					// print file permissions in ls form by checking st_modes against file mode bits
					printf((S_ISDIR(buf.st_mode)) ? "d" : "-");
    					printf((buf.st_mode & S_IRUSR) ? "r" : "-");
				    	printf((buf.st_mode & S_IWUSR) ? "w" : "-");
				    	printf((buf.st_mode & S_IXUSR) ? "x" : "-");
				    	printf((buf.st_mode & S_IRGRP) ? "r" : "-");
				    	printf((buf.st_mode & S_IWGRP) ? "w" : "-");
				    	printf((buf.st_mode & S_IXGRP) ? "x" : "-");
				    	printf((buf.st_mode & S_IROTH) ? "r" : "-");
				    	printf((buf.st_mode & S_IWOTH) ? "w" : "-");
				    	printf((buf.st_mode & S_IXOTH) ? "x" : "-");
					printf(" ");
					
					// initialze passwd and group structs for username and group info
					struct passwd *pw;
					struct group *grp;
					
					// get user info and check for success
					pw = getpwuid(buf.st_uid);
					if (pw == NULL) {
						perror("getpwuid");
						exit(-1);
					}		
					
					// get group info and check for success
					grp = getgrgid(buf.st_gid);
					if (grp == NULL) {
						perror("getgrgid");
						exit(-1);
					}
					

					// print username and groupname for file with whitespace calculated from previous loop
					printf("%-*s ", max_username, pw->pw_name);
					printf("%-*s ", max_grpname, grp->gr_name);
					
					
					// print filesize with whitespace calculated from previous loop
					printf("%*ld ", max_filesize, buf.st_size);

					// print filename
					printf("%s\n", file->d_name);
				}
			}
		}
		else {
			// print error message for ls called with 1 incorrect cmd line params
			printf("Incompatible parameters, please follow the format below:\n");
                	printf("ls\tor\tls -l\n");	
			exit(-1);
		}	
	
	}
	else {
		// print error message for ls called with more than 1 incorrect cmd line params
		printf("Incompatible parameters, please follow the format below:\n");
		printf("ls\tor\tls -l\n");
		exit(-1);
	}

	return 0;
}
