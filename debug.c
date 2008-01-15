#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>

#include "options.h"

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

void debug_(int level, const char *file, int line, const char *func, const char *fmt, ...)
{
	char buf[1024];
	va_list args;

	if (arguments.debug < level) {
		return;
	}

	va_start(args, fmt);
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

	output_line("DEBUG: %s:%d: %s(): %s", file, line, func, buf);
}

void msg_warn(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	error(0, 0, fmt, args);
	va_end(args);
}

void msg_err(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	error(EXIT_FAILURE, errno, fmt, args);
	va_end(args);
}

void error_file(char *filename, char *msg)
{
	error(EXIT_FAILURE, errno, "\"%s\": %s", filename, msg);
}
