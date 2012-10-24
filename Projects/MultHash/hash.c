/* Adriana Sperlea and Curtis Mahoney 10/26/2012
Purpose: creating a threadsafe hashtable.

Did most of it together with single-layer thread protection, Curt modified the thread-protection to cover buckets instead of the entire hash, Adriana double checked and found the source of the now-infamous "conditional jump" error (*cough cough*).

Adriana then optimized the hash by splitting 4 chars at a time into their bytes, suming them as unsigned longs
http://research.cs.vt.edu/AVresearch/hashing/strings.php
and made the size of the array be the nearest largest prime number to the hint.

Curt then debuggedt the afforementioned optimized hash table allocation scheme

*/

#include "hash.h" //hash.h includes hlinkedlist.h
#include <pthread.h>

#define LOCK pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock

#define abs(a) a > 0 ? a : -a

//extern pthread_mutex_t wholehash; //for locking down the whole table
//extern pthread_mutex_t hashmurder; //for when table is being wiped

// function testing if a number is prime by testing all its divisors with a few optimizations
int isPrime(int x)
{
	if (x < 2) { // there are no prime numbers smaller than 2
		return 0;
	} else if (x == 2) { // 2 is the only even prime numbers
		return 1;
	} else if (x % 2 == 0) { // other even numbers are not prime
		return 0;
	} else {
		int i = 3; // only need to check odd divisors which are < sqrt(n)
		for (; i * i < x; i += 2) {
			if (x % i == 0) {
				return 0;
			}
		}
	}
	return 1; 
}

// function finding the nearest larger prime number
int nearestPrime(int x)
{
	while (!isPrime(x)) x++;
	return x;
}

// create a new hashtable; parameter is a size hint
hashtable_t *hashtable_new(int sizehint) { //don't need to worry about threading for this one	
	hashtable_t *hash = malloc(sizeof(hashtable_t));
	int i = 0;

	sizehint = nearestPrime(sizehint); // using the sizehint to find a prime number
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
void hashtable_free(hashtable_t *hashtable) { //assume that only one thread executes this, no locking/unlocking
	int i = 0;
	for (; i < hashtable->size; i++) {
		clear_list(hashtable->table[i]);
		pthread_mutex_destroy (&(hashtable->mut_table[i])); //destroy mutex
	}
	free(hashtable->table);
	free(hashtable->mut_table);
	free(hashtable);
}

/*calculate proper bucket*/
int hash_sum(const char *s, int size) //helper function
{
	// Using hashing function found at http://research.cs.vt.edu/AVresearch/hashing/strings.php
	// The function will cause overflow but that is not a problem when hashing
	if(s==NULL){
		return -1; //Don't bother to store null pointers
	}
	int i = 0;
	unsigned long sum = 0;
	if(strlen(s)>=4){ //had a persistent problem with this for a bit :/
		for (; i < strlen(s) - 3; i += 4) {
			sum += s[i] * (1 << 24) + s[i + 1] * (1 << 16) + s[i + 2] * (1 << 8) + s[i + 3]; //creating one 32 bit integer out of 4 characters	
		}
		i = 0;
	}
	int len = strlen(s), mlt = 1;
	for (; i < (strlen(s) % 4); i ++) {
		sum += s[len - i] * mlt;
		mlt *= (1 << 8);
	} 
	return (int) (sum % size);
}

// add a new string to the hashtable
void hashtable_add(hashtable_t *hashtable, const char *s) {
	//the hashing method we are using is summing the ASCII codes of the string
	//adding them together and then moding by the number of buckets

	int index = hash_sum(s, hashtable->size);
	
	if(index!=-1){ //actual string inputted. Ignore null pointers
		LOCK(&(hashtable->mut_table[index])); //lock bucket
		insert(s, &hashtable->table[index]);
		UNLOCK(&(hashtable->mut_table[index]));
		//Could save a few instructions in the ll by passing the mutex, but it would murder readability for very little gain.
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
		fflush(stdout); //just to be sure it's all printed
		UNLOCK(&(hashtable->mut_table[i]));
	}
}


