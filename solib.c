#include <bfd.h>
#include <errno.h>
#include <error.h>
#include <libiberty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "debug.h"
#include "elf.h"
#include "maps.h"
#include "solib.h"

static void warning_bfd(const char *filename, const char *msg)
{
	fprintf(stderr, "\"%s\": %s: %s", filename, msg, bfd_errmsg(bfd_get_error()));
}

static void error_bfd(const char *filename, const char *msg)
{
	error(EXIT_FAILURE, 0, "\"%s\": %s: %s", filename, msg,
	      bfd_errmsg(bfd_get_error()));
}

static void resolve_path(char *filename, char **real_filename)
{
	*real_filename = canonicalize_file_name(filename);
	if (*real_filename == NULL)
		error(EXIT_FAILURE, errno, "\"%s\": could not resolve file path",
		      filename);
}

static void find_solib(char *filename, char **real_filename)
{
	resolve_path(filename, real_filename);
}

static void lib_base_address(pid_t pid, char *filename, addr_t *addr)
{
	struct maps_data md;

	*addr = (addr_t)NULL;
	maps_init(&md, pid);
	while (maps_next(&md) == 1) {
		if (MAP_EXEC(&md) && strcmp(md.path, filename) == 0) {
			*addr = md.lo;
			break;
		}
	}
	maps_finish(&md);
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
				break;
			}
		}
		free(symbol_table);
	}
	return symaddr;
}

/* Based on enable_break() code from GDB 6.6 (gdb/solib-svr4.c). */
addr_t solib_dl_debug_address(struct process *proc)
{
	bfd *abfd;
	asection *sect;
	bfd_size_type sect_size;
	unsigned long sym_addr;
	char *buf, *interp_file = NULL;
	addr_t base_addr = 0;
	const bfd_format bfd_fmt = bfd_object;

	abfd = bfd_fopen(proc->filename, "default", "rb", -1);
	if (abfd == NULL)
		error_bfd(proc->filename, "could not open as an executable file");
	if (!bfd_check_format(abfd, bfd_fmt)) {
		warning_bfd(proc->filename, "not in executable format");
		goto close_abfd;
	}

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
	abfd = bfd_fopen(interp_file, "default", "rb", -1);
	if (abfd == NULL) {
		warning_bfd(interp_file, "could not open dynamic linker file");
		goto close_abfd;
	}
	if (!bfd_check_format(abfd, bfd_fmt)) {
		warning_bfd(interp_file, "not in shared file format");
		goto close_abfd;
	}
#if 0 /* XXX unnecessary? */
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
	lib_base_address(proc->pid, interp_file, &base_addr);

close_abfd:
	if (!bfd_close(abfd))
		error_bfd(interp_file, "could not close file");

	return (base_addr + sym_addr);
}

static void current_solibs(struct process *proc, struct solib_list **solist)
{
	struct maps_data md;
	struct solib_list *so = NULL;
	char *exec_path;

	resolve_path(proc->filename, &exec_path);
	maps_init(&md, proc->pid);
	while (maps_next(&md) == 1) {
		if (MAP_EXEC(&md) && md.off == 0 && md.inum != 0
		    && strcmp(md.path, exec_path) != 0) {
			struct solib_list *tmp = xmalloc(sizeof(struct solib_list));

			tmp->base_addr = md.lo;
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

static void solib_read_library(struct library_symbol **syms, char *filename, addr_t base_addr)
{
	bfd *abfd;
	long storage_needed, number_of_symbols;
	asymbol *sym, **symbol_table;
	addr_t symaddr;
	struct callback *cb = cb_get();
	const bfd_format bfd_fmt = bfd_object;
	const flagword flags = BSF_EXPORT | BSF_FUNCTION;

	abfd = bfd_fopen(filename, "default", "rb", -1);
	if (abfd == NULL)
		error_bfd(filename, "could not open shared library file");
	if (!bfd_check_format(abfd, bfd_fmt)) {
		warning_bfd(filename, "not in shared library format");
		goto err;
	}
	storage_needed = bfd_get_dynamic_symtab_upper_bound(abfd);
	if (storage_needed > 0) {
		int i;

		symbol_table = xmalloc((unsigned)storage_needed);
		number_of_symbols = bfd_canonicalize_dynamic_symtab(abfd, symbol_table);
		for (i = 0; i < number_of_symbols; i++) {
			sym = symbol_table[i];
			if ((sym->flags & flags) == flags && cb && cb->library.match(sym->name)) {
				struct library_symbol *tmp;

				/* Bfd symbols are section relative. */
				symaddr = base_addr + sym->value + sym->section->vma;
				debug(3, "new symbol: name=\"%s\", addr=0x%x", sym->name, symaddr);
				tmp = xmalloc(sizeof(struct library_symbol));
				tmp->name = strdup(sym->name);
				tmp->enter_addr = symaddr;
				tmp->next = *syms;
				*syms = tmp;
			}
		}
		free(symbol_table);
	}
err:
	if (!bfd_close(abfd))
		error_bfd(filename, "could not close file");
}

/* Based on update_solib_list() code from GDB 6.6 (gdb/solib.c). */
void solib_update_list(struct process *proc)
{
	struct solib_list *cur_sos;
	struct solib_list *k = proc->solib_list;
	struct solib_list **k_link = &proc->solib_list;

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
			debug(3, "solib unloaded: base=0x%x, name=%s", k->base_addr, k->path);
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
			debug(3, "solib loaded: base=0x%x, name=%s", c->base_addr, c->path);
			solib_read_library(&proc->symbols, c->path, c->base_addr);
		}
	}
}
