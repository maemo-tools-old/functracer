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
 */

#include <stdlib.h>
#include <stdio.h> /* XXX DEBUG */
#include <string.h> /* XXX DEBUG */
#include <sys/types.h>
#include <libunwind-ptrace.h>

#include "backtrace.h"
#include "config.h"
#include "debug.h"
#include "options.h"

struct bt_data {
	unw_addr_space_t as;
	struct UPT_info *ui;
};

struct bt_data *bt_init(pid_t pid)
{
	struct bt_data *btd;

	debug(3, "bt_init(pid=%d)", pid);
	btd = malloc(sizeof(struct bt_data));
	if (!btd)
		error_exit("bt_init(): malloc");
	btd->as = unw_create_addr_space(&_UPT_accessors, 0);
	unw_set_caching_policy (btd->as, UNW_CACHE_GLOBAL);
	if (!btd->as)
		error_exit("bt_init(): unw_create_addr_space() failed");
	btd->ui = _UPT_create(pid);

	return btd;
}

int bt_backtrace(struct bt_data *btd, char **buffer, int size)
{
	unw_cursor_t c;
	unw_word_t ip, off;
	int n = 0, ret;
	char buf[512];
	size_t len = 0;

	if ((ret = unw_init_remote(&c, btd->as, btd->ui)) < 0) {
		debug(1, "bt_backtrace(): unw_init_remote() failed, ret=%d", ret);
		return -1;
	}

	do {
		if ((ret = unw_get_reg(&c, UNW_REG_IP, &ip)) < 0) {
			debug(1, "bt_backtrace(): unw_get_reg() failed, ret=%d", ret);
			return -1;
		}
		if (arguments.resolve_name) {
			ret = unw_get_proc_name(&c, buf, sizeof(buf), &off);
			if (ret < 0)
				sprintf(buf, "<undefined> ");
			else if (off) {
				len = strlen(buf);
				if (len >= sizeof(buf) - 64)
					len = sizeof(buf) - 64;
				sprintf(buf + len, "+0x%lx ", (unsigned long)off);
			}
			len = strlen(buf);
			if (len >= sizeof(buf) - 64)
				len = sizeof(buf) - 64;
		}
		/* Decrement breakpoint size from the first address in the
		 * backtrace
		 */
		if (n == 0)
			ip = (uintptr_t)ip - DECR_PC_AFTER_BREAK;
		/* Small workaround: decrement the current address to get the
		 * correct line in post-processing
		 */
		sprintf(buf + len, "[0x%x]", (uintptr_t)(ip - 1));

		buffer[n++] = strdup(buf);
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
