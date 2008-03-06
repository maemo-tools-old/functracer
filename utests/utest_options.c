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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "utest_options.h"
#include "options.h"
#include "options.c"


static char **uargv;
static int uargc, remaining;


static void setup(void)
{
	int i;

	char *argv_def[] = {
		"utest",
		"-p", "100",
		"-t", "2",
		"-d",
		"-s",
		"-f",
		"-l", "/tmp/",
		"ls",
	};

	uargc = sizeof(argv_def) / sizeof(char *);
	uargv = malloc(sizeof(argv_def));

	for (i = 0; i < uargc; i++)
		uargv[i] = argv_def[i];
}


static void teardown(void)
{
	free(uargv);
}


START_TEST (test_process_options_p)
{
	int ret, argc, i;
	char **argv;

	process_options(uargc, uargv, &remaining);
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Failing with basic arguments");

	uargv[2] = "-1";
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == EINVAL, "Accepting invalid PID number");

	uargv[2] = "0";
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == EINVAL, "Accepting invalid PID number");

	argc = (MAX_NPIDS + 1) * 2 + 1;
	argv = malloc(argc * sizeof(char *));

	for (i = 1; i < argc; i += 2) {
		argv[i] = "-p";
		argv[i + 1] = "100";
	}

	ret = process_options(argc, argv, &remaining);
	fail_unless(ret == EINVAL, "Exceeding max number of PIDs");

	ret = process_options(argc - 2, argv, &remaining);
	fail_unless(ret == 0, "Not accepting the max number of PIDs");
}
END_TEST


START_TEST (test_process_options_t)
{
	int ret;
	char buffer[20];

	process_options(uargc, uargv, &remaining);
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Failing with basic arguments");

	uargv[4] = "-1";
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == EINVAL, "Accepting invalid backtrace depth");

	uargv[4] = "0";
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == EINVAL, "Accepting invalid backtrace depth");

	snprintf(buffer, sizeof(buffer), "%d", MAX_BT_DEPTH + 1);
	uargv[4] = buffer;
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == EINVAL, "Exceeding max backtrace depth");
}
END_TEST


START_TEST (test_process_options_d)
{
	int ret;

	process_options(uargc, uargv, &remaining);
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Failing with basic arguments");
	fail_unless(arguments.debug == 1, "Not incrasing debug level");

	uargv[5] = "-ddddd";
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Not accepting multiple debug levels");
	fail_unless(arguments.debug == 5, "Not incrasing debug level");
}
END_TEST


START_TEST (test_process_options_s)
{
	int ret;

	process_options(uargc, uargv, &remaining);
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Failing with basic arguments");
	fail_unless(arguments.enabled == 1, "Failed to active -s option");
}
END_TEST


START_TEST (test_process_options_f)
{
	int ret;

	process_options(uargc, uargv, &remaining);
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Failing with basic arguments");
	fail_unless(arguments.save_to_file == 1, "Failed to active -f option");
}
END_TEST


START_TEST (test_process_options_l)
{
	int ret;

	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == 0, "Failing with basic arguments");

	uargv[9] = "/invalid_path/";
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == ENOENT, "Accepting invalid path");

	uargv[9] = "/proc/meminfo";
	ret = process_options(uargc, uargv, &remaining);
	fail_unless(ret == ENOENT, "Accepting a file as path");
}
END_TEST


TCase *options_tcase_create(void)
{
	TCase *tc = tcase_create("options");
	tcase_add_checked_fixture(tc, setup, teardown);

	tcase_add_test(tc, test_process_options_p);
	tcase_add_test(tc, test_process_options_t);
	tcase_add_test(tc, test_process_options_d);
	tcase_add_test(tc, test_process_options_s);
	tcase_add_test(tc, test_process_options_f);
	tcase_add_test(tc, test_process_options_l);

	return tc;
}

