/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2011-2012 by Nokia Corporation
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

#include <stdlib.h>
#include <sp_rtrace_context.h>

int main(void)
{
	char* ptr = malloc(100);
	if (ptr == NULL) return -1;
	int ctx1 = sp_context_create("1st context");
	int ctx2 = sp_context_create("2nd context");
	sp_context_enter(ctx1);
	ptr = realloc(ptr, 200);
	if (ptr == NULL) return -1;
	sp_context_enter(ctx2);
	ptr = realloc(ptr, 300);
	if (ptr == NULL) return -1;
	sp_context_exit(ctx1);
	ptr = realloc(ptr, 400);
	if (ptr == NULL) return -1;
	sp_context_exit(ctx2);
	free(ptr);
	return 0;
}
