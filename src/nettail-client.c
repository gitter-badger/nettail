/*
 * nettail-client.c
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

#include "nettail.h"

bool kill_sent;

static void
hdl (int sig, siginfo_t *siginfo, void *context)
{
  printf ("SIGINT received");
  kill_sent = 1;
}

int
main (int argc, char *argv[])
{
  int sockfd = 0, n = 0;
  char recvBuff[LINE_BUF_SIZE];
  struct sockaddr_in serv_addr;

  memset (recvBuff, '0', sizeof (recvBuff));
  if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf ("\n Error : Could not create socket \n");
    return 1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (5000);

  /*
   * FIXME: Needs better checking of command line arguments
   */
  if (argc < 3)
  {
    fprintf (stderr, "Usage: %s, <ip> <remote filename>\n", argv[0]);
    return 1;
  }

  char lan_ip[24];
  strcpy (lan_ip, argv[1]);

  printf ("%s\n", lan_ip);
  serv_addr.sin_addr.s_addr = inet_addr (lan_ip);

  char remote_filename[PATH_MAX];

  if (strlen (argv[2]) > 2)
    strcpy (remote_filename, argv[2]);
  else
    fprintf (stderr, "Use a better filename\n");

  if (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) <
      0)
  {
    printf ("\n Error : Connect Failed \n");
    return 1;
  }

  if (write (sockfd, remote_filename, sizeof (remote_filename)) == -1)
    perror ("write");

  struct sigaction act;

  memset (&act, '\0', sizeof(act));

  act.sa_sigaction = &hdl;

  act.sa_flags = SA_SIGINFO;

  sigaction (SIGINT, &act, NULL);

  do
  {
    n = read (sockfd, recvBuff, sizeof (recvBuff));

    /* Adding NULL terminator required */
    recvBuff[n] = '\0';

    fprintf (stdout, "%s", recvBuff);
  } while (n > 0);

  if (n == -1)
  {
    perror ("read");
  }

  if (kill_sent)
  {
    write (sockfd, "die", 4);
    printf ("Received SIGINT\n");

  }

  //while (1)
   // sleep (10);

  return 0;
}
