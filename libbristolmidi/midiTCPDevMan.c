
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <unistd.h>

#include "bristol.h"
#include "bristolmidi.h"

struct sockaddr address;

extern bristolMidiMain bmidi;
extern int bristolMidiTCPPassive();
int bristolMidiTCPActive();
extern int bristolMidiFindDev();
extern int initControlPort();
extern int bristolFreeHandle();
extern int bristolFreeDevice();

extern void checkcallbacks(bristolMidiMsg *);

static char bristol_sockname[128];

int
bristolMidiTCPClose(int handle)
{
#ifdef DEBUG
	printf("bristolMidiTCPClose()\n");
#endif

	/*
	 * Check to see if the associated device has multiple handles associated
	 */
	if (bmidi.dev[bmidi.handle[handle].dev].handleCount > 1)
	{
		bmidi.dev[bmidi.handle[handle].dev].handleCount--;
		bristolFreeHandle(handle);

		return(BRISTOL_MIDI_OK);
	}

	close(bmidi.dev[bmidi.handle[handle].dev].fd);
	bmidi.dev[bmidi.handle[handle].dev].fd = -1;

	if (bmidi.dev[bmidi.handle[handle].dev].flags & BRISTOL_ACCEPT_SOCKET)
		unlink(bristol_sockname);

	bristolFreeDevice(bmidi.handle[handle].dev);
	bristolFreeHandle(handle);

	return(BRISTOL_MIDI_OK);
}

int
bristolMidiTCPOpen(char *host, int flags, int chan, int msgs,
int (*callback)(), void *param, int dev, int handle)
{
/* printf("bristolMidiTCPOpen(%s, %i, %i)\n", host, chan, dev); */
	/*
	 * See if we are active or passive.
	 */
	bmidi.dev[dev].handleCount = 1;
	if (flags & BRISTOL_CONN_PASSIVE)
	{
		/*
		 * This is the server, or in our case the engine, side of operations.
		 * we need to open a listening socket, and bind it to an address.
		 */
		return(bristolMidiTCPPassive(host, flags, chan, msgs, callback,
			param, dev, handle));
	} else {
		/*
		 * This is the client side, we should have a station address in "dev"
		 * and we connect to this address.
		 */
		return(bristolMidiTCPActive(host, flags, chan, msgs, callback,
			param, dev, handle));
	}
}

int
bristolMidiTCPActive(char *host, int conntype, int chan, int msgs,
int (*callback)(), void *param, int dev, int handle)
{
	struct linger blinger;

#ifdef DEBUG
	printf("bristolMidiTCPActive(%s, %i)\n", host, handle);
#endif

	if ((conntype & BRISTOL_CONN_UNIX) ||
		((strncmp("unix", host, 4) == 0)
		&& (strlen(host) > 4)
		&& (host[4] == ':')))
		conntype = 0;
	else
		conntype = 1;

	/*
	 * Otherwise try and find a device that is free.
	if ((dev = bristolMidiFindDev(NULL)) < 0)
		return(dev);
	 */

	bmidi.dev[dev].flags |= BRISTOL_CONN_TCP;

	if (chan == -1)
		chan = 5028;

	if (conntype)
	{
		if ((bmidi.dev[dev].fd = initControlPort(host, chan)) < 0)
		{
			printf("connfailed\n");
			return(BRISTOL_MIDI_CHANNEL);
		}
		bmidi.dev[dev].flags = BRISTOL_CONN_TCP;
	} else {
		/*
		 * We need to open a control socket to the bristol engine. For the time
		 * being we will only be concerned with a unix domain socket.
		 */
		if ((bmidi.dev[dev].fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
		{
			printf("Could not get a socket\n");
			exit(-2);
		}

#ifdef DEBUG
		printf("Opened the bristol control socket: %i\n", bmidi.dev[dev].fd);
#endif

		address.sa_family = AF_LOCAL;
		if ((strlen(host) > 5) && (host[4] == ':'))
			snprintf(bristol_sockname, 128, "/tmp/br.%s", &host[5]);
		else
			snprintf(bristol_sockname, 128, "%s", BRISTOL_SOCKNAME);

		snprintf(&address.sa_data[0], 14, "%s", bristol_sockname);

		if (connect(bmidi.dev[dev].fd, &address, sizeof(struct sockaddr)) < 0)
		{
			printf("Could not connect to the bristol control socket\n");
			return(BRISTOL_MIDI_CHANNEL);
		}
		bmidi.dev[dev].flags = BRISTOL_CONN_TCP;
	}

	printf("Connected to the bristol control socket: %i (dev=%i)\n",
		bmidi.dev[dev].fd, dev);

	blinger.l_onoff = 1;
	blinger.l_linger = 2;

	if (setsockopt(bmidi.dev[dev].fd, SOL_SOCKET, SO_LINGER,
		&blinger, sizeof(struct linger)) < 0)
		printf("client linger failed\n");

	return(handle);
}

/*
 * select for data on every TCP connection. Hm, this is problematic for the
 * monolithic process as it owns all the sockets.
 */
int
bristolMidiTCPRead(bristolMidiMsg *msg)
{
	int parsed, dev, offset, space, dc = 0;
	struct timeval timeout;
	fd_set read_set[BRISTOL_MIDI_DEVCOUNT << 1];

#ifdef DEBUG
	printf("bristolMidiTCPRead()\n");
#endif

	FD_ZERO(read_set);

	/*
	 * Find ports of type TCP, see if they have buffer spaace then schedule them
	 * to be read.
	 */
	for (dev = 0; dev < BRISTOL_MIDI_DEVCOUNT; dev++)
	{
		/* Only want TCP data connections */
		if ((bmidi.dev[dev].fd > 0)
			&& (BRISTOL_MIDI_BUFSIZE - bmidi.dev[dev].bufcount > 0)
			&& ((bmidi.dev[dev].flags & BRISTOL_CONTROL_SOCKET) == 0)
			&& (bmidi.dev[dev].flags & BRISTOL_CONN_TCP))
		{
			FD_SET(bmidi.dev[dev].fd, read_set);
			dc++;
		}
	}

	/* This is really just a poll operation */
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;

	if (dc == 0)
		return(-1);

	if (select(dev + 1, read_set, NULL, NULL, &timeout) == 0)
		return(0);

	/* We have data somewhere. Get it */
	for (dev = 0; dev < BRISTOL_MIDI_DEVCOUNT; dev++)
	{
		if (bmidi.dev[dev].fd < 0)
			continue;
		if (FD_ISSET(bmidi.dev[dev].fd, read_set))
		{
			if ((offset = bmidi.dev[dev].bufindex + bmidi.dev[dev].bufcount)
				>= BRISTOL_MIDI_BUFSIZE)
				offset -= BRISTOL_MIDI_BUFSIZE;

			if ((space = BRISTOL_MIDI_BUFSIZE - offset)
				> sizeof(bristolMidiMsg))
				space = sizeof(bristolMidiMsg);

			if ((space =
				read(bmidi.dev[dev].fd, &bmidi.dev[dev].buffer[offset], space))
				< 0)
			{
				printf("no data in tcp buffer for %i (close)\n", dev);
				msg->command = -1;
				/*
				 * We should consider closing whatever synth was listening
				 * here. This would require build an EXIT message.
				 */
				return(BRISTOL_MIDI_DEV);
			}

/*
int j;
printf("read");
for (j = 0; j < space; j++)
printf(" %x", bmidi.dev[dev].buffer[offset + j]);
printf("\n");
*/

			bmidi.dev[dev].bufcount+=space;

			/*
			 * I really want to parse these, and then forward the messages to
			 * callback routines registered by the bristolMidiOpen() command.
			 */
			while ((parsed = bristolMidiRawToMsg(&bmidi.dev[dev].buffer[0],
				bmidi.dev[dev].bufcount, bmidi.dev[dev].bufindex, dev, msg))
				> 0)
			{
				/* 
				 * Reduce count of available space.
				 */
				if ((bmidi.dev[dev].bufcount -= parsed) < 0)
				{
					/*
					 * We have an issue here, count has gone negative. Reset
					 * all buffer pointers, and just write an error for now.
					 */
					bmidi.dev[dev].bufcount = 0;
					bmidi.dev[dev].bufindex = 0;
					printf("Issue with buffer capacity going negative\n");
				}
				/*
				 * Wrap index as necessary.
				 */
				if ((bmidi.dev[dev].bufindex += parsed) >= BRISTOL_MIDI_BUFSIZE)
					bmidi.dev[dev].bufindex -= BRISTOL_MIDI_BUFSIZE;

				/*
				 * We should now go through all the handles, and find out who
				 * has registered for these events - then forward them the event
				 * via the callback. For now we are just going to send it to the
				 * one existing callback.
				 */
				msg->params.bristol.from = dev;
/*				bristolMidiToGM2(msg); */

				if (msg->params.bristol.msgLen == 0)
					msg->params.bristol.msgLen = parsed;

				if (msg->command != 0x0ff)
				{
//if ((msg->command == 0xc0) || (msg->command == 0xc0))
//printf("tcp comm %x %i\n", msg->command, msg->params.program.p_id);

					checkcallbacks(msg);
				}

				msg->command = -1;
			}
		}
	}

	return(1);
}

