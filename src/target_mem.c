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

#include <stdio.h>
#include <sys/ptrace.h>

#include "debug.h"
#include "target_mem.h"
#include "util.h"

#define WORD_SIZE	sizeof(long)

long trace_mem_readw(struct process *proc, addr_t addr)
{
	return xptrace(PTRACE_PEEKTEXT, proc->pid, (void *)addr, NULL);
}

void trace_mem_writew(struct process *proc, addr_t addr, long w)
{
	xptrace(PTRACE_POKETEXT, proc->pid, (void *)addr, (void *)w);
}

long trace_user_readw(struct process *proc, long offset)
{
	return xptrace(PTRACE_PEEKUSER, proc->pid, (void *)offset, NULL);
}

void trace_user_writew(struct process *proc, long offset, long w)
{
	xptrace(PTRACE_POKEUSER, proc->pid, (void *)offset, (void *)w);
}

static void trace_mem_io(struct process *proc, addr_t addr, void *buf, size_t count, int write)
{
	long w;
	unsigned int i, j;
	unsigned char *buf_bytes = buf;
	unsigned char *w_bytes = (unsigned char *)&w;

	debug(4, "trace_mem_io(pid=%d, addr=0x%x, buf=%p, count=%d, write=%d", proc->pid, addr, buf, count, write);
	for (i = 0; i < 1 + ((count - 1) / WORD_SIZE); i++) {
		w = trace_mem_readw(proc, addr + i * WORD_SIZE);
		debug(5, "trace_mem_io (read): w = 0x%08lx", w);
		for (j = 0; j < WORD_SIZE && i * WORD_SIZE + j < count; j++) {
			if (write)
				w_bytes[j] = buf_bytes[i * WORD_SIZE + j];
			else
				buf_bytes[i * WORD_SIZE + j] = w_bytes[j];
		}
		if (write) {
			trace_mem_writew(proc, addr + i * WORD_SIZE, w);
			debug(4, "trace_mem_io (write): w = 0x%08lx", w);
		}
	}
}

void trace_mem_read(struct process *proc, addr_t addr, void *buf, size_t count)
{
	trace_mem_io(proc, addr, buf, count, 0);
}

void trace_mem_write(struct process *proc, addr_t addr, void *buf, size_t count)
{
	trace_mem_io(proc, addr, buf, count, 1);
}

void trace_getregs(struct process *proc, void *regs)
{
	xptrace(PTRACE_GETREGS, proc->pid, NULL, regs);
}

void trace_setregs(struct process *proc, void *regs)
{
	xptrace(PTRACE_SETREGS, proc->pid, NULL, regs);
}

size_t trace_mem_readstr(struct process* proc, size_t addr, char* buffer, size_t size)
{
	int bytes = 0;
	unsigned char c = 0xff;
	int i = 0;
	int limit = size - 1;

	while (c) {
		size_t n = trace_mem_readw(proc, addr);
		addr += sizeof(char*);
		for (i = 0; i < 4; i++) {
			c = n & 0xff;
			if (!c) break;
			if (limit > bytes++) {
				*buffer++ = c;
			}
			n >>= 8;
		}
	}
	if (size) *buffer = '\0';
	return bytes;
}


size_t trace_mem_readwstr(struct process* proc, size_t addr, int* buffer, size_t size)
{
	int bytes = 0;
	int c = 0xff;
	int limit = size - 1;

	while ( (c = trace_mem_readw(proc, addr)) ) {
		addr += sizeof(int*);
		if (limit > bytes++) {
			*buffer++ = c;
		}
	}
	if (size) *buffer = L'\0';
	return bytes;
}

