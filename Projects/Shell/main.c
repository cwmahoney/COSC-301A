/*
 * project 1 (shell) main.c template 
 *
 * Curtis Mahoney, Adriana Sperlea
 *
 */

/* you probably won't need any other header files for this project */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

void killComments(char *input,int len){
	int i = 0;
	while(input[i]!='\0'){
		if(input[i]=='#'){
			input[i]='\0';
			return;
		}
		i++;
	}
}

//From lab01
void removeWhitespace(char *instr){ //Not clever, not efficient, but it will work
	if(instr==NULL){
		return;
	}

	unsigned int len = strlen(instr);
	char temp_str[len+1];
	int i = 0;
	int j = 0;

	for(;i<len;i++){ //if you hit a whitespace, don't add it to teh temp_str
		if(!isspace(instr[i])){
			temp_str[j++]=instr[i];
		}
	}
	temp_str[j]='\0';
	
	i = 0;
	for(;i<strlen(temp_str)+1;i++){ //going up to and including the \0 at the end of temp_str
		instr[i]=temp_str[i];
	}
}

/*Returns a malloced array of malloced strings. Double freedom necessary*/
char** tokenify(char *s, const char delim[]) //Adriana's code, slightly modified to add strtok_r, fix string-mangling
{
	char *copy, *word = NULL, **result = NULL;
	// use strtok to count how many tokens are in the string in order to be able to allocate the correct
	// amount of memory
	copy = strdup(s);
	int count = 0;
	char *temp;
	char **saveptr = &temp;

	word = strtok_r(copy, delim,saveptr);
	while (word != NULL) {
		count++;
		word = strtok_r(NULL, delim,saveptr);
	}
	free(copy);

	// allocate pointers for each token + 1 for the NULL character at the end
	copy = strdup(s);
	result = malloc((count + 1) * sizeof(char*));
	word = strtok_r(copy, delim,saveptr);
	count = 0;
	while (word != NULL) {
		// allocate memory for each token
		printf("Word: %s\n",word);
		result[count] = strdup(word); //malloc
		word = strtok_r(NULL, delim,saveptr);
		count++;
	}
	free(copy);

	// add the NULL terminator
	result[count] = NULL;
	// return values
	return result;
}

/*Returns length of an array ending with a NULL in it's last index, regardless of array type*/
int arrLen(char **inarr){ //have to have different types anyway, for ** and ***, so might as well not get yelled at by the compiler
	int i = 0;
	while(inarr[i++]!=NULL){
		;
	}
	return i;
}
int arrLen2(char ***inarr){
	int i = 0;
	while(inarr[i++]!=NULL){
		;
	}
	return i;
}

/*Returns malloced array of strings formatted {<command>,<modifiers>,".",NULL} if a valid input or just {NULL} otherwise*/
char **breakCommand(char *instr){
	if(instr==NULL){
		return NULL;
	}

	int i =0;
	while(isspace(instr[i])){ //necessary for super-trailing whitespace
		i++;
	}

	if(instr[i]=='\0'){
		return NULL;
	}

	char **tokened=tokenify(instr," \t\n"); //everthing in tokened is malloced
	int t_len = arrLen(tokened);

	if (tokened==NULL){
		return NULL;
	}

	char **ret_arr = malloc((t_len+1)*sizeof(char *)); //+2 for "." and NULL, -1 for the extraneous NULL in tokened
	ret_arr[t_len-1]=".";
	ret_arr[t_len]=NULL;

	i = 0;
	for(;i<t_len-1;i++){ //don't want trailing NULL
		ret_arr[i]=tokened[i];
	}

	free(tokened);
	return ret_arr;
}

int main(int argc, char **argv) {
    char *prompt = "hitme> ";
    printf("%s", prompt);
    fflush(stdout);
	const int buffer_len=1024;
    
    char *buffer=malloc(sizeof(char *)*buffer_len);
	buffer[0]='\0'; //initial value
	char **cmd_arr;

    while (fgets(buffer, buffer_len, stdin) != NULL) {
        /* process current command line in buffer */
        /* just a hard-coded command here right now */

		killComments(buffer,buffer_len); //Buffer is always a string
		cmd_arr = tokenify(buffer,";"); //array of strings, everything is on the heap
		
		int i = 0;
		int clean_len = 0;
		char ***clean_cmd_arr=malloc(sizeof(char **)*arrLen(cmd_arr));
		char **temp_c;

		printf("Length of cmd_arr: %d\n",arrLen(cmd_arr));

		while(cmd_arr[i]!=NULL){
			printf("TESTING\n");
			temp_c=breakCommand(cmd_arr[i]); //malloced, remember
			free(cmd_arr[i]);
			if(temp_c!=NULL){
				clean_cmd_arr[clean_len++]=temp_c;
			}
			i++;
		}
		free(cmd_arr);
		clean_cmd_arr[clean_len]=NULL;
		
		printf("cca len: %d\n",arrLen2(clean_cmd_arr));
		
		/*
		printf("Buffer: %s",buffer);
		printf("cmd_arr[0]: %s",cmd_arr[0]);*/
        
        //char *cmd[] = { "/bin/ls", "-l","-t", "-r", ".", NULL };

		i=0;

		for(;clean_cmd_arr[i]!=NULL;i++)  //i < length of clean_cmd_array
		{
			char **cmd = clean_cmd_arr[i];
			printf("Command: _%s_\n",cmd[0]);

		    pid_t p = fork();
		    if (p == 0) {
		        /* in child */
		        if (execv(cmd[0], cmd) < 0) {
		            fprintf(stderr, "execv failed: %s\n", strerror(errno));
		        }

		    } else if (p > 0) {
		        /* in parent */
		        int rstatus = 0;
		        pid_t childp = wait(&rstatus);

		        /* for this simple starter code, the only child process we should
		           "wait" for is the one we just spun off, so check that we got the
		           same process id */ 
		        assert(p == childp);

		        printf("Parent got carcass of child process %d, return val %d\n", childp, rstatus);
		    } else {
		        /* fork had an error; bail out */
		        fprintf(stderr, "fork failed: %s\n", strerror(errno));
		    }
			
			int j = 0;
			for(;j<arrLen(clean_cmd_arr[i]-2);j++){
				free(clean_cmd_arr[i][j]); //freeing each individual string, minus the const "." and NULL
			}
			free(clean_cmd_arr[i]);
		}
		printf("%s", prompt);
		fflush(stdout);

		free(clean_cmd_arr);
		//printf("TESTING\n");
    }
	free(buffer);

    return 0;
}

