/*
 * shmposix is a functracer module
 * Posix shared memory tracking module.
 *
 * This file is part of Functracer.
 *
 * Copyright (C) 2010 by Nokia Corporation
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <search.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sp_rtrace_formatter.h>
#include <sp_rtrace_defs.h>

#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"
#include "target_mem.h"
#include "context.h"

#define MODULE_API_VERSION "2.0"

static char module_api_version[] = MODULE_API_VERSION;

/* resource identifiers */
static sp_rtrace_resource_t res_pshmmap = {
	.type = "pshmmap",
	.desc = "posix shared memory object mapping",
	.flags = SP_RTRACE_RESOURCE_DEFAULT,
	.id = 1,
};

static sp_rtrace_resource_t res_fshmmap = {
	.type = "fshmmap",
	.desc = "file shared memory mapping",
	.flags = SP_RTRACE_RESOURCE_DEFAULT,
	.id = 2,
};

static sp_rtrace_resource_t res_shmmap = {
	.type = "shmmap",
	.desc = "generic shared memory mapping",
	.flags = SP_RTRACE_RESOURCE_DEFAULT,
	.id = 3,
};

static sp_rtrace_resource_t res_shmobj = {
	.type = "pshmobj",
	.desc = "posix shared memory objects",
	.flags = SP_RTRACE_RESOURCE_DEFAULT,
	.id = 4,
};

static sp_rtrace_resource_t res_shmfd = {
	.type = "pshmfd",
	.desc = "descriptors of opened posix shared memory objects",
	.flags = SP_RTRACE_RESOURCE_DEFAULT,
	.id = 5,
};


/*
 * Name registry implementation
 *
 * Name registry manages posix shared object name association with
 * unique (in context) resource identifier. The identifier is calculated
 * from the name hash and incremented if it conflicts with already
 * registered resource identifiers.
 */

/**
 * Name resgistry node
 */
typedef struct nreg_node_t {
	/* the object name */
	char* name;
	/* the unique resource identifier */
	unsigned int hash;
} nreg_node_t;

/* the name registry root*/
static void* nreg_root = NULL;

/* the following variables are used to store hash validation data */

/* the hash value to validate */
static unsigned int nreg_validate_hash_value = 0;
/* the hash validation result */
static bool nreg_validate_hash_result = false;
/**/

/**
 * Compares two registry nodes.
 *
 * @param[in] item1  the first node.
 * @param[in] item2  the second node.
 * @return           -1 - item1 < item2
 *                    0 - teim1 == item2
 *                    1 - item1 > item2
 */
static int nreg_compare_name(const void* item1, const void* item2)
{
	const nreg_node_t* node1 = item1;
	const nreg_node_t* node2 = item2;
	return strcmp(node1->name, node2->name);
}

/**
 * Releases resource allocated for name registry node.
 *
 * @param[in] item   the name registry node to free.
 */
static void nreg_free_node(void* item)
{
	nreg_node_t* pnode = (nreg_node_t*)item;
	if (pnode->name) free(pnode->name);
	free(pnode);
}

/**
 * Calculates hash from the specified string.
 *
 * @param[in] name   the name to calculate cash for.
 * @return           the calculated hash code.
 */
static unsigned int nreg_calc_raw_hash(const char* name)
{
	unsigned int hash = 0, i;
	size_t len = strlen(name);
	for (i = 0; i < len; i++) {
		hash += *name++;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/**
 * Validates hash code.
 *
 * This function is used as action for twalk() function. It checks if the
 * nreg_validate_hash_value conflicts with an existing hash value and sets
 * nreg_validate_hash_result to false in such case.
 * @param[in] item    the item to validate.
 * @param[in] which
 * @param[in] depth
 */
static void nreg_validate_hash(const void* item, const VISIT which, int depth)
{
	if (nreg_validate_hash_result) {
		switch (which) {
			case preorder:
				break;

			case postorder:
			case leaf: {
				const nreg_node_t* pnode = *(nreg_node_t**)item;
				if (pnode->hash == nreg_validate_hash_value) nreg_validate_hash_result = false;
				break;
			}
			case endorder:
				break;
		}
	}
}

/**
 * Checks if the specified hash isn't already registered.
 * @param[in] hash  the hash value to check.
 * @return          true if the hash value is already registered, false otherwise.
 */
static bool nreg_hash_exists(unsigned int hash)
{
	nreg_validate_hash_result = true;
	nreg_validate_hash_value = hash;
	twalk(nreg_root, nreg_validate_hash);
	return !nreg_validate_hash_result;
}

/**
 * Calculates hash value (resource identifier) for the specified name.
 *
 * @param[in] name   the name.
 * @return           the name hash code (resource identifier).
 */
static unsigned int nreg_calc_hash(const char* name)
{
	/* first calculate hash for the name string */
	unsigned int hash = nreg_calc_raw_hash(name);
	/* then ensure that it doesn't conflict with other objects */
	while (nreg_hash_exists(hash)) hash++;
	return hash;
}

/**
 * Retrieves hash for the specified name.
 *
 * If the specified name is already registered, it's hash code is
 * returned. Otherwise a new hash code is calculated, registered and
 * returned.
 * @param[in] name  the obejct name.
 * @return          the hash code.
 */
static unsigned int nreg_get_hash(const char* name)
{
	nreg_node_t node = {.name = (char*)name};

	nreg_node_t** ppnode = tfind(&node, &nreg_root, nreg_compare_name);
	if (!ppnode) {
		nreg_node_t* pnode = malloc(sizeof(nreg_node_t));
		if (!pnode) return -1;
		pnode->name = strdup(name);
		if (!pnode->name) return -1;
		pnode->hash = nreg_calc_hash(name);
		ppnode = tsearch(pnode, &nreg_root, nreg_compare_name);
	}
	return (*ppnode)->hash;
}

/**
 * Releases resources allocated by name registry.
 */
static void hash_cleanup() __attribute__((unused));

static void hash_cleanup()
{
	tdestroy(nreg_root, nreg_free_node);
}


/*
 * File descriptor registry implementation.
 *
 * File descriptor registry is used to track the opened file descriptors
 * so more information can be provided for mmap functions.
 */

/**
 * Descriptor type enumeration
 */
enum {
	FD_UNKNOWN = 0,
	/* posix shared memory object */
	FD_POSIX = 1,
	/* file */
	FD_FILE = 2,
};

/**
 * File descriptor registry node
 */
typedef struct fdreg_node_t {
	/* file descriptor type */
	unsigned int type;
	/* file/object name */
	char* name;
	/* the decriptor value */
	int fd;
	/* the file/object open mode */
	int mode;
} fdreg_node_t;

/* the file descriptor registry root node */
static void* fdreg_root;

/**
 * Releases resources allocated for file descriptor registry node.
 *
 * @param[in] item   the file descriptor registry node.
 */
static void fdreg_free_node(void* item)
{
	fdreg_node_t* pnode = item;
	if (pnode->name) free(pnode->name);
	free(pnode);
}

/**
 * Compares two file descriptor registry nodes by their file descriptors.
 *
 * @param[in] item1  the first node.
 * @param[in] item2  the second node.
 * @return           -1 - item1 < item2
 *                    0 - teim1 == item2
 *                    1 - item1 > item2
 */
static int fdreg_compare_fd(const void* item1, const void* item2)
{
	const fdreg_node_t* node1 = item1;
	const fdreg_node_t* node2 = item2;
	return node1->fd - node2->fd;
}

/**
 * Stores file descriptor into resgitry.
 *
 * Existing file descriptor data in registry is overwritten.
 * @param[in] fd     the file descriptor value.
 * @param[in] name   the name associated with the descriptor.
 * @param[in] type   the file descriptor type (FD_POSIX | FD_FILE).
 * @param[in] mode   the file descriptor open mode.
 */
static void fdreg_store_fd(int fd, const char* name, unsigned int type, int mode)
{
	fdreg_node_t node = {.fd = fd};
	fdreg_node_t** ppnode = tfind(&node, &fdreg_root, fdreg_compare_fd);
	if (ppnode) {
		/* file descriptor was already registered, update it's data */
		fdreg_node_t* pnode = *ppnode;
		if (pnode->name) free(pnode->name);
		pnode->name = strdup(name);
		pnode->type = type;
		pnode->mode = mode;
		return;
	}
	/* store the file descriptor into registry */
	fdreg_node_t* pnode = malloc(sizeof(fdreg_node_t));
	if (!pnode) return;
	pnode->name = strdup(name);
	pnode->type = type;
	pnode->fd = fd;
	pnode->mode = mode;
	tsearch(pnode, &fdreg_root, fdreg_compare_fd);
}

/**
 * Retrieves file descriptor data from the registry.
 * @param[in] fd  the file descriptor to retrieve data for.
 * @return        the file descriptor data or NULL.
 */
static fdreg_node_t* fdreg_get_fd(int fd)
{
	fdreg_node_t node = {.fd = fd};
	fdreg_node_t** ppnode = tfind(&node, &fdreg_root, fdreg_compare_fd);
	if (ppnode) return *ppnode;
	return NULL;
}


/**
 * Removes file descriptor data from the registry.
 *
 * @param[in] fd  the file descriptor.
 */
static void fdreg_remove(int fd) __attribute__((unused));

static void fdreg_remove(int fd)
{
	fdreg_node_t node = {.fd = fd};
	fdreg_node_t** ppnode = tfind(&node, &fdreg_root, fdreg_compare_fd);
	if (ppnode) {
		fdreg_free_node(*ppnode);
		tdelete(&node, &fdreg_root, fdreg_compare_fd);
	}
}

/**
 * Releases resources allocated by file descriptor registry.
 */
static void fdreg_cleanup() __attribute__((unused));

static void fdreg_cleanup()
{
	tdestroy(fdreg_root, fdreg_free_node);
}
/**/

/*
 * Addrses mapping registry
 *
 * The address mapping registry is used to associate descriptors mapped
 * by mmap() function with returned addresses.
 */

/*
 * The address mapping node.
 */
typedef struct addr_node_t {
	/* the mapped address */
	addr_t addr;
	/* the associated file descriptor */
	int fd;
} addr_node_t;

/* the address mapping root */
static void* addr_root;

/**
 * Compares two address mapping registry nodes.
 *
 * @param item1
 * @param item2
 * @return
 */
static int addr_compare(const void* item1, const void* item2)
{
	const addr_node_t* node1 = item1;
	const addr_node_t* node2 = item2;
	if (node1->addr < node2->addr) return -1;
	if (node1->addr > node2->addr) return 1;
	return 0;
}

/**
 * Stores new address mapping into registry.
 *
 * @param[in] addr   the new address.
 * @param[in] fd     the mapped descriptor.
 */
static void addr_store(addr_t addr, int fd)
{
	addr_node_t node = {.addr = addr};
	addr_node_t** ppnode = tfind(&node, &addr_root, addr_compare);
	if (ppnode) {
		addr_node_t* pnode = *ppnode;
		pnode->fd = fd;
		return;
	}
	addr_node_t* pnode = malloc(sizeof(addr_node_t));
	if (!pnode) return;
	pnode->addr = addr;
	pnode->fd = fd;
	tsearch(pnode, &addr_root, addr_compare);
}

/**
 * Retrieves address mapping data from registry.
 *
 * @param[in] addr    the address to retrieve data for.
 * @return            the mapping data or NULL.
 */
static addr_node_t* addr_get(addr_t addr)
{
	addr_node_t node = {.addr = addr};
	addr_node_t** ppnode = tfind(&node, &addr_root, addr_compare);
	if (ppnode) return *ppnode;
	return NULL;
}

/**
 * Releases resources allocated by adress mapping registry.
 *
 * @return
 */
static void addr_cleanup() __attribute__((unused));

static void addr_cleanup()
{
	tdestroy(addr_root, free);
}
/**/

static void module_function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;
	int is_free = 0;
	assert(proc->rp_data != NULL);
	addr_t rc = fn_return_value(proc);
	
	if (false);
	else if (strcmp(name, "shm_open") == 0) {
		if (rc == -1) return;
		char arg_name[PATH_MAX]; trace_mem_readstr(proc, fn_argument(proc, 0), arg_name, sizeof(arg_name));
		addr_t oflag = fn_argument(proc, 1);
		char arg_oflag[16]; snprintf(arg_oflag, sizeof(arg_oflag), "0x%lx", (unsigned long)oflag);
		char arg_mode[16]; snprintf(arg_mode, sizeof(arg_mode), "0x%lx", fn_argument(proc, 2));

		fdreg_store_fd(rc, arg_name, FD_POSIX, oflag);
		
		if (oflag & O_CREAT) {
			sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.res_type = (void*)res_shmobj.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
				.name = "shm_open",
				.res_id = (pointer_t)nreg_get_hash(arg_name),
				.res_size = (size_t)1,
				.index = rd->rp_number++,
			};
			sp_rtrace_print_call(rd->fp, &call);

			sp_rtrace_farg_t args[] = {
				{.name="name", .value=arg_name},
				{.name="oflag", .value=arg_oflag},
				{.name="mode", .value=arg_mode},
				{0}
			};
			sp_rtrace_print_args(rd->fp, args);

			if (arguments.enable_free_bkt) rp_write_backtraces(proc);
			else sp_rtrace_print_comment(rd->fp, "\n");

		}
		sp_rtrace_fcall_t call = {
			.type = SP_RTRACE_FTYPE_ALLOC,
			.context = context_mask,
			.timestamp = RP_TIMESTAMP,
			.res_type = (void*)res_shmfd.type,
			.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			.name = "shm_open",
			.res_id = (pointer_t)rc,
			.res_size = (size_t)1,
			.index = rd->rp_number,
		};
		sp_rtrace_print_call(rd->fp, &call);

		sp_rtrace_farg_t args[] = {
			{.name="name", .value=arg_name},
			{.name="oflag", .value=arg_oflag},
			{.name="mode", .value=arg_mode},
			{0}
		};
		sp_rtrace_print_args(rd->fp, args);
	}
	else if (strcmp(name, "shm_unlink") == 0) {
		if (rc == -1) return;
		is_free = 1;
		char arg_name[PATH_MAX]; trace_mem_readstr(proc, fn_argument(proc, 0), arg_name, sizeof(arg_name));

		sp_rtrace_fcall_t call = {
			.type = SP_RTRACE_FTYPE_FREE,
			.context = context_mask,
			.timestamp = RP_TIMESTAMP,
			.res_type = (void*)res_shmobj.type,
			.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			.name = "shm_unlink",
			.res_id = (pointer_t)nreg_get_hash(arg_name),
			.res_size = (size_t)0,
			.index = rd->rp_number,
		};
		sp_rtrace_print_call(rd->fp, &call);
		
		sp_rtrace_farg_t args[] = {
			{.name="name", .value=arg_name},
			{0}
		};
		sp_rtrace_print_args(rd->fp, args);
	}
	else if (strcmp(name, "open") == 0) {
		if (rc == -1) return;
		char arg_name[PATH_MAX]; trace_mem_readstr(proc, fn_argument(proc, 0), arg_name, sizeof(arg_name));
		fdreg_store_fd(rc, arg_name, FD_FILE, fn_argument(proc, 1));
		return;
	}
	else if (strcmp(name, "open64") == 0) {
		if (rc == -1) return;
		char arg_name[PATH_MAX]; trace_mem_readstr(proc, fn_argument(proc, 0), arg_name, sizeof(arg_name));
		fdreg_store_fd(rc, arg_name, FD_FILE, fn_argument(proc, 1));
		return;
	}
	else if (strcmp(name, "creat") == 0) {
		if (rc == -1) return;
		char arg_name[PATH_MAX]; trace_mem_readstr(proc, fn_argument(proc, 0), arg_name, sizeof(arg_name));
		fdreg_store_fd(rc, arg_name, FD_FILE, O_CREAT|O_WRONLY|O_TRUNC);
		return;
	}
	/* handle mmap2 together with mmap as the offset is ignored by tracker */
	else if (strcmp(name, "mmap") == 0 || strcmp(name, "mmap2") == 0 || strcmp(name, "mmap64") == 0) {
		if ((int)rc == -1) return;
		addr_t fd = fn_argument(proc, 4);
		addr_store(rc, fd);
		addr_t flags = fn_argument(proc, 3);

		fdreg_node_t* pfd = fdreg_get_fd(fd);

		sp_rtrace_fcall_t call = {
			.type = SP_RTRACE_FTYPE_ALLOC,
			.context = context_mask,
			.timestamp = RP_TIMESTAMP,
			.res_type = (void*)(pfd ? (pfd->type == FD_POSIX ? res_pshmmap.type : res_fshmmap.type) : res_shmmap.type),
			.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			.name = "mmap",
			.res_id = (pointer_t)rc,
			.res_size = (size_t)fn_argument(proc, 1),
			.index = rd->rp_number,
		};
		sp_rtrace_print_call(rd->fp, &call);
		
		char arg_length[16]; snprintf(arg_length, sizeof(arg_length), "0x%lx", fn_argument(proc, 1));
		char arg_prot[16]; snprintf(arg_prot, sizeof(arg_prot), "0x%lx", fn_argument(proc, 2));
		char arg_flags[16]; snprintf(arg_flags, sizeof(arg_flags), "0x%x", flags);
		char arg_fd[16]; snprintf(arg_fd, sizeof(arg_fd), "0x%lx", (unsigned long)fd);
		char arg_offset[16]; snprintf(arg_offset, sizeof(arg_offset), "0x%lx", fn_argument(proc, 5));
		char arg_mode[16];
		sp_rtrace_farg_t args[] = {
			{.name="length", .value=arg_length},
			{.name="prot", .value=arg_prot},
			{.name="flags", .value=arg_flags},
			{.name="offset", .value=arg_offset},
			{.name="fd", .value=arg_fd},
			{0}, // reserved for fd name
			{0}, // reserved for fd mode
			{0}
		};
		if (flags & (MAP_ANONYMOUS | MAP_ANON)) {
			/* hide the fd argument for anonymous mappings */
			args[4].name = NULL;
			args[4].value = NULL;
		}
		else if (pfd) {
			args[5].name = "name";
			args[5].value = pfd->name;
			args[6].name = "mode";
			args[6].value = arg_mode;
			snprintf(arg_mode, sizeof(arg_mode), "0x%x", pfd->mode);
		}
		sp_rtrace_print_args(rd->fp, args);

	}
	else if (strcmp(name, "munmap") == 0) {
		addr_t addr = fn_argument(proc, 0);
		fdreg_node_t* pfd = NULL;
		addr_node_t* paddr = addr_get(addr);
		if (paddr) {
			pfd = fdreg_get_fd(paddr->fd);
		}

		sp_rtrace_fcall_t call = {
			.type = SP_RTRACE_FTYPE_FREE,
			.context = context_mask,
			.timestamp = RP_TIMESTAMP,
			.res_type = (void*)(pfd ? (pfd->type == FD_POSIX ? res_pshmmap.type : res_fshmmap.type) : res_shmmap.type),
			.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
			.name = "munmap",
			.res_id = (pointer_t)addr,
			.res_size = (size_t)0,
			.index = rd->rp_number,
		};
		sp_rtrace_print_call(rd->fp, &call);
		
		char arg_length[16]; snprintf(arg_length, sizeof(arg_length), "%ld", fn_argument(proc, 1));
		sp_rtrace_farg_t args[] = {
			{.name="length", .value=arg_length},
			{0}
		};
		sp_rtrace_print_args(rd->fp, args);
	}
	else if (strcmp(name, "close") == 0) {
		addr_t fd = fn_argument(proc, 0);
		fdreg_node_t* pfd = fdreg_get_fd(fd);
		if (pfd) {
			if (pfd->type == FD_POSIX) {
				is_free = 1;
				sp_rtrace_fcall_t call = {
					.type = SP_RTRACE_FTYPE_FREE,
					.context = context_mask,
					.timestamp = RP_TIMESTAMP,
					.res_type = (void*)(res_shmfd.type),
					.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
					.name = "close",
					.res_id = (pointer_t)fd,
					.res_size = (size_t)0,
					.index = rd->rp_number,
				};
				sp_rtrace_print_call(rd->fp, &call);
			}
			//fdreg_remove(fd);
		}
		if (!is_free) return;
	}
	else {
		msg_warn("unexpected function exit (%s)\n", name);
		return;
	}
	(rd->rp_number)++;
	if (!is_free || arguments.enable_free_bkt) {
		rp_write_backtraces(proc);
	}
	else {
		sp_rtrace_print_comment(rd->fp, "\n"); 
	}
}

static int module_library_match(const char *symname)
{
	return(
		strcmp(symname, "shm_open") == 0 ||
		strcmp(symname, "shm_unlink") == 0 ||
		strcmp(symname, "open") == 0 ||
		strcmp(symname, "open64") == 0 ||
		strcmp(symname, "creat") == 0 ||
		strcmp(symname, "mmap") == 0 ||
		strcmp(symname, "mmap2") == 0 ||
		strcmp(symname, "mmap64") == 0 ||
		strcmp(symname, "munmap") == 0 ||
		strcmp(symname, "close") == 0 ||
		false);
}

static void module_report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_pshmmap);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_fshmmap);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_shmmap);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_shmobj);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_shmfd);
}

struct plg_api *init()
{
	static struct plg_api ma = {
		.api_version = module_api_version,
		.function_exit = module_function_exit,
		.library_match = module_library_match,
		.report_init = module_report_init,
	};
	return &ma;
}
