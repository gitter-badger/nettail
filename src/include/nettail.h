/*
 * nettail.h
 *
 * Copyright 2016 Andy <andy@oceanus>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

#ifndef NETAIL_INC_H
#define NETAIL_INC_H

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#define LINE_BUF_SIZE 512

#define THREAD_MAX 10

#define EXIT_PTHREAD_CREATE_FAILURE 2
#define EXIT_ACCEPT_FAILURE 3
#define EXIT_IN_FAILURE 4
#define EXIT_READ_FD_FAILURE 5
#define EXIT_WRITE_FD_FAILURE 6
#define EXIT_OPEN_FD_FAILURE 7
#define EXIT_CLOSE_FD_FAILURE 8
#define EXIT_FSEEK_FAILURE 9
#define EXIT_SOCKET_FAILURE 10
#define EXIT_BIND_FAILURE 11

#endif
