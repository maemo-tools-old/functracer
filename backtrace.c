#include <stdlib.h>
#include <stdio.h> /* XXX DEBUG */
#include <string.h> /* XXX DEBUG */
#include <sys/types.h>
#include <libunwind-ptrace.h>

#include "backtrace.h"
#include "debug.h"

struct bt_data *bt_init(pid_t pid)
{
	struct bt_data *btd;

	debug(3, "bt_init(pid=%d)", pid);
	btd = malloc(sizeof(struct bt_data));
	if (!btd)
		error_exit("bt_init(): malloc");
	btd->as = unw_create_addr_space(&_UPT_accessors, 0);
	if (!btd->as)
		error_exit("bt_init(): unw_create_addr_space() failed");
	btd->ui = _UPT_create(pid);

	return btd;
}

int bt_backtrace(struct bt_data *btd, void **buffer, int size)
{
	unw_cursor_t c;
	unw_word_t ip, off;
	int n = 0, ret;
	char buf[512];

	if ((ret = unw_init_remote(&c, btd->as, btd->ui)) < 0) {
		debug(1, "bt_backtrace(): unw_init_remote() failed, ret=%d", ret);
		return -1;
	}

	do {
		if ((ret = unw_get_reg(&c, UNW_REG_IP, &ip)) < 0) {
			debug(1, "bt_backtrace(): unw_get_reg() failed, ret=%d", ret);
			return -1;
		}
		unw_get_proc_name(&c, buf, sizeof(buf), &off);
		if (off) {
			size_t len = strlen(buf);
			if (len >= sizeof(buf) - 32)
				len = sizeof(buf) - 32;
			sprintf(buf + len, "+0x%lx", (unsigned long)off);
		}
		printf("XXX DEBUG: name = \"%s\", addr = %p\n", buf,(void *)(uintptr_t)ip);
		buffer[n++] = (void *)(uintptr_t)ip;

		if ((ret = unw_step(&c)) < 0) {
			debug(1, "bt_backtrace(): unw_step() failed, ret=%d", ret);
			return -1;
		}
	} while (ret > 0 && n < size);

	return n;
}

void bt_finish(struct bt_data *btd)
{
	_UPT_destroy(btd->ui);
	free(btd);
}
