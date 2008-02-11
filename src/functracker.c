/*
 * This file is part of Functracker.
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
 */

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "callback.h"
#include "debug.h"
#include "options.h"
#include "process.h"
#include "trace.h"

#define CAPACITY(a)        (sizeof(a) / sizeof(*a))

/* Send a signal to a specific thread or process.
 * tkill() is used so the signal is reliably received
 * by the correct thread on a multithreaded application.
 */
static int my_kill(int pid, int signo)
{
	static int tkill_failed;

	if (!tkill_failed) {
		int ret = syscall(__NR_tkill, pid, signo);
		if (errno != ENOSYS)
			return ret;
		errno = 0;
		tkill_failed = 1;
	}
	return kill(pid, signo);
}

static void signal_exit(int sig)
{
	exiting = 1;
	debug(1, "Received interrupt signal; exiting...");
	struct process *tmp = get_list_of_processes();
	while (tmp) {
		debug(2, "Sending SIGSTOP to process %u\n", tmp->pid);
		my_kill(tmp->pid, SIGSTOP);
		tmp = tmp->next;
	}
}

static void signal_attach(void)
{
	const int signals[] = { SIGINT, SIGTERM };
	struct sigaction sa;
	int i;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = signal_exit;

	for (i = 0; i < CAPACITY(signals); i++)
		sigaction(signals[i], &sa, NULL);
}

int main(int argc, char *argv[])
{
	int prog_index, ret, i;

	signal_attach();
	process_options(argc, argv, &prog_index);

	cb_init();
	for (i = 0; i < arguments.npids; i++)
		trace_attach(arguments.pid[i]);
	if (prog_index < argc)
		trace_execute(argv[prog_index], argv + prog_index);
	ret = trace_main_loop();

	return ret;
}
