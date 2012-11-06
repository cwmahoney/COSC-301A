/*
Curt Mahoney and Adriana Sperlea
Purpose: Create the code base for a concurrent web server
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <semaphore.h>

#include "network.h"
#include "wlinkedlist.h"

#define LOCK pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock
#define WAIT pthread_cond_wait
#define SIGNAL pthread_cond_signal

pthread_mutex_t cmas = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t consumer = PTHREAD_COND_INITIALIZER;
sem_t threads;

// global variable; can't be avoided because
// of asynchronous signal interaction
int still_running = TRUE;
void signal_handler(int sig) {
    still_running = FALSE;
}


void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

void consume(struct node **head_ptr, struct node **tail_ptr){
	//Need condition variable incase list is empty
	LOCK(&cmas); //locked over all threads
	while(*head_ptr==NULL){
		WAIT(&consumer,&cmas);
	}
	int socket = (*head_ptr)->socket;
	killHead(head_ptr,tail_ptr);
	SIGNAL(&consumer); //wake up exactly one consumer function. No need to waste time on making the others spin wait
	UNLOCK(&cmas);
	
	//do something with the socket now that you have it
	sem_post(&threads); //releasing semaphore back to the wild
}

void produce(int socket, struct node **head_ptr, struct node **tail_ptr){ //returns int for socket
	//don't need condition variable here
	LOCK(&cmas); //locked over all threads
	insert(socket, head_ptr, tail_ptr);
	UNLOCK(&cmas);
}

void runserver(int numthreads, unsigned short serverport) {
    //////////////////////////////////////////////////

    // create your pool of threads here

    //////////////////////////////////////////////////

	//Curtiness
	struct node *head = NULL; //consider the pool created
	struct node *tail = NULL;
	sem_init(&threads, 0, numthreads); //initializing GLOBAL VARIABLE!!!!
    
    int main_socket = prepare_server_socket(serverport);
    if (main_socket < 0) {
        exit(-1);
    }
    signal(SIGINT, signal_handler);

    struct sockaddr_in client_address;
    socklen_t addr_len;

    fprintf(stderr, "Server listening on port %d.  Going into request loop.\n", serverport);
    while (still_running) {
        struct pollfd pfd = {main_socket, POLLIN};
        int prv = poll(&pfd, 1, 10000);

        if (prv == 0) {
            continue;
        } else if (prv < 0) {
            PRINT_ERROR("poll");
            still_running = FALSE;
            continue;
        }
        
        addr_len = sizeof(client_address);
        memset(&client_address, 0, addr_len);

        int new_sock = accept(main_socket, (struct sockaddr *)&client_address, &addr_len);
        if (new_sock > 0) {
			sem_wait(&threads); //wait until a thread is open for use
            
            fprintf(stderr, "Got connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

			//wrong right now
			produce(new_sock, &head, &tail); //inserting socket at end of linked list, should be its own thread now
			consume(&head,&tail); //consuming the head of the linked list I just threw the last socket on
			//above should both be new threads. Our "pool" of threads is an abstraction, with the reality being a continuom of creation and destruction of new threads with the linked list used by the producer/consumer functions as the only thing that lasts

			////////////////////////////////////////////////////////
			/* You got a new connection.  Hand the connection off
			* to one of the threads in the pool to process the
			* request.
			*
			* Don't forget to close the socket (in the worker thread)
			* when you're done.
			*/
			////////////////////////////////////////////////////////


        }
    }
    fprintf(stderr, "Server shutting down.\n");
        
    close(main_socket);
}


int main(int argc, char **argv) {
    unsigned short port = 3000;
    int num_threads = 1;

    int c;
    while (-1 != (c = getopt(argc, argv, "hp:"))) {
        switch(c) {
            case 'p':
                port = atoi(optarg);
                if (port < 1024) {
                    usage(argv[0]);
                }
                break;

            case 't':
                num_threads = atoi(optarg);
                if (num_threads < 1) {
                    usage(argv[0]);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }

    runserver(num_threads, port);
    
    fprintf(stderr, "Server done.\n");
    exit(0);
}
