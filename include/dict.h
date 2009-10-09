/*
 * This file is part of Functracer.
 *
 * Copyright (C) 1997-2007 Juan Cespedes <cespedes@debian.org>
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
 * Based on dictionary code from ltrace.
 */

/*
  Dictionary based on code by Morten Eriksen <mortene@sim.no>.
*/

struct dict;

extern struct dict *dict_init(unsigned int (*key2hash) (void *),
			      int (*key_cmp) (void *, void *));
extern void dict_clear(struct dict *d);
extern int dict_enter(struct dict *d, void *key, void *value);
extern void *dict_find_entry(struct dict *d, void *key);
extern void dict_apply_to_all(struct dict *d,
			      void (*func) (void *key, void *value, void *data),
			      void *data);

extern unsigned int dict_key2hash_string(void *key);
extern int dict_key_cmp_string(void *key1, void *key2);
extern unsigned int dict_key2hash_int(void *key);
extern int dict_key_cmp_int(void *key1, void *key2);
