#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char *x = NULL;

	printf("before malloc(): x = %p\n", x);
	x = malloc(100 * sizeof(char));
	printf("after malloc(): x = %p\n", x);
	free(x);

	return 0;
}
