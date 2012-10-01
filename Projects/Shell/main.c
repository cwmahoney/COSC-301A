/*
 * project 1 (shell) main.c template 
 *
 * Curtis Mahoney, Adriana Sperlea
 * -Curt worked on the backbone code, borrowing Adriana's functioning tokenify and implementing all Stage 1 functionality besides implementation of CPU time.
 *
 */

//Favorite test case: /bin/ls;;;asfdasdf  ;;asdf  ;; sfd; /sdf.' ;/bin/sleep 5;/bin/echo done;mode s;/bin/sleep 5;/bin/ls;exit

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


/*Removes given characters*//*
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
*/
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

	char **tokened=tokenify(instr," \t\n"); //everthing in tokened is malloced

	return tokened;
}

/*
printf("Child freeing cmd[%d]: %s\n",0,cmd[0]);
int j = 0;
while(cmd[j]!=NULL){
	free(cmd[j++]); //freeing each individual string
}
printf("Child freeing cmd[%d]: %s\n",j,cmd[j]);
free(cmd);
exit(0);
*/

/*Executes a single command cmd*/
pid_t execCmd(char **cmd, char mode){
	pid_t p = fork();
    if (p == 0){
        /* in child */
		//printf("Running Child #\n");		
        if (execv(cmd[0], cmd) < 0) {
            fprintf(stderr, "execv failed: %s\n", strerror(errno));
			exit(0);
        }
    } else if (p > 0) {
        /* in parent */
		if(mode=='s'){ //only wait in sequential mode
		    int rstatus = 0;
		    waitpid(p,&rstatus,0); //will continue if child errors-out
			return 0;
		}
		//printf("Parent done\n");
    } else {
        /* fork had an error; bail out */
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
    }
	return p;
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

//Need to deal with string-literal input, parallel vs. sequential, user vs. kernal time, EOF-checkery
int main(int argc, char **argv){
    char *prompt = "CMAS> ";
    printf("%s", prompt);
    fflush(stdout);
	const int buffer_len=1024;
    char *buffer=malloc(sizeof(char *)*buffer_len);
	buffer[0]='\0'; //initial value
	char **cmd_arr;
	char mode = 's'; //Mode "bit." 's' means sequential

    while (fgets(buffer, buffer_len, stdin) != NULL) { //works as an EOF checker, according to Prof.
        /* process current command line in buffer */

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
		pid_t *kids = malloc(sizeof(pid_t)*(clean_len)); //num of cmds minus NULL
		for(;clean_cmd_arr[i]!=NULL;i++)  //i < length of clean_cmd_array
		{
			kids[i]=0;
			cmd = clean_cmd_arr[i];
			printf("Command: _%s_\n",cmd[0]);

			if(0==strcmp(cmd[0],"exit")){ //exit command given
				will_exit = 1; //will exit later
			}else
			{	 //not an "exit" command
				temp_m = modeCheck(cmd,mode);
				if('n'!=temp_m){
					mode=temp_m;
				}else //no "mode" starting cmd
				{
					kids[i] = execCmd(cmd, mode);
				}
			}
		}
		//after all commands executed
		
		i=0;
		for(;i<clean_len;i++){ //parallel processing, only works for Stage 1
			if(kids[i]>0){ //only checks for real kids, sequential parents return 0, errors are -1
				waitpid(kids[i],NULL,0);
			}
		}
		free(kids);

		/*
		if(mode=='p'){ //Should wait for all children if main program. Doesn't
			pid_t pid;
			//waits for all children
			while ((pid = waitpid(0, NULL, 0))) { //the internet is beautiful
				printf("STALLING\n");
				if (errno == ECHILD) {
					printf("Done Stalling\n");
					break;
				}
			}
		}//works if I switch mode as part of the command input
		*/

		//freeing everything at once		
		i = 0;
		while(clean_cmd_arr[i]!=NULL){
			j=0;
			for(;j<arrLen(clean_cmd_arr[i]);j++){
				free(clean_cmd_arr[i][j]);
			}
			free(clean_cmd_arr[i]);
			i++;
		}
		free(clean_cmd_arr);

		if(will_exit){//finished commands and exit given at some point
			break; //leave while loop
		}

		printf("%s", prompt);
		fflush(stdout);
		//printf("TESTING\n");
    }
	free(buffer);

    return 0;
}

