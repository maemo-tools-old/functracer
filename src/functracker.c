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
