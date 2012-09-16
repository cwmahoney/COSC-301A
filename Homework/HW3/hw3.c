#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
	char name[128];
	struct node *next; 
};

/*
Assuming that the linked list is ordered up to this point in time, insert a new node with name variable "newname" into the list so as to preserve a lexigraphical order going from smallest at the head of the list to largest at the tail, from A->Z, for example.
*/
void list_insert_ordered(char *newname,  struct node **head) {
	struct node *newnode=malloc(sizeof(struct node)); //Declaring new node
	strncpy(newnode->name,newname,127);
	struct node *temp=*head; //start at head and march through list linearly. Can't binary search with linked lists.

	if((temp==NULL)||(strcasecmp(temp->name,newname)>=0)){ //No values in the list or the new node is lexigraphically smaller than or equal to the head
		newnode->next = *head;
		*head = newnode; //Place newnode as head of list
		return;
	}

	while(temp->next!=NULL){ //Keep testing whether next node in line is greater than newnode, keep going if it's not. Stop at end of list.
		if(strcasecmp(temp->next->name, newname)>=0){ //(next in line) >= newname, lexigraphically
			struct node *t2 = temp->next;
			temp->next=newnode;
			newnode->next=t2; //Insert newnode after temp, with newnode->next being what temp used to point to.
			return;
		}
		else{ //next value is smaller than newnode's, roll on
			temp=temp->next;
		}
	}
	temp->next=newnode;
	newnode->next = NULL; //Inserting node at the end of the linked list; newnode is lexigraphically greater than everything else
}

void list_insert(char *name, struct node **head) {
	struct node *newnode = malloc(sizeof(struct node));
	strncpy(newnode->name, name, 127);

	newnode->next = *head;
	*head = newnode;
}

void list_dump(struct node *list) {
	int i = 0;
	printf("\n\nDumping list\n");
	while (list != NULL) {
		printf("%d: %s\n", i++, list->name); //You're original "i+1" didn't help much b/c it was a while loop, not a for
		list = list->next;
	}
}

void list_clear(struct node *list) {
	while (list != NULL) {
		struct node *tmp = list;
		list = list->next;
		free(tmp);
	}
}

int main(int argc, char **argv)
{
	char buffer[128];

	struct node *head = NULL;

	printf("Next string to add: ");
	fflush(stdout);
	while (fgets(buffer, 128, stdin) != NULL) {

		// old call to list_insert
		// list_insert(buffer, &head);

		list_insert_ordered(buffer, &head);

		printf("Next string to add: ");
		fflush(stdout);
	}

	list_dump(head); //No "list_print" exists
	list_clear(head);

	return 0;
}
