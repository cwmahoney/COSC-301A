/*
Curtis Mahoney
Cosc 301 HW 2
9/4/2012
*/

#include <stdio.h>;

int avgchar(const char *mystring);
char charat(const char *mystring, int index);
int * string_in(const char *str1, char *str2);
int strncmp(const char *str1, char *str2, int max_len);

int main(int argc, char **argv){

	const char *teststr = "This is a wonderful day";
	const char *teststr2 = "derful";
	const int index = 5;

	lconst char comp1 = "Testing Pie";
	const char comp2 = "Testing Applies";
	const int max_comp = 8;

	printf("The average of %s's characters is %d\n", teststr, avgchar(teststr));
	printf("%c is at index %d of the string in question\n", charat(teststr, index), index);
	printf("%s is contained within %s starting at memory location %p\n", teststr2, teststr, string_in(teststr, teststr2));
	printf("%s\nVS.\n%s\nResult: %d\n", comp1, comp2, strncmp(comp1,comp2)
	

	return 0;
}

/*Returns the average decimal value of the characters of a string*/
int avgchar(const char *mystring){
	unsigned int len = strlen(mystring); //Assume len is smaller than an int
	unsigned long total = 0;
	int i = 0;
	unsigned int overflow = 0; //Overflow will contain the 'average' of part of the string if an overflow is imminent. Will make it slightly less precise
	unsigned int temp;
	
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
	if(index>=len || len < 0){
		return -1;
	}
	return mystring[index];
	
}

/*Returns the address (pointer) of where the second string begine iff the second string is contained within the first, and NULL ('\0') otherwise*/
int * string_in(const char *str1, char *str2){
	int len1 = strlen(str1);
	int len2 = strlen(str2);
	int i = j = start = 0;

	while(start<(len1-len2)){ //stop when impossible for the remainder of str1 to contain str2
		i = start++; //restarting search at each index, no matter how far the last search got
		while(str1[i++]==str2[j]){
			if(str2[++j]=='\0'){
				return &str1[start-1];
			}
		}
		i = start;
		j=0;
	}
}

/*Compares (case-sensitive) the two strings, with -1 if the first is lexigraphically earlier, 1 if the second, and 0 if they are the same */
int strncmp(const char *str1, char *str2, int max_len){
	int len1 = strlen(str1);
	int len2 = strlen(str2);
	int i = 0;

	while(i<len1 && i<len2 && i < max_len){
		if(str1[i]<str2[i]){
			return -1;
		}
		else if(str1[i]>[str2[i]){
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













