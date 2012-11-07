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

#include "network.h"
#include "wlinkedlist.h"

#define LOCK pthread_mutex_lock
#define UNLOCK pthread_mutex_unlock
#define WAIT pthread_cond_wait
#define SIGNAL pthread_cond_signal

pthread_mutex_t sock_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t consumer = PTHREAD_COND_INITIALIZER;

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

int consume_cnct(struct node **head_ptr, struct node **tail_ptr){ //working with sockets right now, not sure if it's right
	//Need condition variable incase list is empty
	LOCK(&sock_mut); //locked over all threads
	while((*head_ptr==NULL)&&(still_running==TRUE)){
		WAIT(&consumer,&sock_mut);
	}
	if(still_running==FALSE){
		return -1;
	}
	int socket = (*head_ptr)->socket;
	killHead(head_ptr,tail_ptr);
	UNLOCK(&sock_mut);
	return socket;
}

void produce_cnct(int socket, struct node **head_ptr, struct node **tail_ptr){ //returns int for socket
	//don't need condition variable here
	LOCK(&sock_mut); //locked over all threads
	insert(socket, head_ptr, tail_ptr);
	UNLOCK(&sock_mut);
	SIGNAL(&consumer); //wake up one consumer function. No need to waste time on making the others spin wait
}

void *worker(void *v){
	struct node ***sock_list = (struct node ***)v;
	while(still_running){
		int socket = consume_cnct(sock_list[0],sock_list[1]);
		if(socket==-1){ //still_running==FALSE
			return NULL; //stock running because we're done
		}
		//STUFF!!

		shutdown(socket, 2); //shutting down the socket at the end of use
	}
	return NULL;
}

void runserver(int num_threads, unsigned short serverport) {
    struct node **sock_list[2]; //to contain pointers to head and tail for passing to threads
	struct node *head = NULL;
	struct node *tail = NULL;
	sock_list[0] = &head;
	sock_list[1] = &tail; //to pass to new threads

	pthread_t threads[num_threads];
    int i = 0;

    // start up the threads; they'll start trying to consume immeidately
    for (i = 0; i < num_threads; i++) {
        if (0 > pthread_create(&threads[i], NULL, worker, (void*)&sock_list)) {
            fprintf(stderr, "Error creating thread: %s\n", strerror(errno));
        }
    }
    
    int main_socket = prepare_server_socket(serverport);
    if (main_socket < 0){
        exit(-1);
    }
    signal(SIGINT, signal_handler);

    struct sockaddr_in client_address;
    socklen_t addr_len;

    fprintf(stderr, "Server listening on port %d.  Going into request loop.\n", serverport);


	//thread pool set up and a-okay by now
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
            fprintf(stderr, "Got connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

			//wrong right now
			produce_cnct(new_sock, &head, &tail); //inserting socket at end of linked list. Only executed on main thread

			//Idea: Have all the waiting threads in the pool and never call them explicitely, with the threads instead constantly trying to consume

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

	// threads are done doing work    
    // wait for workers to complete
	pthread_cond_broadcast(&consumer); //wakes up all the consumers, which will then escape and stop running
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

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
