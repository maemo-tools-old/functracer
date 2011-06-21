/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 2008 Free Software Foundation, Inc.
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
 * Based on backtrace code from GDB.
 */

#ifndef FTK_SOLIB_H
#define FTK_SOLIB_H

#include <bfd.h>

#include "process.h"
#include "target_mem.h"


/**
 * Operation mode specific symbol access functions.
 *
 * In normal mode functracer reads symbol table from the target file.
 * In audit mode (-a) functracer attempts to locate debug symbols and
 * use the debug symbol table from it.
 */
struct solib_data {
	/**
	 * Opens target file.
	 *
	 * The opened file must be closed with close() function.
	 * @param[in] solib       the solib data structure.
	 * @param[in] filename    the file name.
	 * @return                the opened file or NULL if failed.
	 */
	bfd *(*open)(struct solib_data *solib, const char *filename);

	/**
	 * Closes the opened file.
	 *
	 * @param[in] solib       the solib data structure.
	 * @param[in] file        the file reference returned by open() function.
	 */
	int (*close)(struct solib_data *solib, bfd *file);

	/**
	 * Reads symbol table from the specified file.
	 *
	 * This function allocates the necessary space which
	 * must be freed afterwards with free_symbols() function.
	 * @param[in] file          the file to read from.
	 * @param[out] symbols      the read symbols.
	 * @return                  the number of symbols in table.
	 */
	long (*read_symbols)(bfd *file, asymbol ***symbols);


	/**
	 * Frees symbol table allocated by read_symbols() function.
	 *
	 * @param[in] symbols   the symbol table to free.
	 */
	void (*free_symbols)(asymbol **symbols);

	/* name of the file containing debug symbols */
	char *debug_name;
};


struct solib_list {
	addr_t start_addr;
	addr_t end_addr;
	char *path;
	struct solib_list *next;
};

typedef void (*new_sym_t)(struct process *, const char *, const char *,
			  addr_t);

extern void solib_update_list(struct process *proc, new_sym_t callback);
extern addr_t solib_dl_debug_address(struct process *proc);
extern void free_all_solibs(struct process *proc);

/**
 * Initializes solib symbol access handler.
 *
 * @param[in] proc   the process data;
 */
extern void solib_initialize(struct process *proc);

#endif /* !FTK_SOLIB_H */
