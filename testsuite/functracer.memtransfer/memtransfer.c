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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

void test_char(void)
{
	char src[1024] = "source string", dst[1024], *ptr;
	int len = strlen(src) + 1; /* copy also terminator */

	strcpy(dst, src);
	memcpy(dst, src, len);
	memmove(dst, src, len);
	ptr = memccpy(dst, src, '\0', len);
	mempcpy(dst, src, len);
	memset(dst, 0, 100);
	strncpy(dst, src, 5);
	stpcpy(dst, src);
	strcat(dst, src);
	strncat(dst, src, 5);
	bcopy(src, dst, len);
	bzero(dst, 100);
	ptr = strdup(src);
	free(ptr);
	ptr = strndup(src, 5);
	free(ptr);
}

void test_wchar(void)
{
	wchar_t src[1024] = L"source string", dst[1024], *ptr;
	int len = wcslen(src) + 1; /* copy also terminator */
	wmemcpy(dst, src, len);
	wmempcpy(dst, src, len);
	wmemmove(dst, src, len);
	wmemset(dst, 0, 100);
	wcscpy(dst, src);
	wcsncpy(dst, src, 5);
	wcpcpy(dst, src);
	wcpncpy(dst, src, 5);
	wcscat(dst, src);
	wcsncat(dst, src, 5);
	len = wcsnlen(src, 5);
	ptr = wcsdup(src);
	free(ptr);
}

int main(void)
{
	test_char();
	test_wchar();
	return 0;
}
