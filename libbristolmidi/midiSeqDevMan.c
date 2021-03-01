
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
#include <stdlib.h>

#include "bristolmidi.h"

#define DEBUG

extern bristolMidiMain bmidi;
extern int bristolFreeHandle();
extern int bristolFreeDevice();
extern int checkcallbacks();

/*
 * This is the ALSA code. Should separate it out already.
 */
int
bristolMidiSeqOpen(char *devname, int flags, int chan, int messages,
int (*callback)(), void *param, int dev, int handle)
{
#if (BRISTOL_HAS_ALSA == 1)
	int nfds, err, caps = 0, polldir = 0;
	int client, queue;
	char linkname[256];
	snd_seq_port_info_t *pinfo;
  
#ifdef DEBUG
	if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
		printf("bristolMidiSeqOpen(%s)\n", devname);
#endif

#if (SND_LIB_MAJOR == 1)
	caps = SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_SYNTHESIZER | SND_SEQ_PORT_TYPE_SOFTWARE | SND_SEQ_PORT_TYPE_SYNTH | SND_SEQ_PORT_TYPE_APPLICATION;

	/*
	 * Well, we got here, so I could perhaps consider assuming that we have
	 * ALSA 0.9 or greater SEQ support.
	 */
	bmidi.dev[dev].flags = 0;

/*
	if (flags & BRISTOL_WRONLY)
	{
		bmidi.dev[dev].flags |= SND_SEQ_OPEN_OUTPUT;
		caps |= SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
	}
*/

	if (flags & BRISTOL_RDONLY)
	{
		polldir |= POLLIN;
		bmidi.dev[dev].flags |= SND_SEQ_OPEN_INPUT;
		caps |= SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
	}

	if (flags & BRISTOL_WRONLY)
	{
		polldir |= POLLOUT;
		bmidi.dev[dev].flags |= SND_SEQ_OPEN_OUTPUT;
		caps |= SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
	}

	if (snd_seq_open(&bmidi.dev[dev].driver.seq.handle, "default",
		bmidi.dev[dev].flags, 0) != 0)
	{
		/* 
		 * Print a debug message, this is problematic.
		 */
		 printf("Could not open the MIDI interface.\n");
		 return(BRISTOL_MIDI_DRIVER);
	}

	/*
	 * This use of a fixed name, bristol, is broken. We can better find out our
	 * process name, or better still, if we are linking to from the interface
	 * then pass another devname option if this is a SEQ interface
	 */
	if ((err = snd_seq_set_client_name(bmidi.dev[dev].driver.seq.handle,
		devname)) < 0)
	{
		printf("Set client info error: %s\n", snd_strerror(err));
		return(BRISTOL_MIDI_DRIVER);
	}

	if ((client = snd_seq_client_id(bmidi.dev[dev].driver.seq.handle)) < 0) {
		printf(
			"Cannot determine client number: %s\n", snd_strerror(client));
		return(BRISTOL_MIDI_DRIVER);
	}
	printf("Client ID = %i\n", client);
	if ((queue = snd_seq_alloc_queue(bmidi.dev[dev].driver.seq.handle)) < 0) {
		printf( "Cannot allocate queue: %s\n", snd_strerror(queue));
		return(BRISTOL_MIDI_DRIVER);
	}
	printf("Queue ID = %i\n", queue);
	if ((err = snd_seq_nonblock(bmidi.dev[dev].driver.seq.handle, 1)) < 0)
		printf( "Cannot set nonblock mode: %s\n", snd_strerror(err));

	snd_seq_port_info_alloca(&pinfo);

	sprintf(linkname, "%s io", devname);

	if (~bmidi.dev[dev].flags & SND_SEQ_OPEN_INPUT)
		sprintf(linkname, "%s output", devname);
	else if (~bmidi.dev[dev].flags & SND_SEQ_OPEN_OUTPUT)
		sprintf(linkname, "%s input", devname);

	snd_seq_port_info_set_name(pinfo, linkname);
	snd_seq_port_info_set_capability(pinfo, caps);
    snd_seq_port_info_set_type (pinfo, caps);

	if ((err = snd_seq_create_port(bmidi.dev[dev].driver.seq.handle, pinfo))
		< 0)
	{
		printf( "Cannot create input port: %s\n", snd_strerror(err));
		return(BRISTOL_MIDI_DRIVER);
	}

/*
	port = snd_seq_port_info_get_port(pinfo);

	if ((bmidi.dev[dev].flags & SND_SEQ_OPEN_INPUT)
		&& ((err = snd_seq_start_queue(bmidi.dev[dev].driver.seq.handle,
		queue, NULL)) < 0))
	   printf( "Timer event output error: %s\n", snd_strerror(err));

	while (snd_seq_drain_output(bmidi.dev[dev].driver.seq.handle) > 0)
		sleep(1);

	snd_seq_port_subscribe_alloca(&sub);
	addr.client = SND_SEQ_CLIENT_SYSTEM;
	addr.port = SND_SEQ_PORT_SYSTEM_ANNOUNCE;

	printf("Registering %i %i\n", addr.client, addr.port);

	snd_seq_port_subscribe_set_sender(sub, &addr);
	addr.client = client;
	addr.port = port;

	printf("Registered %i %i\n", addr.client, addr.port);

	snd_seq_port_subscribe_set_dest(sub, &addr);
	snd_seq_port_subscribe_set_queue(sub, queue);
	snd_seq_port_subscribe_set_time_update(sub, 1);
	snd_seq_port_subscribe_set_time_real(sub, 1);
	if ((err = snd_seq_subscribe_port(bmidi.dev[dev].driver.seq.handle, sub))
		< 0)
	{
		printf("Cannot subscribe announce port: %s\n", snd_strerror(err));
		return(BRISTOL_MIDI_DRIVER);
	}

	addr.client = SND_SEQ_CLIENT_SYSTEM;
	addr.port = SND_SEQ_PORT_SYSTEM_TIMER;
	snd_seq_port_subscribe_set_sender(sub, &addr);
	if ((err = snd_seq_subscribe_port(bmidi.dev[dev].driver.seq.handle, sub))
		< 0)
	{
		printf( "Cannot subscribe timer port: %s\n", snd_strerror(err));
		return(BRISTOL_MIDI_DRIVER);
	}

	snd_seq_port_subscribe_set_time_real(sub, 0);
*/

		/*
		if (tolower(*ptr) == 'r') {
			snd_seq_port_subscribe_set_time_real(sub, 1);
			ptr++;
		}
		*/

/*
	if (sscanf(devname, "%i.%i", &v1, &v2) != 2) {
		printf( "Device name \"%s\" did not parse, defaults 128.0\n",
			devname);
		v1 = 128;
		v2 = 0;
	}
	addr.client = v1;
	addr.port = v2;

	snd_seq_port_subscribe_set_sender(sub, &addr);
	if ((err = snd_seq_subscribe_port(bmidi.dev[dev].driver.seq.handle, sub))
		< 0)
	{
		printf( "Cannot subscribe port %i from client %i: %s\n",
			v2, v1, snd_strerror(err));
	}
*/

	/*
	 * Now we need to find a file descriptor.
	 */
	if ((nfds = snd_seq_poll_descriptors_count(bmidi.dev[dev].driver.seq.handle,
		polldir)) < 1)
		printf("issue getting descriptors: %i\n", nfds);
	else {
		struct pollfd *pfds;

		pfds = (struct pollfd *) malloc(sizeof (struct pollfd) * nfds);
		snd_seq_poll_descriptors(bmidi.dev[dev].driver.seq.handle,
			pfds, nfds, polldir);

		bmidi.dev[dev].fd = pfds[0].fd;

		free(pfds);
	}

	bmidi.dev[dev].flags = BRISTOL_CONN_SEQ;

	return(handle);

#endif /* ADLIB version */
	return(BRISTOL_MIDI_DRIVER);
#endif /* ADLIB */
}

int
bristolMidiSeqClose(int handle)
{
#ifdef DEBUG
	printf("bristolMidiSeqClose()\n");
#endif

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

	snd_seq_close(bmidi.dev[bmidi.handle[handle].dev].driver.seq.handle);

	bristolFreeDevice(bmidi.handle[handle].dev);
	bristolFreeHandle(handle);
#endif /* ADLIB */
#endif /* ADLIB */
	return(BRISTOL_MIDI_OK);
}
#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
/*
static char *event_names[256] = {
	[SND_SEQ_EVENT_SYSTEM]=	"System",
	[SND_SEQ_EVENT_RESULT]=	"Result",
	[SND_SEQ_EVENT_NOTE]=	"Note",
	[SND_SEQ_EVENT_NOTEON]=	"Note On",
	[SND_SEQ_EVENT_NOTEOFF]=	"Note Off",
	[SND_SEQ_EVENT_KEYPRESS]=	"Key Pressure",
	[SND_SEQ_EVENT_CONTROLLER]=	"Controller",
	[SND_SEQ_EVENT_PGMCHANGE]=	"Program Change",
	[SND_SEQ_EVENT_CHANPRESS]=	"Channel Pressure",
	[SND_SEQ_EVENT_PITCHBEND]=	"Pitchbend",
	[SND_SEQ_EVENT_CONTROL14]=	"Control14",
	[SND_SEQ_EVENT_NONREGPARAM]=	"Nonregparam",
	[SND_SEQ_EVENT_REGPARAM]=		"Regparam",
	[SND_SEQ_EVENT_SONGPOS]=	"Song Position",
	[SND_SEQ_EVENT_SONGSEL]=	"Song Select",
	[SND_SEQ_EVENT_QFRAME]=	"Qframe",
	[SND_SEQ_EVENT_TIMESIGN]=	"SMF Time Signature",
	[SND_SEQ_EVENT_KEYSIGN]=	"SMF Key Signature",
	[SND_SEQ_EVENT_START]=	"Start",
	[SND_SEQ_EVENT_CONTINUE]=	"Continue",
	[SND_SEQ_EVENT_STOP]=	"Stop",
	[SND_SEQ_EVENT_SETPOS_TICK]=	"Set Position Tick",
	[SND_SEQ_EVENT_SETPOS_TIME]=	"Set Position Time",
	[SND_SEQ_EVENT_TEMPO]=	"Tempo",
	[SND_SEQ_EVENT_CLOCK]=	"Clock",
	[SND_SEQ_EVENT_TICK]=	"Tick",
	[SND_SEQ_EVENT_TUNE_REQUEST]=	"Tune Request",
	[SND_SEQ_EVENT_RESET]=	"Reset",
	[SND_SEQ_EVENT_SENSING]=	"Active Sensing",
	[SND_SEQ_EVENT_ECHO]=	"Echo",
	[SND_SEQ_EVENT_OSS]=	"OSS",
	[SND_SEQ_EVENT_CLIENT_START]=	"Client Start",
	[SND_SEQ_EVENT_CLIENT_EXIT]=	"Client Exit",
	[SND_SEQ_EVENT_CLIENT_CHANGE]=	"Client Change",
	[SND_SEQ_EVENT_PORT_START]=	"Port Start",
	[SND_SEQ_EVENT_PORT_EXIT]=	"Port Exit",
	[SND_SEQ_EVENT_PORT_CHANGE]=	"Port Change",
	[SND_SEQ_EVENT_PORT_SUBSCRIBED]=	"Port Subscribed",
	[SND_SEQ_EVENT_PORT_UNSUBSCRIBED]=	"Port Unsubscribed",
	[SND_SEQ_EVENT_SAMPLE]=	"Sample",
	[SND_SEQ_EVENT_SAMPLE_CLUSTER]=	"Sample Cluster",
	[SND_SEQ_EVENT_SAMPLE_START]=	"Sample Start",
	[SND_SEQ_EVENT_SAMPLE_STOP]=	"Sample Stop",
	[SND_SEQ_EVENT_SAMPLE_FREQ]=	"Sample Freq",
	[SND_SEQ_EVENT_SAMPLE_VOLUME]=	"Sample Volume",
	[SND_SEQ_EVENT_SAMPLE_LOOP]=	"Sample Loop",
	[SND_SEQ_EVENT_SAMPLE_POSITION]=	"Sample Position",
	[SND_SEQ_EVENT_SAMPLE_PRIVATE1]=	"Sample Private1",
	[SND_SEQ_EVENT_USR0]=	"User 0",
	[SND_SEQ_EVENT_USR1]=	"User 1",
	[SND_SEQ_EVENT_USR2]=	"User 2",
	[SND_SEQ_EVENT_USR3]=	"User 3",
	[SND_SEQ_EVENT_USR4]=	"User 4",
	[SND_SEQ_EVENT_USR5]=	"User 5",
	[SND_SEQ_EVENT_USR6]=	"User 6",
	[SND_SEQ_EVENT_USR7]=	"User 7",
	[SND_SEQ_EVENT_USR8]=	"User 8",
	[SND_SEQ_EVENT_USR9]=	"User 9",
	[SND_SEQ_EVENT_INSTR_BEGIN]=	"Instr Begin",
	[SND_SEQ_EVENT_INSTR_END]=	"Instr End",
	[SND_SEQ_EVENT_INSTR_INFO]=	"Instr Info",
	[SND_SEQ_EVENT_INSTR_INFO_RESULT]=	"Instr Info Result",
	[SND_SEQ_EVENT_INSTR_FINFO]=	"Instr Font Info",
	[SND_SEQ_EVENT_INSTR_FINFO_RESULT]=	"Instr Font Info Result",
	[SND_SEQ_EVENT_INSTR_RESET]=	"Instr Reset",
	[SND_SEQ_EVENT_INSTR_STATUS]=	"Instr Status",
	[SND_SEQ_EVENT_INSTR_STATUS_RESULT]=	"Instr Status Result",
	[SND_SEQ_EVENT_INSTR_PUT]=	"Instr Put",
	[SND_SEQ_EVENT_INSTR_GET]=	"Instr Get",
	[SND_SEQ_EVENT_INSTR_GET_RESULT]=	"Instr Get Result",
	[SND_SEQ_EVENT_INSTR_FREE]=	"Instr Free",
	[SND_SEQ_EVENT_INSTR_LIST]=	"Instr List",
	[SND_SEQ_EVENT_INSTR_LIST_RESULT]=	"Instr List Result",
	[SND_SEQ_EVENT_INSTR_CLUSTER]=	"Instr Cluster",
	[SND_SEQ_EVENT_INSTR_CLUSTER_GET]=	"Instr Cluster Get",
	[SND_SEQ_EVENT_INSTR_CLUSTER_RESULT]=	"Instr Cluster Result",
	[SND_SEQ_EVENT_INSTR_CHANGE]=	"Instr Change",
	[SND_SEQ_EVENT_SYSEX]=	"Sysex",
	[SND_SEQ_EVENT_BOUNCE]=	"Bounce",
	[SND_SEQ_EVENT_USR_VAR0]=	"User Var0",
	[SND_SEQ_EVENT_USR_VAR1]=	"User Var1",
	[SND_SEQ_EVENT_USR_VAR2]=	"User Var2",
	[SND_SEQ_EVENT_USR_VAR3]=	"User Var3",
	[SND_SEQ_EVENT_USR_VAR4]=	"User Var4",
#if 0
	[SND_SEQ_EVENT_IPCSHM]=	"IPC Shm",
	[SND_SEQ_EVENT_USR_VARIPC0]=	"User IPC0",
	[SND_SEQ_EVENT_USR_VARIPC1]=	"User IPC1",
	[SND_SEQ_EVENT_USR_VARIPC2]=	"User IPC2",
	[SND_SEQ_EVENT_USR_VARIPC3]=	"User IPC3",
	[SND_SEQ_EVENT_USR_VARIPC4]=	"User IPC4",
#endif
	[SND_SEQ_EVENT_NONE]=	"None",
};
*/

int
translate_event(snd_seq_event_t *ev, bristolMidiMsg *msg, int dev)
{
#ifdef DEBUG
	if ((bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
		&& (ev->type != SND_SEQ_EVENT_CLOCK)) /* Very chatty, naturally */
	{
		char *space = "	 ";

		printf("\nEVENT>>> Type = %d, flags = 0x%x", ev->type, ev->flags);

		switch (ev->flags & SND_SEQ_TIME_STAMP_MASK) {
			case SND_SEQ_TIME_STAMP_TICK:
				printf(", time = %d ticks", ev->time.tick);
				break;
			case SND_SEQ_TIME_STAMP_REAL:
				printf(", time = %d.%09d",
					(int)ev->time.time.tv_sec,
					(int)ev->time.time.tv_nsec);
				break;
		}

		printf("\n%sSource = %d.%d, dest = %d.%d, queue = %d\n",
			space,
			ev->source.client,
			ev->source.port,
			ev->dest.client,
			ev->dest.port,
			ev->queue);
	}
#endif

	msg->command = 0xff;

	/*
	 * Go look at the event. We are only concerned for now (ns-18/06/02) with
	 * midi realtime messages - note on/off, controllers, pressures, pitch.
	 */
	switch (ev->type) {
		case SND_SEQ_EVENT_NOTE:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf(
				"; ch=%d, note=%d, velocity=%d, off_velocity=%d, duration=%d\n",
					ev->data.note.channel,
					ev->data.note.note,
					ev->data.note.velocity,
					ev->data.note.off_velocity,
					ev->data.note.duration);
#endif
			break;

		case SND_SEQ_EVENT_NOTEON:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("c%i-%02x/%02x/%02x ",
					dev,
					ev->data.note.channel,
					ev->data.note.note,
					ev->data.note.velocity);
#endif
			if (ev->data.note.velocity == 0) {
				msg->command = MIDI_NOTE_OFF| ev->data.note.channel;
				msg->params.key.velocity = 64;
			} else {
				msg->command = MIDI_NOTE_ON| ev->data.note.channel;
				msg->params.key.velocity = ev->data.note.velocity;
			}
			msg->channel = ev->data.note.channel;
			msg->params.key.key = ev->data.note.note;
			msg->sequence = bmidi.dev[dev].sequence++;
			msg->params.bristol.msgLen = 3;
			break;

		case SND_SEQ_EVENT_NOTEOFF:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("c%i-%02x/%02x/%02x ",
					dev,
					ev->data.note.channel,
					ev->data.note.note,
					ev->data.note.velocity);
#endif
			msg->command = MIDI_NOTE_OFF| ev->data.note.channel;
			msg->channel = ev->data.note.channel;
			msg->params.key.key = ev->data.note.note,
			msg->params.key.velocity = ev->data.note.velocity;
			msg->sequence = bmidi.dev[dev].sequence++;
			msg->params.bristol.msgLen = 3;
			break;

		case SND_SEQ_EVENT_KEYPRESS:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("; ch=%d, note=%d, velocity=%d\n",
					ev->data.note.channel,
					ev->data.note.note,
					ev->data.note.velocity);
#endif
			msg->command = MIDI_POLY_PRESS| ev->data.note.channel;
			msg->channel = ev->data.note.channel;
			msg->params.pressure.key = ev->data.note.note,
			msg->params.pressure.pressure = ev->data.note.velocity;
			msg->params.bristol.msgLen = 3;
			break;

		case SND_SEQ_EVENT_CONTROLLER:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("c%i-%02x/%02x/%02x ",
					dev,
					ev->data.control.channel,
					ev->data.control.param,
					ev->data.control.value);
#endif

			/*
			 * Hm, we may need to recode these to 7 bit midi values?
			 */
			msg->command = MIDI_CONTROL| ev->data.note.channel;
			msg->channel = ev->data.control.channel;
			msg->params.controller.c_id = ev->data.control.param,
			msg->params.controller.c_val = ev->data.control.value;
			msg->sequence = bmidi.dev[dev].sequence++;
			msg->params.bristol.msgLen = 3;

			/*
			 * We should also stuff the GM-2 table for defined controller 
			 * numbers.
			 */
/*			bristolMidiToGM2(msg); */
			break;

		case SND_SEQ_EVENT_PGMCHANGE:
			/*
			 * May need to have a registered callback here from GUI.
			 */
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("c%i-%02x/%02x ",
					dev,
					ev->data.control.channel,
					ev->data.control.value);
#endif

			msg->command = MIDI_PROGRAM| ev->data.note.channel;
			msg->channel = ev->data.control.channel;
			msg->params.program.p_id = ev->data.control.value;
			msg->sequence = bmidi.dev[dev].sequence++;
			msg->params.bristol.msgLen = 2;

			break;

		case SND_SEQ_EVENT_CHANPRESS:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("c%i-%02x/%02x ",
					dev,
					ev->data.control.channel,
					ev->data.control.value);
#endif
			msg->command = MIDI_CHAN_PRESS;
			msg->channel = ev->data.control.channel;
			msg->params.channelpress.pressure = ev->data.control.value;
			msg->sequence = bmidi.dev[dev].sequence++;
			msg->params.bristol.msgLen = 2;
			break;

		case SND_SEQ_EVENT_PITCHBEND:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("c%i-%02x/%02x ",
					dev,
					ev->data.control.channel,
					ev->data.control.value);
#endif

			/*
			 * Hm, we may need to recode these to 7 bit midi values?
			 */
			msg->command = MIDI_PITCHWHEEL| ev->data.note.channel;
			msg->channel = ev->data.control.channel;
			ev->data.control.value += 8192;
			msg->params.pitch.lsb = ev->data.control.value & 0x7f;
			msg->params.pitch.msb = ev->data.control.value >> 7;
			msg->sequence = bmidi.dev[dev].sequence++;
			msg->params.bristol.msgLen = 3;
			break;

		case SND_SEQ_EVENT_SYSEX:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
			{
				unsigned char *sysex =
					(unsigned char *) ev + sizeof(snd_seq_event_t);
				unsigned int c;

				printf("; len=%d [", ev->data.ext.len);

				for (c = 0; c < ev->data.ext.len; c++) {
					printf("%02x%s",
						sysex[c], c < ev->data.ext.len - 1 ? ":" : "");
				}
				printf("]\n");
			}
#endif
			/*
			 * Hm, we should at least see if this is a SLab message?
			 */
			break;

		case SND_SEQ_EVENT_QFRAME:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("; frame=0x%02x\n", ev->data.control.value);
#endif
			break;

		case SND_SEQ_EVENT_CLOCK:
		case SND_SEQ_EVENT_START:
		case SND_SEQ_EVENT_CONTINUE:
		case SND_SEQ_EVENT_STOP:
#ifdef DONT_DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("; queue = %i\n", ev->data.queue.queue);
#endif
			break;

		case SND_SEQ_EVENT_SENSING:
#ifdef DEBUG
		if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
			printf("bristol does not support active sensing\n");
#endif
			break;

		case SND_SEQ_EVENT_ECHO:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
			{
				int i;
				printf("; ");
				for (i = 0; i < 8; i++) {
					printf("%02i%s", ev->data.raw8.d[i], i < 7 ? ":" : "\n");
				}
			}
#endif
			break;

		case SND_SEQ_EVENT_CLIENT_START:
		case SND_SEQ_EVENT_CLIENT_EXIT:
		case SND_SEQ_EVENT_CLIENT_CHANGE:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("; client=%i\n", ev->data.addr.client);
#endif
			break;

		case SND_SEQ_EVENT_PORT_START:
		case SND_SEQ_EVENT_PORT_EXIT:
		case SND_SEQ_EVENT_PORT_CHANGE:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("; client=%i, port = %i\n",
					ev->data.addr.client, ev->data.addr.port);
#endif
			break;

		case SND_SEQ_EVENT_PORT_SUBSCRIBED:
		case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
#ifdef DEBUG
			if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
				printf("; %i:%i -> %i:%i\n",
					ev->data.connect.sender.client,
					ev->data.connect.sender.port,
					ev->data.connect.dest.client,
					ev->data.connect.dest.port);
#endif
			break;

		default:
			printf("; not implemented\n");
	}

	switch (ev->flags & SND_SEQ_EVENT_LENGTH_MASK) {
		case SND_SEQ_EVENT_LENGTH_FIXED:
			return sizeof(snd_seq_event_t);
		case SND_SEQ_EVENT_LENGTH_VARIABLE:
			return sizeof(snd_seq_event_t) + ev->data.ext.len;
	}

	return 0;
}

#endif /* ADLIB */
#endif /* ADLIB */

/*
 * Note that we should not read on a handle - we should read on a device, and
 * then forward to all the handles.
 */
int
bristolMidiSeqRead(int dev, bristolMidiMsg *msg)
{
#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
	snd_seq_event_t *ev;
	int res;

	if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
		printf("bristolMidiSeqRead()\n");

	/*
	 * Get the message, convert its type into a bristol message. Then see if
	 * we have callbacks defined.
	 */
	while ((res = snd_seq_event_input(bmidi.dev[dev].driver.seq.handle, &ev))
		> 0)
	{
		translate_event(ev, msg,
			bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG? dev: 0);

		if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
			printf("msg->command = %02x\n", msg->command);

		if (msg->command != 0xff)
		{
			msg->params.bristol.from = dev;
			/*
			 * See if we have callbacks.
			 */
			checkcallbacks(msg);
		}

		snd_seq_free_event(ev);
	}
#endif
#endif

	return(BRISTOL_MIDI_OK);
}

/*
 * Note that we should not read on a handle - we should read on a device, and
 * then forward to all the handles.
 */
int
bristolMidiSeqKeyEvent(int dev, int op, int ch, int key, int velocity)
{
#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
	snd_seq_event_t ev;

#ifdef DEBUG
	if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
		printf("bristolMidiSeqKeyEvent(%i, %i, %i, %i)\n",
			op, ch, key, velocity);
#endif

	/*
	 * Build seq event, send it.
	 */
	snd_seq_ev_clear(&ev); 
	if (op == BRISTOL_EVENT_KEYON)
		ev.type = SND_SEQ_EVENT_NOTEON;
	else
		ev.type = SND_SEQ_EVENT_NOTEOFF;

	ev.queue = SND_SEQ_QUEUE_DIRECT;

	ev.dest.client = ch;
	ev.dest.port = 0;

	ev.data.note.channel = 0;
	ev.data.note.note = key;
	ev.data.note.velocity = velocity;

	if (snd_seq_event_output_direct(bmidi.dev[dev].driver.seq.handle, &ev) < 0)
	{
		printf("SeqSend failed: %p\n", bmidi.dev[dev].driver.seq.handle);
		return(BRISTOL_MIDI_DRIVER);
	}
#endif
#endif

	return(BRISTOL_MIDI_OK);
}

/*
 * Note that we should not read on a handle - we should read on a device, and
 * then forward to all the handles.
 */
int
bristolMidiSeqPPressureEvent(int dev, int op, int ch, int k, int p)
{
#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
	snd_seq_event_t ev;

#ifdef DEBUG
	if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
		printf("bristolMidiPressureEvent(%i, %i, %i)\n",
			op, ch, p);
#endif
	/*
	 * Build seq event, send it.
	 */
	ev.type = SND_SEQ_EVENT_KEYPRESS;

	ev.queue = SND_SEQ_QUEUE_DIRECT;

	ev.dest.client = op;
	ev.dest.port = 0;

	ev.data.control.channel = ch;
	ev.data.control.param = k;
	ev.data.control.value = p;

	if (snd_seq_event_output_direct(bmidi.dev[dev].driver.seq.handle, &ev) < 0)
	{
		printf("SeqSend failed: %p\n", bmidi.dev[dev].driver.seq.handle);
		return(BRISTOL_MIDI_DRIVER);
	}
#endif
#endif

	return(BRISTOL_MIDI_OK);
}

/*
 * Note that we should not read on a handle - we should read on a device, and
 * then forward to all the handles.
 */
int
bristolMidiSeqPressureEvent(int dev, int op, int ch, int p)
{
#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
	snd_seq_event_t ev;

#ifdef DEBUG
	if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
		printf("bristolMidiPressureEvent(%i, %i, %i)\n",
			op, ch, p);
#endif
	/*
	 * Build seq event, send it.
	 */
	ev.type = SND_SEQ_EVENT_CHANPRESS;

	ev.queue = SND_SEQ_QUEUE_DIRECT;

	ev.dest.client = op;
	ev.dest.port = 0;

	ev.data.control.channel = ch;
	ev.data.control.value = p;

	if (snd_seq_event_output_direct(bmidi.dev[dev].driver.seq.handle, &ev) < 0)
	{
		printf("SeqSend failed: %p\n", bmidi.dev[dev].driver.seq.handle);
		return(BRISTOL_MIDI_DRIVER);
	}
#endif
#endif

	return(BRISTOL_MIDI_OK);
}

/*
 * Note that we should not read on a handle - we should read on a device, and
 * then forward to all the handles.
 */
int
bristolMidiSeqCCEvent(int dev, int op, int ch, int CC, int CV)
{
#if (BRISTOL_HAS_ALSA == 1)
#if (SND_LIB_MAJOR == 1)
	snd_seq_event_t ev;

#ifdef DEBUG
	if (bmidi.dev[dev].flags & _BRISTOL_MIDI_DEBUG)
		printf("bristolMidiCCKeyEvent(%i, %i, %i, %i)\n",
			op, ch, CC, CV);
#endif

	/*
	 * Build seq event, send it.
	 */
	ev.type = SND_SEQ_EVENT_CONTROLLER;

	ev.queue = SND_SEQ_QUEUE_DIRECT;

	ev.dest.client = ch;
	ev.dest.port = 0;

	ev.data.control.channel = 0;
	ev.data.control.param = CC;
	ev.data.control.value = CV;

	if (snd_seq_event_output_direct(bmidi.dev[dev].driver.seq.handle, &ev) < 0)
	{
		printf("SeqSend failed: %p\n", bmidi.dev[dev].driver.seq.handle);
		return(BRISTOL_MIDI_DRIVER);
	}
#endif
#endif

	return(BRISTOL_MIDI_OK);
}

