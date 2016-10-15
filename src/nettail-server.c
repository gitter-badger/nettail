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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>
#include <pthread.h>

#include "include/nettail.h"    /* Need a Makefile */

struct thread_info
{
  int fd;
  int death;
};

/* arguments passed to pthread_create() */
struct tail
{
  int inotify_fd;
  int connfd;

};

static void*
die (void *arg);

/* tail_file: intended to run after a connection is accepted
 */
static void*
tail_file (void *arg);

int
main (int argc, char *argv[])
{
  int listenfd = 0;

  struct sockaddr_in serv_addr;

  listenfd = socket (AF_INET, SOCK_STREAM, 0);
  if (listenfd == -1)
  {
    perror ("socket:");
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
    perror ("inotify:");
    exit (EXIT_IN_FAILURE);
  }

  serv_addr.sin_port = htons (5000);

  int state;

  state = bind (listenfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
  if (state == -1)
  {
    perror ("bind:");
    exit (EXIT_BIND_FAILURE);
  }

  if (listen (listenfd, 10) == -1)
  {
    printf ("Failed to listen\n");
    return -1;
  }

  char recv_buf[PATH_MAX];

  pthread_t thread_id[THREAD_MAX];

  int connfd[THREAD_MAX];

  int thread_ctr = 0;

  /* WIP */
  while (1)
  {
    /* accept awaiting request */
    connfd[thread_ctr] = accept (listenfd, (struct sockaddr *) NULL, NULL);
    if (connfd[thread_ctr] < 0)
    {
      perror ("accept");
      exit (EXIT_ACCEPT_FAILURE);
    }

    struct tail tail_reqs;

    tail_reqs.inotify_fd = inotify_fd;
    tail_reqs.connfd = connfd[thread_ctr];

    /* start a new thread, run tail_file() */
    state = pthread_create (&thread_id[thread_ctr], NULL, tail_file, &tail_reqs);
    if (state != 0)
    {
      fprintf (stderr, "Error: pthread_create");
      exit (EXIT_PTHREAD_CREATE_FAILURE);
    }

    thread_ctr++;
  }
  return 0;
}

/* WIP */
static void*
die (void *arg)
{
  struct thread_info *ti = arg;

  char cmd[80];
  read (ti->fd, cmd, sizeof (cmd));
  printf ("Death in function\n");
  if (strcmp (cmd, "die") == 0)
    ti->death = 1;

  return (void*)NULL;
}

static void*
tail_file (void *arg)
{
  struct tail *tail_reqs = arg;

  char filename_requested[PATH_MAX];

  int inotify_fd = tail_reqs->inotify_fd;
  int connfd = tail_reqs->connfd;

  read (connfd, filename_requested, sizeof (filename_requested));

  printf ("%s filename\n", filename_requested);

  int wd = inotify_add_watch (inotify_fd, filename_requested, IN_MODIFY);
  if (wd == -1)
  {
    perror ("inotify_add_watch:");
    exit (EXIT_IN_FAILURE);
  }

  ssize_t num_read;

  char line[LINE_BUF_SIZE];

  FILE *fp = fopen (filename_requested, "r");
  if (fp == NULL)
  {
    perror ("fopen:");
    exit (EXIT_OPEN_FD_FAILURE);
  }

  int state;

  state = fseek (fp, -LINE_BUF_SIZE, SEEK_END);
  if (state == -1)
  {
    perror ("fseek:");
    exit (EXIT_FSEEK_FAILURE);
  }

  while (fgets (line, LINE_BUF_SIZE, fp) != NULL)
  {
    line[strlen (line) - 1] = '\0';
    state = write (connfd, line, strlen (line));
    if (state == -1)
    {
      perror ("write:");
      exit (EXIT_WRITE_FD_FAILURE);
    }
  }

  state = fseek (fp, 0, SEEK_END);
  if (state == -1)
  {
    perror ("fseek:");
    exit (EXIT_FSEEK_FAILURE);
  }

  int debug_ctr = 0;

  char sendBuff[1025];

  for (;;)
  {
    printf ("%d\n", debug_ctr++);

    num_read = read (inotify_fd, sendBuff, 1024);
    if (num_read > 0)
    {
      while (fgets (sendBuff, sizeof (sendBuff), fp) != NULL)
      {
        sendBuff[strlen (sendBuff) - 1] = '\0';

        state = write (connfd, sendBuff, LINE_BUF_SIZE);
        if (state == -1)
        {
          perror ("write:");
          exit (EXIT_WRITE_FD_FAILURE);
        }
      }

      state = fseek (fp, 0, SEEK_END);
      if (state == -1)
      {
        perror ("fseek:");
        exit (EXIT_FSEEK_FAILURE);
      }
    }
    else if (num_read == -1)
    {
      perror ("read:");
      exit (EXIT_READ_FD_FAILURE);
    }
  }

  state = close (connfd);
  if (state == -1)
  {
    perror ("close:");
    exit (EXIT_CLOSE_FD_FAILURE);
  }

  state = fclose (fp);
  if (state == EOF)
  {
    perror ("fclose:");
    exit (EXIT_CLOSE_FD_FAILURE);
  }

  sleep (1);

  return (void*)NULL;
}
