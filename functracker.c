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

	memset(&arguments, 0, sizeof(struct arguments));
	process_options(argc, argv, &prog_index, &arguments);

	cb_init();
	trace_execute(argv[prog_index], argv + prog_index);
	ret = trace_main_loop();

	return ret;
}
