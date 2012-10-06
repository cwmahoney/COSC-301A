/*
 * project 1 (shell) main.c
 *
 * 10/07/2012
 * Purpose: Implement a shell.
 *
 * Authors: Curtis Mahoney, Adriana Sperlea
 * -Curt worked on the backbone code, borrowing Adriana's functioning tokenify and implementing all Stage 1 functionality besides implementation of CPU time and a basic linked list.
 * - Curt also implemented all Stage 2 functionality, including adding extra parameters and functions for linked lists
 *
 */

//Favorite test case: mode p;/bin/ls;;;asfdasdf  ;;asdf  ;; sfd; /sdf.' ;/bin/sleep 5;/bin/echo done;mode s;/bin/sleep 5;/bin/ls;exit

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

#include "tokenify.h" //helper functions built by us
#include "plinkedlist.h"

/*Turns the first comment character '#' into and end string char*/
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

/*Return length of an array ending with a NULL in it's last index, for char ** (arrLen) and char *** (arrLen2)*/
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

/*Returns malloced array of strings formatted {<command>,<arg0>, <arg1>,...,NULL} if a valid input or just NULL otherwise*/
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

/*Executes a single command cmd*/
pid_t execCmd(char **cmd, char mode){
	pid_t p = fork();
    if (p == 0){
        /* in child */
		//printf("Running Child #\n");		
        if (execv(cmd[0], cmd) < 0) {
            fprintf(stderr, "Command \"%s\" failed: %s\n", cmd[0], strerror(errno));
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
		return -1;
    }
	return p;
}


/*Checks command array for a mode switch order, the returns mode to switch to/stay in (if mode order exists) or 'n' otherwise, indicating that no mode command is in the cmd*/
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
			if(mode=='s'){ //s means sequential
				printf("Switched to PARALLEL mode\n");
				return('p');
			}
			else{
				printf("Already in PARALLEL mode\n");
			}
		}else if((0==strcmp(cmd[1],"s"))||(0==strcmp(cmd[1],"sequential"))){
			if(mode=='p'){ //p means parallel
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

/*Builds an array of all the valid paths to search, manually input by programemr*/
struct node *buildPaths(){
	struct node *paths=NULL;
	//file reading from the internet
	
	FILE *fr;           //declar file printer
	char line[80]; //read up to 79 characters of a path
	char temp_s[81];

	fr = fopen ("shell-config", "rt");  //open for reading

	while(fgets(line, 80, fr) != NULL) //80 char line, done with NULL
	{
		line[strcspn(line, "\n")] = '\0'; //get rid of newline
		strcpy(temp_s,line);
		strcat(temp_s,"/"); //throw on extra slash for ease of use later

		//printf("PATH: %s\n",temp_s);

		insert(temp_s,0,&paths); //hijaking linked list, pid of 0
	}
	fclose(fr);  // close the file

	return paths; //pointer to the head of a list of "cmds" which are paths
	
}

/*See if there's a valid file at the end of some path, appends correct path to beginning of instr if it exists*/
int testCmdReal(char **instr,struct node *paths, int max_len){
	char temp_c[max_len*2]; //covers all string lens possible

	struct stat statresult;
	int rv;

	rv = stat(*instr, &statresult );
	if(rv>=0){
		return 1; //original command was valid
	}

	struct node *copy = paths;

	while(copy!=NULL){
		strcpy(temp_c,copy->cmd);
		strcat(temp_c,*instr);	
		//printf("TESTING _%s_\n",temp_c);
		rv = stat(temp_c, &statresult );
		if(rv>=0){ //stat succeeded
			free(*instr);
			*instr=strdup(temp_c);
			return 1; //success
		}
		copy=copy->next;
	}
	return 0; //no valid path found
}

/*Print off all running processes*/
void printProcs(struct node *head){
	struct node *copy = head;
	char *running;
	while(copy!=NULL){
		if(1==copy->run){
			running = "Running";
		}else{
			running = "Paused";
		}
		printf("PID: %d\tCommand: %s\tState: %s\n",copy->proc,copy->cmd,running);
		copy=copy->next;
	}
}

int main(int argc, char **argv){
    char *prompt = "CMAS> ";
    printf("%s", prompt);
    fflush(stdout);

	const int buffer_len=1024;
    char *buffer=malloc(sizeof(char *)*buffer_len);
	buffer[0]='\0'; //initial value
	char **cmd_arr;
	char mode = 's'; //Mode "bit" - 's' means sequential
	int stage2 = 1; //"bool" for Stage 2 (1 if stage 2 active), not waiting for parallels to finish before prompting
	struct node *paths = buildPaths(); //paths to check if a file could be in

	//linked lists for active and dead processes
	struct node *kids = NULL; //for tracking running processes
	struct node *deadkids = NULL; //for tracking completed processes that haven't been output yet	

	//just declaring some temporary vars for later use
	struct node *copy;
	struct node *tmp;
	
    while (fgets(buffer, buffer_len, stdin) != NULL) { //works as an EOF checker, according to Prof.
        /* process current command line in buffer */

		killComments(buffer,buffer_len); //Buffer is always a string
		cmd_arr = tokenify(buffer,";"); //array of strings, everything is on the heap
		
		int i = 0;
		int clean_len = 0;
		char ***clean_cmd_arr=malloc(sizeof(char **)*arrLen(cmd_arr));
		char **temp_c;

		//printf("Length of cmd_arr: %d\n",arrLen(cmd_arr));

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
		
		//printf("cca len: %d\n",arrLen2(clean_cmd_arr));
        
        //char *cmd[] = { "/bin/ls", "-l","-t", "-r", ".", NULL };

		//temporary variables. Curt don't like declaring new vars several times
		i=0;
		char **cmd;
		char temp_m;
		int j;
		_Bool will_exit = 0; //if on, will try to exit at the end of the instructions
		char temp_s[buffer_len]; //to store commands for output prettyness
		pid_t p;
		for(;clean_cmd_arr[i]!=NULL;i++)  //i < length of clean_cmd_array
		{
			cmd = clean_cmd_arr[i];
			//printf("Command: _%s_\n",cmd[0]); //testing				

			if(cmd[0]==NULL){ //no string, I don't think this can happen, but better safe
				printf("Execution failed, no input\n");
			}else if(0==strcmp(cmd[0],"exit")){ //exit command given
				if(1==stage2){
					if(kids!=NULL){
						printf("Cannot EXIT, jobs running in background\n");
					}
					else{
						will_exit = 1; //will try to exit later in stage2
					}
				}else{
					will_exit = 1; //will exit later in stage 1
				}
			}else if(0==strcmp(cmd[0],"jobs")){ //print out running (to our knowledge) jobs
				printProcs(kids);
			}else if((0==strcmp(cmd[0],"pause"))&&(cmd[0]!=NULL)){ //pausing and resuming processes
				int pid = strtol(cmd[1],NULL,10);

				if(0==kill(strtol(cmd[1],NULL,10), SIGSTOP)){ //cast as pointer, 0 means success
					copy = kids;
					while(copy!=NULL){ //run through the whole list to find it, if necessary. O(n).
						if(copy->proc==pid){
							copy->run=0; //not running any more
							break;
						}
					}
				}else{
					printf("Pause failed: ");
					if(errno==EINVAL){
						printf("Invalid signal number"); //never be tripped, I don't think
					}else if(errno==EPERM){
						printf("Insuficient permissions");
					}else{ //ESRCH
						printf("Invalid PID");
					}
					printf("\n");
				}
			}else if((0==strcmp(cmd[0],"resume"))&&(cmd[1]!=NULL)){
				int pid = strtol(cmd[1],NULL,10);

				if(0==kill(pid, SIGCONT)){ //cast as pointer, 0 means success
					copy = kids;
					while(copy!=NULL){ //know that it's one of these guys
						if(copy->proc==pid){
							copy->run=1; //running again
							break;
						}
					}
				}else{
					printf("Resume failed: ");
					if(errno==EINVAL){
						printf("Invalid signal number"); //never be tripped, I don't think
					}else if(errno==EPERM){
						printf("Insuficient permissions");
					}else{ //ESRCH
						printf("Invalid PID");
					}
					printf("\n");
				}
			}else
			{ //mode checking
				temp_m = modeCheck(cmd,mode);
				if('n'!=temp_m){
					mode=temp_m;
				}else
				{
					strcpy(temp_s,cmd[0]); //wipe out previous value

					j = 1;
					while(cmd[j]!=NULL){ //one unbroken sentence rather than a set of strings
						strcat(temp_s," ");
						strcat(temp_s,cmd[j]);
						j++;
					}
					if(0==testCmdReal(&cmd[0],paths,buffer_len)){ //stat failed, no file
						printf("Execution of \"%s\" failed, invalid file\n",temp_s);
					}else
					{
						p = execCmd(cmd, mode); //p>0 if a child is still alive at the end of execCmd
						if(p>0){ //only add living (to our knowledge) children to the linked list
							insert(temp_s,p, &kids); //temp_s stores the whole command line, including modifiers
						}				
					}
				}
			}

			j=0;
			for(;j<arrLen(clean_cmd_arr[i]);j++){ //free all of cmd
				free(clean_cmd_arr[i][j]);
			}
			free(clean_cmd_arr[i]);
		}//after all commands executed		
		free(clean_cmd_arr);

		if(stage2==0){	//Stage 1 functionality
			copy = kids; //start at head of list
			while (copy != NULL){
				waitpid(copy->proc, NULL, 0);
				tmp=copy;
				copy = copy->next;
				killNode(tmp,&kids); //removes node of dead child
			}
		}


		if(will_exit){//finished commands and exit given at some point
			if(kids!=NULL){ //in stage 2 and kids were created after an exit command, on the same input line
				printf("Jobs started after EXIT command, EXIT failed\n");
			}
			else{
				struct rusage usage_self, usage_children; //print off CPU usage
				getrusage(RUSAGE_SELF, &usage_self);
				getrusage(RUSAGE_CHILDREN, &usage_children);
				printf("%ld.%06ld seconds spent in user mode\n", usage_self.ru_utime.tv_sec + usage_children.ru_utime.tv_sec, usage_self.ru_utime.tv_usec + usage_children.ru_utime.tv_usec);
				printf("%ld.%06ld seconds spent in kernel mode\n", usage_self.ru_stime.tv_sec + usage_children.ru_stime.tv_sec, usage_self.ru_stime.tv_usec + usage_children.ru_stime.tv_usec);

				break; //leave while-loop
			}
		}

		printf("%s", prompt); //only initial prompt for stage 2, the only prompt for stage 1
		fflush(stdout);

		if(stage2==1){ //in stage 2			
			while (1) //stolen from piazza
			{
				struct pollfd pfd = { 0, POLLIN }; // file descriptor is 0, I want to know when there are "IN" events pending
				int rv = poll(&pfd, 1, 250); // wait for an input event for 250 miliseconds (0.25 sec).
				if (rv == 0) {
					copy = kids;
					int rstatus;
					int pstatus;
					while(copy!=NULL){
						rstatus = 0;
						pstatus = waitpid(copy->proc,&rstatus,WNOHANG); //doesn't HANG around, just checks status

						tmp=copy;
						copy=copy->next;
						if(pstatus>0){ //information found on process
							if(WIFEXITED(rstatus)){ //process exited
								insert(tmp->cmd,tmp->proc,&deadkids); //put in dead kids linked list to be printed out later
								killNode(tmp,&kids); //remove exited processes from kids list
							}
						}
					}
					//printing processes that recently finished before we give the prompt
					tmp=NULL;
					copy = deadkids;
					while(copy!=NULL){
						printf("Process %d (%s) completed\n",copy->proc,copy->cmd);
						tmp=copy;
						copy=copy->next;
						killNode(tmp,&deadkids);
					}
					if(tmp!=NULL){ //at least one process finished, was reported on
						printf("%s", prompt); //re-prompt after printing out finished processes
						fflush(stdout);
					}
				} else if (rv < 0) {
					printf("Polling Error\n");
					printf("%s", prompt);
					fflush(stdout);
					break;
					// some kind of error happened with poll.  this is probably bad and we
					// probably want to break out of the while loop
				} else {
					break; //will lead to top-level while-loop being evaluated

					// user must have typed something (rv would be 1 in this case, since there's only 1
					// file descriptor we registered to detect events on)
					// time to call fgets to get what the user typed.
				}
			}
		}
    }
	//out of all loops, about to return
	clear_list(kids); //only really needed for EOF where we don't wait for stuff to finish in a polite manner
	clear_list(paths); //clears out paths

	free(buffer);

    return 0;
}

//ls;ls;ps;ps;exit;psadf;os;ls;sleep 10

