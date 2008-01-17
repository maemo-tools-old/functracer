#if 0
#include <stdlib.h>
#include <unistd.h>
#endif
#include <string.h>

#include "callback.h"
#include "options.h"
#include "trace.h"

int main(int argc, char *argv[])
{
	int prog_index, ret, i;

	process_options(argc, argv, &prog_index);

	cb_init();
	for (i = 0; i < arguments.npids; i++)
		trace_attach(arguments.pid[i]);
	if (prog_index < argc)
		trace_execute(argv[prog_index], argv + prog_index);
	ret = trace_main_loop();

	return ret;
}
