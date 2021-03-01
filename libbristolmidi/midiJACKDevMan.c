
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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bristolmessages.h"
#include "bristol.h"
#include "bristolmidi.h"

#ifdef _BRISTOL_JACK_MIDI
#include <jack/jack.h>
#include <jack/midiport.h>

extern bristolMidiMain bmidi;

/* We only have one of these for a single registration per engine. */
static jack_client_t *client = NULL;
static int deviceIndex = -1;

static jack_port_t *input_port = NULL;
extern void checkcallbacks(bristolMidiMsg *);

void
jackMidiRegisterPort(jack_port_t *midi_input_port)
{
	input_port = midi_input_port;
}

int
jackMidiRoutine(jack_nframes_t nframes, void *arg)
{
	int i;
	void* port_buf;
	jack_midi_event_t in_event;
	jack_nframes_t event_count;
	bristolMidiMsg msg;

	if (input_port == NULL)
		return(0);

	port_buf = jack_port_get_buffer(input_port, nframes);
	event_count = jack_midi_get_event_count(port_buf);

	bmidi.dev[deviceIndex].flags = BRISTOL_CONN_JACK|0x40000000;

	if (event_count <= 0)
		return(0);

	/*
	else
		printf("received %i events\n", event_count);
`	*/
	for (i = 0; i < event_count; i++)
	{
		memset(&in_event, 0, sizeof(jack_midi_event_t));
		memset(&msg, 0, sizeof(bristolMidiMsg));

		if (jack_midi_event_get(&in_event, port_buf, i) != 0)
			continue;

		/*
		printf("    event %d time is %d. %x/%i/%i (%i)\n", i, in_event.time,
			*(in_event.buffer), *(in_event.buffer+1), *(in_event.buffer+2),
			in_event.size);
		*/

		/*
		 * Convert these events into our internal format and then have then
		 * dispatched. We could probably call a quite early routine since we
		 * appear to have reasonably raw midi.
		 *
		 * Jack always gives me a status byte. We need to parse this then parse
		 * the actual midi message.
		 */
		bristolMidiRawToMsg(in_event.buffer, in_event.size, 0, deviceIndex,
			&msg);
		if (bristolMidiRawToMsg(in_event.buffer + 1, in_event.size - 1, 0,
			deviceIndex, &msg) > 0)
		{
			msg.params.bristol.msgLen = in_event.size;
			msg.params.bristol.from = deviceIndex;
			msg.offset = in_event.time;

			checkcallbacks(&msg);
		} else if (bmidi.flags & BRISTOL_BMIDI_DEBUG)
			printf("unknown jack midi event\n");
	}

	/*
	jack_midi_event_get(&in_event, port_buf, 0);

	for (i=0; i<nframes; i++)
	{
		if ((in_event.time == i) && (event_index < event_count))
		{
			if( ((*(in_event.buffer) & 0xf0)) == 0x90 )
			{
				note = *(in_event.buffer + 1);
				note_on = 1.0;
			}
			else if( ((*(in_event.buffer)) & 0xf0) == 0x80 )
			{
				note = *(in_event.buffer + 1);
				note_on = 0.0;
			}
			event_index++;
			if(event_index < event_count)
				jack_midi_event_get(&in_event, port_buf, event_index);
		}
	}
	*/
	return 0;      
}

void jack_midi_shutdown(void *arg)
{
	exit(1);
}

int
bristolMidiJackOpen(char *devname, int flags, int chan, int messages,
int (*callback)(), void *param, int dev, int handle)
{
	printf("bristolMidiJackOpen(%s, %i, %x)\n", devname, dev, flags);

	deviceIndex = dev;

//	bmidi.flags &= ~BRISTOL_MIDI_FORWARD;
//	bmidi.dev[dev].flags = BRISTOL_CONN_JACK|0x40000000;

	if (client == 0)
	{
		if ((client = jack_client_open(devname, 0, NULL)) == 0)
		{
			fprintf(stderr, "jack server not running?\n");
			return 1;
		}
		printf("registered jack midi name %s\n", devname);
	
		jack_set_process_callback(client, jackMidiRoutine, (void *) dev);

		jack_on_shutdown(client, jack_midi_shutdown, 0);

		input_port = jack_port_register(client, "midi_in",
			JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

		if (jack_activate(client))
		{
			fprintf(stderr, "cannot activate client");
			return 1;
		}
	} else {
		input_port = jack_port_register(client, "midi_in",
			JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
		printf("reused jack registration\n");
	}

	return(handle);
}

void
bristolJackSetMidiHandle(jack_client_t *jackHandle)
{
	client = jackHandle;
}

int
bristolMidiJackClose(int handle)
{
	jack_client_close(client);
	return(handle);
}
#else /* _BRISTOL_JACK */
/*
 * No jack so if we got here we need a couple of error messages to say so.
 */
int
bristolMidiJackOpen(char *devname, int flags, int chan, int messages,
int (*callback)(), void *param, int dev, int handle)
{
	printf("Jack MIDI requested but not linked into the library\n");
	return(BRISTOL_MIDI_DRIVER);
}

int
bristolMidiJackClose(int handle)
{
	printf("Jack MIDI close not linked into the library\n");
	return(BRISTOL_MIDI_DRIVER);
}
#endif
