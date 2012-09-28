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

/*Executes a single command cmd*/
void execCmd(char **cmd){
	pid_t p = fork();
    if (p == 0) {
        /* in child */
		//printf("Running Child #\n");
        if (execv(cmd[0], cmd) < 0) {
            fprintf(stderr, "execv failed: %s\n", strerror(errno));
			exit(0);
        }
    } else if (p > 0) {
        /* in parent */
        int rstatus = 0;
        waitpid(p,&rstatus,0);; //will continue if child errors-out

		//pid_t childp = 
        /* for this simple starter code, the only child process we should
           "wait" for is the one we just spun off, so check that we got the
           same process id
        if(p == childp){
        	printf("Parent got carcass of child process %d, return val %d\n", childp, rstatus);
		}
		else{
			;//printf("Parent got carcass of stillborn child process %d, return val %d\n", childp, rstatus);
		} */ 
    } else {
        /* fork had an error; bail out */
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
    }
}

int modeCheck(char **cmd, _Bool mode){
	if(!strcmp(cmd[0],"mode")){ //mode command given
		if((0==strcmp(cmd[1],"p"))||(0==strcmp(cmd[1],"parallel"))){
			if(mode){ //bool == 1 means sequential
				printf("Switched to PARALLEL mode\n");
				mode = !mode;
			}
			else{
				printf("Already in PARALLEL mode\n");
			}
		}else if((0==strcmp(cmd[1],"s"))||(0==strcmp(cmd[1],"sequential"))){
			if(!mode){ //bool == 0 means parallel
				printf("Switched to SEQUENTIAL mode\n");
				mode = !mode;
			}
			else{
				printf("Already in SEQUENTIAL mode\n");
			}
		}else{ //Not sure if invalid follow-up to mode should be allowed. I assume it is.
			printf("Current mode is ");
			if(!mode){
				printf("PARALLEL");
			}else{
				printf("SEQUENTIAL");
			}
			printf("\n");
		}
		return mode;		
	}
	return -1; //an int, not a bool
}

int main(int argc, char **argv) {
    char *prompt = "hitme> ";
    printf("%s", prompt);
    fflush(stdout);
	const int buffer_len=1024;
    
    char *buffer=malloc(sizeof(char *)*buffer_len);
	buffer[0]='\0'; //initial value
	char **cmd_arr;

	pid_t p = 1; //initial value for p

    while (fgets(buffer, buffer_len, stdin) != NULL) {
        /* process current command line in buffer */
        /* just a hard-coded command here right now */

		if(buffer[0]==EOF){
			printf("Got it\n\n");
			free(buffer); //flushing memory
			exit(0);
		}

		killComments(buffer,buffer_len); //Buffer is always a string
		cmd_arr = tokenify(buffer,";"); //array of strings, everything is on the heap
		
		int i = 0;
		int clean_len = 0;
		char ***clean_cmd_arr=malloc(sizeof(char **)*arrLen(cmd_arr));
		char **temp_c;

		printf("Length of cmd_arr: %d\n",arrLen(cmd_arr));

		while(cmd_arr[i]!=NULL){
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
        
        //char *cmd[] = { "/bin/ls", "-l","-t", "-r", ".", NULL };

		i=0;
		char **cmd;
		_Bool mode = 1; //1 means sequential
		int temp;
		int j;
		for(;clean_cmd_arr[i]!=NULL;i++)  //i < length of clean_cmd_array
		{		
			cmd = clean_cmd_arr[i];
			printf("Command: _%s_\n",cmd[0]);

			if(0==strcmp(cmd[0],"exit")){ //exit command given
				printf("Exiting\n");

				//flushing memory				
				for(;clean_cmd_arr[i]!=NULL;i++){
					j = 0;
					for(;j<arrLen(clean_cmd_arr[i])-2;j++){
						free(clean_cmd_arr[i][j]);
					}
					free(clean_cmd_arr[i]);
				}
				free(clean_cmd_arr);
				free(buffer);
			}
			
			temp = modeCheck(cmd,mode);
			if(-1!=temp){ //flushing memory if we hit "mode"
				mode=temp;
				j = 0;
				for(;j<arrLen(clean_cmd_arr[i])-2;j++){
					free(clean_cmd_arr[i][j]);
				}
				free(clean_cmd_arr[i]);
			}else
			{ //no "mode" starting cmd
				p = fork();
				if (p == 0) {
					/* in child */
					execCmd(cmd); //carry out one process at a time
					break; //break out of for-loop for child
				} else if (p > 0) {
					/* in parent */
					if(mode){ //I have not the slightest idea as to whether or not this works
						int rstatus = 0;
						waitpid(p,&rstatus,0); //will continue if/when child errors-out
					}
				} else {
					/* fork had an error; bail out */
					fprintf(stderr, "fork failed: %s\n", strerror(errno));
				}			

				j = 0;
				for(;j<arrLen(clean_cmd_arr[i])-2;j++){
					free(clean_cmd_arr[i][j]); //freeing each individual string, minus the const "." and NULL
				}
				free(clean_cmd_arr[i]);
				//printf("TESTING\n");
			}
		}
		if(p<=0){
			break; //break out of while-loop for child
		}
		if(!mode){
			int nstatus = 0;
			waitpid(0,&nstatus,0);
		}
		printf("%s", prompt);
		fflush(stdout);

		free(clean_cmd_arr);
		//printf("TESTING\n");
    }
	if(p>0){ //only free buffer in parent at end
		free(buffer);
	}

    return 0;
}

