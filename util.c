#include <errno.h>
#include <error.h> /* TODO: use our error functions */
#include <stdlib.h>

#include "util.h"

/* based on xmalloc() implementation in GDB 6.6 (gdb/utils.c) */
void *xmalloc(size_t size)
{
	void *val;

	val = malloc(size);
	if (val == NULL)
		error(EXIT_FAILURE, errno, "malloc");

	return val;
}
