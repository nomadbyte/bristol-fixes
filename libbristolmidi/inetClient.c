
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

/* #define DEBUG */

/*
 * audioNetClient.c
 *
 * This will initialise an audio connection if an IP and port is specified. All
 * audio data is copied to this tap if it is active. Only use at the moment will
 * be the audio IO Oscilloscope. Should support copy of both input or output
 * data by selections.
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <strings.h>
#include <errno.h>

#define SERVICE "bristol"

#include "socketToolKit.h"

#define MAX_MTU 1024

#define SOCKTYPE SOCK_STREAM
#define	PROTOCOL "tcp"
/*
 * For the service name, defaults to "slab", and can be entered in services
 * file (although this is not implemented yet!).
 */
#define NAME_LENGTH 32

int		socket_descriptor = -1, recvFlag;

void	clientCheckSocket();

int
initControlPort(host, port)
char	*host;
{
struct	sockaddr_in	connect_socket_addr;
char				hostname[NAME_LENGTH];
struct	servent		*service, service_tmp;
struct	hostent		*hstp;
char 				*tport;

	/*
	 * Set up our default parameters.
	 */
	(void) gethostname(hostname, NAME_LENGTH);

	/* 
	 * See if we have been given new ones.
	 */
	if (host != (char *) NULL)
		strcpy(hostname, host);

	if (port <= 0)
		port = 5028;

	printf("hostname is %s, %s\n", hostname, SERVICE);

	if ((tport = index(hostname, ':')) != NULL)
	{
		*tport = '\0';

		if ((port = atoi(++tport)) <= 0)
			port = 5028;
	}

	/*
	 * Check out a few default signals. This should be dropped, since they were
	 * actually for some other programmes.
	 */
	if ((hstp = gethostbyname(hostname)) == NULL)
	{
		printf("could not resolve %s, defaulting to localhost\n", hostname);
		hstp = gethostbyname("localhost");
	}

	/*
	 * Get a socket in the inet domain
	 */
	if ((socket_descriptor = socket(PF_INET, SOCKTYPE, IPPROTO_TCP))
		== -1) {
		perror ( "socket failed" );
		exit(-1);
	}
	/*
	 * Zero the addr struct, set a few parts of it.
	 */
	(void) bzero((char *) &connect_socket_addr, sizeof (connect_socket_addr));

	/*
	 * Add our service specific components. The use of a pointer is actually
	 * superfluous, but is residual from some more complex service lookup
	 * schemes that could be implemented (ie, from /etc/services).
	 */
	service = &service_tmp;
	service->s_port = port;
	service->s_name = SERVICE;

	connect_socket_addr.sin_family = AF_INET;
	connect_socket_addr.sin_port = htons(service->s_port);

printf("TCP port: %i\n", service->s_port);
	/*
	 * Got to check the address parameter for sockaddr. Grey area!
	 */
	if (hstp == NULL) {
		(void) printf("%s: %s", hostname, "Unknown host?!"); /* no such host */
	}

/*	if (hstp->h_length != sizeof(u_long)) */
/*		return(-1);		 this is fundamental */

	(void) bcopy(hstp->h_addr, (char *) &connect_socket_addr.sin_addr,
				hstp->h_length);
	/*
	 * Attempt a connection on the known socket.
	 */
	if (connect(socket_descriptor, (struct sockaddr *) &connect_socket_addr,
			sizeof(connect_socket_addr)) == -1)
	{
		char errtext[1024];

		sprintf(errtext, "connect failed on %i", port);
		perror(errtext);
		(void) close(socket_descriptor);
		return(-2);
	}

	return(socket_descriptor);
}

