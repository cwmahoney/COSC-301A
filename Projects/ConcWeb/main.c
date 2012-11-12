/*
Curt Mahoney and Adriana Sperlea
Purpose: Create the code base for a concurrent web server

Curt implemented the most of the code dealing with thread concurrency and managing the sockets.
Adriana fixed some bugs relating to that code and then implemented most of the worker function.
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
pthread_mutex_t file_mut = PTHREAD_MUTEX_INITIALIZER;
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

void produce_cnct(int socket, struct sockaddr_in client_address, struct node **head_ptr, struct node **tail_ptr){ //returns int for socket
	//don't need condition variable here
	LOCK(&sock_mut); //locked over all threads
	insert(socket, client_address, head_ptr, tail_ptr);
	UNLOCK(&sock_mut);
	SIGNAL(&consumer); //wake up one consumer function. No need to waste time on making the others spin wait
}

void logToFile(char *buffer, char *filename, int bytes)
{
	// needs to be locked with a separate mutex to maximize concurrency
	LOCK(&file_mut);
	FILE *filedesc = fopen(filename, "a");
	fprintf(filedesc, "%s", buffer);
	fprintf(filedesc, " %d\n", bytes);
	UNLOCK(&file_mut);
	fclose(filedesc);
}

char *intToString(int x)
{
	// home grown function converting an int to a string for concantenation in the worker function
	char *digits;
	int len = 0, power = 1;
	while (power < x) {
		len ++;
		power *= 10;
	}
	digits = malloc(sizeof(char) * (len + 1));
	int nulpos = len;
	while (x > 0) {
		digits[len - 1] = '0' + (x % 10);
		len --;
		x /= 10;
	}
	digits[nulpos] = '\0';
	return digits;
}

void *worker(void *v){
	// locked for every malloc and free because their thread safety is reliant on library implementation

	struct node ***sock_list = (struct node ***)v;
	int bufferlen = 1024;
	LOCK(&sock_mut);
	char *reqbuffer = malloc(sizeof(char) * bufferlen);
	UNLOCK(&sock_mut);
	struct stat statresult;
	struct node **head_ptr = sock_list[0];
	struct node **tail_ptr = sock_list[1];
	while(still_running){
		int socket;
		struct sockaddr_in client_address;
		LOCK(&sock_mut); //locked over all threads
		while((*head_ptr==NULL)&&(still_running==TRUE)){
			WAIT(&consumer,&sock_mut);
		}
		if(still_running==FALSE){
			socket = -1;
		}else{
			socket = (*head_ptr)->socket;
			client_address = (*head_ptr)->client_address;
			killHead(head_ptr,tail_ptr);
		}
		UNLOCK(&sock_mut); //had to copy to here instead of its own function so I can print using client_address
		time_t t;
		if(socket!=-1){ //still_running!=FALSE		
			int success = getrequest(socket, reqbuffer, bufferlen);
			LOCK(&sock_mut);
			char *final_text = malloc(sizeof(char) * 10000);
			char *output = malloc(sizeof(char) * 1000); 
			UNLOCK(&sock_mut);			
			
			if (success == 0) {
				int bytes;
				if (stat(reqbuffer + 1, &statresult) >= 0) { // file exists (200)
					// opening file and reading contents
					LOCK(&sock_mut); // needs to be locked because otherwise different threads might be rewinding or opening/closing the same file
					//taken from www.cplusplus.com
					FILE * pFile;
  					long lSize;
  					char * buffer;
  					size_t result;
	
					pFile = fopen (reqbuffer + 1, "rb" );
 					if (pFile==NULL) {
						fputs ("File error",stderr); 
						exit (1);
					}

 					// obtain file size:
  					fseek (pFile , 0 , SEEK_END);
  					lSize = ftell (pFile);
  					rewind (pFile);

				  	// allocate memory to contain the whole file:
  					buffer = malloc (sizeof(char) * (lSize + 1));
  					if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

				  	// copy the file into the buffer:
  					result = fread (buffer,1,lSize,pFile);
  					if (result != lSize) {fputs ("Reading error",stderr); exit (3);}

  					/* the whole file is now loaded in the memory buffer. */

  					fclose (pFile);
					UNLOCK(&sock_mut);
				
					sprintf(final_text, HTTP_200, strlen(buffer)); // adding HTTP message
					strcat(final_text, buffer); // file contents

					bytes = senddata(socket, final_text, strlen(final_text));
					strcpy(output, inet_ntoa(client_address.sin_addr)); // creating the log
					char *temp = intToString(ntohs(client_address.sin_port));
					strcat(output,temp);
					LOCK(&sock_mut);					
					free(temp);
					free(buffer);
					UNLOCK(&sock_mut);
	
					// adding date and rest of info to the log
					time(&t);
					strcat(output, " ");
					strcat(output, ctime(&t));
					output[strlen(output) - 1] = ' ';
					strcat(output, "\"GET /");
					strcat(output, reqbuffer);
					strcat(output, "\" 200 ");			
				} else { // file does not exist (404)
					// sending data
					bytes = senddata(socket, HTTP_404, strlen(HTTP_404));
					strcpy(output, inet_ntoa(client_address.sin_addr));
					char *temp = intToString(ntohs(client_address.sin_port));
					strcat(output,temp);
					LOCK(&sock_mut);
					free(temp);
					UNLOCK(&sock_mut);
					// adding date and rest of info to the log
					time(&t);
					strcat(output, " ");
					strcat(output, ctime(&t));
					strcat(output, "\"GET /");
					strcat(output, reqbuffer);
					strcat(output, "\" 404 ");
				}
				// logging to file
				logToFile(output, "weblog.txt", bytes);
				LOCK(&sock_mut);
				free(final_text);
				free(output);
				UNLOCK(&sock_mut);
			}
			shutdown(socket, 2); //shutting down the socket at the end of use
		}
	}
	LOCK(&sock_mut);
	free(reqbuffer);
	UNLOCK(&sock_mut);
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

			produce_cnct(new_sock, client_address, &head, &tail); //inserting socket at end of linked list. Only executed on main thread
        }
    }
    fprintf(stderr, "Server shutting down.\n");

	// threads are done doing work    
    // wait for workers to complete
	pthread_cond_broadcast(&consumer); //wakes up all the consumers, which will then escape and stop running
	for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
	printf("threads woken up");

    close(main_socket);
}


int main(int argc, char **argv) {
	printf("Hi, I'm in main!\n");
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
	num_threads = 3;
    runserver(num_threads, port);
    
    fprintf(stderr, "Server done.\n");
    exit(0);
}
