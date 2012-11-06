//From lab01
//all thread protection done at higher level


/*
Adriana and Curt
We did all of this together, essenitally, modifying the code from the CMAS shell.
*/

#include <stdlib.h>
#include <string.h>
#include "wlinkedlist.h"

//threading taken care of at level of hashtable buckets

/*Inserts a new node with a string value at the end of the linked list. Updates tail to point to new; otherwise assigns both head and tail to newnode, if list was empty*/
void insert(int socket, struct node **head, struct node **tail) {
    struct node *newnode = malloc(sizeof(struct node));
	newnode->socket = socket;
	newnode->next = NULL;

	if(*head == NULL) { //if list empty, this is both head and tail
		*head = newnode;
	}else{ //list not empty, tail exists as a node by construction
		(*tail)->next = newnode; //adding new nodes at tail if list not empty
	}

	*tail = newnode;
}

/*Wipes out the entire list after curnode*//*
void clear_list(struct node *curnode) { //not sure if we need this function
	struct node *tmp;
	while (curnode != NULL) {
		tmp = curnode;
		curnode = curnode->next;
		shutdown(tmp->socket, 2); //closing socket, stopping both output and input
		free(tmp);
	}
}*/

/*Removes a node from the ll starting at head and terminating at tail*/
void killHead(struct node **head_ptr, struct node **tail_ptr){
	struct node *after = (*head_ptr)->next; //for ease of reading

	if(after != NULL){ //head is not alone	
		*head_ptr = after;
	}else{
		*head_ptr = NULL; //entire list is empty now
		*tail_ptr = NULL;
	}

	//shutdown((*head_ptr)->socket, 2); //closing socket, stopping both output and input
	free(*head_ptr);
}
