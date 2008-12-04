/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
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
 * To compile:
 * 	gcc `pkg-config --cflags --libs /usr/lib/pkgconfig/gthread-2.0.pc` -o \
 * 	gthreads gthreads.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <sys/syscall.h>

#define VERBOSE 0

#if VERBOSE
#define info(...) do { printf(__VA_ARGS__); } while (0)
#else
#define info(...) do { } while (0)
#endif

pid_t gettid(void) {
	return syscall(SYS_gettid);
}

void *thread1(void *arg)
{
	info("CHILD: TID is %d, TGID is %d\n", gettid(), getpid());
	printf("%s\n", (char *)arg);
	malloc(456);
	info("CHILD: finished\n");
/*	g_thread_exit(0); */
	return NULL;
}

int main(int argc, char *argv[])
{
	GThread *th;
	char *hello;

	info("PARENT: TID is %d\n", gettid());
	g_thread_init(NULL);
	hello = strdup("hello, world!");
	th = g_thread_create(&thread1, hello, TRUE, NULL);
	if (th == NULL)
		g_error("can't create the thread");
	g_thread_join(th);
	free(hello);
	info("PARENT: finished\n");

	return 0;
}
