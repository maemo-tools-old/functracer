#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char *x = NULL, *y = NULL;

	printf("before malloc(): x = %p, y = %p\n", x, y);
	x = malloc(123 * sizeof(char));
	y = malloc(456 * sizeof(char));
	printf("after malloc(): x = %p, y = %p\n", x, y);
	free(x);
	free(y);

	return 0;
}
