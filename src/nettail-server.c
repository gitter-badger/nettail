/*
 * nettail-server.c
 *
 * Copyright (C) 2016  Andy Alt (andy400-dev@yahoo.com)
 *
 * This file is part of nettail
 * (https://github.com/andy5995/nettail/wiki)
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
 */

#include <nettail-server.h>

int
main (int argc, char *argv[])
{
  int listenfd = 0;

  struct sockaddr_in serv_addr;

  listenfd = socket (AF_INET, SOCK_STREAM, 0);
  if (listenfd == -1)
  {
    perror ("socket");
    exit (EXIT_SOCKET_FAILURE);
  }

  printf ("socket retrieve success\n");

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);

  /* initialize inotify, which is used to let us know a file has been
   * modified
   * FIXME: portability issue, consider using GAMIN, or adding support
   * for both
   */
  int inotify_fd = inotify_init ();
  if (inotify_fd == -1)
  {
    perror ("inotify");
    exit (EXIT_IN_FAILURE);
  }

  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s <port>\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  /* FIXME: check the number */

  ushort arg_port = 0;
  arg_port =  atoi (argv[1]);

  serv_addr.sin_port = htons (arg_port);

  int state;

  state = bind (listenfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
  if (state == -1)
  {
    perror ("bind");
    exit (EXIT_BIND_FAILURE);
  }

  if (listen (listenfd, 10) == -1)
  {
    printf ("Failed to listen\n");
    return -1;
  }

  pthread_t thread_id[THREAD_MAX];

  /* FIXME: THREAD_MAX not yet enforced */
  int connfd[THREAD_MAX];
  bool threads_free [THREAD_MAX];

  int i;
  for (i = 0; i < THREAD_MAX;i++)
    threads_free[i] = 0;

  struct tail tail_args[THREAD_MAX];

  while (1)
  {
    int tid;
    if (pthread_mutex_lock (&mtx_array) != 0)
      fprintf (stderr, "pthread_mutex_lock : error : main\n");
    for (i = 0; i < THREAD_MAX;i++)
    {
      if (threads_free[i] == 0)
      {
        threads_free[i] = 1;
        tid = i;
        printf ("tid %d/%d is now reserved for the next connection\n",
                tid, THREAD_MAX);
        break;
      }
      else if (i == THREAD_MAX - 1 && threads_free[i] == 1)
        printf ("No free threads! Max is %d\n", THREAD_MAX);
    }
    if (pthread_mutex_unlock (&mtx_array) != 0)
      fprintf (stderr, "pthread_mutex_unlock : error : main\n");

    /* accept awaiting request */
    connfd[tid] = accept (listenfd, (struct sockaddr *) NULL, NULL);
    if (connfd[tid] < 0)
    {
      perror ("accept");
      exit (EXIT_ACCEPT_FAILURE);
    }

    printf ("tid = %d\n", tid);
    tail_args[tid].inotify_fd = &inotify_fd;
    tail_args[tid].connfd = &connfd[tid];
    tail_args[tid].threads_free = threads_free;
    tail_args[tid].tid = tid;

    /* start a new thread, run tail_file() */
    state = pthread_create (&thread_id[tid], NULL, tail_file, &tail_args[tid]);
    if (state != 0)
    {
      fprintf (stderr, "Error: pthread_create");
      exit (EXIT_PTHREAD_CREATE_FAILURE);
    }

    state = pthread_detach (thread_id[tid]);
    if (state != 0)
      fprintf (stderr, "Error: pthread_detach");
  }
  return 0;
}
