/*
 * This file is part of Functracer.
 *
 * Copyright (C) 2011 by Nokia Corporation
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
#ifndef FILTER_H
#define FILTER_H

#include <stdbool.h>

/**
 * @file filter.h
 *
 * This file contains library filter API. It allows to specify a list
 * of libraries that must be monitored and provides validation against
 * it.
 * By default (if not initialized with library list or initialized with
 * a NULL string) the filter matches all libraries.
 */


/**
 * Initializes library filter from list.
 *
 * The library list contains comma separated list of libraries,
 * taken from -L option argument.
 * @param[in] libraries  the list of libraries
 */
void filter_initialize(const char *list);

/**
 * Frees resources allocated by filter
 */
void filter_free();

/**
 * Validates the specified library name against the filter.
 *
 * @param[in] library  the name to validate
 * @return             true if the library matches any of names in the filter.
 */
bool filter_validate(const char *library);



#endif
