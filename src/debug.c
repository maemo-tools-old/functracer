#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "debug.h"
#include "options.h"

static void output_line(const char *fmt, ...)
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
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void msg_err(const char *fmt, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	perror(buf);
	raise(SIGINT);
}

void error_file(const char *filename, const char *msg)
{
	msg_err("\"%s\": %s", filename, msg);
}
