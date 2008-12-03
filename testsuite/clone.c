/* Based on trace-clone.c from ltrace, originally written by
 * Yao Qi <qiyao@cn.ibm.com>. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#define VERBOSE 0

#if VERBOSE
#define info(...) do { printf(__VA_ARGS__); } while (0)
#else
#define info(...) do { } while (0)
#endif

int child(void *arg)
{
	info("CHILD: PID is %d\n", getpid());
	malloc(456);
	info("CHILD: finished\n");
	return 0;
}

#define STACK_SIZE 1024

int main(void)
{
	pid_t pid, pid2;
	int status;
	char stack[STACK_SIZE];

	info("PARENT: PID is %d\n", getpid());
	malloc(123);
	pid = clone(child, stack + STACK_SIZE, CLONE_FS, NULL);
	assert(pid != -1);
	pid2 = waitpid(-1, &status, __WALL);
	assert(pid == pid2);
	malloc(789);
	info("PARENT: finished\n");

	return 0;
}
