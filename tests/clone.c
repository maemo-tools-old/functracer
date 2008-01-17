/* Based on trace-clone.c from ltrace, originally written by
 * Yao Qi <qiyao@cn.ibm.com>. */

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

int child(void *arg)
{
	malloc(456);
	return 0;
}

#define STACK_SIZE 1024

int main(void)
{
	pid_t pid;
	char stack[STACK_SIZE];

	malloc(123);
	pid = clone(child, stack + STACK_SIZE, CLONE_FS, NULL);
	if (pid == -1) {
		perror("clone failed");
		return 1;
	}
	malloc(789);

	return 0;
}
