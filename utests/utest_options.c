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

#include "utest_options.h"
#include "options.c"


static void setup(void)
{
	return;
}


static void teardown(void)
{
	return;
}


START_TEST (test_process_options)
{
	return;
}
END_TEST


TCase *options_tcase_create(void)
{
	TCase *tc = tcase_create("options");
	tcase_add_checked_fixture(tc, setup, teardown);

	tcase_add_test(tc, test_process_options);

	return tc;
}

