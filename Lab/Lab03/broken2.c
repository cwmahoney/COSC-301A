#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    char name[128];
    struct node *next; 
};

void list_insert_ordered(char *name,  struct node **head) {
    struct node *newnode = malloc(sizeof(struct node));
    strncpy(newnode->name, name, 127);
    
    if (name == NULL)
        return;
        
    if (*head == NULL) {
        *head = newnode;
    }
    
    if (strncasecmp(name, (*head)->name, 127) < 0) {
        *head = newnode;    
        newnode->next = *head;
    }

    struct node *curr = malloc(sizeof(head));
    curr = *head;

    while (curr->next != NULL && strncasecmp(name, curr->next->name, 127) < 0) {
        curr = curr->next;
    }
    
    curr->next = newnode;
}


void list_print(struct node *list) {
    int i = 0;
    printf("\n\nDumping list\n");
    while (list != NULL) {
        printf("%d: %s\n", i+1, list->name);
        i += 1;
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

        list_insert_ordered(buffer, &head);

        printf("Next string to add: ");
        fflush(stdout);
    }

    list_print(head);
    list_clear(head);

    return 0;
}
