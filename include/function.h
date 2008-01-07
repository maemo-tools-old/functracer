#ifndef FTK_FUNCTION_H
#define FTK_FUNCTION_H

struct process;

extern long fn_argument(struct process *proc, int arg_num);
extern long fn_return_value(struct process *proc);
extern void fn_return_address(struct process *proc, void **addr);
extern void fn_save_arg_data(struct process *proc);
extern void fn_invalidate_arg_data(struct process *proc);

#endif /* !FTK_FUNCTION_H */
