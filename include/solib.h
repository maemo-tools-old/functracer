#ifndef FTK_SOLIB_H
#define FTK_SOLIB_H

#include "process.h"

struct solib_list {
	void *base_addr;
	char *path;
	struct solib_list *next;
};

#if 0
struct library_symbol {
	char *name;
	void *enter_addr;
	char needs_init;
	enum toplt plt_type;
	char is_weak;
	struct library_symbol *next;
};
#endif

extern void solib_update_list(struct process *proc);
extern void *solib_dl_debug_address(struct process *proc);

#endif /* !FTK_SOLIB_H */
