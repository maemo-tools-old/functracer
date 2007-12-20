#include <stdarg.h>
#include <stdio.h>

static void output_line(char *fmt, ...)
{
	va_list args;

	if (!fmt) {
		return;
	}
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void debug_(int level, const char *file, int line, const char *func,
	const char *fmt, ...)
{
	char buf[1024];
	va_list args;

	/* TODO: handle level argument */
	va_start(args, fmt);
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

//#warning "debug message output disabled"
	output_line("DEBUG: %s:%d: %s(): %s", file, line, func, buf);
}
