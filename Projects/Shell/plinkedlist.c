//From lab01

#include <stdlib.h>
#include <string.h>

// Linked list implementation
struct node {
    pid_t proc;
	char *cmd;
	int run;
    struct node *next; 
	struct node *last; //need it for removal of arbitrary nodes without O(n) cost
};

/*Inserts a new node with a command, a process id, and the address of the pointer to its head. Updates head to point to new*/
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

/*Wipes out the entire list after curnode*/
void clear_list(struct node *curnode) {
	struct node *tmp;
	while (curnode != NULL){
		tmp = curnode;
		curnode = curnode->next;
		free(tmp->cmd);
		free(tmp);
	}
}

/*removes a node from the ll starting at head*/
void killNode(struct node *curnode, struct node **head){ //Curt's
	struct node *after = curnode->next;
	struct node *before = curnode->last; //for ease of reading

	if(*head==curnode){ //curnode is head
		if(after!=NULL){	 //head is not alone		
			after->last=NULL;
			*head=after;
		}else{
			*head = NULL; //entire list is empty now
		}
	}else if(after!=NULL){ //curnode is in middle somewhere
		before->next = after;
		after->last = before;
	}else{ //tail of list
		before->next=NULL;
	}
	free(curnode->cmd);
	free(curnode);
}
