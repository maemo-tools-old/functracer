#include <stdlib.h>

static void lib_do_backtrace(void)
{
	free(malloc(100));
}

static void lib_h(int i)
{
	lib_do_backtrace();
}

static void lib_g(int i)
{
	lib_h(i + 1);
}

void lib_f(int i)
{
	lib_g(i + 1);
}
