/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 2008 Free Software Foundation, Inc.
 *
 * Contact: Eero Tamminen <eero.tamminen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
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
 * Based on backtrace code from GDB.
 */

#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

// GDB includes
#include "gdb/defs.h"
#include "gdb/block.h"
#include "gdb/call-cmds.h"
#include "gdb/exceptions.h"
#include "gdb/frame.h"
#include "gdb/gdbcore.h"
#include "gdb/regcache.h"
#include "gdb/symtab.h"
#include "gdb/symfile.h"
#include "gdb/target.h"
#include "gdb/top.h"
#include "gdb/arm-tdep.h"
#include "gdb/arm-linux-tdep.h"
#include "gdb/gdb_proc_service.h"
#include "gdb/gregset.h"
#include "gdb/inferior.h"
#include "gdb/solist.h"
#include "gdb/objfiles.h"
#include "gdb/exec.h"
//#include "gdb/cli/cli-decode.h" // for struct cmd_list_element
//#include "gdb/cli/cli-cmds.h"   // for setdebuglist

#include "backtrace.h"
#include "debug.h"
#include "maps.h"

#define FROM_TTY	0

static void local_gdb_init(void)
{
	getcwd(gdb_dirbuf, sizeof(gdb_dirbuf));
	current_directory = gdb_dirbuf;

	/* Set defaults for some global variables. */
	gdb_sysroot = "";
	gdb_stdout = stdio_fileopen(stdout);
	gdb_stderr = stdio_fileopen(stderr);
	gdb_stdlog = gdb_stderr;	/* for moment */
	gdb_stdtarg = gdb_stderr;	/* for moment */
	gdb_stdin = stdio_fileopen(stdin);
	gdb_stdtargerr = gdb_stderr;	/* for moment */
	gdb_stdtargin = gdb_stdin;	/* for moment */

	initialize_targets();	/* Setup target_terminal macros for utils.c */
	initialize_utils();	/* Make errors and warnings possible */
	initialize_all_files();
	//initialize_current_architecture ();
}

static int get_thread_id(ptid_t ptid)
{
	int tid = TIDGET(ptid);
	if (0 == tid)
		tid = PIDGET(ptid);
	return tid;
}
#define GET_THREAD_ID(PTID)	get_thread_id ((PTID));

/* Get the value of a particular register from the floating point
   state of the process and store it into regcache.  */
static void fetch_fpregister(int regno)
{
	int ret, tid;
	gdb_byte fp[ARM_LINUX_SIZEOF_NWFPE];

	/* Get the thread id for the ptrace call.  */
	tid = GET_THREAD_ID(inferior_ptid);

	/* Read the floating point state.  */
	ret = xptrace(PT_GETFPREGS, tid, 0, fp);
	if (ret < 0) {
		warning(_("Unable to fetch floating point register."));
		return;
	}

	/* Fetch fpsr.  */
	if (ARM_FPS_REGNUM == regno)
		regcache_raw_supply(current_regcache, ARM_FPS_REGNUM, fp + NWFPE_FPSR_OFFSET);

	/* Fetch the floating point register.  */
	if (regno >= ARM_F0_REGNUM && regno <= ARM_F7_REGNUM)
		supply_nwfpe_register(current_regcache, regno, fp);
}

/* Get the whole floating point state of the process and store it
   into regcache.  */
static void fetch_fpregs(void)
{
	int ret, regno, tid;
	gdb_byte fp[ARM_LINUX_SIZEOF_NWFPE];

	/* Get the thread id for the ptrace call.  */
	tid = GET_THREAD_ID(inferior_ptid);

	/* Read the floating point state.  */
	ret = xptrace(PT_GETFPREGS, tid, 0, fp);
	if (ret < 0) {
		warning(_("Unable to fetch the floating point registers."));
		return;
	}

	/* Fetch fpsr.  */
	regcache_raw_supply(current_regcache, ARM_FPS_REGNUM, fp + NWFPE_FPSR_OFFSET);

	/* Fetch the floating point registers.  */
	for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
		supply_nwfpe_register(current_regcache, regno, fp);
}

extern int arm_apcs_32;

/* Fetch a general register of the process and store into
   regcache.  */
static void fetch_register(int regno)
{
	int ret, tid;
	elf_gregset_t regs;

	/* Get the thread id for the ptrace call.  */
	tid = GET_THREAD_ID(inferior_ptid);

	ret = xptrace(PTRACE_GETREGS, tid, 0, &regs);
	if (ret < 0) {
		warning(_("Unable to fetch general register."));
		return;
	}

	if (regno >= ARM_A1_REGNUM && regno < ARM_PC_REGNUM)
		regcache_raw_supply(current_regcache, regno, (char *)&regs[regno]);

	if (ARM_PS_REGNUM == regno) {
		if (arm_apcs_32)
			regcache_raw_supply(current_regcache, ARM_PS_REGNUM, (char *)&regs[ARM_CPSR_REGNUM]);
		else
			regcache_raw_supply(current_regcache, ARM_PS_REGNUM, (char *)&regs[ARM_PC_REGNUM]);
	}

	if (ARM_PC_REGNUM == regno) {
		regs[ARM_PC_REGNUM] = ADDR_BITS_REMOVE(regs[ARM_PC_REGNUM]);
		regcache_raw_supply(current_regcache, ARM_PC_REGNUM, (char *)&regs[ARM_PC_REGNUM]);
	}
}

/* Fetch all general registers of the process and store into
   regcache.  */
static void fetch_regs(void)
{
	int ret, regno, tid;
	elf_gregset_t regs;

	/* Get the thread id for the ptrace call.  */
	tid = GET_THREAD_ID(inferior_ptid);

	ret = xptrace(PTRACE_GETREGS, tid, 0, &regs);
	if (ret < 0) {
		warning(_("Unable to fetch general registers."));
		return;
	}

	for (regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
		regcache_raw_supply(current_regcache, regno, (char *)&regs[regno]);

	if (arm_apcs_32)
		regcache_raw_supply(current_regcache, ARM_PS_REGNUM, (char *)&regs[ARM_CPSR_REGNUM]);
	else
		regcache_raw_supply(current_regcache, ARM_PS_REGNUM, (char *)&regs[ARM_PC_REGNUM]);

	regs[ARM_PC_REGNUM] = ADDR_BITS_REMOVE(regs[ARM_PC_REGNUM]);
	regcache_raw_supply(current_regcache, ARM_PC_REGNUM, (char *)&regs[ARM_PC_REGNUM]);
}

static void local_get_registers(int regno)
{
	if (-1 == regno) {
		fetch_regs();
		fetch_fpregs();
	} else {
		if (regno < ARM_F0_REGNUM || regno > ARM_FPS_REGNUM)
			fetch_register(regno);

		if (regno >= ARM_F0_REGNUM && regno <= ARM_FPS_REGNUM)
			fetch_fpregister(regno);
	}
}

static LONGEST local_xfer_partial(struct target_ops *ops, enum target_object
		object, const char *annex, gdb_byte *readbuf,
		const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
	pid_t pid = ptid_get_pid(inferior_ptid);

	switch (object) {
	case TARGET_OBJECT_MEMORY:
		{
			union {
				PTRACE_TYPE_RET word;
				gdb_byte byte[sizeof(PTRACE_TYPE_RET)];
			} buffer;
			ULONGEST rounded_offset;
			LONGEST partial_len;

			/* Round the start offset down to the next long word
			   boundary.  */
			rounded_offset = offset & -(ULONGEST) sizeof(PTRACE_TYPE_RET);

			/* Since ptrace will transfer a single word starting at that
			   rounded_offset the partial_len needs to be adjusted down to
			   that (remember this function only does a single transfer).
			   Should the required length be even less, adjust it down
			   again.  */
			partial_len = (rounded_offset + sizeof(PTRACE_TYPE_RET)) - offset;
			if (partial_len > len)
				partial_len = len;

			if (writebuf) {
				/* If OFFSET:PARTIAL_LEN is smaller than
				   ROUNDED_OFFSET:WORDSIZE then a read/modify write will
				   be needed.  Read in the entire word.  */
				if (rounded_offset < offset || (offset + partial_len < rounded_offset + sizeof(PTRACE_TYPE_RET)))
					/* Need part of initial word -- fetch it.  */
					buffer.word = xptrace(PT_READ_I, pid, (PTRACE_TYPE_ARG3) (long)
							      rounded_offset, 0);

				/* Copy data to be written over corresponding part of
				   buffer.  */
				memcpy(buffer.byte + (offset - rounded_offset), writebuf, partial_len);

				errno = 0;
				xptrace(PT_WRITE_D, pid, (PTRACE_TYPE_ARG3) (long)rounded_offset, buffer.word);
				if (errno) {
					/* Using the appropriate one (I or D) is necessary for
					   Gould NP1, at least.  */
					errno = 0;
					xptrace(PT_WRITE_I, pid, (PTRACE_TYPE_ARG3) (long)rounded_offset, buffer.word);
					if (errno)
						return 0;
				}
			}

			if (readbuf) {
				errno = 0;
				buffer.word = xptrace(PT_READ_I, pid, (PTRACE_TYPE_ARG3) (long)
						      rounded_offset, 0);
				if (errno)
					return 0;
				/* Copy appropriate bytes out of the buffer.  */
				memcpy(readbuf, buffer.byte + (offset - rounded_offset), partial_len);
			}

			return partial_len;
		}

	case TARGET_OBJECT_UNWIND_TABLE:
		return -1;

	case TARGET_OBJECT_AUXV:
		return -1;

	case TARGET_OBJECT_WCOOKIE:
		return -1;

	default:
		return -1;
	}

	return 0;
}

static struct target_ops local_ops;

static void local_ops_init(void)
{
	local_ops.to_shortname = "FIXME";
	local_ops.to_longname = "FIXME";
	local_ops.to_doc = "FIXME";
	local_ops.to_fetch_registers = local_get_registers;
	local_ops.to_xfer_partial = local_xfer_partial;
	local_ops.to_stratum = dummy_stratum;
	local_ops.to_has_memory = 1;
	local_ops.to_has_stack = 1;
	local_ops.to_has_registers = 1;
	local_ops.to_magic = OPS_MAGIC;

	push_target(&local_ops);
}

/* Link map info to include in an allocated so_list entry */
struct lm_info {
	/* Pointer to copy of link map from inferior.  The type is char *
	   rather than void *, so that we may use byte offsets to find the
	   various fields without the need for a cast.  */
	gdb_byte *lm;

	/* Amount by which addresses in the binary should be relocated to
	   match the inferior.  This could most often be taken directly
	   from lm, but when prelinking is involved and the prelink base
	   address changes, we may need a different offset, we want to
	   warn about the difference and compute it only once.  */
	CORE_ADDR l_addr;
};

static void error_bfd(const char *filename, const char *msg)
{
	fprintf(stderr, "\"%s\": %s: %s", filename, msg, bfd_errmsg(bfd_get_error()));
	exit(1);
}

static void warning_bfd(const char *filename, const char *msg)
{
	fprintf(stderr, "\"%s\": %s: %s", filename, msg, bfd_errmsg(bfd_get_error()));
}

static int is_prelinked(const char *filename)
{
	bfd *abfd;
	asection *sect;
	const bfd_format bfd_fmt = bfd_object;
	int prelinked = 0;

	abfd = bfd_fopen(filename, "default", "rb", -1);
	if (abfd == NULL)
		error_bfd(filename, "could not open as an executable file");
	if (!bfd_check_format(abfd, bfd_fmt)) {
		warning_bfd(filename, "not in executable format");
		goto close_abfd;
	}
	sect = bfd_get_section_by_name(abfd, ".gnu.prelink_undo");
	if (sect != NULL)
		prelinked = 1;

close_abfd:
	if (!bfd_close(abfd))
		error_bfd(filename, "could not close file");

	return prelinked;
}

static struct so_list *maps_current_sos(void)
{
	struct maps_data md;
	struct so_list *head = NULL;
	char *exec_path;
	pid_t pid = ptid_get_pid(inferior_ptid);

//      resolve_path(proc->filename, &exec_path);
	if (maps_init(&md, pid) == -1)
		return NULL;
	while (maps_next(&md) == 1) {
		if (MAP_EXEC(&md) && md.off == 0 && md.inum != 0
		    /*&& strcmp(md.path, exec_path) != 0 */ ) {
			struct so_list *tmp = xmalloc(sizeof(struct so_list));

			memset(tmp, 0, sizeof(struct so_list));
			tmp->lm_info = xmalloc(sizeof(struct lm_info));
			if (is_prelinked(md.path))
				tmp->lm_info->l_addr = 0;
			else
				tmp->lm_info->l_addr = md.lo;
			/* Nothing will ever check the cached copy of the link
			   map if we set l_addr. */
			tmp->lm_info->lm = NULL;
			strncpy(tmp->so_name, md.path, SO_NAME_MAX_PATH_SIZE - 1);
			tmp->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
			strncpy(tmp->so_original_name, md.path, SO_NAME_MAX_PATH_SIZE - 1);
			tmp->so_original_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
			tmp->next = head;
			head = tmp;
		}
	}
	maps_finish(&md);
	return head;
}

struct bt_data {
	ptid_t inferior_ptid;
};

static struct bt_data *current_btd = NULL;

static void save_gdb_context(struct bt_data *btd)
{
	if (btd == NULL)
		return;

	btd->inferior_ptid = inferior_ptid;
}

static void restore_gdb_context(struct bt_data *btd)
{
	if (current_btd == btd)
		return;

	save_gdb_context(current_btd);
	inferior_ptid = btd->inferior_ptid;
	current_btd = btd;
}

#if 0
/* XXX Debugging only function. */
static void print_sharedlibraries(void)
{
  struct so_list *so = NULL;	/* link map state variable */
  int header_done = 0;
  int addr_width;

  /* "0x", a little whitespace, and two hex digits per byte of pointers.  */
  addr_width = 4 + (TARGET_PTR_BIT / 4);

  printf_unfiltered("-----------------------------------------------\n");
  for (so = master_so_list(); so; so = so->next)
    {
      if (so->so_name[0])
	{
	  if (!header_done)
	    {
	      printf_unfiltered ("%-*s%-*s%-12s%s\n", addr_width, "From",
				 addr_width, "To", "Syms Read",
				 "Shared Object Library");
	      header_done++;
	    }

	  printf_unfiltered ("%-*s", addr_width,
			     so->textsection != NULL 
			       ? hex_string_custom (
			           (LONGEST) so->textsection->addr,
	                           addr_width - 4)
			       : "");
	  printf_unfiltered ("%-*s", addr_width,
			     so->textsection != NULL 
			       ? hex_string_custom (
			           (LONGEST) so->textsection->endaddr,
	                           addr_width - 4)
			       : "");
	  printf_unfiltered ("%-12s", so->symbols_loaded ? "Yes" : "No");
	  printf_unfiltered ("%s\n", so->so_name);
	}
    }
  if (master_so_list() == NULL)
    {
      printf_unfiltered (_("No shared libraries loaded at this time.\n"));
    }
  printf_unfiltered("-----------------------------------------------\n");
}
#endif

static const char *function_name(struct frame_info *fi)
{
	struct symbol *func;
	struct minimal_symbol *msymbol;
	CORE_ADDR addr;

	addr = get_frame_address_in_block(fi);
	msymbol = lookup_minimal_symbol_by_pc(addr);
	func = find_pc_function(addr);
	if (func) {
		if (msymbol != NULL
		    && (SYMBOL_VALUE_ADDRESS(msymbol)
			> BLOCK_START(SYMBOL_BLOCK_VALUE(func))))
			return DEPRECATED_SYMBOL_NAME(msymbol);
		else
			return DEPRECATED_SYMBOL_NAME(func);
	} else if (msymbol != NULL)
		return DEPRECATED_SYMBOL_NAME(msymbol);

	return "??";
}

int bt_backtrace(struct bt_data *btd, char **buffer, int size)
{
	struct frame_info *fi, *trailing;
	char buf[100];
	int n;

	restore_gdb_context(btd);
	solib_add(NULL, FROM_TTY, NULL, 1);

//	print_sharedlibraries();

	/* XXX Use flush_cached_frames() instead of reinit_frame_cache() ? */
	reinit_frame_cache();
	registers_changed();

	trailing = get_current_frame();
	if (!trailing)
		/* No stack. */
		return 0;

	for (fi = trailing, n = 0; fi && n < size; fi = get_prev_frame(fi), n++) {
		char *frame_pc = paddr_nz(get_frame_pc(fi));
		const char *fn_name = function_name(fi);
		snprintf(buf, sizeof(buf), "%s [0x%s]", fn_name, frame_pc);
		buffer[n] = strdup(buf);
	}

	return n;
}

#if 0
/* XXX Debugging only function. */
void set_frame_debug(int n)
{
	struct cmd_list_element *cl;

	for (cl = setdebuglist; cl; cl = cl->next) {
		if (strcmp(cl->name, "frame") == 0 && cl->var) {
			int *frame_debug = cl->var;
			*frame_debug = 1;
		}
	}
}
#endif

struct bt_data *bt_init(pid_t pid)
{
	char *filename;
	static int gdb_initialized = 0;
	struct bt_data *btd;

	debug(3, "bt_init(pid=%d)", pid);
	btd = xmalloc(sizeof(struct bt_data));
	btd->inferior_ptid = pid_to_ptid(pid);

	if (!gdb_initialized) {
		local_gdb_init();
		local_ops_init();
		//set_frame_debug(1);
		filename = name_from_pid(pid);
		if (catch_command_errors(exec_file_attach, filename, FROM_TTY, RETURN_MASK_ALL))
			catch_command_errors(symbol_file_add_main, filename, FROM_TTY, RETURN_MASK_ALL);
		free(filename);
		current_target_so_ops->current_sos = maps_current_sos;
		gdb_initialized = 1;
	}

	return btd;
}

void bt_finish(struct bt_data *btd)
{
	/* FIXME: free GDB resources. */
	free(btd);
}
