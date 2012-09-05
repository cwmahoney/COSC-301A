#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> //for issspace()

/*
Curtis Mahoney
9/5/12
Lab 01
Annoyed Adrianna occasionally for syntax questions
*/

void removewhitespace(char *instr){ //Not clever, not efficient, but it will work
	if(instr==NULL){
		return;
	}

	unsigned int len = strlen(instr);
	char temp_str[len+1];
	int i = 0;
	int j = 0;

	for(;i<len;i++){
		if(!isspace(instr[i])){
			temp_str[j++]=instr[i];
		}
	}
	temp_str[j]='\0';
	
	i = 0;
	for(;i<strlen(temp_str)+1;i++){
		instr[i]=temp_str[i];
	}
}

char *c2pascal(char *instr){
	int len = strlen(instr);

	if(instr==NULL || len>256){
		return NULL;
	}

	unsigned char p_length = len; //maximum of 255 characters, len of 256

	char *pasc=malloc((len+1)*sizeof(char)); //Specified new array, not in-place
	pasc[0]=p_length;

	int i = 0; //there might be a limited copy function I could use instead, but I don't know it and am not inclined to look at this very moment
	for(;i<len;i++){
		pasc[i+1]=instr[i];
	}
	return pasc;
}

char *pascal2c(char *pstr){
	unsigned char len = pstr[0]; //length already in pstr, know it's a char. Memory saving is fun

	if(pstr==NULL){
		return NULL;
	}

	char *ostr=malloc((len+1)*sizeof(char)); //MALLOC IT UP!
	ostr[len]='\0';

	int i = 0;
	for(;i<len;i++){
		ostr[i]=pstr[i+1];
	}
	return ostr;
}

char **tokenify(char *instr){
	return NULL;
}














