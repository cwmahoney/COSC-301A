#include <stdio.h>

int main(){
	pid_t p = fork()
	if(p==0){
		exec(killfork.c);
	}
	else if (p==-1){
		printf("Fork error\n");
	}
	else{
		printf("Can't stop the signal\n");
	}
	return 0;
}
