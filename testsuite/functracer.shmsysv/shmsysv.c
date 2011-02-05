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

#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>


int main(void)
{
	int shmid = shmget(ftok("sp-rtrace/tests/shmseg_test", 1), 1024, IPC_CREAT | 0666);
	if (shmid != -1) {
		void *ptr = shmat(shmid, NULL, 0);
		void *ptr2 = shmat(shmid, NULL, 0);

		shmctl(shmid, IPC_RMID, NULL);

		if (ptr != (void*)-1) {
			shmdt(ptr);
		}
		if (ptr2 != (void*)-1) {
			shmdt(ptr2);
		}
	}
	return 0;
}