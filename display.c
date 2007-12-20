#include <stdio.h>
#include <ctype.h>		/* for isprint() */
#include <sys/ptrace.h>

#include "process.h"

static void display_char(int what)
{
	if (isprint(what))
		printf("%c", what);
	else
		printf("\\%03o", (unsigned char)what);
}

/* Read a series of bytes starting at the process's memory address
 * 'addr' and continuing until a NUL ('\0') is seen or 'len' bytes
 * have been read.
 */
static int umovestr(struct process *proc, void *addr, int len, char *laddr)
{
	union {
		long a;
		char c[sizeof(long)];
	} a;
	int i;
	int offset = 0;

	while (offset < len) {
		a.a = ptrace(PTRACE_PEEKTEXT, proc->pid, addr + offset, 0);
		for (i = 0; i < sizeof(long); i++) {
			if (a.c[i] && offset + (signed)i < len) {
				laddr[offset + i] = a.c[i];
			} else {
				laddr[offset + i] = '\0';
				return 0;
			}
		}
		offset += sizeof(long);
	}
	laddr[offset] = '\0';
	return 0;
}

#define MAX_STRING	255

void display_arg(struct process *proc, void *addr)
{
	char argstr[MAX_STRING];
	int i;

	umovestr(proc, addr, MAX_STRING, argstr);
	printf("\"");
	for (i = 0; i < MAX_STRING; i++) {
		if (argstr[i])
			display_char(argstr[i]);
		else
			break;
	}
	if (argstr[i])
		printf("...");
	printf("\"");
}
