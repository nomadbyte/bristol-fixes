
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

/*
 * This defines the format of messages sent to the SLab GUI.
 *
 * All the messages, for no real purpose, carry the text "SLab" in the first
 * four bytes of the message. It is intended to prevent spurious reception of
 * UDP messages, since the receiving socket is not actually connec()ed.
 *

	TRACK OPERATIONS:
	"S		L		a		b	"
	vers	comm	length	length
	track	op		cont	value
	value

 * Not really thought much about these, but I want some consideration for 
 * MIDI Time Code, or MIDI Clock to sync output to MTC. May be a big deal?

	MIDI OPERATIONS:
	"S		L		a		b	"
	vers	comm	length	length
	channel	op		data	data

 *
 * Tracks are up to 64, operators are reasonably unlimited, but there is a total
 * pool of 256. Controllers are up to 8, they are the number of controllers for
 * an operator.
 */

/*
 * Define the current version. Will allow for interoperability later.
 */
#define SLAB_CONTROL_VERSION 1

#define SLAB_CONTROL_SET 0x40
#define SLAB_CONTROL_ACK 0x80 /* Used to request acknowledgement */

#define SLAB_MAX_SWING 32767

/*
 * Commands are get or set for track/bus/tape, etc:
 * NOTES: as of version 1 there is only support for set operations, no GETs.
 *
 * Gets will be implemented at a later date, will require monitoring of the
 * 0x40 flag.
 *
 */
#define SLAB_T_SET	TRACK_EVENT
#define SLAB_T_GET	(TRACK_EVENT | SLAB_CONTROL_SET)
#define SLAB_M_SET	MASTER_EVENT
#define SLAB_M_GET	(MASTER_EVENT | SLAB_CONTROL_SET)
#define SLAB_B_SET	BUS_EVENT /* FX send return busses */
#define SLAB_B_GET	(BUS_EVENT | SLAB_CONTROL_SET)
#define SLAB_SB_SET	SBUS_EVENT /* Stereo bus group operations */
#define SLAB_SB_GET	(SBUS_EVENT | SLAB_CONTROL_SET)
#define SLAB_FX_SET	EFFECT_EVENT
#define SLAB_FX_GET	(EFFECT_EVENT | SLAB_CONTROL_SET)
#define SLAB_FB_SET	FB_EVENT
#define SLAB_FB_GET	(FB_EVENT | SLAB_CONTROL_SET)
#define SLAB_TAPE_GET	TAPE_EVENT
#define SLAB_TAPE_SET	(TAPE_EVENT | SLAB_CONTROL_SET)
#define SLAB_DEV_GET	DEV_EVENT
#define SLAB_DEV_SET	(DEV_EVENT | SLAB_DEV_SET)
#define SLAB_CTL_GET	CONTROL_EVENT
#define SLAB_CTL_SET	(CONTROL_EVENT | SLAB_CONTROL_SET)
#define SLAB_MIDI_OP	MIDI_EVENT
#define SLAB_OP_MAX 12

#ifndef GUI_DEBUG_EXT
#define GUI_DEBUG_EXT (0x2 << 20)
#endif

/*
 * Define the commands that can be executed.
 * At the moment, only TRACK, MASTER, SBUS and BUS are implemented, although
 * FX params should also work. In short, this relates to the components of the
 *  GUI that can be automated with session recording.
 */
#include "messageOps.h"

#define NAME_LEN 256
#define SLAB_MIN_MSG_SIZE 12
#define SLAB_MAX_MSG_SIZE 512

typedef struct songmessage {
	int length; /* length of the songname datafield */
} songMessage;

typedef struct devicemessage {
	int deviceID;
	int devNameLength; /* length of the device name datafield */
	int mixerNameLength; /* length of the mixer name datafield */
	int flags;
} deviceMessage;

typedef struct trackmessage {
	unsigned char track;
	unsigned char operator;
	unsigned char controller;
	unsigned short int value;
} trackMessage;

/*
 * This is intended primarily for MTC and Song Pointer support.
 */
typedef struct midiMessage {
	unsigned char channel;
	unsigned char operation;
	unsigned char key;
	unsigned char param;
	unsigned int time;
} midiMessage;

typedef struct slabMessage {
	char S;
	char L;
	char a;
	char b;
	char version;
	char command;
	short length;
	union {
		trackMessage trkMsg;
		songMessage songMsg;
		deviceMessage devMsg;
		midiMessage midiMsg;
	} message;
	char data;
} slabmessage;

/*
 * The interpreters is an array of routines for encoding and decoding messages.
 */
typedef int (*interpret)();

typedef struct Interpreter {
	interpret encode;
	interpret decode;
} interpreter;

interpreter interpreters[SLAB_OP_MAX];

extern slabmessage *getSLabBuffer();
extern int extracSLabMessage();
extern int extractV1Message();

