#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "backtrace.h"
#include "debug.h"
#include "report.h"

static unsigned step = 0;
 
static void rp_copy_file(const char *src, const char *dst)
{
	char line[256];
	FILE *fpi, *fpo;

	debug(3, "rp_copy_file(src=\"%s\", dst=\"%s\"", src, dst);

	fpi = fopen(src, "r");
	if (!fpi) {
		debug(1, "rp_copyfile(): fopen: \"%s\": %s", src, strerror(errno));
		return;
	}
	fpo = fopen(dst, "w");
	if (!fpo) {
		debug(1, "rp_copyfile(): fopen: \"%s\": %s", dst, strerror(errno));
		fclose(fpi);
		return;
	}
	while (fgets(line, sizeof(line), fpi)) {
		fputs(line, fpo);
	}
	fclose(fpo);
	fclose(fpi);
}

static void rp_copy_maps(pid_t pid)
{
	char src[256], dst[256];

	snprintf(src, sizeof(src), "/proc/%d/maps", pid);
	snprintf(dst, sizeof(dst), "%s/allocs-%d.%d.map", getenv("HOME"), pid, step);
	rp_copy_file(src, dst);
}

void rp_dump(struct rp_data *rd)
{
	struct rp_allocinfo *rai;
	int i = 0, j;
	char path[256];
	
	snprintf(path, sizeof(path), "%s/allocs-%d.%d.trace", getenv("HOME"), rd->pid, step);
	rd->fp = fopen(path, "w");
	if (rd->fp == NULL)
		error_exit("rp_dump(): fopen");

	fprintf(rd->fp, "information about %ld allocations\n", rd->nallocs);
	rai = rd->allocs;
	while (rai) {
		fprintf(rd->fp, "%d. block at %p with size %d\n", i++, rai->addr, rai->size);
		for (j = 0; j < rai->bt_depth; j++)
			fprintf(rd->fp, "   [%p]\n", rai->backtrace[j]);
		rai = rai->next;
	}
	fclose(rd->fp);
	rp_copy_maps(rd->pid);
	step++;
}

void rp_new_alloc(struct rp_data *rd, void *addr, size_t size)
{
	struct rp_allocinfo *rai;
	static int alloc_overflow = 0;

	debug(3, "rp_new_alloc(pid=%d, addr=%p, size=%d)", rd->pid, addr, size);
	if (rd->nallocs >= MAX_NALLOC) {
		if (!alloc_overflow) {
			debug(1, "maximum number of allocations (%d) reached, new allocations will be ignored!", MAX_NALLOC);
			alloc_overflow = 1;
		}
		return;
	}
	rai = calloc(1, sizeof(struct rp_allocinfo));
	if (rai == NULL)
		error_exit("rp_new_alloc(): calloc");
	rai->addr = addr;
	rai->size = size;
	rai->bt_depth = bt_backtrace(rd->btd, rai->backtrace, MAX_BT_DEPTH);
	rai->next = rd->allocs;
	rd->allocs = rai;
	rd->nallocs++;
}

struct rp_data *rp_init(pid_t pid)
{
	struct rp_data *rd;

	rd = calloc(1, sizeof(struct rp_data));
	if (rd == NULL)
		error_exit("rp_init(): calloc");
	rd->pid = pid;
	rd->btd = bt_init(pid);
	
	return rd;
}

void rp_finish(struct rp_data *rd)
{
	bt_finish(rd->btd);
	free(rd);
}
