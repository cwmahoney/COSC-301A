#include <pthread.h>
#include "list.h"

/* ************************************** 
 *
 * ************************************** */
pthread_mutex_t happy_mutex;

void list_init(list_t *list) {
	pthread_mutex_init(&happy_mutex, NULL);
	list->head = NULL;
}


/* ************************************** 
 * print the contents of the list to file f.
 * ************************************** */
void list_print(list_t *list, FILE *f) {
    printf("*** List Contents Begin ***\n");
	pthread_mutex_lock(&happy_mutex);
    struct __list_node *tmp = list->head;
    while (tmp) {
        fprintf(f, "%d\n", tmp->data);
        tmp = tmp->next;
    }
	pthread_mutex_unlock(&happy_mutex);
    printf("*** List Contents End ***\n");
}


/* ************************************** 
 * add item "val" to the list, in order.
 * ************************************** */
void list_add(list_t *list, int val) {
    struct __list_node *new_node = (struct __list_node *)malloc (sizeof(struct __list_node));
    if (!new_node) {
        fprintf(stderr, "No memory while attempting to create a new list node!\n");
        abort();
    }

    new_node->data = val;
    new_node->next = NULL;

	pthread_mutex_lock(&happy_mutex);
    struct __list_node *tmp = list->head;

    /* special case: list is currently empty */
    if (list->head == NULL) {
        list->head = new_node;
    } else if (val <= list->head->data) {
        new_node->next = tmp;
        list->head = new_node;
    } else {
        int added = 0;
        while (tmp->next) {
            if (val >= tmp->data && val <= tmp->next->data) {
                new_node->next = tmp->next;
                tmp->next = new_node;
                added = 1;
                break;
            }
            tmp = tmp->next; 
        }

        if (!added) {
            tmp->next = new_node;
        }
    }
	pthread_mutex_unlock(&happy_mutex);
}


/* ************************************** 
 * remove all items equal to "target" from 
 * the list. return the number of items
 * removed.
 * ************************************** */
int list_remove(list_t *list, int target) {
    int removed = 0;
	pthread_mutex_lock(&happy_mutex);
    /* short cut: is the list empty? */
    if (list->head == NULL)
        return removed;

    /* check for removing items at the head */
    struct __list_node *dead = NULL;
    struct __list_node *tmp = list->head;
    while (tmp != NULL && tmp->data == target) {
        dead = tmp;
        tmp = tmp->next;
        free(dead);
        removed += 1;
    }
    list->head = tmp;

    /* if we removed anything or the data at the head is greater than
       the target, we're done (since the list is sorted */
    if (removed > 0 || target < tmp->data) {
		pthread_mutex_unlock(&happy_mutex);
        return removed;
    }

    /* find the target to destroy (if it exists). 
       keep track of previous node using dead.  */
    dead = tmp;
    while (dead != NULL && dead->data != target) {
        tmp = dead;
        dead = dead->next;
    }

    /* if there's something to destroy... */
    if (dead != NULL) { 

        // dead is node we want to remove, tmp is
        // node previous to dead
        while (dead != NULL && dead->data == target) {
            tmp->next = dead->next;
            free(dead);
            dead = tmp->next;    
            removed += 1;
        }
    }
	pthread_mutex_unlock(&happy_mutex);
    return removed;
}


/* ************************************** 
 * clear out the entire list, freeing all
 * elements.
 * ************************************** */
void list_clear(list_t *list) {
	pthread_mutex_lock(&happy_mutex);
    struct __list_node *tmp = list->head;
    while (tmp) {
        struct __list_node *tmp2 = tmp->next;
        free(tmp);
        tmp = tmp2;
    }
    list->head = NULL;
	pthread_mutex_unlock(&happy_mutex);
}

