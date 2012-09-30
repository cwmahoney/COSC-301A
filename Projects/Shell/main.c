/*
 * project 1 (shell) main.c template 
 *
 * Curtis Mahoney, Adriana Sperlea
 * -Curt worked on the backbone code, borrowing Adriana's functioning tokenify and implementing all Stage 1 functionality besides implementation of parallel/sequential modes, proper handling of string literal on input, and CPU time.
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

/*Removes given characters*/
void removeCharacters(char *instr, const char *tgt){ //Not clever, not efficient, but it will work
	if(instr==NULL){
		return;
	}

	unsigned int len = strlen(instr);
	char temp_str[len+1];
	int i = 0;
	int j = 0;
	int k = 0;

	for(;i<len;i++){ //if you hit a whitespace, don't add it to teh temp_str
		k=0;		
		for(;k<strlen(tgt);k++){
			if(tgt[k]!=instr[i]){
				temp_str[j++]=instr[i];
			}
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

	/*
	int i = 0;

	while(copy[i]!='\0'){
		if(copy[i]=='\"'){
			while(copy[i]!='\0'){
				if(copy[i]=='\"'){
					
				}
			}
		}

	}*/

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

/*Returns malloced array of strings formatted {<command>,<arg0>, <arg1>,...} if a valid input or just NULL otherwise*/
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

	char* copy = strdup(instr);
	removeCharacters(copy,"\""); //takes out double-quotation marks
	printf("Copy: %s\n",copy);

	char **tokened=tokenify(copy," \t\n"); //everthing in tokened is malloced

	free(copy);

	return tokened;
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

char modeCheck(char **cmd, char mode){
	if(!strcmp(cmd[0],"mode")){ //mode command given
		if(cmd[1]==NULL){ //nothing trailing
			printf("Current mode: ");
			if(mode=='p'){
				printf("PARALLEL");
			}else{
				printf("SEQUENTIAL");
			}
			printf("\n");
			return mode;
		}

		if((0==strcmp(cmd[1],"p"))||(0==strcmp(cmd[1],"parallel"))){
			if(mode=='s'){ //bool == 1 means sequential
				printf("Switched to PARALLEL mode\n");
				return('p');
			}
			else{
				printf("Already in PARALLEL mode\n");
			}
		}else if((0==strcmp(cmd[1],"s"))||(0==strcmp(cmd[1],"sequential"))){
			if(mode=='p'){ //bool == 0 means parallel
				printf("Switched to SEQUENTIAL mode\n");
				return('s');
			}
			else{
				printf("Already in SEQUENTIAL mode\n");
			}
		}else{ //Not sure if invalid follow-up to mode should be allowed. I assume it is.
			printf("Current mode: ");
			if(mode=='p'){
				printf("PARALLEL");
			}else{
				printf("SEQUENTIAL");
			}
			printf("\n");
		}
		return mode;		
	}
	return 'n'; //an int, not a bool
}

//Need to deal with string-literal input, parallel vs. sequential, user vs. kernal time
int main(int argc, char **argv){
    char *prompt = "hitme> ";
    printf("%s", prompt);
    fflush(stdout);
	const int buffer_len=1024;
    char *buffer=malloc(sizeof(char *)*buffer_len);
	buffer[0]='\0'; //initial value
	char **cmd_arr;
	char mode = 's'; //Mode "bit." 's' means sequential

	pid_t p = 1; //initial value for p

    while (fgets(buffer, buffer_len, stdin) != NULL) {
        /* process current command line in buffer */

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
		char temp_m;
		int j;
		_Bool will_exit = 0;
		for(;clean_cmd_arr[i]!=NULL;i++)  //i < length of clean_cmd_array
		{
			cmd = clean_cmd_arr[i];
			printf("Command: _%s_\n",cmd[0]);
			//printf("Num 2: _%s_\n",cmd[1]); //problem with (/bin/echo "str str"), broken up like nobody's business by tokenify

			if(0==strcmp(cmd[0],"exit")){ //exit command given
				will_exit = 1; //will exit later
				j = 0;
				for(;j<arrLen(clean_cmd_arr[i]);j++){
					free(clean_cmd_arr[i][j]); //freeing each individual string
				}
				free(clean_cmd_arr[i]);
			}else
			{	 //not an "exit" command		
				temp_m = modeCheck(cmd,mode);
				if('n'!=temp_m){ //flushing memory if we hit "mode"
					mode=temp_m;
					j = 0;
					for(;j<arrLen(clean_cmd_arr[i]);j++){
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
						if(mode=='s'){ //I have not the slightest idea as to whether or not this works
							int rstatus = 0;
							waitpid(p,&rstatus,0); //will continue if/when child errors-out
						}
					} else {
						/* fork had an error; bail out */
						fprintf(stderr, "fork failed: %s\n", strerror(errno));
					}			

					j = 0;
					for(;j<arrLen(clean_cmd_arr[i]);j++){
						free(clean_cmd_arr[i][j]); //freeing each individual string
					}
					free(clean_cmd_arr[i]);
					//printf("TESTING\n");
				}
			}
			if((clean_cmd_arr[i+1]==NULL)&&(will_exit)){//finished commands and exit given at some point
				printf("Exiting\n");
				//flushing memory
				free(clean_cmd_arr);
				free(buffer);

				exit(0); //just a random # here
			}
		}
		//after all commands executed

		if(p<=0){ 
			break; //break out of while-loop for child
		}

		if(mode=='p'){ //pathetic attempt at sequential/parallel
			int nstatus = 0;
			waitpid(0,&nstatus,0);
		}
		free(clean_cmd_arr);

		printf("%s", prompt);
		fflush(stdout);
		//printf("TESTING\n");
    }
	if(p>0){ //only free buffer in parent at end
		free(buffer);
	}

    return 0;
}

