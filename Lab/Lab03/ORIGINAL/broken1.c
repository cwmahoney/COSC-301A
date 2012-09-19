#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void
removewhitespace(char *s1)
{
    // remove whitespace from a string, in place
    if (!s1)
        return;
    
    char *s2 = strdup(s1);
    free(s1);
    char *buffer = malloc(sizeof(s2));
    int i = 0;
    int j = 0;
    for (; i < strlen(s2); i++) {
        if (!isspace(s2[i])) {
            buffer[j] = s2[i];
            j++;
        }
    }
    buffer[j] = '\0';
    strcpy(s1, buffer);
}


char**
tokenify(char *s)
{
    const char *sep=" \t\n";
    char *word = NULL;

    // find out exactly how many tokens we have
    int words = 0;
    for (word = strtok(s, sep);
         word;
         word = strtok(NULL, sep)) words++ ;

    printf("words: %d\n", words);
    printf("s: %s\n", s);

    // allocate the array of char *'s, with one additional
    char **array = (char **)malloc(sizeof(char)*(words+1));
    int i = 0;
    for (word = strtok(s, sep);
         word;
         word = strtok(NULL, sep)) {
        printf("adding word %s to array pos %d\n", word, i);
        array[i] = strdup(word);
        i++;
    }
    return array;
}

void
printtokens(char **tokenlist) {
    char *tmp = tokenlist[0];
    int toknum = 0;
    printf("Printing tokens:\n");
    while (tmp != NULL) {
        printf("\t%d: <%s>\n", toknum, tmp);
        toknum++;
    }
}

int
main(int argc, char **argv) {
    char s1[] = "  the \tinternet is a series of tubes  ";
    char s2[] = "   \t\n";
    char s3[] = "  the \tinternet is not a series of tubes  ";
    char s4[] = "   \t\n";
    char s5[] = "8";
    
    removewhitespace(s1);
    removewhitespace(s2);
    printf("Remove whitespace - s1: %s\n", s1);
    printf("Remove whitespace - s2: %s\n", s2);

    printtokens(tokenify(s3));
    printtokens(tokenify(s4));
    printtokens(tokenify(s5));

    return 0;
}

