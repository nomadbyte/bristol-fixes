
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

#include <jack/jack.h>
#ifdef _BRISTOL_JACK_SESSION
#include <jack/session.h>
#endif

#define BRISTOL_JACK_STARTED	0x0001
/* 2 legacy plus 16 multiIO */
#define BRISTOL_JACK_PORTS		(2 + BRISTOL_JACK_MULTI)

#define BRISTOL_JACK_CHAN_0		0
#define BRISTOL_JACK_CHAN_1		1
#define BRISTOL_JACK_CHAN_2		2
#define BRISTOL_JACK_CHAN_3		3
#define BRISTOL_JACK_CHAN_4		4
#define BRISTOL_JACK_CHAN_5		5
#define BRISTOL_JACK_CHAN_6		6
#define BRISTOL_JACK_CHAN_7		7
#define BRISTOL_JACK_CHAN_8		8
#define BRISTOL_JACK_CHAN_9		9
#define BRISTOL_JACK_CHAN_10	10
#define BRISTOL_JACK_CHAN_11	11
#define BRISTOL_JACK_CHAN_12	12
#define BRISTOL_JACK_CHAN_13	13
#define BRISTOL_JACK_CHAN_14	14
#define BRISTOL_JACK_CHAN_15	15
/* We only do 16 for now, more maybe later */
#define BRISTOL_JACK_CHAN_16	16
#define BRISTOL_JACK_CHAN_17	17
#define BRISTOL_JACK_CHAN_18	18
#define BRISTOL_JACK_CHAN_19	19
#define BRISTOL_JACK_CHAN_20	20
#define BRISTOL_JACK_CHAN_21	21
#define BRISTOL_JACK_CHAN_22	22
#define BRISTOL_JACK_CHAN_23	23
#define BRISTOL_JACK_CHAN_24	24
#define BRISTOL_JACK_CHAN_25	25
#define BRISTOL_JACK_CHAN_26	26
#define BRISTOL_JACK_CHAN_27	27
#define BRISTOL_JACK_CHAN_28	28
#define BRISTOL_JACK_CHAN_29	29
#define BRISTOL_JACK_CHAN_30	30
#define BRISTOL_JACK_CHAN_31	31

#define BRISTOL_JACK_STDINL		BRISTOL_JACK_MULTI
#define BRISTOL_JACK_STDINR		(BRISTOL_JACK_MULTI + 1)
#define BRISTOL_JACK_STDOUTL	BRISTOL_JACK_MULTI
#define BRISTOL_JACK_STDOUTR	(BRISTOL_JACK_MULTI + 1)

typedef struct jackDev {
	jack_client_t *handle;
	jack_port_t *jack_out[BRISTOL_JACK_PORTS];
	jack_port_t *jack_in[BRISTOL_JACK_PORTS];
	jack_port_t *jack_midi_in[BRISTOL_JACK_PORTS];
	jack_default_audio_sample_t *outbuf[BRISTOL_JACK_PORTS];
	jack_default_audio_sample_t *inbuf[BRISTOL_JACK_PORTS];
	audioMain *audiomain;
	u_int64_t flags;
	const char **ports;
	int iocount;
#ifdef _BRISTOL_JACK_SESSION
	jack_session_event_t *sEvent;
#endif
} jackDev;

