#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "list.h"

struct thread_args {
    list_t *list;
    int values_to_add;
};

void *worker_thread(void *v) {
    fprintf(stderr,"Thread 0x%0lx started.\n", (long)pthread_self());
    
    struct thread_args *targs = (struct thread_args*)v;
    const int value_range = 1000;

    int i = 0;
    for (i = 0; i < targs->values_to_add; i++) {
        // add a random value between 0 and value_range-1 to the list
        int value = random() % value_range;
        list_add(targs->list, value);
    }
    
    // sleep a random amount of time (up to 1 millisecond)
    usleep(random() % 1000);
    
    for (i = 0; i < targs->values_to_add; i++) {
        // try to remove a random value between 0 and value_range-1 from the list        
        int value = random() % value_range;
        list_remove(targs->list, value);
    }
    
    fprintf(stderr,"Thread 0x%0lx done.\n", (long)pthread_self());    
    return NULL;
}


void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-h] [-t threads] [-m max_values_to_add]\n", progname);
    fprintf(stderr, "\t-m: num of values for each thread to add to list (default 10)\n");
    fprintf(stderr, "\t-t: number of threads to start up (default 1)\n");        
    fprintf(stderr, "\t-h: show this help\n");
    exit(0);
}

int main(int argc, char **argv) {
    int num_threads = 1;
    int values_to_add = 10;
    
    int c;
    while ((c = getopt(argc, argv, "t:m:h")) != -1) {
        switch(c) {
            case 't':
                num_threads = atoi(optarg);
                if (num_threads < 1 || num_threads > 100) {
                    usage(argv[0]);
                }
                break;
            case 'm':
                values_to_add = atoi(optarg);
                if (values_to_add < 1) {
                    usage(argv[0]);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }

    // see the RNG
    srandom(42);
    
    list_t *thelist = (list_t*)malloc(sizeof(list_t));
    list_init(thelist);

    // set up thread arguments
    printf("Starting %d thread%s\n", num_threads, num_threads == 1 ? "." : "s.");
    printf("Each thread will add %d random values to the list\n", values_to_add/num_threads);
    struct thread_args targs = {thelist, values_to_add/num_threads};

    // here are our threads...
    pthread_t threads[num_threads];
    int i = 0;

    // start up the threads; they'll start adding to the hashtable
    // immediately.
    for (i = 0; i < num_threads; i++) {
        if (0 > pthread_create(&threads[i], NULL, worker_thread, (void*)&targs)) {
            fprintf(stderr, "Error creating thread: %s\n", strerror(errno));
        }
    }

    // wait for workers to complete
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // commented out by default...
    list_print(thelist, stdout);

    list_clear(thelist);
    free(thelist);

    exit(0);
}
