
/*
 *  Diverse Bristol audio routines.
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

//#define DEBUG
#ifdef BRISTOL_PA
#include <pulse/pulseaudio.h>

#define BRISTOL_PULSE_STARTED	0x0001
/* 2 legacy plus 16 multiIO */
#define BRISTOL_PULSE_MULTI		0
#define BRISTOL_PULSE_PORTS		(2 + BRISTOL_PULSE_MULTI)

#define BRISTOL_PULSE_CHAN_0	0
#define BRISTOL_PULSE_CHAN_1	1

#define BRISTOL_PULSE_STDINL	BRISTOL_PULSE_MULTI
#define BRISTOL_PULSE_STDINR	(BRISTOL_PULSE_MULTI + 1)
#define BRISTOL_PULSE_STDOUTL	BRISTOL_PULSE_MULTI
#define BRISTOL_PULSE_STDOUTR	(BRISTOL_PULSE_MULTI + 1)

typedef struct PulseDev {
	pa_context *context;
	pa_stream *stream;
	pa_mainloop *loop;
	pa_mainloop_api *m_api;
	pa_threaded_mainloop *bLoop;
	pa_sample_spec sample_spec;
	pa_channel_map channel_map;
	char *stream_name;
	char *client_name;
	char *pa_device;
	char *server;

	float *outbuf, *inbuf;
	unsigned char *data;

	pa_buffer_attr attr;

	audioMain *audiomain;
	u_int64_t flags;
	const char **ports;
	int iocount;
} pulseDev;

#endif
