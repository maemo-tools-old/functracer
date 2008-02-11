#include <sys/ptrace.h>
#include <linux/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

// GDB includes
#include "backtrace_gdb/defs.h"
#include "backtrace_gdb/call-cmds.h"
#include "backtrace_gdb/exceptions.h"
#include "backtrace_gdb/frame.h"
#include "backtrace_gdb/gdbcore.h"
#include "backtrace_gdb/regcache.h"
#include "backtrace_gdb/symtab.h"
#include "backtrace_gdb/symfile.h"
#include "backtrace_gdb/target.h"
#include "backtrace_gdb/top.h"
#include "backtrace_gdb/arm-tdep.h"
#include "backtrace_gdb/arm-linux-tdep.h"
#include "backtrace_gdb/gdb_proc_service.h"
#include "backtrace_gdb/gregset.h"
#include "backtrace_gdb/inferior.h"
#include "backtrace_gdb/solist.h"
#include "backtrace_gdb/objfiles.h"
#include "backtrace_gdb/exec.h"

#include "backtrace.h"
#include "debug.h"
#include "maps.h"

static void indt_gdb_init(void)
{
	getcwd(gdb_dirbuf, sizeof(gdb_dirbuf));
	current_directory = gdb_dirbuf;

	/* INdT hacks */
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

/* On GNU/Linux, threads are implemented as pseudo-processes, in which
   case we may be tracing more than one process at a time.  In that
   case, inferior_ptid will contain the main process ID and the
   individual thread (process) ID.  get_thread_id () is used to get
   the thread id if it's available, and the process id otherwise.  */

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

static void get_indt_registers(int regno)
{
//      fprintf(stderr, "XXX DEBUG: %s(regno=%d)\n", __FUNCTION__, regno);
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

static LONGEST indt_xfer_partial(struct target_ops *ops, enum target_object
		object, const char *annex, gdb_byte *readbuf,
		const gdb_byte *writebuf, ULONGEST offset, LONGEST len)
{
	pid_t pid = ptid_get_pid(inferior_ptid);
//      fprintf(stderr, "XXX DEBUG: %s(pid=%d, object=%d, offset=%llx, len=%lld)\n", __FUNCTION__, pid, object, offset, len);

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

static struct target_ops indt_ops;

static void indt_ops_init(void)
{
	indt_ops.to_shortname = "FIXME";
	indt_ops.to_longname = "FIXME";
	indt_ops.to_doc = "FIXME";
	indt_ops.to_fetch_registers = get_indt_registers;
	indt_ops.to_xfer_partial = indt_xfer_partial;
	indt_ops.to_stratum = dummy_stratum;
	indt_ops.to_has_memory = 1;
	indt_ops.to_has_stack = 1;
	indt_ops.to_has_registers = 1;
	indt_ops.to_magic = OPS_MAGIC;

	push_target(&indt_ops);
}

static const char *function_name(struct frame_info *fi)
{
	struct minimal_symbol *msymbol = lookup_minimal_symbol_by_pc(get_frame_address_in_block(fi));

	if (msymbol != NULL)
		return DEPRECATED_SYMBOL_NAME(msymbol);
	return "";
}

static int missing_symbols(struct frame_info *trailing, int size)
{
	struct objfile *ofp;
#if 0
	struct partial_symtab *ps;
	struct frame_info *fi;
	int n;
#endif
	fprintf(stderr, "-----------------------\n");
	ALL_OBJFILES(ofp) {
		if (ofp->psymtabs == NULL && ofp->symtabs == NULL) {
			fprintf(stderr, "Warning: missing debug symbols for \"%s\", backtrace will not be generated\n", ofp->name);
//      return 1;
		}
	}
	fprintf(stderr, "-----------------------\n");
#if 0
	/* Read in symbols for all of the frames.  Need to do this in a
	   separate pass so that "Reading in symbols for xxx" messages
	   don't screw up the appearance of the backtrace.  Also if
	   people have strong opinions against reading symbols for
	   backtrace this may have to be an option.  */
	for (fi = trailing, n = 0; fi && n < size; fi = get_prev_frame(fi), n++) {
		ps = find_pc_psymtab(get_frame_address_in_block(fi));
		if (ps)
			PSYMTAB_TO_SYMTAB(ps);	/* Force syms to come in.  */
	}
#endif
	return 0;
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
#if 0
	fprintf(stderr, "XXX DEBUG 1\n");
	if (!bfd_check_format(abfd, bfd_fmt)) {
		fprintf(stderr, "XXX DEBUG 1.5\n");
		warning_bfd(filename, "not in executable format");
		goto close_abfd;
	}
	fprintf(stderr, "XXX DEBUG 2\n");
#endif
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
			fprintf(stderr, "XXX DEBUG: maps_current_sos(): md.path=\"%s\"\n", md.path);
			if (is_prelinked(md.path))
				tmp->lm_info->l_addr = 0;
			else
				tmp->lm_info->l_addr = md.lo;
			/* Nothing will ever check the cached copy of the link
			   map if we set l_addr. */
			tmp->lm_info->lm = NULL;
			strncpy(tmp->so_name, md.path, SO_NAME_MAX_PATH_SIZE - 1);
			tmp->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
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

int bt_backtrace(struct bt_data *btd, char **buffer, int size)
{
	struct frame_info *fi, *trailing;
	char buf[100];
	int n;

	restore_gdb_context(btd);
	fprintf(stderr, "XXX DEBUG: bt_backtrace(): pid=%d\n", ptid_get_pid(inferior_ptid));
	solib_add(NULL, 0, NULL, 1);
	// XXX use flush_cached_frames() instead of reinit_frame_cache() ?
	reinit_frame_cache();
	registers_changed();

	trailing = get_current_frame();
	if (!trailing)
		/* No stack. */
		return 0;
//      if (missing_symbols(trailing, size))
//              return 0;

	for (fi = trailing, n = 0; fi && n < size; fi = get_prev_frame(fi), n++) {
//      fprintf(stderr, "XXX DEBUG: func_addr = 0x%s\n", paddr_nz(frame_func_unwind(fi)));
//      if (!frame_func_unwind(fi))
//              fprintf(stderr, "XXX DEBUG: no symbols!\n");

		snprintf(buf, sizeof(buf), "%s [0x%s]", function_name(fi), paddr_nz(get_frame_pc(fi)));
		buffer[n] = strdup(buf);
	}

	return n;
}

extern int frame_debug;

struct bt_data *bt_init(pid_t pid)
{
	char *filename;
	static int gdb_initialized = 0;
	struct bt_data *btd;

	debug(3, "bt_init(pid=%d)", pid);
	btd = xmalloc(sizeof(struct bt_data));
	btd->inferior_ptid = pid_to_ptid(pid);

//      frame_debug = 1;
	if (!gdb_initialized) {
		indt_gdb_init();
		indt_ops_init();
		filename = name_from_pid(pid);
		if (catch_command_errors(exec_file_attach, filename, 0, RETURN_MASK_ALL))
			catch_command_errors(symbol_file_add_main, filename, 0, RETURN_MASK_ALL);
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
