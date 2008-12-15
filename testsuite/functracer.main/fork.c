/*
 * This file is part of Functracer.
 *
 * Copyright (C) 1997-2007 Juan Cespedes <cespedes@debian.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Based on trace-fork.c from ltrace, originally written by
 * Yao Qi <qiyao@cn.ibm.com>.i
 */

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