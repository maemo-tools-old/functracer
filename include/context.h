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
 * @file contex.h
 * 
 * Function call context support.
 * 
 * Function call contexts are bit values, set by the target process
 * with libsp-rtrace1 context API to mark execution areas. In addition
 * to functions tracked by the loaded module, functracer also tracks 
 * the context management functions functions to provide context 
 * support in reports.
 */
#ifndef CONTEXT_H
#define CONTEXT_H

struct process;

/* the call context value */
extern int context_mask;

/**
 * Checks if the symbol name matches context handling function names.
 */
int context_match(const char *symname);

/**
 * Processes context handling functions.
 *
 * @return  0       if the context handling funciton was processed.
 *          EINVAL  the name doesn't match context handling function.
 */
int context_function_exit(struct process *proc, const char *name);

#endif
