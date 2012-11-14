/*
Curt Mahoney
11/14/2012
Implement a basic "ls" function which just prints off file names
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>


/*Very basic ls, only prints off filenames of the inputted directory. Directory is assumed to be the argv[1] index, or ./ otherwise*/
int main(int argc, char **argv){
	char *directory;
	if(argv[1]==NULL){
		directory = strdup("./");
	}else{
		directory = strdup(argv[1]);
	}

	DIR *stream = opendir(directory);
	free(directory);
	struct stat statresult;

	if(stream==NULL){ //not a real directory address
		printf("\"%s\" is not a valid address\n", argv[1]); //will only happen with inputted directory, "./" will always exist
	}else{ //real directory, start reading
		struct dirent *entry = readdir(stream);
		while(entry!=NULL){
			stat(entry->d_name,&statresult); //don't care about return since we know it exists or entry would have failed out
			if(entry->d_type==DT_REG){ //file
				printf("%s %s %jdB\n",entry->d_name,"file",(unsigned long long)(&statresult)->st_size);
			}else{ //directory, we assume, since it's not a file
				printf("%s %s\n",entry->d_name,"directory");
			}
			entry = readdir(stream);
		}
	}

	closedir(stream); //frees stream

	return 0;
}

/*
opendir
closedir
readdir
stat

*/
