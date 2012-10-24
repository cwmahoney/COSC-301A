#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * worker process entry point
 */
void worker(long *counter)
{
    printf("child process %d starting\n", getpid());
    int i = 0;
    for (; i < 100000; i += 1) {
        *counter += 1;
    }
    printf("child process %d ending with counter %ld\n", getpid(), *counter);

    return;
}


int
main(int argc, char **argv)
{
    // the shared variable - allocated on the heap
    long *counter = malloc(sizeof(long));
    *counter = 0UL;
    
    pid_t p1 = fork();
    if (p1 == 0) {
        // child process 1        
        worker(counter);
        exit(0);
    } 

    pid_t p2 = fork();
    if (p2 == 0) {
        // child process 2
        worker(counter);
        exit(0);
    }

    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);

    printf("end counter value (parent process): %ld\n", *counter);

    free(counter);

    return 0;
}
