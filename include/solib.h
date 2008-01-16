#ifndef FTK_SOLIB_H
#define FTK_SOLIB_H

#include "process.h"
#include "target_mem.h"

struct library_symbol {
	char *name;
	addr_t enter_addr;
	struct library_symbol *next;
};

struct solib_list {
	addr_t base_addr;
	char *path;
	struct library_symbol *symbols;
	struct solib_list *next;
};

extern void solib_update_list(struct process *proc);
extern addr_t solib_dl_debug_address(struct process *proc);
extern struct library_symbol *solib_read_symbols(char *filename);

#endif /* !FTK_SOLIB_H */
