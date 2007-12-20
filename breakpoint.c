#if 0
void enable_breakpoint(pid_t pid, struct breakpoint *sbp)
{
        unsigned int i, j;

        debug(1, "enable_breakpoint(%d,%p)", pid, sbp->addr);

        for (i = 0; i < 1 + ((BREAKPOINT_LENGTH - 1) / sizeof(long)); i++) {
                long a =
                    ptrace(PTRACE_PEEKTEXT, pid, sbp->addr + i * sizeof(long),
                           0);
                for (j = 0;
                     j < sizeof(long)
                     && i * sizeof(long) + j < BREAKPOINT_LENGTH; j++) {
                        unsigned char *bytes = (unsigned char *)&a;

                        sbp->orig_value[i * sizeof(long) + j] = bytes[j];
                        bytes[j] = break_insn[i * sizeof(long) + j];
                }
                ptrace(PTRACE_POKETEXT, pid, sbp->addr + i * sizeof(long), a);
        }
}

void disable_breakpoint(pid_t pid, const struct breakpoint *sbp)
{
        unsigned int i, j;

        debug(2, "disable_breakpoint(%d,%p)", pid, sbp->addr);

        for (i = 0; i < 1 + ((BREAKPOINT_LENGTH - 1) / sizeof(long)); i++) {
                long a =
                    ptrace(PTRACE_PEEKTEXT, pid, sbp->addr + i * sizeof(long),
                           0);
                unsigned char *bytes = (unsigned char *)&a;
                debug(1, "XXX breakpoint: pid=%d, addr=%p, current=0x%lx, orig_value=0x%lx", pid, sbp->addr, a, *(long *)&sbp->orig_value);
                for (j = 0;
                     j < sizeof(long)
                     && i * sizeof(long) + j < BREAKPOINT_LENGTH; j++) {

                        bytes[j] = sbp->orig_value[i * sizeof(long) + j];
                }
                ptrace(PTRACE_POKETEXT, pid, sbp->addr + i * sizeof(long), a);
        }
}
#endif

#if 0
void insert_function_bp(struct process *proc, const char *funcname)
{
	struct library_symbol *tmp = proc->list_of_symbols;

	while (tmp) {
		if (!strcmp(tmp->name, funcname)) {
			if (proc->cur_bkpt) {
				fprintf(stderr, "ERROR: pid %d already has a breakpoint enabled!\n", proc->pid);
				return;
			}
			proc->cur_bkpt = calloc(1, sizeof(struct breakpoint));
			if (!proc->cur_bkpt) {
				perror("calloc");
				exit(1);
			}
			proc->cur_bkpt->addr = tmp->enter_addr;
			proc->cur_bkpt->libsym = tmp;
			enable_breakpoint(proc, proc->cur_bkpt);
			proc->cur_bkpt->enabled = 1;
		}
	}
}
#endif

