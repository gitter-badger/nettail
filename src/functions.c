/*
 * functions.c
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

#include <nettail.h>
#include <functions.h>

/* WIP */
void*
die (void *arg)
{
  struct die_args *args = arg;

  int *connfd = args->connfd;
  FILE *fp = args->fp;

  int state;
  char recv_buf[LINE_BUF_SIZE];

  ssize_t cmd_read;

  recv_buf[0] = '\0';

  while (strcmp (recv_buf, "die") != 0)
  {
    /* FIXME: need error checking */
    cmd_read = read (*connfd, recv_buf, sizeof (recv_buf));
    recv_buf[cmd_read] = '\0';
    printf ("cmd = %s\n", recv_buf);

    if (strcmp (recv_buf, "die") == 0)
    {
      state = close (*connfd);
      if (state == -1)
      {
        perror ("close: connfd : die");
        exit (EXIT_CLOSE_FD_FAILURE);
      }

      state = fclose (fp);
      if (state == EOF)
      {
        perror ("fclose:");
        exit (EXIT_CLOSE_FD_FAILURE);
      }

      if (pthread_mutex_lock (&mtx_array) != 0)
        fprintf (stderr, "pthread_mutex_lock : error : die\n");

      args->threads_free[*args->tid] = 0;

      if (pthread_mutex_unlock (&mtx_array) != 0)
        fprintf (stderr, "pthread_mutex_unlock : error : die\n");

      printf ("tid %d is free\n", *args->tid);

      if (pthread_cancel (args->thread_id) != 0)
        fprintf (stderr, "pthread_cancel: error");

      return (void*)NULL;
    }
  }

  printf ("End of function reached\n");
  return (void*)NULL;

}

void*
tail_file (void *arg)
{
  struct tail *tail_reqs = arg;

  char filename_requested[PATH_MAX];

  int *inotify_fd = tail_reqs->inotify_fd;
  int *connfd = tail_reqs->connfd;

  read (*connfd, filename_requested, sizeof (filename_requested));

  printf ("%s filename\n", filename_requested);

  int wd = inotify_add_watch (*inotify_fd, filename_requested, IN_MODIFY);
  if (wd == -1)
  {
    perror ("inotify_add_watch");
    exit (EXIT_IN_FAILURE);
  }

  ssize_t num_read;

  char line[LINE_BUF_SIZE];

  FILE *fp = fopen (filename_requested, "r");
  if (fp == NULL)
  {
    perror ("fopen");
    exit (EXIT_OPEN_FD_FAILURE);
  }

  int state;

  state = fseek (fp, -LINE_BUF_SIZE, SEEK_END);
  if (state == -1)
  {
    perror ("fseek");
    exit (EXIT_FSEEK_FAILURE);
  }

  while (fgets (line, LINE_BUF_SIZE, fp) != NULL)
  {
    state = write (*connfd, line, strlen (line));
    if (state == -1)
    {
      perror ("write");
      exit (EXIT_WRITE_FD_FAILURE);
    }
  }

  state = fseek (fp, 0, SEEK_END);
  if (state == -1)
  {
    perror ("fseek");
    exit (EXIT_FSEEK_FAILURE);
  }

  int debug_ctr = 0;
  char recv_buf[LINE_BUF_SIZE];
  char sendBuff[LINE_BUF_SIZE];

  struct die_args args;

  args.connfd = connfd;
  args.fp = fp;
  args.threads_free = tail_reqs->threads_free;
  args.tid = &tail_reqs->tid;
  args.thread_id = pthread_self();

  pthread_t thread_id;

  pthread_create (&thread_id, NULL, die, &args);
  pthread_detach (thread_id);

  for (;;)
  {
    printf ("%d - connfd: %d\n", debug_ctr++, *connfd);
    num_read = read (*inotify_fd, sendBuff, LINE_BUF_SIZE);
    if (num_read > 0)
    {
      while (fgets (sendBuff, sizeof (sendBuff), fp) != NULL)
      {
        state = write (*connfd, sendBuff, LINE_BUF_SIZE);
        if (state == -1)
        {
          perror ("write");
          exit (EXIT_WRITE_FD_FAILURE);
        }
      }

      state = fseek (fp, 0, SEEK_END);
      if (state == -1)
      {
        perror ("fseek");
        exit (EXIT_FSEEK_FAILURE);
      }
    }
    else if (num_read == -1)
    {
      perror ("read");
      exit (EXIT_READ_FD_FAILURE);
    }
  }

  sleep (1);

  return (void*)NULL;
}
