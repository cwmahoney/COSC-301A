/*
Curtis Mahoney
Cosc 301 HW 2
9/4/2012
*/

#include <stdio.h>
#include <string.h>

int avgchar(const char *mystring);
char charat(const char *mystring, int index);
const char * (string_in(const char *str1, const char *str2));
int strncmp2(const char *str1, const char *str2, int max_len); //Apparently this is a string function, so I'll just renaim it a bit

int main(int argc, char **argv){

	const char *teststr = "Wonderful Life is Nice";
	const char *teststr2 = "derful L";
	const int index = 1;

	const char *comp1 = "Testing Pies";
	const char *comp2 = "Testing Applies";
	const int max_comp = 154;

	int temp;
	const char *temp_ptr;

	temp = avgchar(teststr);
	if(temp==-1){
		printf("There were no characters in %s\n", teststr);
	}
	else{
		printf("The average of \"%s\"\'s characters is %d\n", teststr, temp);
	}
	
	temp = charat(teststr, index);
	if(temp==-1){
		printf("There is no character at index %d\n",index);
	}
	else{
		printf("The character \'%c\' is at index %d of the string in question\n", temp, index);
	}

	temp_ptr = string_in(teststr, teststr2);
	if(temp_ptr=='\0'){
		printf("\"%s\" is not contained within \"%s\"\n", teststr2, teststr);
	}
	else{
		printf("\"%s\" is contained within \"%s\" starting at memory location %p\n", teststr2, teststr, temp_ptr);
	}


	printf("\"%s\"\nVS.\n\"%s\"\n(max of %d characters)\nResult: ", comp1, comp2, max_comp);

	temp = strncmp2(comp1,comp2,max_comp);
	if(temp==1){
		printf("\"%s\" is lower!\n",comp2);
	}
	else if(temp==-1){
		printf("\"%s\" is lower!\n",comp1);
	}
	else{
		printf("Tie!\n");
	}

	return 0;
}

/*Returns the average decimal value of the characters of a string, -1 if it's the empty string*/
int avgchar(const char *mystring){
	unsigned int len = strlen(mystring); //Assume len is smaller than an int
	unsigned long total = 0;
	int i = 0;
	unsigned int overflow = 0; //Overflow will contain the 'average' of part of the string if an overflow is imminent. Will make it slightly less precise
	unsigned int temp;

	if(!len){
		return -1;
	}
	
	for(;i<len;i++){
		temp = total + mystring[i];
		if(temp>=total){
			total = temp;
		}
		else{
			overflow += total/len;
			total = 0;
		}			
	}
	return overflow+total/len;
}

/*Returns character at the given index of the given string, or -1 for an invalid index*/
char charat(const char *mystring, int index){
	unsigned int len = strlen(mystring);
	if(index>=len || len <= 0){
		return -1;
	}
	return mystring[index];
	
}

/*Returns the address (pointer) of where the second string begine iff the second string is contained within the first, and NULL ('\0') otherwise*/
const char * (string_in(const char *str1, const char *str2)){
	unsigned int len1 = strlen(str1);
	unsigned int len2 = strlen(str2);
	unsigned int j, start;
	unsigned int i = j = start = 0;

	if(len1<len2){
		return '\0';
	}

	while(start<(len1-len2)){ //stop when impossible for the remainder of str1 to contain str2
		i = start++; //restarting search at each index, no matter how far the last search got
		while(str1[i++]==str2[j]){
			if(str2[++j]=='\0'){
				return &(str1[start-1]);
			}
		}
		i = start;
		j=0;
	}
	return '\0';
}

/*Compares (case-sensitive) the two strings, with -1 if the first is lexigraphically earlier, 1 if the second, and 0 if they are the same */
int strncmp2(const char *str1, const char *str2, int max_len){
	unsigned int len1 = strlen(str1);
	unsigned int len2 = strlen(str2);
	unsigned int i = 0;

	while(i<len1 && i<len2 && i < max_len){
		if(str1[i]<str2[i]){
			return -1;
		}
		else if(str1[i]>str2[i]){
			return 1;
		}
		else{
			++i;
		}
	}
	if(i<max_len-1){ //max_len is number of chars, not an index
		if(len1<len2){ //longer word is lexigraphically later
			return -1;
		}
		else if(len2>len1){
			return 1;
		}
	}
	return 0;
}













