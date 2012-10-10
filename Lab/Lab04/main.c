/*
using the rdtsc assembly instruction for very high-precision timestamping::
Curt Mahoney
10/10/2012
*/

/* using the x86 cycle timer (rdtsc) to obtain a quasi-timestamp */
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

/* macro to use rdtsc assembly instruction */
#define rdtscll(val) \
     __asm__ __volatile__ ("rdtsc" : "=A" (val))

int main(int argc, char **argv) 
{
    /* long long is a 64-bit integer */
	struct timeval t;;
    long long begin = 0LL, end = 0LL, diff = 0LL;
    rdtscll(begin);
	int i = 0;
	for(;i<10000000;i++)
	    gettimeofday(&t, NULL);
	//stuff
    rdtscll(end);
    diff = ((end - begin)/CLOCKS_PER_SEC)/1000; //ticks per millisecond, so over 1000
    printf("cycle timer begin: %lld\n", begin);
    printf("cycle timer end  : %lld\n", end);
    printf("difference       : %lld\n", diff);
    return 0;
}
