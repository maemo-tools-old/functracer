#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/ptrace.h>

#include "util.h"

long xptrace(int request, pid_t pid, void *addr, void *data)
{
	long ret;

	errno = 0;
	ret = ptrace((enum __ptrace_request)request, pid, addr, data);
	if (ret == -1 && errno)
		msg_err("ptrace");

	return ret;
}
