
/*
 *  Diverse Bristol midi routines.
 *  Copyright (c) by Nick Copeland <nickycopeland@hotmail.com> 1996,2012
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * socketToolKit.c
 *
 * Initiate socket at GUI side. The client side is within the file
 * clientSide.c
#define GLIBC_21
 */

#define _GNU_SOURCE
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <strings.h>
#include <stdio.h>
#include <sys/time.h>

#define SOCKTYPE SOCK_STREAM

#include "socketToolKit.h"

static int remote_socket_descriptor;

struct  sockaddr_un local_socket_addr;

/*
 * Set up a socket for the client programmes to talk to mixslab GUI.
 * This is called from the serverSide.c routines.
 */
int
open_remote_socket(name, port, listens, reqsig)
char		*name; /* service name, must be known */
int 		listens; /* Parameter for the number of connections accepted */
{
	struct	servent		*service, service_tmp;
	struct  sockaddr_in remote_socket_addr;

#ifdef NEW_DEBUG
	printf("open_remote_socket(%s, %i, %i, %i)\n",
		name, port, listens, reqsig);
#endif
	/*
	 * Open a socket with minimal error handling.
	 */
	while((remote_socket_descriptor = socket(PF_INET, SOCKTYPE, IPPROTO_TCP))
		== -1)
	{
		perror("remote_socket_descriptor");
	}

	service = &service_tmp;
	service->s_port = port;
	service->s_name = "bristol";

	/*
	 * zero the struct (fixed size)
	 */
	bzero((char *) &remote_socket_addr, sizeof (remote_socket_addr));

	/*
	 * Setup the structure values for subroutines.
	 */
	remote_socket_addr.sin_family = AF_INET;
	remote_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	remote_socket_addr.sin_port = htons(service->s_port);
	/*
	 * Bind the socket:
	 */
	if ((bind(remote_socket_descriptor, (struct sockaddr *) &remote_socket_addr,
		sizeof(remote_socket_addr))) < 0)
	{
		perror("open_remote_socket");
		printf("socket id %i\n", port);
		return(-1);
	}

	if (listen(remote_socket_descriptor, listens) < 0)
		printf("Cannot listen to socket\n");

	//fcntl(remote_socket_descriptor, F_SETFL, O_NONBLOCK);

	return(remote_socket_descriptor);
}

