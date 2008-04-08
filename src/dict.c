/*
 * This file is part of Functracer.
 *
 * Copyright (C) 1997-2007 Juan Cespedes <cespedes@debian.org>
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
 * Based on dictionary code from ltrace.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "debug.h"

/*
  Dictionary based on code by Morten Eriksen <mortene@sim.no>.
*/

#include "dict.h"

struct dict_entry {
	unsigned int hash;
	void *key;
	void *value;
	struct dict_entry *next;
};

/* #define DICTTABLESIZE 97 */
#define DICTTABLESIZE 997	/* Semi-randomly selected prime number. */
/* #define DICTTABLESIZE 9973 */
/* #define DICTTABLESIZE 99991 */
/* #define DICTTABLESIZE 999983 */

struct dict {
	struct dict_entry *buckets[DICTTABLESIZE];
	unsigned int (*key2hash) (void *);
	int (*key_cmp) (void *, void *);
};

struct dict *dict_init(unsigned int (*key2hash) (void *),
		       int (*key_cmp) (void *, void *))
{
	struct dict *d;
	int i;

	d = malloc(sizeof(struct dict));
	if (!d) {
		perror("malloc()");
		exit(1);
	}
	for (i = 0; i < DICTTABLESIZE; i++) {	/* better use memset()? */
		d->buckets[i] = NULL;
	}
	d->key2hash = key2hash;
	d->key_cmp = key_cmp;
	return d;
}

void dict_clear(struct dict *d)
{
	int i;
	struct dict_entry *entry, *nextentry;

	assert(d);
	for (i = 0; i < DICTTABLESIZE; i++) {
		for (entry = d->buckets[i]; entry != NULL; entry = nextentry) {
			nextentry = entry->next;
			free(entry);
		}
		d->buckets[i] = NULL;
	}
	free(d);
}

int dict_enter(struct dict *d, void *key, void *value)
{
	struct dict_entry *entry, *newentry;
	unsigned int hash = d->key2hash(key);
	unsigned int bucketpos = hash % DICTTABLESIZE;

	assert(d);
	newentry = malloc(sizeof(struct dict_entry));
	if (!newentry) {
		perror("malloc");
		exit(1);
	}

	newentry->hash = hash;
	newentry->key = key;
	newentry->value = value;
	newentry->next = NULL;

	entry = d->buckets[bucketpos];
	while (entry && entry->next)
		entry = entry->next;

	if (entry)
		entry->next = newentry;
	else
		d->buckets[bucketpos] = newentry;

	debug(3, "new dict entry at %p[%d]: (%p,%p)", (void *)d, bucketpos, key, value);
	return 0;
}

void *dict_find_entry(struct dict *d, void *key)
{
	unsigned int hash = d->key2hash(key);
	unsigned int bucketpos = hash % DICTTABLESIZE;
	struct dict_entry *entry;

	assert(d);
	for (entry = d->buckets[bucketpos]; entry; entry = entry->next) {
		if (hash != entry->hash) {
			continue;
		}
		if (!d->key_cmp(key, entry->key)) {
			break;
		}
	}
	return entry ? entry->value : NULL;
}

void
dict_apply_to_all(struct dict *d,
		  void (*func) (void *key, void *value, void *data), void *data)
{
	int i;

	if (!d) {
		return;
	}
	for (i = 0; i < DICTTABLESIZE; i++) {
		struct dict_entry *entry = d->buckets[i];
		while (entry) {
			func(entry->key, entry->value, data);
			entry = entry->next;
		}
	}
}

/*****************************************************************************/

unsigned int dict_key2hash_string(void *key)
{
	const char *s = (const char *)key;
	unsigned int total = 0, shift = 0;

	assert(key);
	while (*s) {
		total = total ^ ((*s) << shift);
		shift += 5;
		if (shift > 24)
			shift -= 24;
		s++;
	}
	return total;
}

int dict_key_cmp_string(void *key1, void *key2)
{
	assert(key1);
	assert(key2);
	return strcmp((const char *)key1, (const char *)key2);
}

unsigned int dict_key2hash_int(void *key)
{
	return (unsigned long)key;
}

int dict_key_cmp_int(void *key1, void *key2)
{
	return (uintptr_t)key1 - (uintptr_t)key2;
}
