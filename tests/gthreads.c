/*
 * Author: Anderson Lizardo <anderson.lizardo@indt.org.br>
 * 
 * (C) 2007  Instituto Nokia de Tecnologia
 *
 * To compile:
 * 	gcc `pkg-config --cflags --libs /usr/lib/pkgconfig/gthread-2.0.pc` -o gthreads gthreads.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

void *thread1(void *arg)
{
	printf("%s\n", (char *)arg);
//	g_thread_exit(0);
	return NULL;
}

int main(int argc, char *argv[])
{
	GThread *th;
	char *hello;

	g_thread_init(NULL);

	hello = strdup("hello, world!");
	th = g_thread_create(&thread1, hello, TRUE, NULL);
	if (th == NULL)
		g_error("can't create the thread");
	g_thread_join(th);
	free(hello);

	return 0;
}
