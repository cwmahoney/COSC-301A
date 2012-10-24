/* Adriana Sperlea and Curtis Mahoney 10/26/2012
Purpose: creating a threadsafe hashtable.*/

#include "hash.h" //hash.h includes hlinkedlist.h
#include <pthread.h>

#define LOCK pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock

extern pthread_mutex_t cmas; //for locking down the whole table

// create a new hashtable; parameter is a size hint
hashtable_t *hashtable_new(int sizehint) {
	hashtable_t *hash = malloc(sizeof(hashtable_t));
	int i = 0;

	struct node *temp; // need to change this but sizeof was being stupid!!!
    LOCK(&cmas);
	hash->table	= malloc(sizeof(temp) * sizehint); //allocating array of size sizehint for the linkedlists
	for(;i<sizehint;i++){ //setting all to NULL initially
		hash->table[i]=NULL;
	}
	hash->mut_table = malloc(sizeof(pthread_mutex_t) * sizehint); //allowing for sizehint mutexes

	hash->size = sizehint; // storing the size of the array
	UNLOCK(&cmas);
	return hash;
}

// free anything allocated by the hashtable library
void hashtable_free(hashtable_t *hashtable) {
	int i = 0;
	LOCK(&cmas); //lock down entire hashtable
	for (; i < hashtable->size; i++) {
		if(hashtable->table[i]!=NULL){
			pthread_mutex_destroy (&(hashtable->mut_table[i])); //destroy mutex if initialized, doesn't do anything otherwise
		}
		clear_list(hashtable->table[i]);
	}
	free(hashtable->table);
	free(hashtable->mut_table);
	free(hashtable);
	UNLOCK(&cmas);
}

/*calculate proper bucket*/
int hash_sum(const char *s, int size) //helper function
{
	int i = 0, sum = 0;
	for (; i < strlen(s); i++) {
		sum += s[i];
	    sum %= size;	
	}
	return sum;
}

// add a new string to the hashtable
void hashtable_add(hashtable_t *hashtable, const char *s) {
	//the hashing method we are using is summing the ASCII codes of the string
	//adding them together and then moding by the number of buckets

	int index = hash_sum(s, hashtable->size);
	if(hashtable->table[index]==NULL){
		pthread_mutex_init(&(hashtable->mut_table[index]),NULL); //initialize mutex for that linked list if first entry
	}
	LOCK(&(hashtable->mut_table[index]));	
	insert(s, &hashtable->table[index]);
	UNLOCK(&(hashtable->mut_table[index]));
}

// remove a string from the hashtable; if the string
// doesn't exist in the hashtable, do nothing
void hashtable_remove(hashtable_t *hashtable, const char *s) {
	int index = hash_sum(s, hashtable->size);

	if(hashtable->table[index]!=NULL){ // if its bucket exists
		LOCK(&(hashtable->mut_table[index]));

		struct node *curnode = hashtable->table[index];
		while (curnode != NULL) { //iterate through its linked list
			if (strcmp(curnode->value, s) == 0) {
				killNode(curnode, &hashtable->table[index]); // if found delete
				break;
			}
			curnode = curnode->next;
		}
		UNLOCK(&(hashtable->mut_table[index]));

		if(hashtable->table[index]==NULL){ //empty bucket, at some point down the road even if threading gets frisky. A dealy is okay, though.
			pthread_mutex_destroy(&(hashtable->mut_table[index]));
		}
	}
	//if bucket doesn't exist, do nothing
}

// print the contents of the hashtable
void hashtable_print(hashtable_t *hashtable) {
	int i = 0;
	LOCK(&cmas); //lockdown entire table
	struct node *curnode;

	printf("Printing the contents of the hashtable:\n");
	for (; i < hashtable->size; i++) {
		curnode = hashtable->table[i];

		if (curnode != NULL) {
			printf("Bucket %d: ", i);
			while (curnode != NULL) {
				printf("%s <--> ", curnode->value);
				curnode = curnode->next;
			}
			printf("\n");
		}
	}
	UNLOCK(&cmas);
}


