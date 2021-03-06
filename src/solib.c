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

#include <bfd.h>
#include <errno.h>
#include <libiberty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#include "callback.h"
#include "debug.h"
#include "maps.h"
#include "solib.h"
#include "options.h"
#include "filter.h"

/**
 * Program header structure, taken from binutils (include/elf/internal)
 */
struct elf_internal_phdr {
  unsigned long	p_type;			/* Identifies program segment type */
  unsigned long	p_flags;		/* Segment flags */
  bfd_vma	p_offset;		/* Segment file offset */
  bfd_vma	p_vaddr;		/* Segment virtual address */
  bfd_vma	p_paddr;		/* Segment physical address */
  bfd_vma	p_filesz;		/* Segment size in file */
  bfd_vma	p_memsz;		/* Segment size in memory */
  bfd_vma	p_align;		/* Segment alignment, file & memory */
};
typedef struct elf_internal_phdr Elf_Internal_Phdr;

struct elf_internal_sym {
  bfd_vma	st_value;		/* Value of the symbol */
  bfd_vma	st_size;		/* Associated symbol size */
  unsigned long	st_name;		/* Symbol name, index in string tbl */
  unsigned char	st_info;		/* Type and binding attributes */
  unsigned char	st_other;		/* Visibilty, and target specific */
  unsigned char st_target_internal;	/* Internal-only information */
  unsigned int  st_shndx;		/* Associated section index */
};
typedef struct elf_internal_sym Elf_Internal_Sym;

typedef struct
{
  /* The BFD symbol.  */
  asymbol symbol;
  /* ELF symbol information.  */
  Elf_Internal_Sym internal_elf_sym;
} elf_symbol_type;

enum arm_st_branch_type {
  ST_BRANCH_TO_ARM,
  ST_BRANCH_TO_THUMB,
  ST_BRANCH_LONG,
  ST_BRANCH_UNKNOWN
};

#define ARM_SYM_BRANCH_TYPE(SYM) \
  ((enum arm_st_branch_type) (SYM)->st_target_internal)

#define ELF_ST_TYPE(val)		((val) & 0xF)

/* Check whether the symbol is a thumb function, based on a hint from
 *     http://sources.redhat.com/ml/gdb-patches/2011-03/msg01105.html
 */
static int is_thumb_func(asymbol *sym)
{
#ifdef __arm__
	Elf_Internal_Sym *internal_sym =
		&((elf_symbol_type *)sym)->internal_elf_sym;
	/* STT_ARM_TFUNC was used before binutils-2.22 */
	if (ELF_ST_TYPE(internal_sym->st_info) == STT_ARM_TFUNC)
		return 1;
#ifdef HAVE_BINUTILS_2_22_OR_NEWER
	/* ST_BRANCH_TO_THUMB is used starting from binutils-2.22 */
	if (ARM_SYM_BRANCH_TYPE(internal_sym) == ST_BRANCH_TO_THUMB)
		return 1;
#endif
#endif
	return 0;
}

/* */

static void warning_bfd(const char *filename, const char *msg)
{
	fprintf(stderr, "\"%s\": %s: %s", filename, msg, bfd_errmsg(bfd_get_error()));
}

static void error_bfd(const char *filename, const char *msg)
{
	msg_warn("\"%s\": %s: %s", filename, msg, bfd_errmsg(bfd_get_error()));
	exit(EXIT_FAILURE);
}


static void resolve_path(char *filename, char **real_filename)
{
	*real_filename = canonicalize_file_name(filename);
	if (*real_filename == NULL)
		error_file(filename, "could not resolve file path");
}

static void find_solib(char *filename, char **real_filename)
{
	resolve_path(filename, real_filename);
}

static void lib_base_address(pid_t pid, char *filename, addr_t *addr)
{
	struct maps_data md;

	*addr = (addr_t)NULL;
	if (maps_init(&md, pid) == -1)
		return;
	while (maps_next(&md) == 1) {
		if (MAP_EXEC(&md) && strcmp(md.path, filename) == 0) {
			*addr = md.lo;
			break;
		}
	}
	maps_finish(&md);
}

/*
 * solib symbol handling functions for default mode
 */
static bfd *solib_open(__attribute__((unused)) struct solib_data *solib, const char *filename)
{
	bfd *abfd = bfd_fopen(filename, "default", "rb", -1);
	if (abfd == NULL) {
		warning_bfd(filename, "could not open shared library file");
		return NULL;
	}
	if (!bfd_check_format(abfd, bfd_object)) {
		warning_bfd(filename, "not in shared library format");
		bfd_close(abfd);
		return NULL;
	}
	return abfd;
}

static int solib_close(__attribute__((unused)) struct solib_data *solib, bfd *file)
{
	return bfd_close(file);
}

static long solib_read_symbols(bfd *file, asymbol ***symbols)
{
	long storage_needed = bfd_get_dynamic_symtab_upper_bound(file);
	if (storage_needed > 0) {
		*symbols = xmalloc((unsigned)storage_needed);
		return bfd_canonicalize_dynamic_symtab(file, *symbols);
	}
	return 0;
}

/*
 * solib symbol handling functions for debug mode
 */
static bfd *solib_debug_open(struct solib_data *solib, const char *filename)
{
	static const char *places[] = {
		".", "./.debug", "/usr/lib/debug", NULL
	};
	/* locate and open the symbol file */
	bfd *abfd = solib_open(solib, filename);
	if (!abfd) return NULL;

	int index = 0;
	while (places[index]) {
		solib->debug_name = bfd_follow_gnu_debuglink (abfd, places[index]);
		if (solib->debug_name) {
			bfd_close(abfd);
			abfd = solib_open(solib, solib->debug_name);
			if (!abfd) return NULL;
			break;
		}
		index++;
	}
	/* read the symbol table from opened file */
	if ((bfd_get_file_flags(abfd) & HAS_SYMS) == 0) {
		warning_bfd(solib->debug_name ? solib->debug_name : filename, "not in shared library format");
		solib->close(solib, abfd);
		abfd = NULL;
	}
	return abfd;
}

static int solib_debug_close(struct solib_data *solib, bfd *file)
{
	int rc = bfd_close(file);
	if (solib->debug_name) {
		free(solib->debug_name);
		solib->debug_name = NULL;
	}
	return rc;
}

static long solib_debug_read_symbols(bfd *file, asymbol ***symbols)
{
	unsigned int size;
	long nsymbols = bfd_read_minisymbols(file, FALSE, (void *) symbols, &size);
	if (nsymbols == 0) {
		nsymbols = bfd_read_minisymbols(file, TRUE, (void *) symbols, &size);
	}
	return nsymbols;
}


/* bfd_lookup_symbol -- lookup the value for a specific symbol */
static unsigned long bfd_lookup_symbol(bfd *abfd, const char *symname, flagword sect_flags)
{
	long storage_needed, number_of_symbols;
	asymbol *sym, **symbol_table;
	unsigned long symaddr = 0;

	debug(4, "symname=%s", symname);
	storage_needed = bfd_get_symtab_upper_bound(abfd);
	if (storage_needed > 0) {
		int i;

		symbol_table = xmalloc((unsigned)storage_needed);
		number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);
		for (i = 0; i < number_of_symbols; i++) {
			sym = symbol_table[i];
			if (strcmp(sym->name, symname) == 0
			    && (sym->section->flags & sect_flags) == sect_flags) {
				/* Bfd symbols are section relative. */
				symaddr = sym->value + sym->section->vma;
				if (is_thumb_func(sym))
					symaddr |= 1;
				break;
			}
		}
		free(symbol_table);
	}
	if (symaddr)
		return symaddr;

	storage_needed = bfd_get_dynamic_symtab_upper_bound(abfd);
	if (storage_needed > 0) {
		int i;

		symbol_table = xmalloc((unsigned)storage_needed);
		number_of_symbols = bfd_canonicalize_dynamic_symtab(abfd, symbol_table);
		for (i = 0; i < number_of_symbols; i++) {
			sym = symbol_table[i];
			if (strcmp(sym->name, symname) == 0
			    && (sym->section->flags & sect_flags) == sect_flags) {
				/* Bfd symbols are section relative. */
				symaddr = sym->value + sym->section->vma;
				if (is_thumb_func(sym))
					symaddr |= 1;
				break;
			}
		}
		free(symbol_table);
	}
	return symaddr;
}

static int solib_is_prelinked(bfd *abfd)
{
	asection *sect;
	int prelinked = 0;

	sect = bfd_get_section_by_name(abfd, ".gnu.prelink_undo");
	if (sect != NULL)
		prelinked = 1;

	return prelinked;
}


/* Based on enable_break() code from GDB 6.6 (gdb/solib-svr4.c). */
addr_t solib_dl_debug_address(struct process *proc)
{
	bfd *abfd;
	asection *sect;
	bfd_size_type sect_size;
	unsigned long sym_addr = 0;
	char *buf, *interp_file = NULL;
	addr_t start_addr = 0;
	addr_t load_addr = 0;
	const bfd_format bfd_fmt = bfd_object;

	abfd = bfd_fopen(proc->filename, "default", "rb", -1);
	if (abfd == NULL)
		error_bfd(proc->filename, "could not open as an executable file");
	if (!bfd_check_format(abfd, bfd_fmt)) {
		warning_bfd(proc->filename, "not in executable format");
		goto close_abfd;
	}
	/* Read the process entry point address. This address is used to
	 * set program start breakpoint to check if all of plugin symbols
	 * were located in the loaded libraries. */
	long phdr_size = bfd_get_elf_phdr_upper_bound(abfd);
	Elf_Internal_Phdr *phdr_table = xmalloc(phdr_size);
	int i, phdr_count = bfd_get_elf_phdrs(abfd, phdr_table);
	if (phdr_count == -1) {
		error_bfd(proc->filename, "failed to read elf program headers");
		goto close_abfd;
	}
	for (i = 0; i < phdr_count; i++) {
		if ((phdr_table[i].p_type == PT_LOAD) && (!phdr_table[i].p_offset)) {
			if (!phdr_table[i].p_vaddr) {
				lib_base_address(proc->pid, proc->filename, &load_addr);
			}
			break;
		}
	}
	proc->start_address = abfd->start_address + load_addr;

	/* */
	sym_addr = bfd_lookup_symbol(abfd, "_dl_debug_state", SEC_CODE);

	/* Find the .interp section. */
	sect = bfd_get_section_by_name(abfd, ".interp");
	if (sect == NULL) {
		warning_bfd(proc->filename, "could not read .interp section");
		goto close_abfd;
	}

	/* Read the contents of the .interp section into a local buffer;
	   the contents specify the dynamic linker this program uses. */
	sect_size = bfd_section_size(abfd, sect);
	buf = xmalloc((unsigned)sect_size);
	bfd_get_section_contents(abfd, sect, buf, (file_ptr)0, sect_size);

	if (!bfd_close(abfd))
		error_bfd(proc->filename, "could not close file");

	/* Now we need to figure out where the dynamic linker was
	   loaded so that we can load its symbols and place a breakpoint
	   in the dynamic linker itself. */
	find_solib(buf, &interp_file);
	if (interp_file == NULL)
		goto error;
	free(buf);
	abfd = bfd_fopen(interp_file, "default", "rb", -1);
	if (abfd == NULL) {
		warning_bfd(interp_file, "could not open dynamic linker file");
		goto error;
	}
	if (!bfd_check_format(abfd, bfd_fmt)) {
		warning_bfd(interp_file, "not in shared file format");
		goto close_abfd;
	}
#if 0 /* FIXME: unnecessary? */
	/* Find the .text section in interpreter code. */
	sect = bfd_get_section_by_name(interp_bfd, ".text");
	if (sect == NULL) {
		warning_bfd(interp_file, "could not read .text section");
		goto error_interp;
	}
#endif
	/* Now try to set a breakpoint in the dynamic linker. */
	sym_addr = bfd_lookup_symbol(abfd, "_dl_debug_state", SEC_CODE);
	if (sym_addr == 0) {
		warning_bfd(interp_file, "could not find _dl_debug_state symbol");
		goto close_abfd;
	}
	if (!solib_is_prelinked(abfd))
		lib_base_address(proc->pid, interp_file, &start_addr);

	free(interp_file);
close_abfd:
	if (!bfd_close(abfd))
		error_bfd(interp_file, "could not close file");
error:
	return (start_addr + sym_addr);
}

static void current_solibs(struct process *proc, struct solib_list **solist)
{
	struct maps_data md;
	struct solib_list *so = NULL;

	*solist = NULL;
	if (maps_init(&md, proc->pid) == -1)
		return;
	while (maps_next(&md) == 1) {
		if (MAP_EXEC(&md) && md.off == 0 && md.inum != 0) {
			struct solib_list *tmp = xmalloc(sizeof(struct solib_list));

			tmp->start_addr = md.lo;
			tmp->end_addr = md.hi;
			tmp->path = strdup(md.path);
			tmp->next = so;
			so = tmp;
		}
	}
	maps_finish(&md);
	*solist = so;
}

static void free_solib(struct solib_list *solib)
{
	if (solib->path)
		free(solib->path);
	free(solib);
}

void free_all_solibs(struct process *proc)
{
	struct solib_list *tmp = proc->shared->solib_list, *tmp2;

	while (tmp) {
		tmp2 = tmp;
		tmp = tmp->next;
		free_solib(tmp2);
	}
	proc->shared->solib_list = NULL;
}

static void solib_read_library(struct process *proc, char *filename,
			       addr_t start_addr, new_sym_t callback)
{
	bfd *abfd;
	long number_of_symbols;
	asymbol *sym, **symbol_table;
	addr_t symaddr;
	const flagword flags = BSF_FUNCTION;

	/* Do not read symbols from the target program itself. */
	if (strcmp(proc->filename, filename) == 0)
		return;

	/* Do not read symbols from the dynamic linker.
	   FIXME: ideally this should be done on the callback, but filtering it
	   here is avoids reading the library altogether. On future, it should
	   match the library SONAME instead of the path. */
	if (strcmp(filename, "/lib/ld-2.5.so") == 0)
		return;

	abfd = proc->solib->open(proc->solib, filename);
	if (abfd == NULL) {
		return;
	}
	number_of_symbols = proc->solib->read_symbols(abfd, &symbol_table);
	if (number_of_symbols) {
		int i;
		for (i = 0; i < number_of_symbols; i++) {
			sym = symbol_table[i];
			if ((sym->flags & flags) == flags) {
				symaddr = sym->value + sym->section->vma;
				/* Ignore symbols with no defined address. */
				if (symaddr == 0)
					continue;
				if (!solib_is_prelinked(abfd))
					symaddr += start_addr;
				if (is_thumb_func(sym))
					symaddr |= 1;
				/* Bfd symbols are section relative. */
				/* FIXME: pass SONAME instead of library filename. */
				callback(proc, filename, sym->name, symaddr);
			}
		}
		proc->solib->free_symbols(symbol_table);
	}
	if (!proc->solib->close(proc->solib, abfd))
		error_bfd(filename, "could not close file");
}

/* Based on update_solib_list() code from GDB 6.6 (gdb/solib.c). */
void solib_update_list(struct process *proc, new_sym_t callback)
{
	struct solib_list *cur_sos;
	struct solib_list *k = proc->shared->solib_list;
	struct solib_list **k_link = &proc->shared->solib_list;
	struct callback *cb = cb_get();
	current_solibs(proc, &cur_sos);
	while (k) {
		struct solib_list *c = cur_sos;
		struct solib_list **c_link = &cur_sos;

		while (c) {
			if (strcmp(k->path, c->path) == 0)
				break;
			c_link = &c->next;
			c = *c_link;
		}
		if (c) {
			/* solib is already known */
			/* equivalent to: parent->next = current->next */
			*c_link = c->next;
			free_solib(c);
			k_link = &k->next;
			k = *k_link;
		} else {
			/* solib was unloaded */
			debug(3, "solib unloaded: start=0x%x, end=0x%x, \
			      name=%s", k->start_addr, k->end_addr, k->path);
			*k_link = k->next;
			free_solib(k);
			k = *k_link;
		}
	}
	if (cur_sos) {
		struct solib_list *c;

		*k_link = cur_sos;
		for (c = cur_sos; c; c = c->next) {
			/* solib was loaded */
			debug(3, "solib loaded: start=0x%08x, end=0x%08x, \
			      name=%s", c->start_addr, c->end_addr, c->path);
			if (cb && cb->library.load)
				cb->library.load(proc, c->start_addr,
						 c->end_addr, c->path);
			if (filter_validate(c->path)) {
				solib_read_library(proc, c->path, c->start_addr, callback);
			}
		}
	}
}

/**
 * Symbol access functions for normal mode
 */
static struct solib_data solib_data_default = {
	.open = solib_open,
	.close = solib_close,
	.read_symbols = solib_read_symbols,
	.free_symbols = (void(*)(asymbol **))free,
};

/**
 * Symbol access functions for audit mode
 */
static struct solib_data solib_data_debug = {
	.open = solib_debug_open,
	.close = solib_debug_close,
	.read_symbols = solib_debug_read_symbols,
	.free_symbols = (void(*)(asymbol **))free,
};

void solib_initialize(struct process *proc)
{
	proc->solib = arguments.audit ? &solib_data_debug : &solib_data_default;
}
