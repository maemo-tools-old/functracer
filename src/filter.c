#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "filter.h"

struct filter_data {
	char* name;
	struct filter_data *next;
};

static struct filter_data *filter_root = NULL;

/**
 * Frees filter item and the resources allocated by it.
 *
 * @param[in] filter   the filter item to free.
 */
static void filter_free_item(struct filter_data *filter)
{
	if (filter->name) free(filter->name);
	free(filter);
}


/**
 * Inserts a new item at the beginning of filter list.
 *
 * @param[in] name  the filter item name.
 */
static void filter_insert_item(const char *name)
{
	struct filter_data *filter = xmalloc(sizeof(struct filter_data));
	filter->next = filter_root;
	filter->name = strdup(name);
	if (!filter->name) {
		filter_free_item(filter);
	}
	else {
		filter_root = filter;
	}
}

void filter_initialize(const char *list)
{
	char* split;
	char* delim = ",";
	char buffer[PATH_MAX];
	strcpy(buffer, list);
	char* ptr = strtok_r(buffer, delim, &split);
	while (ptr) {
		filter_insert_item(ptr);
		ptr = strtok_r(NULL, delim, &split);
	}
}

void filter_free()
{
	while (filter_root) {
		struct filter_data *filter = filter_root->next;
		filter_free_item(filter_root);
		filter_root = filter;
	}
}


bool filter_validate(const char *library)
{
	if (!filter_root) return true;
	struct filter_data *filter = filter_root;
	while (filter) {
		if (strstr(library, filter->name)) return true;
		filter = filter->next;
	}
	return false;
}
