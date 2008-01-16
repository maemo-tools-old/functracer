#include <stdio.h>
#include <sys/types.h>

#include "target_mem.h"

#define MAX_NALLOC	(32 * 1024)	/* maximum number of allocations */
#define MAX_BT_DEPTH	7		/* maximum backtrace depth */

struct rp_allocinfo {
	struct rp_allocinfo *next;
	addr_t addr;
	size_t size;
	char *backtrace[MAX_BT_DEPTH];
	int bt_depth;
};

struct bt_data;

struct rp_data {
	pid_t pid;
	FILE *fp;
	struct bt_data *btd;
	struct rp_allocinfo *allocs;
	long nallocs;
};

extern struct rp_data *rp_init(pid_t pid);
extern void rp_new_alloc(struct rp_data *rd, addr_t addr, size_t size);
extern void rp_dump(struct rp_data *rd);
extern void rp_finish(struct rp_data *rd);
