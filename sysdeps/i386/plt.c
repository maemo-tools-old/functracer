#include <gelf.h>

#include "elf.h"
#include "library.h"

GElf_Addr arch_plt_sym_val(struct ltelf *lte, size_t ndx, GElf_Rela *rela)
{
	return lte->plt_addr + (ndx + 1) * 16;
}

void *sym2addr(struct process *proc, struct library_symbol *sym)
{
	return sym->enter_addr;
}
