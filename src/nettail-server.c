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

struct thread_info {
  int fd;
  int death;
};

static void*
die (void *arg)
{
  struct thread_info *ti = arg;

  char cmd[80];
  read (ti->fd, cmd, sizeof (cmd));
  printf ("Death in function\n");
  if (strcmp (cmd, "die") == 0)
    ti->death = 1;

  return;
}

int
main (int argc, char *argv[])
{
  int listenfd = 0, connfd = 0;

  struct sockaddr_in serv_addr;

  char sendBuff[1025];

  listenfd = socket (AF_INET, SOCK_STREAM, 0);
  printf ("socket retrieve success\n");

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);

  int inotify_fd = inotify_init ();

  if (inotify_fd == -1)
  {
    perror ("inotify:");
    exit (EXIT_FAILURE);
  }

  serv_addr.sin_port = htons (5000);

  bind (listenfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr));

  if (listen (listenfd, 10) == -1)
  {
    printf ("Failed to listen\n");
    return -1;
  }

  char recv_buf[PATH_MAX];
  char filename_requested[PATH_MAX];
  int wd;

  connfd = accept (listenfd, (struct sockaddr *) NULL, NULL);   // accept awaiting request

  pthread_t thread_id;
  struct thread_info ti;
  ti.death = 0;
  ti.fd = connfd;

  pthread_create(&thread_id, NULL, &die, &ti);

  read (connfd, filename_requested, sizeof (filename_requested));

  printf ("%s filename\n", filename_requested);

  wd = inotify_add_watch (inotify_fd, filename_requested, IN_MODIFY);

  if (wd == -1)
  {
    perror ("inotify:");
    exit (EXIT_FAILURE);
  }

  ssize_t num_read;

  char line[LINE_BUF_SIZE];
  FILE *fp = fopen (filename_requested, "r");
  fseek (fp, -LINE_BUF_SIZE, SEEK_END);

  while (fgets (line, LINE_BUF_SIZE, fp) != NULL)
  {
    line[strlen (line) - 1] = '\0';
    write (connfd, line, strlen (line));
  }

  fseek (fp, 0, SEEK_END);

  int debug_ctr = 0;

  for (;;)
  {
    printf ("%d\n", debug_ctr++);

    num_read = read (inotify_fd, sendBuff, 1024);

    if (num_read > 0)
    {
      while (fgets (sendBuff, sizeof (sendBuff), fp) != NULL)
      {
        sendBuff[strlen (sendBuff) - 1] = '\0';
        write (connfd, sendBuff, LINE_BUF_SIZE);
      }

      fseek (fp, 0, SEEK_END);

      if (ti.death == 1)
      {
        fclose (fp);
        printf ("Death received\n");
        break;
      }
    }
  }

  close (connfd);
  sleep (1);

  return 0;
}
