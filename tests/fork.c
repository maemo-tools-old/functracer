/* Based on trace-fork.c from ltrace, originally written by
 * Yao Qi <qiyao@cn.ibm.com>. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define VERBOSE 0

#if VERBOSE
#define info(...) do { printf(__VA_ARGS__); } while (0)
#else
#define info(...) do { } while (0)
#endif

void child(void)
{
	info("CHILD: PID is %d\n", getpid());
	malloc(456);
	info("CHILD: finished\n");
}

int main(void)
{
	pid_t pid, pid2;
	int status;

	info("PARENT: PID is %d\n", getpid());
	malloc(123);
	pid = fork();
	assert(pid != -1);
	if (pid == 0) { /* child */
		child();
		return 0;
	}

	/* parent */
	pid2 = waitpid(-1, &status, __WALL);
	malloc(789);
	info("PARENT: finished\n");

	return 0;
}
