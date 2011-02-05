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

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

int main(void)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	int sockfds[2];
	socketpair(AF_UNIX, SOCK_STREAM, 0, sockfds);
	if (sockfds[0] > 0) close(sockfds[0]);
	if (sockfds[1] > 0) close(sockfds[1]);

	if (sockfd > 0) close(sockfd);

	return 0;
}
