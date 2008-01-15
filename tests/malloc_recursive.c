#include <stdio.h>
#include <stdlib.h>

void malloc_recursive(int i)
{
	size_t size = i * 1024 * sizeof(char);
	void *ptr;

	if (i == 0)
		return;
	ptr = malloc(size);
	printf("addr=%p, size=%d\n", ptr, size);
	malloc_recursive(i - 1);
	free(ptr);
}

int main(void)
{
	malloc_recursive(100);

	return 0;
}
