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

#include <check.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "callback.h"
#include "trace.h"

#include "utest_report.h"
#include "report.h"
#include "report.c"

/* XXX: Ubber-hack to don't mess with the linker */
#define main main_functracker
int main_functracker(int argc, char *argv[]);
#include "functracker.c"

#define STDOUTFILE "/tmp/utest_report.txt"


static char *uargv[] = {
	"utest",
	"-s",
	"-f",
	"-l", "/tmp/",
	"true",
	NULL,
};

static int uargc = sizeof(uargv) / sizeof(char *);


static int file_size(const char *path)
{
	struct stat buf;

	stat(path, &buf);

	return buf.st_size;
}



static void setup(void)
{
	signal_attach();
}


START_TEST (test_process_report_screen)
{
	int remaining, ret, fd;

	/* This will make logging to stdout redirect to file */
	close(STDOUT_FILENO);
	fd = open(STDOUTFILE, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	fail_unless(fd == STDOUT_FILENO, "Redirect stdout to file trick not working");

	/* XXX: double -s is harmless */
	uargv[2] = "-s";

	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Failing with basic arguments");

	cb_init();
	trace_execute(uargv[remaining], uargv + remaining);
	trace_main_loop();

	close(STDOUT_FILENO);
}
END_TEST


START_TEST (test_process_report_file)
{
	int remaining, ret, pid;
	char path[PATH_MAX];

	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Failing with basic arguments");

	cb_init();
	pid = trace_execute(uargv[remaining], uargv + remaining);
	trace_main_loop();

	snprintf(path, sizeof(path), "/tmp/allocs-%d.trace", pid);

	fail_if(file_size(STDOUTFILE) != file_size(path), "Dump to screen "
			"differs from dump to file");
}
END_TEST


TCase *report_tcase_create(void)
{
	TCase *tc = tcase_create("report");
	tcase_add_checked_fixture(tc, setup, NULL);

	tcase_add_test(tc, test_process_report_screen);
	tcase_add_test(tc, test_process_report_file);

	return tc;
}

