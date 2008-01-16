#ifndef FTK_SYSDEPS_H
#define FTK_SYSDEPS_H

#include "process.h"
#include "target_mem.h"

extern int get_instruction_pointer(struct process *proc, addr_t *addr);
extern int set_instruction_pointer(struct process *proc, addr_t addr);

#endif /* FTK_SYSDEPS_H */
