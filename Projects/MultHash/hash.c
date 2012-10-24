/* Adriana Sperlea and Curtis Mahoney 10/26/2012
Purpose: creating a threadsafe hashtable.*/

#include "hash.h" //hash.h includes hlinkedlist.h
#include <pthread.h>

#define LOCK pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock

//extern pthread_mutex_t wholehash; //for locking down the whole table
//extern pthread_mutex_t hashmurder; //for when table is being wiped

// create a new hashtable; parameter is a size hint
hashtable_t *hashtable_new(int sizehint) { //don't need to worry about threading for this one	
	hashtable_t *hash = malloc(sizeof(hashtable_t));
	int i = 0;

	struct node *temp; // need to change this but sizeof was being stupid!!!
	hash->table	= malloc(sizeof(temp) * sizehint); //allocating array of size sizehint for the linkedlists
	hash->mut_table = malloc(sizeof(pthread_mutex_t) * sizehint); //allowing for sizehint mutexes

	for(;i<sizehint;i++){ //setting all to NULL initially
		hash->table[i]=NULL;
		pthread_mutex_init(&(hash->mut_table[i]),NULL);
	}

	hash->size = sizehint; // storing the size of the array
	return hash;
}

// free anything allocated by the hashtable library
void hashtable_free(hashtable_t *hashtable) {
	int i = 0;
	for (; i < hashtable->size; i++) {
		LOCK(&(hashtable->mut_table[i])); //Lock and unlock each ll in sequence, then destroy their mutex
		clear_list(hashtable->table[i]);
		UNLOCK(&(hashtable->mut_table[i]));

		pthread_mutex_destroy (&(hashtable->mut_table[i])); //destroy mutex
	}
	free(hashtable->table);
	free(hashtable->mut_table);
	free(hashtable);
}

/*calculate proper bucket*/
int hash_sum(const char *s, int size) //helper function
{
	if(s==NULL){ //looking out for weirdness
		return -1; //Don't bother to store nulls
	}
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

	if(index!=-1){ //actual string inputted
		LOCK(&(hashtable->mut_table[index])); //lock bucket
		insert(s, &hashtable->table[index]);
		UNLOCK(&(hashtable->mut_table[index]));
	}
}

// remove a string from the hashtable; if the string
// doesn't exist in the hashtable, do nothing
void hashtable_remove(hashtable_t *hashtable, const char *s) {
	int index = hash_sum(s, hashtable->size);

	if(index!=-1){	//valid index, s was a good string
		LOCK(&(hashtable->mut_table[index]));

		if(hashtable->table[index]!=NULL){ // if its bucket exists
			struct node *curnode = hashtable->table[index];
			while (curnode != NULL) { //iterate through its linked list
				if (strcmp(curnode->value, s) == 0) {
					killNode(curnode, &hashtable->table[index]); // if found delete
					break;
				}
				curnode = curnode->next;
			}
		}
	
		UNLOCK(&(hashtable->mut_table[index]));
	}
}

// print the contents of the hashtable
void hashtable_print(hashtable_t *hashtable) {
	int i = 0;
	struct node *curnode;

	printf("Printing the contents of the hashtable:\n");
	for (; i < hashtable->size; i++) {
		LOCK(&(hashtable->mut_table[i])); //lock ll we're on
		curnode = hashtable->table[i];

		if (curnode != NULL) {
			printf("Bucket %d: ", i);
			while (curnode != NULL) {
				printf("%s", curnode->value);
				curnode = curnode->next;
				if(curnode!=NULL){
					printf(" <--> ");
				}
			}
			printf("\n");
		}
		UNLOCK(&(hashtable->mut_table[i]));
	}
}


