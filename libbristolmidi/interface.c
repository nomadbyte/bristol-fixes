
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
 * This code should open the midi device and read data from it.
 *
 * All requests will be dispatched to the relevant handlers, with support for
 * OSS (/dev/midi*), ALSA (hw:0,0) and as of release 0.8 also the ALSA sequencer
 * (with client.port config).
 *
 * All events will be converted into a bristol native message format for
 * independency from the driver.
 */
#include <unistd.h>
#include <strings.h>

#include "bristolmessages.h"
#include "bristol.h"
#include "bristolmidi.h"

bristolMidiMain bmidi;

extern int bristolMidiFindDev();
extern int bristolMidiFindFreeHandle();

extern int bristolMidiTCPOpen();
extern int bristolMidiOSSOpen();
extern int bristolMidiALSAOpen();
extern int bristolMidiSeqOpen();
extern int bristolMidiSeqKeyEvent();
extern int bristolMidiSeqPressureEvent();
extern int bristolMidiSeqPPressureEvent();
extern int bristolMidiSeqCCEvent();
#ifdef _BRISTOL_JACK_MIDI
extern int bristolMidiJackOpen();
#endif

extern int bristolMidiTCPClose();
extern int bristolMidiOSSClose();
extern int bristolMidiALSAClose();
extern int bristolMidiSeqClose();
#ifdef _BRISTOL_JACK_MIDI
extern int bristolMidiJackClose();
#endif

extern int bristolMidiSanity();
extern int bristolMidiDevSanity();
extern int bristolMidiALSARead();
extern int bristolMidiSeqRead();
extern int bristolPhysWrite();

extern int initMidiLib();

static char devname[64] = "localhost";

/*
 * This is the generic code. Calls the desired (configured) drivers.
 */
int
bristolMidiOpen(char *dev, int flags, int chan, int msgs, int (*callback)(),
void *param)
{
	int handle, devnum;

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiOpen(%s, %x)\n", dev, flags);

	if (dev == NULL)
		dev = devname;
	else {
		if ((strncmp(dev, "unix", 4) == 0) && (strlen(dev) > 5)
			&& (dev[4] == ':'))
		snprintf(devname, 64, "%s", &dev[5]);
	}

	initMidiLib(flags);

	/*
	 * The value of -1 indicates all channels, otherwise we are only interested
	 * in messages on the given channel.
	 */
	if ((chan < -1) || (chan >= 16))
	{
		/*
		 * If this is a TCP connection then we can specify a channel greater
		 * than 1024 - it is interpreted as a TCP port, all channels.
		 */
		if ((chan < 1024)
			|| ((flags & BRISTOL_CONNMASK & BRISTOL_CONN_TCP) == 0))
			return(BRISTOL_MIDI_CHANNEL);
	}

	/*
	 * Make sure we have a handle available
	 */
	if ((handle = bristolMidiFindFreeHandle()) < 0)
		return(handle);

	//bmidi.msgforwarder = NULL;
	bmidi.handle[handle].handle = handle; /* That looks kind of wierd! */
	bmidi.handle[handle].state = BRISTOL_MIDI_OK;
	bmidi.handle[handle].channel = chan;
	bmidi.handle[handle].dev = -1;
	bmidi.handle[handle].flags = BRISTOL_MIDI_OK;
	bmidi.handle[handle].messagemask = msgs;

	/*
	 * We should then check to see if this dev is open, and if so link
	 * another handle to it. We should support multiple destinations of the
	 * same midi information.
	 */
	if (((flags & BRISTOL_CONN_FORCE) == 0)
		&& ((devnum = bristolMidiFindDev(dev)) >= 0))
	{
		/*
		 * We have the device open, configure the new handle, and return it.
		 */
		bmidi.dev[devnum].handleCount++;
		bmidi.handle[handle].dev = devnum;
		bmidi.handle[handle].callback = callback;
		bmidi.handle[handle].param = param;
		bmidi.handle[handle].flags = bmidi.dev[devnum].flags;

		if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
			printf("reusing connection %x\n", bmidi.dev[devnum].flags);
printf("reusing connection %x\n", bmidi.dev[devnum].flags);

		return(handle);
	}

	/*
	 * Now try and find a device that is free.
	 */
	if ((devnum = bristolMidiFindDev(NULL)) < 0)
		return(devnum);

	bmidi.dev[devnum].state = BRISTOL_MIDI_OK;

	/*
	 * We have early returns here and we should consider cleaning up. The thing
	 * is that if this call fails then typically the engine will exit. Future
	 * code may change that where we poke around for driver availability.
	 */
	switch (flags & BRISTOL_CONNMASK) {
		case BRISTOL_CONN_TCP:
			if (handle != bristolMidiTCPOpen(dev, flags,
				chan, msgs, callback, param, devnum, handle))
			{
				bmidi.dev[devnum].state = -1;
				bmidi.handle[handle].state = -1;
				return(BRISTOL_MIDI_DRIVER);
			}
			bmidi.handle[handle].channel = -1;
			break;
		case BRISTOL_CONN_MIDI:
			if (handle != bristolMidiALSAOpen(dev, flags,
					chan, msgs, callback, param, devnum, handle))
			{
				bmidi.dev[devnum].state = -1;
				bmidi.handle[handle].state = -1;
				return(BRISTOL_MIDI_DRIVER);
			}
			break;
		case BRISTOL_CONN_OSSMIDI:
			if (handle != bristolMidiOSSOpen(dev, flags,
				chan, msgs, callback, param, devnum, handle))
			{
				bmidi.dev[devnum].state = -1;
				bmidi.handle[handle].state = -1;
				return(BRISTOL_MIDI_DRIVER);
			}
			break;
		case BRISTOL_CONN_SEQ:
			if (handle != bristolMidiSeqOpen(dev, flags,
				chan, msgs, callback, param, devnum, handle))
			{
				bmidi.dev[devnum].state = -1;
				bmidi.handle[handle].state = -1;
				return(BRISTOL_MIDI_DRIVER);
			}
			break;
		case BRISTOL_CONN_JACK:
#ifdef _BRISTOL_JACK_MIDI
			if (handle != bristolMidiJackOpen(dev, flags,
				chan, msgs, callback, devnum, devnum, handle))
			{
				bmidi.dev[devnum].state = -1;
				bmidi.handle[handle].state = -1;
				return(BRISTOL_MIDI_DRIVER);
			}
#endif
			bmidi.dev[devnum].fd = -1;
			break;
		case BRISTOL_CONN_UNIX:
		default:
			printf("conn type %x not supported\n", flags & BRISTOL_CONNMASK);
			bmidi.dev[devnum].state = -1;
			return(BRISTOL_MIDI_DRIVER);
			break;
	}

	sprintf(bmidi.dev[devnum].name, "%s", dev);

	bmidi.dev[devnum].state = BRISTOL_MIDI_OK;
	bmidi.dev[devnum].handleCount = 1;
	bmidi.dev[devnum].bufindex = 0;
	bmidi.dev[devnum].bufcount = 0;
	bmidi.dev[devnum].sequence = 0;
	bmidi.handle[handle].dev = devnum;
	bmidi.handle[handle].callback = callback;
	bmidi.handle[handle].param = param;

printf("returning handle %i, flags %x, fd %i\n", handle, bmidi.dev[devnum].flags, bmidi.dev[devnum].fd);

	return(handle);
}

void
bristolMidiRegisterForwarder(int (*msgforwarder)())
{
	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("register message forwarder\n");

	bmidi.msgforwarder = msgforwarder;
}

int
bristolMidiOption(int handle, int option, int value)
{
	switch (option) {
		case BRISTOL_NRP_DEBUG:
		{
			int i;

			if (bristolMidiSanity(handle) < 0)
				return(bristolMidiSanity(handle));

			if (value == 0)
			{
				bmidi.flags &= ~BRISTOL_BMIDI_DEBUG;
				for (i = 0; i < BRISTOL_MIDI_DEVCOUNT; i++)
					if (bmidi.handle[i].dev >= 0)
						bmidi.dev[bmidi.handle[i].dev].flags
							&= ~_BRISTOL_MIDI_DEBUG;
			} else if (value == 1) {
				for (i = 0; i < BRISTOL_MIDI_DEVCOUNT; i++)
					if (bmidi.handle[i].dev >= 0)
						bmidi.dev[bmidi.handle[i].dev].flags
							|= _BRISTOL_MIDI_DEBUG;
			}
			if (value >= 5)
				bmidi.flags |= BRISTOL_BMIDI_DEBUG;
			break;
		}
		case BRISTOL_NRP_FORWARD:
			/* Disable event forwarding globally */
			if (value == 0)
				bmidi.flags &= ~(BRISTOL_MIDI_FORWARD|BRISTOL_MIDI_FHOLD);
			else
				bmidi.flags |= (BRISTOL_MIDI_FORWARD|BRISTOL_MIDI_FHOLD);
			break;
		case BRISTOL_NRP_MIDI_GO:
			if (value == 0)
				bmidi.flags &= ~(BRISTOL_MIDI_GO|BRISTOL_MIDI_FORWARD);
			else
				bmidi.flags |= (bmidi.flags & BRISTOL_MIDI_FHOLD)?
					(BRISTOL_MIDI_GO|BRISTOL_MIDI_FORWARD):BRISTOL_MIDI_GO;
			break;
		case BRISTOL_NRP_SYSID_H:
			bmidi.SysID = (bmidi.SysID & 0x0000ffff)
				| ((value << 16) & 0xffff0000);
			break;
		case BRISTOL_NRP_SYSID_L:
			bmidi.SysID = (bmidi.SysID & 0xffff0000) | (value & 0x0000ffff);
			break;
		case BRISTOL_NRP_REQ_SYSEX:
			/*
			 * This is for the local side only and will prevent event forwarding
			 * to the GUI (primarily). It prevents events being duplicated on
			 * the receiving end
			 */
			if (bristolMidiSanity(handle) < 0)
				return(bristolMidiSanity(handle));

			if (value == 1)
				bmidi.handle[handle].flags |= BRISTOL_CONN_SYSEX;
			else
				bmidi.handle[handle].flags &= ~BRISTOL_CONN_SYSEX;
			bmidi.handle[handle].flags |= BRISTOL_CONN_SYSEX;
			break;
		case BRISTOL_NRP_REQ_FORWARD:
			if (bristolMidiSanity(handle) < 0)
				return(bristolMidiSanity(handle));

			if (value != 0)
				bmidi.dev[bmidi.handle[handle].dev].flags
					|= BRISTOL_CONN_FORWARD;
			else
				bmidi.dev[bmidi.handle[handle].dev].flags
					&= ~BRISTOL_CONN_FORWARD;
			if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
				printf("	Requested forward on handle %i %i %x %i\n",
					handle,
					value,
					bmidi.dev[bmidi.handle[handle].dev].flags,
					bmidi.handle[handle].dev);
			break;
	}

	return(0);
}

int
bristolMidiClose(int handle)
{
	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiClose(%i) %x\n", handle,
			bmidi.dev[bmidi.handle[handle].dev].flags & BRISTOL_CONNMASK);

	/*
	 * We close a handle, not a device. We need to clean out the handle, and if
	 * no other handles have this device open then close the physical device as
	 * well.
	 */
	if (bristolMidiSanity(handle) < 0)
		return(bristolMidiSanity(handle));

	if (bmidi.dev[0].flags & BRISTOL_ACCEPT_SOCKET)
	{
		char filename[128];

		snprintf(filename, 128, "/tmp/br.%s", devname);

		unlink(filename);
	}

	switch (bmidi.dev[bmidi.handle[handle].dev].flags & BRISTOL_CONNMASK) {
		case BRISTOL_CONN_OSSMIDI:
			return(bristolMidiOSSClose(handle));
		case BRISTOL_CONN_MIDI:
			return(bristolMidiALSAClose(handle));
		case BRISTOL_CONN_SEQ:
			return(bristolMidiSeqClose(handle));
		case BRISTOL_CONN_JACK:
#ifdef _BRISTOL_JACK_MIDI
			return(bristolMidiJackClose(handle));
#endif
			break;
		case BRISTOL_CONN_TCP:
			return(bristolMidiTCPClose(handle));
		default:
			printf("DID NOT CLOSE A HANDLE!!!\n");
	}

	return(BRISTOL_MIDI_DRIVER);
}

int
bristolMidiDevRead(int dev, bristolMidiMsg *msg)
{
	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiDevRead(%i)\n", dev);

	if (bristolMidiDevSanity(dev) < 0)
		return(bristolMidiDevSanity(dev));

	/*
	 * ALSARead handles all the TCP interface as well. It should be reworked to
	 * manage OSS raw interfaces as well.....
	 */
	switch (bmidi.dev[dev].flags & BRISTOL_CONNMASK) {
		case BRISTOL_CONN_MIDI:
		case BRISTOL_CONN_TCP:
		case BRISTOL_CONN_OSSMIDI:
			return(bristolMidiALSARead(dev, msg));
		case BRISTOL_CONN_SEQ:
			return(bristolMidiSeqRead(dev, msg));
	}

	return(BRISTOL_MIDI_DRIVER);
}

int
bristolGetMidiFD(int handle)
{
	return(bmidi.handle[handle].dev);
}

static bristolMidiMsg post;

void
bristolMidiPost(bristolMidiMsg *msg)
{
	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("post %i bytes\n", (int) sizeof(bristolMidiMsg));
	bcopy(msg, &post, sizeof(bristolMidiMsg));
}

int
bristolMidiRead(int handle, bristolMidiMsg *msg)
{
	int c = 50;

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiRead(%i): %i/%i\n", handle,
			bmidi.handle[handle].dev, bmidi.dev[bmidi.handle[handle].dev].fd);

	if (bristolMidiSanity(handle) < 0)
		return(bristolMidiSanity(handle));

//	msg->command = 0xff;

	if (bmidi.handle[handle].callback == NULL)
	{
		while (msg->command == 0xff)
		{
			if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
				printf("reading type %x\n",
					bmidi.dev[bmidi.handle[handle].dev].flags); 

			switch (bmidi.dev[bmidi.handle[handle].dev].flags
				& BRISTOL_CONNMASK)
			{
				case BRISTOL_CONN_TCP:
				case BRISTOL_CONN_MIDI:
				case BRISTOL_CONN_OSSMIDI:
					if (bristolMidiALSARead(bmidi.handle[handle].dev, msg) < 0)
						return(-1);
					break;
				case BRISTOL_CONN_SEQ:
					if (bristolMidiSeqRead(bmidi.handle[handle].dev, msg) < 0)
						return(-1);
					break;
			}
		}

		return(BRISTOL_MIDI_OK);
	} else {
		switch (bmidi.dev[handle].flags & BRISTOL_CONNMASK) {
			case BRISTOL_CONN_TCP:
				/*
				 * If this is the TCP port we should have a wait loop for a
				 * response from the engine, the event comes in another thread.
				 */
				if (bmidi.dev[handle].fd < 0)
					return(BRISTOL_MIDI_DRIVER);

				while (post.channel == 0xff)
				{
					usleep(100000);

					if (--c <= 0) {
						int i;

						if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
							printf("MIDI/TCP read failure\n");

						for (i = 0; i < BRISTOL_MIDI_DEVCOUNT; i++)
						{
							if ((bmidi.dev[i].fd > 0)
								&& ((bmidi.dev[i].flags & BRISTOL_CONTROL_SOCKET) == 0)
								&& (bmidi.dev[i].flags & BRISTOL_CONN_TCP))
								bristolMidiTCPClose(bmidi.dev[i].fd);
						}

						printf("closing down TCP sockets\n");

						return(BRISTOL_MIDI_DRIVER);
					}
				}
				bcopy(&post, msg, sizeof(bristolMidiMsg));
				post.channel = 0xff;
				return(BRISTOL_MIDI_OK);
			case BRISTOL_CONN_MIDI:
			case BRISTOL_CONN_OSSMIDI:
				return(bristolMidiALSARead(bmidi.handle[handle].dev, msg));
			case BRISTOL_CONN_SEQ:
				return(bristolMidiSeqRead(bmidi.handle[handle].dev, msg));
		}
	}

	return(BRISTOL_MIDI_DRIVER);
}

int
bristolMidiTerminate()
{
	printf("terminate MIDI signalling\n");
	/*
	 * We need this since the library never actually returns when we are
	 * using the midiCheck routines. We are going to close all handles.
	 */
	bmidi.flags = BRISTOL_MIDI_TERMINATE;

	if (bmidi.dev[0].flags & BRISTOL_CONTROL_SOCKET)
	{
		char filename[128];

		/*
		 * We should send an all_notes_off and terminate the socket layer with
		 * a REQSTOP
		 */
		sleep(1);
		close(bmidi.dev[0].fd);
		bmidi.dev[0].fd = -1;

		snprintf(filename, 128, "/tmp/br.%s", devname);

		unlink(filename);
	}

	/*
	 * Free up the semaphores
	 */

	return(0);
}

int
bristolMidiRawWrite(int dev, bristolMidiMsg *msg, int size)
{
	if (bristolMidiDevSanity(dev) < 0)
		return(bristolMidiDevSanity(dev));

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiRawWrite (%x) %x/%i\n",
			msg->command,
			msg->channel,
			size);

	/*
	 * For short messages we are going to join the channel back together. This
	 * is not needed for SYSEX/Bristol messages.
	 */
	if (msg->params.bristol.msgLen < 4)
	{
		unsigned char ch = (msg->command & 0xf0)|msg->channel;

		if (bristolPhysWrite(bmidi.dev[dev].fd, (unsigned char *) &ch, 1) != 0)
			return(1);
	} else
		if (bristolPhysWrite(bmidi.dev[dev].fd,
			(unsigned char *) &msg->command, 1) != 0)
			return(1);

	if (msg->command != MIDI_SYSEX)
	{
		if (bristolPhysWrite(bmidi.dev[dev].fd,
			(unsigned char *) &msg->params, size - 1) != 0)
			return(1);
	} else {
		if (msg->params.bristol.msgType < 8)
		{
			if (bristolPhysWrite(bmidi.dev[dev].fd,
				(unsigned char *) &msg->params, size) != 0)
				return(1);
		} else {
			if (bristolPhysWrite(bmidi.dev[dev].fd,
				(unsigned char *) &msg->params, 12) != 0)
				return(1);
			if (bristolPhysWrite(bmidi.dev[dev].fd,
				(unsigned char *) msg->params.bristolt2.data,
					msg->params.bristol.msgLen - 12) != 0)
				return(1);
		}
	}

	if (msg->command == MIDI_SYSEX)
	{
		unsigned char comm = MIDI_EOS;

		return(bristolPhysWrite(bmidi.dev[dev].fd, (unsigned char *) &comm, 1));
	}

	return(0);
}

int
bristolMidiWrite(int dev, bristolMsg *msg, int size)
{
	unsigned char byte;

	if (bristolMidiDevSanity(dev) < 0)
		return(bristolMidiDevSanity(dev));

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiWrite %i/%i, %i\n", dev, bmidi.dev[dev].fd, size);

	byte = MIDI_SYSEX;
	if (bristolPhysWrite(bmidi.dev[dev].fd, &byte, 1) != 0)
		return(1);

	if (bristolPhysWrite(bmidi.dev[dev].fd, (unsigned char *) msg, size) != 0)
		return(1);

	byte = MIDI_EOS;
	if (bristolPhysWrite(bmidi.dev[dev].fd, &byte, 1) != 0)
		return(1);

	return(0);
}

int
bristolMidiControl(int handle, int channel, int operator, int controller,
	int value)
{
	unsigned char comm = 0xb0, ctr, val;

/*	printf("Midi Control %i %i %i %i\n", channel, operator, controller, value); */

	comm |= channel;
	ctr = controller;
	val = value & 0x7f;

	switch (bmidi.dev[bmidi.handle[handle].dev].flags & BRISTOL_CONNMASK) {
		case BRISTOL_CONN_SEQ:
			return(bristolMidiSeqCCEvent(bmidi.handle[handle].dev,
				operator, channel, controller, value));
		default:
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &comm, 1);
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &ctr, 1);
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &val, 1);
			break;
	}

	return(0);
}

int
bristolPitchEvent(int handle, int op, int channel, int key)
{
	unsigned char comm = 0xe0, lsb, msb;

/*	printf("pitch event %i %i %i\n", op, channel, key); */

	comm |= channel;
	lsb = key & 0x7f;
	msb = (key >> 7) & 0x7f;

/*	printf("Sending %i %i %i on %i\n", comm, msb, lsb, */
/*		bmidi.dev[bmidi.handle[handle].dev].fd); */

	bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &comm, 1);
	bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &lsb, 1);
	bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &msb, 1);

	return(0);
}

#ifdef NOT_THIS_DEBUG
static int toggle = 0;
#endif

int
bristolKeyEvent(int handle, int op, int channel, int key, int velocity)
{
	unsigned char comm;

	key &= 0x7f;
	velocity &= 0x7f;

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("key event ch: %i, key: %i over fd %i\n", channel, key,
			bmidi.dev[bmidi.handle[handle].dev].fd);

	if (bristolMidiSanity(handle) < 0)
		return(bristolMidiSanity(handle));

	if (op == BRISTOL_EVENT_KEYON)
		comm = MIDI_NOTE_ON | channel;
	else
		comm = MIDI_NOTE_OFF | channel;

	switch (bmidi.dev[bmidi.handle[handle].dev].flags & BRISTOL_CONNMASK) {
		case BRISTOL_CONN_SEQ:
			return(bristolMidiSeqKeyEvent(bmidi.handle[handle].dev,
				op, channel, key, velocity));
		default:
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &comm, 1);
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &key, 1);
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &velocity, 1);
			break;
	}

	return(0);
}

int
bristolPolyPressureEvent(int handle, int op, int channel, int key, int press)
{
	unsigned char comm;

	comm = 0xa0 | (channel & 0x0f);
	key &= 0x7f;
	press &= 0x7f;

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("pressure ch: %i, pressure: %i over fd %i\n", channel, press,
			bmidi.dev[bmidi.handle[handle].dev].fd);

	if (bristolMidiSanity(handle) < 0)
		return(bristolMidiSanity(handle));

	switch (bmidi.dev[bmidi.handle[handle].dev].flags & BRISTOL_CONNMASK) {
		case BRISTOL_CONN_SEQ:
			return(bristolMidiSeqPPressureEvent(bmidi.handle[handle].dev,
				op, channel, key, press));
		default:
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &comm, 1);
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &key, 1);
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &press, 1);
	}

	return(0);
}

int
bristolPressureEvent(int handle, int op, int channel, int press)
{
	unsigned char comm;

	comm = 0xd0 | (channel & 0x0f);
	press &= 0x7f;

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("pressure ch: %i, pressure: %i over fd %i\n", channel, press,
			bmidi.dev[bmidi.handle[handle].dev].fd);

	if (bristolMidiSanity(handle) < 0)
		return(bristolMidiSanity(handle));

	switch (bmidi.dev[bmidi.handle[handle].dev].flags & BRISTOL_CONNMASK) {
		case BRISTOL_CONN_SEQ:
			return(bristolMidiSeqPressureEvent(bmidi.handle[handle].dev,
				op, channel, press));
		default:
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &comm, 1);
			bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &press, 1);
	}

	return(0);
}

/*
 * This is the second implementation, the previous uses superfluous params....
 */
int
bristolMidiSendControlMsg(int handle, int channel, int ctrl, int value)
{
	unsigned char comm = 0xb0;

	comm |= channel;
	ctrl &= 0x7f;
	value &= 0x7f;

	/* printf("control event %i %i %i\n", channel, ctrl, value); */

	bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &comm, 1);
	bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &ctrl, 1);
	bristolPhysWrite(bmidi.dev[bmidi.handle[handle].dev].fd, &value, 1);

	return(0);
}

int
bristolMidiSendNRP(int handle, int channel, int nrp, int value)
{
	/*
	 * Send the NRP index.
	 */
	bristolMidiSendControlMsg(handle, channel, 99, (nrp >> 7) & 0x7f);
	bristolMidiSendControlMsg(handle, channel, 98, nrp & 0x7f);

	/*
	 * Send the controller values.
	 */
	bristolMidiSendControlMsg(handle, channel, 6, (value >> 7) & 0x7f);
	bristolMidiSendControlMsg(handle, channel, 38, value & 0x7f);

	return(0);
}

int
bristolMidiSendRP(int handle, int channel, int nrp, int value)
{
	/*
	 * Send the NRP index.
	 */
	bristolMidiSendControlMsg(handle, channel, 101, (nrp >> 7) & 0x7f);
	bristolMidiSendControlMsg(handle, channel, 100, nrp & 0x7f);

	/*
	 * Send the controller values.
	 */
	bristolMidiSendControlMsg(handle, channel, 6, (value >> 7) & 0x7f);
	bristolMidiSendControlMsg(handle, channel, 38, value & 0x7f);

	return(0);
}

int
bristolMidiSendKeyMsg(int handle, int channel, int operator, int key, int volume)
{
	if (operator != BRISTOL_EVENT_KEYON)
		operator = BRISTOL_EVENT_KEYOFF;

	bristolKeyEvent(bmidi.handle[handle].dev, operator, channel, key, volume);

	return(0);
}

static int velocity = 0;

int
bristolMidiSendMsg(int handle, int channel, int operator, int controller,
	int value)
{
	bristolMsg msg;

	bzero(&msg, sizeof(bristolMsg));

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
	{
		printf("bristolMidiSendMsg(%i, %i, %i, %i, %i)\n",
			handle, channel, operator, controller, value);
		printf("%i %i %i\n", handle, bmidi.handle[handle].dev,
			bmidi.dev[bmidi.handle[handle].dev].fd);
	}

	if ((value > C_RANGE_MIN_1) || (value < 0))
		printf("controller %i/%i value %i outside range\n",
			operator, controller, value);

	value &= C_RANGE_MIN_1;

	/*
	 * Key on and off events are NOT sent as system messages, but as normal
	 * key events on the control channel. As such they require separate formats.
	 */
	if (operator == BRISTOL_EVENT_PITCH)
	{
		bristolPitchEvent(bmidi.handle[handle].dev, operator, channel, value);
		return(0);
	}

	if ((operator == BRISTOL_EVENT_KEYON) || (operator == BRISTOL_EVENT_KEYOFF))
	{
		if ((bmidi.flags & BRISTOL_BMIDI_DEBUG) && (++velocity > 127))
		{
			velocity = 0;
			printf("velocity %i\n", velocity);
		} else
			velocity = 120;

		return(bristolKeyEvent(bmidi.handle[handle].dev, operator,
			channel, value & BRISTOL_PARAMMASK, velocity));
	}
	/*
	 * We are going to send this through to the control port as a basic format
	 * message. The parameter is not going to be scaled, just given as a value
	 * from 0 to 16383. The receiver can decide how to scale the value.
	 */
	msg.SysID = (bmidi.SysID >> 24) & 0x0ff;
	msg.L = (bmidi.SysID >> 16) & 0x0ff;
	msg.a = (bmidi.SysID >> 8) & 0x0ff;
	msg.b = bmidi.SysID & 0x0ff;

	msg.msgLen = sizeof(bristolMsg);
	msg.msgType = MSG_TYPE_PARAM;

	msg.channel = channel;
	msg.from = handle;
	msg.operator = operator;
	msg.controller = controller;
	msg.valueLSB = value & 0x007f;
	msg.valueMSB = (value >> 7) & 0x007f;

//	bristolMsgPrint(&msg);

	post.channel = 0xff;

	bristolMidiWrite(bmidi.handle[handle].dev, &msg, sizeof(bristolMsg));

	return(0);
}

