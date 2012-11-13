/*
Curt Mahoney
11/14/2012
Implement a basic "touch" function which just creates a new file if it doesn't exist
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*Very basic touch, only creates a new file if it doesn't exist, doesn't talk with the user at all, doesn't change permissions of existing file*/
int main(int argc, char **argv){
	//printf("Filename: %s\n",argv[1]);
	
	char temp[strlen(argv[1])+3]; //+2 for the ./, +1 for the \0
	
	strcpy(temp,"./"); //append  "./" to filename
	strcat(temp,argv[1]);

	int fd = creat(temp,S_IRWXU); //don't care about fd value
	close(fd);
	return 0;
}
