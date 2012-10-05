/*
 * project 1 (shell) main.c template 
 *
 * Curtis Mahoney, Adriana Sperlea
 * -Curt worked on the backbone code, borrowing Adriana's functioning tokenify and implementing all Stage 1 functionality besides implementation of CPU time.
 *
 */

//Favorite test case: mode p;/bin/ls;;;asfdasdf  ;;asdf  ;; sfd; /sdf.' ;/bin/sleep 5;/bin/echo done;mode s;/bin/sleep 5;/bin/ls;exit

//Still need to clean up finished processes, do polling

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

// Linked list implementation
struct node {
    pid_t proc;
	char *cmd;
	int run;
    struct node *next; 
	struct node *last; //need it for removal without stupidity
};

void insert(char *cmd, pid_t proc_id, struct node **head) {
    struct node *newnode = malloc(sizeof(struct node));
	newnode->last = NULL;
	newnode->run = 1; //starts off running
    newnode->next = *head;
	if(*head!=NULL){
		(*head)->last = newnode;
	}

	newnode->proc = proc_id;
	newnode->cmd = strdup(cmd);
    *head = newnode;
}

void clear_list(struct node *curnode) {
	struct node *tmp;
	while (curnode != NULL) {
		tmp = curnode;
		curnode = curnode->next;
		free(tmp->cmd);
		free(tmp);
	}
}

void killNode(struct node *curnode, struct node **head){ //Curt's --still no good - mode p \n ls;ls;sleep 2;ls;mode s; ls
	struct node *tmp = curnode->last;

	if(*head==curnode){ //curnode is head
		tmp=curnode->next;
		if(tmp!=NULL){	 //head is not alone		
			tmp->last=NULL;
			*head=tmp;
		}else{
			*head = NULL; //entire list is empty now
		}
	}else if(curnode->next!=NULL){ //curnode is in middle somewhere
		tmp->next = curnode->next;
		curnode->next = curnode->last;
	}else{ //tail of list
		tmp = curnode->last;
		tmp->next=NULL;
	}
	free(curnode->cmd);
	free(curnode);
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

/*Builds an array of all the valid paths to search, manually input by programemr*/
char **buildPaths(){
	char **paths = malloc(sizeof(char *)*8); //seven paths to test, then NULL

	paths[0] = strdup("/bin/");
	paths[1] = strdup("/usr/sbin/");
	paths[2] = strdup("/sbin/");
	paths[3] = strdup("/usr/local/bin/");
	paths[4] = strdup("./");
	paths[5] = strdup("/usr/games/");
	paths[6] = strdup("/usr/bin/");
	paths[7] = NULL;

	return paths; //7 malloced strings and 1 malloced string of strings
	
}

int testCmdReal(char **instr,char **paths, int max_len){
	char temp_c[max_len*2]; //covers all string lens possible
	int i = 0;

	struct stat statresult;
	int rv;

	rv = stat(*instr, &statresult );
	if(rv>=0){
		return 1; //original command was valid
	}

	for(;i<arrLen(paths)-1;i++){
		strcpy(temp_c,paths[i]);
		strcat(temp_c,*instr);	
		//printf("TESTING _%s_\n",temp_c);
		rv = stat(temp_c, &statresult );
		if(rv>=0){ //stat succeeded
			free(*instr);
			*instr=strdup(temp_c);
			return 1; //success
		}
	}
	return 0; //no valid path found
}

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

	struct node *kids = NULL; //for tracking running processes
	char **paths = buildPaths();
	struct node *copy; //start at head of list
	struct node *tmp;

	struct node *deadkids = NULL; //for tracking completed processes that haven't been output yet

	int stage2 = 1; //"bool" for Stage 2, not waiting for parallels to finish before prompting

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

		i=0;
		char **cmd;
		char temp_m;
		int j;
		_Bool will_exit = 0;
		char *temp_s; //to store commands for output prettyness
		pid_t p;
		for(;clean_cmd_arr[i]!=NULL;i++)  //i < length of clean_cmd_array
		{
			cmd = clean_cmd_arr[i];
			printf("Command: _%s_\n",cmd[0]);
				

			if(cmd[0]==NULL){
				printf("Execution failed, no input\n");
			}else if(0==strcmp(cmd[0],"exit")){ //exit command given
				if(1==stage2){
					if(kids!=NULL){
						printf("Cannot EXIT, jobs running in background\n");
					}
					else{
						will_exit = 1;
					}
				}else{
					will_exit = 1; //will exit later in stage 1
				}
			}else if(0==strcmp(cmd[0],"jobs")){
				printProcs(kids);
			}else if((0==strcmp(cmd[0],"pause"))&&(cmd[0]!=NULL)){
				int pid = strtol(cmd[1],NULL,10);

				if(0==kill(strtol(cmd[1],NULL,10), SIGSTOP)){ //cast as pointer, 0 means success
					copy = kids;
					while(copy!=NULL){
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
					temp_s=malloc(sizeof(char)*buffer_len);
					strcpy(temp_s,cmd[0]);

					j = 1;
					while(cmd[j]!=NULL){ //one unbroken sentence rather than a set of strings
						strcat(temp_s," ");
						strcat(temp_s,cmd[j]);
						j++;
					}
					if(0==testCmdReal(&cmd[0],paths,buffer_len)){ //stat failed, no file
						printf("Execution of %s failed, invalid file\n",temp_s);
					}else
					{
						p = execCmd(cmd, mode); //p>0 if a child is still alive at the end of execCmd
						if(p>0){ //only add living (to our knowledge) children to the linked list
							insert(temp_s,p, &kids); //temp_s stores the whole command line, including modifiers
						}				
					}
					free(temp_s);
				}
			}

			j=0;
			for(;j<arrLen(clean_cmd_arr[i]);j++){
				free(clean_cmd_arr[i][j]);
			}
			free(clean_cmd_arr[i]);
		}
		//after all commands executed

		if(stage2==0){	//Stage 1 functionality
			copy = kids; //start at head of list
			while (copy != NULL){
				waitpid(copy->proc, NULL, 0);
				tmp=copy;
				copy = copy->next;
				killNode(tmp,&kids); //removes node of dead child
			}
		}
		free(clean_cmd_arr);

		if(will_exit){//finished commands and exit given at some point
			if(kids!=NULL){ //in stage 2 and kids were created after an exit command, on the same input line
				printf("Jobs started after EXIT command, EXIT failed\n");
			}
			else{
				struct rusage usage_self, usage_children;
				getrusage(RUSAGE_SELF, &usage_self);
				getrusage(RUSAGE_CHILDREN, &usage_children);
				printf("%ld.%06ld seconds spent in user mode\n", usage_self.ru_utime.tv_sec + usage_children.ru_utime.tv_sec, usage_self.ru_utime.tv_usec + usage_children.ru_utime.tv_usec);
				printf("%ld.%06ld seconds spent in kernel mode\n", usage_self.ru_stime.tv_sec + usage_children.ru_stime.tv_sec, usage_self.ru_stime.tv_usec + usage_children.ru_stime.tv_usec);
				break; //leave while-loop
			}
		}

		if(stage2==1){ //S2, printing processes that recently finished.
			copy = deadkids;
			while(copy!=NULL){
				printf("ARGHSADKFJAS LProcess %d (%s) completed\n",copy->proc,copy->cmd);
				tmp=copy;
				copy=copy->next;
				killNode(tmp,&deadkids);
			}
		}
		printf("%s", prompt);
		fflush(stdout);
		//printf("TESTING\n");
    }
	clear_list(kids); //only really needed for EOF where we don't wait for stuff to finish in a polite manner
	int k = 0;

	for(;k<arrLen(paths);k++){ //clearing path array
		free(paths[k]);
	}
	free(paths);
	free(buffer);

    return 0;
}

