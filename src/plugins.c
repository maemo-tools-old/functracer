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

#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "debug.h"
#include "options.h"
#include "plugins.h"
#include "process.h"

#define FT_API_VERSION "2.0"

static void *handle = NULL;
static struct plg_api *plg_api;


static void *plg_get_symbol(char *name)
{
    return dlsym(handle, name);
}

static int plg_check_version()
{
	char *version = plg_api->api_version;

	if (version == NULL)
		return 0;
	return (strcmp(version, FT_API_VERSION) == 0);
}

static int plg_load_module(const char *modname)
{
	struct plg_api *(*function)();

	if (modname == NULL)
		return 0;

	if (handle != NULL) /* already loaded */
		return 1;

	handle = dlopen(modname, RTLD_LAZY);
	if (handle == NULL) {
		msg_warn("Could not open plugin: %s", dlerror());
		return 0;
	}

	dlerror();    /* Clear any existing error */
	function = (struct plg_api *(*)()) plg_get_symbol("init");
        if (function == NULL) {
		msg_warn("Could not initialize plugin API");
		goto err;
	}
	plg_api = function();

	if (!plg_check_version()) {
		msg_warn("%s: Module version information not found or"
			 " incompatible. Refusing to use.", modname);
		goto err;
	}

	return 1;

err:
	plg_finish();
	return 0;
}

void plg_function_exit(struct process *proc, const char *name)
{
	if (handle == NULL)
		return;

	if (plg_api->function_exit == NULL) {
		msg_warn("Could not read symbol");
		return;
	}

	plg_api->function_exit(proc, name);
}

int plg_match(const char *symname)
{
	if (handle == NULL)
		return 0;

	if (plg_api->library_match == NULL) {
		msg_warn("Could not read symbol");
		return 0;
	}

	return plg_api->library_match(symname);
}

void plg_init()
{
	char plg_name[PATH_MAX];
	struct stat buf;

	/* First check if arguments.plugin (full path/filename) exists,
 	 * otherwise then prefixing it with default plugin directory and
	 * postfixing with .so */
	if (stat(arguments.plugin, &buf) == 0 && S_ISREG(buf.st_mode))
		snprintf(plg_name, sizeof(plg_name), arguments.plugin);
	else
		snprintf(plg_name, sizeof(plg_name), "%s/%s.so", PLG_PATH,
		 	 arguments.plugin);
	plg_load_module(plg_name);
}

void plg_finish()
{
	if (handle != NULL) {
		dlclose(handle);
		handle = NULL;
	}
}
