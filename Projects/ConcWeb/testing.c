#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	char text[10000], buf[101];
	memset(buf, 0, sizeof(buf));
	FILE *f = fopen("testfiles/56ce3276.html", "r");
	while (fread(buf, 1, 100, f)) {
		strcat(text, buf);
		printf("text = %s\n", text);
	}
	return 0;
}
