#ifndef FTK_SYSDEPS_H
#define FTK_SYSDEPS_H

struct process;

extern int get_instruction_pointer(struct process *proc, void **addr);
extern int set_instruction_pointer(struct process *proc, void *addr);

#endif /* FTK_SYSDEPS_H */
