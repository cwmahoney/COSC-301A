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

void killComments(char input[],int len){
	int i = 0;
	for(;i<len;i++){
		if(input[i]=='#'){
			input[i]='\0';
			return;
		}
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
char **tokenify(char *instr, const char delim[]){
	if(instr==NULL){
		return NULL;
	}
	char *str_copy = strdup(instr); //strtok_r() mangles the string, so I copy it

	char *temp;
	char **saveptr = &temp;

	strtok_r(str_copy,delim,saveptr);
	int tok_count = 1;
	
	while(strtok_r(NULL,delim,saveptr)!=NULL){ //counting the number of tokens
		tok_count++;
	}

	char **ret_arr = malloc((tok_count+1)*sizeof(char *));
	int i = 1;
	free(str_copy); //freeing the first strdup()

	str_copy = strdup(instr);

	ret_arr[0] = strdup(strtok_r(str_copy,delim,saveptr));

	for(;i<tok_count;i++){ //filling the array
		ret_arr[i] = strdup(strtok_r(NULL,delim,saveptr));
	}
	free(str_copy); //freeing second strdup()

	ret_arr[i]=NULL;

	return ret_arr;
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

/*Returns malloced array of strings formatted {<command>,<modifiers>,".",NULL} if a input or just {NULL} otherwise*/
char **breakCommand(char *instr){
	if(instr==NULL){
		return NULL;
	}

	const char temp_d[3]={' ','\t','\n'}; //delimeters for tokenify
	char **tokened=tokenify(instr,temp_d); //everthing in tokened is malloced
	int t_len = arrLen(tokened);

	if (tokened==NULL){
		return NULL;
	}
			

	char **ret_arr = malloc((t_len+2)*sizeof(char *)); //+2 for "." and NULL
	ret_arr[t_len]=".";
	ret_arr[t_len+1]=NULL;

	int i = 0;
	for(;i<t_len;i++){
		ret_arr[i]=tokened[i];
	}

	free(tokened);
	return ret_arr;
}

int main(int argc, char **argv) {
    char *prompt = "hitme> ";
    printf("%s", prompt);
    fflush(stdout);
	int buffer_len=1024;
    
    char buffer[buffer_len];
	char **cmd_arr;
	const char temp_d[2]={';','\n'}; //delimeter for tokenify

    while (fgets(buffer, buffer_len, stdin) != NULL) {
        /* process current command line in buffer */
        /* just a hard-coded command here right now */

		killComments(buffer,buffer_len); //Buffer is always a string
		cmd_arr = tokenify(buffer,temp_d); //array of strings, everything is on the heap
		
		int i = 0;
		int clean_len = 0;
		char ***clean_cmd_arr=malloc(sizeof(char **)*arrLen(cmd_arr));
		char **temp_c;

		printf("Length of cmd_arr: %d\n",arrLen(cmd_arr));

		while(cmd_arr[i]!=NULL){
			printf("cmd: _%s_ %d\n",cmd_arr[i],i);
			temp_c=breakCommand(cmd_arr[i]); //malloced, remember
			free(cmd_arr[i]);
			
			printf("temp_c: %p\n",temp_c);
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
		
		printf("Original: %s\n",clean_cmd_arr[0][0]);

		i=0;
		for(;i<clean_len-1;i++){ //i < length of clean_cmd_array
			char **cmd = clean_cmd_arr[i];
			printf("Command: %s\n",cmd[i]);

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

		    printf("%s", prompt);
		    fflush(stdout);
	
			free(clean_cmd_arr[i]);
		}
		free(clean_cmd_arr);
    }

    return 0;
}

