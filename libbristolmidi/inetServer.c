
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

/*#define DEBUG */

#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "bristol.h"
#include "bristolmidi.h"

extern bristolMidiMain bmidi;
extern int open_remote_socket(char *, int, int, int);
extern int bristolMidiFindDev(char *);
extern int bristolMidiFindFreeHandle();

static char bristol_sockname[128];

int
bristolMidiTCPPassive(char *devname, int conntype, int chan, int msgs,
int (*callback)(), void *param, int dev, int handle)
{
	struct sockaddr address;

	//printf("bristolMidiTCPPassive(%s, %i, %i)\n", devname, dev, handle);

	if ((conntype & BRISTOL_CONN_UNIX) ||
		((strncmp("unix", devname, 4) == 0)
		&& (strlen(devname) > 4)
		&& (devname[4] == ':')))
		conntype = 0;
	else
		conntype = 1;

	if (chan <= 0)
		chan = 5028;

	if (conntype)
	{
		int count = 0;

		while ((bmidi.dev[dev].fd = open_remote_socket(devname, chan, 8, -1))
			< 0)
		{
			printf("Could not open control socket, count %i\n", count);
			if (--count <= 0)
				break;
			sleep(10);
		}

		if (bmidi.dev[dev].fd < 0)
		{
			printf("No controlling socket available: anticipating MIDI\n");
			return(-1);
		} else
			printf("Opened listening control socket: %i\n", chan);
	} else {
		/*
		 * Open a unix domain control socket.
		 */
		unlink(devname);

		if ((bmidi.dev[dev].fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
		{
			printf("Could not get control socket\n");
			return(BRISTOL_MIDI_DEVICE);
		}

		/*
		 * Make it non-blocking
		 */
		if (fcntl(bmidi.dev[dev].fd, F_SETFL, O_NONBLOCK|O_ASYNC) < 0)
			printf("Could not set non-blocking\n");

		printf("Opened Unix named control socket\n");

		address.sa_family = AF_LOCAL;
		if ((strlen(devname) > 5) && (devname[4] == ':'))
			snprintf(bristol_sockname, 64, "/tmp/br.%s", &devname[5]);
		else
			sprintf(bristol_sockname, "%s", BRISTOL_SOCKNAME);

		snprintf(&address.sa_data[0], 14, "%s", bristol_sockname);

		if (bind(bmidi.dev[dev].fd, &address, sizeof(struct sockaddr))) 
			printf("Could not bind name: %s\n", bristol_sockname);
		else
			printf("Bound name to socket: %s\n", bristol_sockname);

		if (listen(bmidi.dev[dev].fd, 8) < 0)
			printf("Could not configure listens\n");
		else
			printf("Activated listens on socket\n");

		/*
		 * Make it world read/writeable.
		 */
		chmod(devname, 0777);
	}

	/*
	 * Configure this device as used, with decoding using the ALSA interface.
	 * We use ALSA basically since it workd.
	 */
	bmidi.dev[dev].flags = BRISTOL_ACCEPT_SOCKET|BRISTOL_CONN_TCP;

	return(handle);
}

int
acceptConnection(int acceptdev)
{
	int dev, handle, chandle;
	socklen_t addrlen;
	struct sockaddr address;
	struct linger blinger;

	/*
	 * We have read on the control socket, attempt to accept the connection.
	 */
	while ((dev = bristolMidiFindDev(NULL)) < 0)
	{
		printf("No dev available for accept()\n");
		/*
		 * We cannot leave the socket dangling, otherwise we get into a closed
		 * loop of accept failing.
		 */
		close(accept(bmidi.dev[acceptdev].fd, &address, &addrlen));
		return(-1);
	}

	addrlen = sizeof(struct sockaddr);

	if ((bmidi.dev[dev].fd =
		accept(bmidi.dev[acceptdev].fd, &address, &addrlen)) < 0)
	{
		//printf("No accepts waiting\n");
		return(-1);
	}

	bmidi.dev[dev].state = BRISTOL_MIDI_OK;
	bmidi.dev[dev].handleCount = 1;
	bmidi.dev[dev].flags = BRISTOL_CONN_TCP|BRISTOL_CONTROL_SOCKET;

	printf("Accepted connection from %i (%i) onto %i (%i)\n",
		acceptdev, bmidi.dev[acceptdev].fd, dev, bmidi.dev[dev].fd);

	/*
	 * Make sure we have a handle available
	 */
	if ((handle = bristolMidiFindFreeHandle()) < 0)
		return(handle);

	for (chandle = 0; chandle < BRISTOL_MIDI_HANDLES; chandle++)
	{
		if ((bmidi.handle[chandle].dev == acceptdev) &&
			(bmidi.dev[bmidi.handle[chandle].dev].flags
				& BRISTOL_ACCEPT_SOCKET))
			break;
	}
	if (chandle == BRISTOL_MIDI_HANDLES)
	{
		printf("Did not find related accept socket\n");
		close(bmidi.dev[dev].fd);
		bmidi.dev[dev].fd = -1;
		return(-1);
	}

	bmidi.handle[handle].handle = handle; /* That looks kind of wierd! */
	bmidi.handle[handle].handle = handle; /* That looks kind of wierd! */
	bmidi.handle[handle].state = BRISTOL_MIDI_OK;
	bmidi.handle[handle].channel = bmidi.handle[chandle].channel;
	bmidi.handle[handle].dev = dev;
	bmidi.handle[handle].flags = BRISTOL_MIDI_OK;
	bmidi.handle[handle].messagemask = bmidi.handle[chandle].messagemask;
	bmidi.handle[handle].callback = bmidi.handle[chandle].callback;
	bmidi.handle[handle].param = bmidi.handle[chandle].param;

	blinger.l_onoff = 1;
	blinger.l_linger = 2;

	if (setsockopt(bmidi.dev[dev].fd, SOL_SOCKET, SO_LINGER,
		&blinger, sizeof(struct linger)) < 0)
		printf("server linger failed\n");

//printf("inetServer received: %i %i %i\n", handle, bmidi.handle[handle].dev, bmidi.dev[bmidi.handle[handle].dev].fd);

	return(0);
}

