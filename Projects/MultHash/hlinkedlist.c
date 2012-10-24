//From lab01
//Declared extern mutex variablein main.c

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "hlinkedlist.h"

//threading taken care of at level of hashtable buckets

/*Inserts a new node with a string value at the beginning of the bucket's linked list. Updates head to point to new*/
void insert(const char *value, struct node **head) {
    struct node *newnode = malloc(sizeof(struct node));
	newnode->prev = NULL;
	newnode->value = strdup(value);	
	newnode->next = *head;
	if(*head != NULL) {
		(*head)->prev = newnode;
	}

	*head = newnode;
}

/*Wipes out the entire list after curnode*/
void clear_list(struct node *curnode) {
	struct node *tmp;
	while (curnode != NULL) {
		tmp = curnode;
		curnode = curnode->next;
		free(tmp->value);
		free(tmp);
	}
}

/*Removes a node from the ll starting at head*/
void killNode(struct node *curnode, struct node **head) {
	struct node *after = curnode->next;
	struct node *before = curnode->prev; //for ease of reading

	if(*head == curnode) { //curnode is head
		if(after != NULL){	 //head is not alone		
			after->prev = NULL;
			*head = after;
		}else{
			*head = NULL; //entire list is empty now
		}
	}else if(after != NULL){ //curnode is in middle somewhere
		before->next = after;
		after->prev = before;
	}else{ //tail of list
		before->next=NULL;
	}
	free(curnode->value);
	free(curnode);
}
