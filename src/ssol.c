/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
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
 */

#include <sys/ptrace.h>
#include <linux/ptrace.h>

#include <assert.h>
#include <errno.h>
#include <libiberty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/procfs.h>

#include "arch-defs.h"
#include "debug.h"
#include "ssol.h"
#include "syscall.h"
#include "target_mem.h"
#include "trace.h"
#include "util.h"

static long syscall_remote(struct process *proc, long sysnum, int argnum,
			   long *args)
{
	elf_gregset_t orig_regs, mmap_regs;
	elf_greg_t *orig_regs_p = (elf_greg_t *)&orig_regs;
	elf_greg_t *mmap_regs_p = (elf_greg_t *)&mmap_regs;
	long ret, retval;
	int status, i;
	const unsigned int ip_reg = syscall_data.ip_reg;
	const unsigned int sysnum_reg = syscall_data.sysnum_reg;
	const unsigned int retval_reg = syscall_data.retval_reg;
	char orig_insns[sizeof(syscall_data.insns)];

	/* only up to six arguments are supported */
	assert(argnum <= ARRAY_SIZE(syscall_data.regs));

	/* save original register values */
	memset(&orig_regs, 0, sizeof(elf_gregset_t));
	trace_getregs(proc, &orig_regs);

	/* save original instructions */
	trace_mem_read(proc, orig_regs_p[ip_reg], orig_insns,
		       sizeof(syscall_data.insns));

	/* set register values for system call */
	memcpy(&mmap_regs, &orig_regs, sizeof(elf_gregset_t));
	mmap_regs_p[sysnum_reg] = sysnum;
	for (i = 0; i < argnum; i++)
		mmap_regs_p[syscall_data.regs[i]] = args[i];
	trace_setregs(proc, &mmap_regs);

	/* set syscall instruction */
	trace_mem_write(proc, orig_regs_p[ip_reg], (char *)syscall_data.insns,
			sizeof(syscall_data.insns));

	/* execute syscall instruction and stop again */
	xptrace(PTRACE_CONT, proc->pid, 0, 0);

	/* wait for child to stop */
	ret = ft_waitpid(proc->pid, &status, __WALL);
	assert(ret == proc->pid && WIFSTOPPED(status) &&
	       WSTOPSIG(status) == SIGTRAP);

	/* get system call return value */
	retval = trace_user_readw(proc, 4 * retval_reg);

	/* restore original register values */
	trace_setregs(proc, &orig_regs);

	/* restore original instructions */
	trace_mem_write(proc, orig_regs_p[ip_reg], orig_insns,
			sizeof(syscall_data.insns));

	return retval;
}

static void *mmap_remote(struct process *proc, void *start, size_t length,
			 int prot, int flags, int fd, off_t offset)
{
	long args[] = { (long)start, (long)length, (long)prot, (long)flags,
			(long)fd, (long)offset };

	return (void *)syscall_remote(proc, SYS_mmap2, ARRAY_SIZE(args), args);
}

static int munmap_remote(struct process *proc, void *start, size_t length)
{
	long args[] = { (long)start, (long)length };

	/* Process is exiting. Return 0 immediately because the remote syscall
	 * will not work and the memory map will be destroyed anyway. */
	if (proc->exiting)
		return 0;
	return (int)syscall_remote(proc, SYS_munmap, ARRAY_SIZE(args), args);
}

#define SSOL_LENGTH	4096

addr_t ssol_new_slot(struct process *proc)
{
	struct ssol *ssol = proc->ssol;

	assert(ssol->last - ssol->first + 1 < SSOL_LENGTH);
	ssol->last += MAX_INSN_SIZE;
	return ssol->last;
}

void ssol_init(struct process *proc)
{
	debug(1, "pid=%d", proc->pid);
	if (proc->ssol == NULL)
		proc->ssol = xcalloc(1, sizeof(struct ssol));
	proc->ssol->first = (addr_t)mmap_remote(proc, NULL, SSOL_LENGTH,
		PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(proc->ssol->first > 0);
	proc->ssol->last = proc->ssol->first;
	debug(1, "mmap_remote() returned %#x", proc->ssol->first);
}

void ssol_finish(struct process *proc)
{
	int ret;

	if (proc->ssol == NULL)
		return;
	debug(1, "pid=%d", proc->pid);
	ret = munmap_remote(proc, (void *)proc->ssol->first, SSOL_LENGTH);
	debug(1, "munmap_remote() returned %d", ret);
	free(proc->ssol);
	proc->ssol = NULL;
}
