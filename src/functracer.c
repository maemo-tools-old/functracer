/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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
#include <stdlib.h>
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
		int ret = syscall(SYS_tkill, pid, signo);
		if (errno != ENOSYS)
			return ret;
		errno = 0;
		tkill_failed = 1;
	}
	return kill(pid, signo);
}

static void kill_process(struct process *proc, void *data)
{
	int ret;
	int signo = (int) data;

	if (arguments.npids)
		signo = SIGSTOP;

	if (proc->exiting)
		return;
	debug(2, "Sending %s to process %d", strsignal(signo), proc->pid);
	ret = my_kill(proc->pid, signo);
	if (ret < 0)
		msg_warn("could not send %s to PID %d: %s", strsignal(signo),
				proc->pid, strerror(errno));
	proc->exiting = 1;
}

static void signal_exit(int sig)
{
	debug(1, "Received interrupt signal; exiting...");
	for_each_process(kill_process, (void *)sig);
}

static void signal_attach(void)
{
	const int signals[] = { SIGINT, SIGTERM };
	struct sigaction sa;
	unsigned int i;

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
	if ((ret = process_options(argc, argv, &prog_index)))
		exit(ret);

	cb_init();
	for (i = 0; i < arguments.npids; i++)
		trace_attach_child(arguments.pid[i]);
	if (!arguments.npids)
		trace_execute(arguments.remaining_args[0],
				arguments.remaining_args);
	ret = trace_main_loop();

	/* Do cleanup before exiting to keep valgrind happy.
	 * FIXME: cleanup when functracer is interrupted with CTRL+C too. */
	cb_finish();
	remove_all_processes();

	return ret;
}
