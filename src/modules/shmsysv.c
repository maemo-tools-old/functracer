/*
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

/**
 * @file shmsysv.c
 *
 * Functracer shared memory tracking module.
 *
 * This module tracks shared memory segment creation/destruction by the target
 * process and the address attching/detaching operations.
 * 
 * When the last address is detached from the segment marked with SHM_DEST, the
 * segment is destroyed. As it's impossible to rerieve shared memory segment id
 * at this moment it has to be tracked locally. This is achieved by storing
 * address->shmid mapping into binary tree for all attached addresses by the
 * target process. 
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <sys/shm.h>
#include <errno.h>
#include <search.h>
#include <sp_rtrace_formatter.h>
#include <sp_rtrace_defs.h>


#include "debug.h"
#include "function.h"
#include "options.h"
#include "plugins.h"
#include "process.h"
#include "report.h"
#include "context.h"

#define SHMSYSV_API_VERSION "2.0"
#define RES_SIZE 1

static sp_rtrace_resource_t res_segment = {
		.id = 1,
		.type = "segment",
		.desc = "shared memory segment",
		.flags = SP_RTRACE_RESOURCE_REFCOUNT,
};

static sp_rtrace_resource_t res_address = {
		.id = 2,
		.type = "address",
		.desc = "shared memory attachments",
		.flags = SP_RTRACE_RESOURCE_DEFAULT,
};


#ifdef __amd64__
 #define IPC_64  0x00
#else
 #define IPC_64 	  0x100
#endif

static char api_version[] = SHMSYSV_API_VERSION;

/* binary tree node to store addr->shmid mapping */
typedef struct {
	int shmid;
	void* addr;
} addrmap_t;

#define TNODE(x) (*(addrmap_t**)x)

/* address->segment id mapping */		
static void* addr2shmid = NULL;

/**
 * Compares address mapping nodes.
 * 
 * The comparison is done by node address.
 * @param[in] node1   the first node.
 * @param[in] node2   the second node.
 * @return
 */
static int compare_nodes(const void* node1, const void* node2)
{
	addrmap_t* map1 = (addrmap_t*)node1;
	addrmap_t* map2 = (addrmap_t*)node2;
	return map1->addr - map2->addr;
}

/**
 * A hook, called after function exit.
 * @param proc
 * @param name
 */
static void function_exit(struct process *proc, const char *name)
{
	struct rp_data *rd = proc->rp_data;
	int is_free = 0;

	addr_t retval = fn_return_value(proc);
	assert(proc->rp_data != NULL);
	if (strcmp(name, "shmget") == 0) {
		int shmflg = fn_argument(proc, 2);
		if (retval == -1 || !(shmflg & IPC_CREAT)) {
			/* skip failure or calls without IPC_CREAT flag */
			return;
		}
		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "shmget",
				.res_size = fn_argument(proc, 1),
				.res_id = (pointer_t)retval,
				.res_type = (void*)res_segment.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);
	}
	else if (strcmp(name, "shmctl") == 0) {
		/*
		 * Handle situation when IPC_RMID command is issued to a segment with no attached addresses. In this case
		 * segment is destroyed and the event must be logged. So check for:
		 * 1) IPC_RMID command
		 * 2) if the command was successfull
		 * 3) if the following stat command for the segment fails with EIDRM (removed identifier) or EINVAL (invalid argument) error.
		 */
		int cmd = fn_argument(proc, 1);
		if (cmd != IPC_RMID || retval == -1) return;
		int shmid = fn_argument(proc, 0);
		struct shmid_ds ds;
		if (shmctl(shmid, IPC_STAT | IPC_64, &ds) != -1 || (errno != EIDRM && errno != EINVAL) ) return;

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "shmctl",
				.res_size = 0,
				.res_id = (pointer_t)shmid,
				.res_type = (void*)res_segment.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);
		is_free = 1;
	}
	else if (strcmp(name, "shmat") == 0) {
		if (retval == -1) return;

		int shmid = fn_argument(proc, 0);
		struct shmid_ds ds;
	
		if (shmctl(shmid, IPC_STAT | IPC_64, &ds) != 0) return;

		/* use segment information as memory attachment parameters */
		char shmid_s[100], cpid_s[100] = {0};
		sprintf(shmid_s, "0x%x", shmid);
		sprintf(cpid_s, "%d", ds.shm_cpid);
		int size = ds.shm_segsz;

		/* check if the segment was created by target process */
		if (ds.shm_cpid == proc->pid) {
			/* store addr->shmid mapping for addresses attached in target process to allow address->shmid lookup later */
			addrmap_t* node = (addrmap_t*)malloc(sizeof(addrmap_t));
			if (node == NULL) return;
			node->shmid = shmid;
			node->addr = (void*)retval;
			void* old_node = tfind((void*)node, &addr2shmid, compare_nodes);
			if (old_node) {
				tdelete(old_node, &addr2shmid, compare_nodes);
			}
			tsearch((void*)node, &addr2shmid, compare_nodes);
		}

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_ALLOC,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "shmat",
				.res_size = size,
				.res_id = (pointer_t)retval,
				.res_type = (void*)res_address.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);

		sp_rtrace_farg_t args[] = {
				{.name = "shmid", .value = shmid_s},
				{.name = "cpid", .value = cpid_s},
				{0}
		};
		sp_rtrace_print_args(rd->fp, args);
	}
	else if (strcmp(name, "shmdt") == 0) {
		if (retval == -1) return;

		is_free = 1;
		struct shmid_ds ds;
		addrmap_t node = {.addr = (void*)fn_argument(proc, 0)};

		sp_rtrace_fcall_t call = {
				.type = SP_RTRACE_FTYPE_FREE,
				.index = rd->rp_number,
				.context = context_mask,
				.timestamp = RP_TIMESTAMP,
				.name = "shmdt",
				.res_size = 0,
				.res_id = (pointer_t)node.addr,
				.res_type = (void*)res_address.type,
				.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
		};
		sp_rtrace_print_call(rd->fp, &call);

		/* if the address was attached by the target process (it's stored in the addr2shmid mapping) check if
		 * the segment is still valid. It might have been destroyed if it was marked with SHM_DEST and the last
		 * address detached. In this case an event must be logged. */ 
		addrmap_t* pnode = (addrmap_t*)tfind((void*)&node, &addr2shmid, compare_nodes);
		if (pnode) {
			/* Register segment deallocation event if the associated segment was removed after detach call */
			if (shmctl(TNODE(pnode)->shmid, IPC_STAT | IPC_64, &ds) == -1 && (errno == EIDRM || errno == EINVAL)) {
				/* write backtrace for address detachment event and increment event index */
				if (arguments.enable_free_bkt) rp_write_backtraces(proc);
				else sp_rtrace_print_comment(rd->fp, "\n"); 

				rd->rp_number++;
				/* write segment destroying event */
				sp_rtrace_fcall_t call = {
						.type = SP_RTRACE_FTYPE_FREE,
						.index = rd->rp_number,
						.context = context_mask,
						.timestamp = RP_TIMESTAMP,
						.name = "shmdt",
						.res_size = 0,
						.res_id = (pointer_t)TNODE(pnode)->shmid,
						.res_type = (void*)res_segment.type,
						.res_type_flag = SP_RTRACE_FCALL_RFIELD_NAME,
				};
				sp_rtrace_print_call(rd->fp, &call);
			}
			/* remove the address->segment mapping */
			tdelete((void*)pnode, &addr2shmid, compare_nodes);
			free(TNODE(pnode));
		}
	}
	rd->rp_number++;

	if (!is_free || arguments.enable_free_bkt) {
		rp_write_backtraces(proc);
	}
	else {
		sp_rtrace_print_comment(rd->fp, "\n"); 
	}
}

/**
 * Verifies if the specified symbol is tracked by the module.
 * @param symname
 * @return
 */
static int library_match(const char *symname)
{
	return(
			strcmp(symname, "shmget") == 0 ||
			strcmp(symname, "shmctl") == 0 ||
			strcmp(symname, "shmdt") == 0 ||
			strcmp(symname, "shmat") == 0);
}

/**
 * Registers tracked resources.
 * @param proc
 */
static void report_init(struct process *proc)
{
	assert(proc->rp_data != NULL);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_segment);
	sp_rtrace_print_resource(proc->rp_data->fp, &res_address);
}

/**
 * Initializes module
 * @return
 */
struct plg_api *init()
{
	static struct plg_api ma = {
		.api_version = api_version,
		.function_exit = function_exit,
		.library_match = library_match,
		.report_init = report_init,
	};
	return &ma;
}
