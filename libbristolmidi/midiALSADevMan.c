
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
 * This code should open the midi device (working with ALSA raw midi only for
 * the moment (9/11/01)), and read data from it. Not sure how it will be read,
 * either buffers, events, or perhaps just raw data. At some point in the 
 * development this will become a separate thread in the synth code.
 */
#include <sys/poll.h>
#include <sys/select.h>
#include <stdlib.h>

#include "bristol.h"
#include "bristolmidi.h"

/* #define DEBUG */

extern bristolMidiMain bmidi;
extern int bristolFreeHandle();
extern int bristolFreeDevice();
extern int bristolMidiRawToMsg();

/*
 * This is the ALSA code. Should separate it out already.
 */
int
bristolMidiALSAOpen(char *devname, int flags, int chan, int messages,
int (*callback)(), void *param, int dev, int handle)
{
#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
	int nfds;
	snd_seq_port_info_t *port_info;

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiALSAOpen(%s)\n", devname);

	bmidi.dev[dev].flags = SND_SEQ_OPEN_INPUT;
	if (snd_rawmidi_open(&bmidi.dev[dev].driver.alsa.handle, NULL,
		devname, bmidi.dev[dev].flags) != 0)
	{
		/* 
		 * Print a debug message, this is problematic.
		 */
		printf("Could not open the MIDI interface.\n");
		return(BRISTOL_MIDI_DRIVER);
	}

	snd_seq_port_info_alloca (&port_info);
	snd_seq_port_info_set_name (port_info, "bristol input port");
	snd_seq_port_info_set_capability (port_info,
		SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE);
	snd_seq_port_info_set_type (port_info,
		SND_SEQ_PORT_TYPE_MIDI_GENERIC|SND_SEQ_PORT_TYPE_SYNTHESIZER);
		/*|SND_SEQ_PORT_TYPE_SPECIFIC); */

	if (snd_seq_create_port (bmidi.dev[dev].driver.seq.handle, port_info))
	      printf ("error creating alsa port\n");
	else if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
	      printf ("created alsa port\n");

	/*
	 * We need to get a file descriptor for this interface, since we are going
	 * to use it with select() over several interfaces.
	if ((bmidi.dev[dev].fd = open("/dev/midi", O_RDONLY)) < 0)
		printf("Issue getting MIDI fd\n");
	 */
	if ((nfds =
		snd_rawmidi_poll_descriptors_count(bmidi.dev[dev].driver.alsa.handle))
		< 1)
		printf("issue getting descriptors: %i\n", nfds);
	else {
		struct pollfd *pfds;

		pfds = (struct pollfd *) malloc(sizeof (struct pollfd) * nfds);
		snd_rawmidi_poll_descriptors(bmidi.dev[dev].driver.alsa.handle,
			pfds, nfds);

		bmidi.dev[dev].fd = pfds[0].fd;
	}
#endif

	bmidi.dev[dev].flags |= BRISTOL_CONN_MIDI;

	return(handle);
#else
	return(0);
#endif /* ADLIB */
}

int
bristolMidiALSAClose(int handle)
{
	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiALSAClose()\n");

#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
	/*
	 * Check to see if the associated device has multiple handles associated
	 */
	if (bmidi.dev[bmidi.handle[handle].dev].handleCount > 1)
	{
		bmidi.dev[bmidi.handle[handle].dev].handleCount--;
		bristolFreeHandle(handle);

		return(BRISTOL_MIDI_OK);
	}

	snd_rawmidi_close(bmidi.dev[bmidi.handle[handle].dev].driver.alsa.handle);

	bristolFreeDevice(bmidi.handle[handle].dev);
	bristolFreeHandle(handle);
#endif /* ADLIB */
#endif /* ADLIB */
	return(BRISTOL_MIDI_OK);
}

/*
 * Check callbacks has two functions, firstly it looks to see if there are any
 * handles registered for this message, then whilst scanning the whole list of
 * handles it will decide if the message also needs redistributing.
 *
 * The decision to redistribute should only be taken if the message came from
 * a midi device that is not a TCP connection. The destination will be selected
 * if it the device is a TCP connection.
 */
void
checkcallbacks(bristolMidiMsg *msg)
{
	int i = 0, message = 1 << ((msg->command & 0x70) >> 4);

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("msg from %i, chan %i, %i bytes\n", msg->params.bristol.from,
			msg->params.bristol.channel, msg->params.bristol.msgLen);

	for (i = 0; i < BRISTOL_MIDI_HANDLES; i++)
	{
		if ((bmidi.dev[bmidi.handle[i].dev].flags & BRISTOL_ACCEPT_SOCKET)
			|| (bmidi.dev[i].flags & BRISTOL_CONN_JACK)
			|| (bmidi.handle[i].state < 0))
			continue;

		if ((bmidi.dev[bmidi.handle[i].dev].fd > 0)
			&& (bmidi.flags & BRISTOL_MIDI_FORWARD)
			&& (bmidi.flags & BRISTOL_MIDI_GO)
			//&& (~bmidi.dev[i].flags & BRISTOL_CONN_JACK)
			&& (~bmidi.dev[msg->params.bristol.from].flags & BRISTOL_CONN_TCP)
			&& (bmidi.dev[bmidi.handle[i].dev].flags & BRISTOL_CONN_TCP)
			&& (bmidi.dev[bmidi.handle[i].dev].flags & BRISTOL_CONN_FORWARD)
			&& (bmidi.handle[i].dev >= 0)
			&& (msg->params.bristol.msgLen != 0))
		{
			if (bmidi.dev[bmidi.handle[i].dev].flags & _BRISTOL_MIDI_DEBUG)
				printf(
					"candidate for forwarding: %i: %i -> %i (%x %x: %i %i)\n",
					i,
					msg->params.bristol.from,
					bmidi.handle[i].dev,
					bmidi.dev[msg->params.bristol.from].flags,
					bmidi.dev[bmidi.handle[i].dev].flags,
					bmidi.dev[bmidi.handle[i].dev].fd,
					msg->params.bristol.msgLen);
			if (bmidi.msgforwarder != NULL) {
				msg->mychannel = bmidi.handle[i].dev;
				bmidi.msgforwarder(msg);
			} else if (bristolMidiRawWrite(bmidi.handle[i].dev, msg,
				msg->params.bristol.msgLen) != 0)
				printf("forward failed\n");
		}
		/*
			printf("Not a forwarding candidate\n%i %x: %x %x %x %x %x %i\n",
				i, //7
				bmidi.flags,
				bmidi.flags & BRISTOL_MIDI_FORWARD, //20000000
				bmidi.flags & BRISTOL_MIDI_GO, //80000000
				~bmidi.dev[msg->params.bristol.from].flags & BRISTOL_CONN_TCP,
				bmidi.dev[bmidi.handle[i].dev].flags & BRISTOL_CONN_TCP,
				bmidi.dev[bmidi.handle[i].dev].flags & BRISTOL_CONN_FORWARD,
				msg->params.bristol.msgLen);
		*/

		if (bmidi.handle[i].callback == NULL)
		{
			if (bmidi.dev[bmidi.handle[i].dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("null callback\n");
			/*
			 * We should not really have null callbacks. The only use for it
			 * was for blocking read operations for the GUI to get acks back
			 * from the engine.
			 *
			 * Registrations with null callback should be given a default
			 * handler which flags the message as 'new' for the reader thread
			 * to take it and return.
			 */
			continue;
		}

		if (bmidi.handle[i].messagemask & message)
		{
			if (msg->command == (MIDI_SYSEX & MIDI_COMMAND_MASK))
			{
				/*
				 * SYSEX should only be sent on a single channel, the one on 
				 * which it arrived.
				 */
				if (msg->params.bristol.from == bmidi.handle[i].dev)
				{
					msg->params.bristol.from = i;
					bmidi.handle[i].callback(msg, bmidi.handle[i].param);
					return;
				}
			} else {
				unsigned char hold = msg->params.bristol.from;

				if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
					printf("callback non sysex: %i %x\n",
						i, bmidi.handle[i].flags);

				if (((~bmidi.flags & BRISTOL_MIDI_GO)
					&& (((msg->command & ~MIDI_STATUS_MASK) >> 4) < 2))
					|| (bmidi.handle[i].flags & BRISTOL_CONN_SYSEX))
					continue;
				msg->params.bristol.from = i;
				bmidi.handle[i].callback(msg, bmidi.handle[i].param);
				msg->params.bristol.from = hold;
			}
		}
	}
}

/*
 * Note that we should not read on a handle - we should read on a device, and
 * then forward to all the handles.
 */
int
bristolMidiALSARead(int dev, bristolMidiMsg *msg)
{
	int parsed, space;

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiALSARead(%i)\n", dev);

	/*
	 * See if we can read more data from the midi device. We need to make sure
	 * we have buffer capacity, and if so attempt to read as many bytes as we
	 * have space for. We need to treat the buffer as endless, ie, rotary. It
	 * does make it pretty ugly if we come to large sysex messages.
	 */
	if ((space = BRISTOL_MIDI_BUFSIZE - bmidi.dev[dev].bufcount) > 0)
	{
		int offset, count = 0;

		/*
		 * We have space in our buffer, but we may have a wrap condition. If 
		 * this is the case, only partially read to the end of the buffer, then
		 * read further into the buffer. Requires two read ops. Bummer.
		 */
		/*
		 * Generate offset for new data, with buffer size corrections.
		 */
		if ((offset = bmidi.dev[dev].bufindex + bmidi.dev[dev].bufcount)
			>= BRISTOL_MIDI_BUFSIZE)
			offset -= BRISTOL_MIDI_BUFSIZE;

		/*
		 * Read new data, if we have any
		 */
		if (bmidi.dev[dev].flags & BRISTOL_CONTROL_SOCKET) {
			count = read(bmidi.dev[dev].fd, &bmidi.dev[dev].buffer[offset], 1);
			/* 
			 * We are going to treat a count of zero as an error.
			 */
			if (count == 0)
				return(BRISTOL_MIDI_DEV);
		} else {
#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
			if (bmidi.dev[dev].flags & BRISTOL_CONN_MIDI)
				count = snd_rawmidi_read(bmidi.dev[dev].driver.alsa.handle,
					&bmidi.dev[dev].buffer[offset], 1);
			else
#endif
#endif
			{
#define B_SEL /* Decide whether to use the select() code or just read() */
#ifdef B_SEL
				int iterations = 3;
				struct timeval timeout;
				fd_set read_set[256];

				/*
				 * We need to poll the FD here to prevent lockouts.
				 */
				while (iterations-- > 0)
				{
					FD_ZERO(read_set);
					FD_SET(bmidi.dev[dev].fd, read_set);

//#warning LINK AN OSC FD REQUEST IN HERE
					/*
					 * OSC will give perhaps another FD to be included. If the
					 * request for the OSC fd > 0 put it in the list
					 */

					if (bmidi.dev[dev].flags & BRISTOL_CONN_NBLOCK)
						timeout.tv_sec = 0;
					else
						timeout.tv_sec = 1;
					timeout.tv_sec = 0;
					timeout.tv_usec = 100000;
					timeout.tv_usec = 10000;

					if (select(bmidi.dev[dev].fd + 1, read_set,
						NULL, NULL, &timeout) == 1)
					{
						count = read(bmidi.dev[dev].fd,
							&bmidi.dev[dev].buffer[offset], 1);
						break;
					}

					if (bmidi.dev[dev].flags & BRISTOL_CONN_NBLOCK)
						return(BRISTOL_MIDI_DEV);

					return(BRISTOL_MIDI_DEV);

					printf("Midi read retry (%i)\n", getpid());
				}
#else
				count = read(bmidi.dev[dev].fd,
						&bmidi.dev[dev].buffer[offset], 1);
#endif
			}
		}

		/*
		 * MIDI byte debugging. This should be under a compilation flag
		 */
		if ((count == 1) && (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG))
			printf("%i-%02x ", dev, bmidi.dev[dev].buffer[offset]);

		if (count < 1)
		{
			if (bmidi.dev[dev].bufcount == 0)
			{
				/*
				 * MARK
				 */
				printf("no data in alsa buffer for %i (close)\n", dev);
				msg->command = -1;
				/*
				 * We should consider closing whatever synth was listening
				 * here. This would require build an EXIT message.
				 */
				return(BRISTOL_MIDI_CHANNEL);
			} else
				count = 0;
		}

		bmidi.dev[dev].bufcount++;
	} else {
		printf("Device buffer exhausted\n");
		bmidi.dev[dev].bufindex = bmidi.dev[dev].bufcount = 0;
		return(BRISTOL_MIDI_DEV);
	}

	/*
	 * I really want to parse these, and then forward the messages to callback
	 * routines registered by the bristolMidiOpen() command.
	if ((parsed = bristolMidiRawToMsg(&bmidi.dev[dev].buffer[0],
	 */
	while ((parsed = bristolMidiRawToMsg(&bmidi.dev[dev].buffer[0],
		bmidi.dev[dev].bufcount, bmidi.dev[dev].bufindex, dev, msg)) > 0)
	{
		if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
			printf("parsed %i\n", parsed);

		if ((bmidi.dev[dev].bufcount -= parsed) < 0)
		{
			/*
			 * We have an issue here, count has gone negative. Reset all buffer
			 * pointers, and just write an error for now.
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
		 * We should now go through all the handles, and find out who has
		 * registered for these events - then forward them the event via the
		 * callback. For now we are just going to send it to the one existing
		 * callback.
		 */
		msg->params.bristol.from = dev;
/*		bristolMidiToGM2(msg); */

		if (msg->params.bristol.msgLen == 0)
			msg->params.bristol.msgLen = parsed;

		if (msg->command != 0x0ff)
			checkcallbacks(msg);
	}

	msg->command = -1;

	return(BRISTOL_MIDI_OK);
}

