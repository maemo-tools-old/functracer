/* Based on trace-fork.c from ltrace, originally written by
 * Yao Qi <qiyao@cn.ibm.com>. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void child(void)
{
	malloc(456);
}

int main(void)
{
	pid_t pid;
	int status;

	malloc(123);
	pid = fork();
	if (pid == -1) {
		perror("fork failed");
		return 1;
	} else if (pid == 0) { /* child */
		child();
		return 0;
	}

	/* parent */
	printf("child pid is %d\n", pid);
	wait(&status);
	malloc(789);

	return 0;
}
