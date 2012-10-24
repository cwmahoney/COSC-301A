/* Adriana Sperlea and Curtis Mahoney 10/26/2012
Purpose: creating a threadsafe hashtable.*/

#include "hash.h" //hash.h includes hlinkedlist.h
#include <pthread.h>

#define LOCK pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock

extern pthread_mutex_t cmas;

// create a new hashtable; parameter is a size hint
hashtable_t *hashtable_new(int sizehint) {
	hashtable_t *hash = malloc(sizeof(hashtable_t));
	struct node *temp; // need to change this but sizeof was being stupid!!!
    hash->table	= malloc(sizeof(temp) * sizehint); //allocating array of size sizehint for the linkedlists
	hash->size = sizehint; // storing the size of the array
	return hash;
}

// free anything allocated by the hashtable library
void hashtable_free(hashtable_t *hashtable) {
	
}

// add a new string to the hashtable
void hashtable_add(hashtable_t *hashtable, const char *s) {

}

// remove a string from the hashtable; if the string
// doesn't exist in the hashtable, do nothing
void hashtable_remove(hashtable_t *hashtable, const char *s) {

}

// print the contents of the hashtable
void hashtable_print(hashtable_t *hashtable) {

}


