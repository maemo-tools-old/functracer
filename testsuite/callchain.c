#include <stdlib.h>

extern void lib_f(int i);

static void do_backtrace(void)
{
	free(malloc(100));
}

static void h(int i)
{
	do_backtrace();
	lib_f(i + 1);
}

static void g(int i)
{
	h(i + 1);
}

static void f(int i)
{
	g(i + 1);
}

int main(void)
{
	f(1);

	return 0;
}
