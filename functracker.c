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
	int prog_index, ret;

	process_options(argc, argv, &prog_index);

	cb_init();
	if (arguments.pid != 0)
		trace_attach(arguments.pid);
	if (prog_index < argc)
		trace_execute(argv[prog_index], argv + prog_index);
	ret = trace_main_loop();

	return ret;
}
