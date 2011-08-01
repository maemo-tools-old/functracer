/*
 * This is a functracer module used to track custom symbol list, specified
 * with functracer -a command line option.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2010 by Nokia Corporation
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <libiberty.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <sp_rtrace_formatter.h>
#include <sp_rtrace_defs.h>


#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"
#include "context.h"
#include "util.h"

#define AUDIT_API_VERSION "2.0"

#define RES_SIZE 	1
#define RES_ID		1

#define SYMBOL_SEPARATOR  ";"

static char audit_api_version[] = AUDIT_API_VERSION;

static sp_rtrace_resource_t res_audit = {
		.id = 1,
		.type = "virtual",
		.desc = "Virtual resource for custom symbol tracking",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};


/* the monitored symbol array */
static struct plg_symbol *symbols = NULL;
/* the number of monitored symbols */
static int plg_symbol_count = 0;
/* the limit of monitored symbol array */
static int plg_symbol_limit = 32;

static void parse_item(char *item);

/**
 * Compare text string against a pattern.
 *
 * The pattern has follwing format: <text>[*]
 * Where ending '*' matches all strings starting with <text>
 * return
 */


static void audit_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;

	assert(proc->rp_data != NULL);

	if (name[0] == 'I' && name[1] == 'A' &&	name[2] == '_' && name[3] == '_') {
		name += 4;
	}

	char *demangled_name = (char*)cplus_demangle(name, DMGL_ANSI | DMGL_PARAMS);
	const char *target_name = demangled_name ? demangled_name : name;

	int i;
	for (i = 0; i < plg_symbol_count; i++) {
		if (!strcmp_pattern(symbols[i].name, target_name)) {
			sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number++,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = (char*)target_name,
				.res_size = RES_SIZE,
				.res_id = (pointer_t)RES_ID,
			};
			sp_rtrace_print_call(rd->fp, &call);
			rp_write_backtraces(proc, &call);
			break;
		}
	}
	if (demangled_name) free(demangled_name);
	if (i == plg_symbol_count) {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
}

/**
 * Loads audit item information from file.
 * @param filename
 */
static void load_config(const char *filename)
{
	FILE* fp = fopen(filename, "r");
	if (fp) {
		char buffer[1024];
		while (fgets(buffer, sizeof(buffer), fp)) {
			int len = strlen(buffer) - 1;
			if (buffer[len] == '\n') buffer[len] = '\0';
			parse_item(buffer);
		}
		fclose(fp);
	}
}

/**
 * Adds symbol to monitored symbol table.
 * @param symbol
 */
static void add_symbol(const char *symbol)
{
	/* increase container size if necessary */
	if (plg_symbol_count == plg_symbol_limit) {
		plg_symbol_limit <<= 1;
		struct plg_symbol *ptr = realloc(symbols, plg_symbol_limit * sizeof(struct plg_symbol));
		if (!ptr) {
			return;
		}
		symbols = ptr;
	}
	symbols[plg_symbol_count].name = strdup(symbol);
	symbols[plg_symbol_count].hit = 0;
	plg_symbol_count++;
}

/**
 * Parses audit item.
 *
 * A audit item is <function>|@<filename> where:
 *   <function> - monitored function name.
 *   <filename> - filename containing audit items (one per line).
 * @param item
 */
static void parse_item(char *item)
{
	while (*item == ' ') item++;
	char* ptr = item + strlen(item);
	while (*--ptr == ' ') *ptr = '\0';

	/* check if the item contains configuration file name */
	if (*item == '@') {
		load_config(item + 1);
		return;
	}
	/* add item to the tracked symbol list */
	add_symbol(item);
}

/**
 * Initializes monitored symbol table.
 *
 * @param[in] arg   the list of audit items in format <item1>[,<item2>...]
 */
static void symbols_init(const char *arg)
{
	symbols = xcalloc(plg_symbol_limit, sizeof(struct plg_symbol));
	if (arg) {

		char *str = strdup(arg);
		if (str == NULL) return;

		char *item = strtok(str, SYMBOL_SEPARATOR);
		while (item) {
			parse_item(item);
			item = strtok(NULL, SYMBOL_SEPARATOR);
		}
		free(str);
	}
}

static int get_symbols(struct plg_symbol **syms)
{
	*syms = symbols;
	return plg_symbol_count;
}

static void audit_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_audit);
}


struct plg_api *init(void)
{
	symbols_init(arguments.audit);

	static struct plg_api ma = {
		.api_version = audit_api_version,
		.function_exit = audit_function_exit,
		.get_symbols = get_symbols,
		.report_init = audit_report_init,
	};
	return &ma;
}
