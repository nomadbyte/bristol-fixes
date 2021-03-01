
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
 * Some structures for device management. These primarily MIDI message control,
 * and eventually some for device management.
 */

#ifndef _BRISTOL_MIDI_API_H
#define _BRISTOL_MIDI_API_H

#include <time.h>
#include <string.h>

#include "bristolmessages.h"

extern int bristolMidiOpen();
extern int bristolMidiClose();
extern int bristolMidiRead();
extern int bristolMidiRawWrite();
extern int bristolMidiWrite();

extern void bristolMidiPrint();
extern void bristolMsgPrint();
extern void bristolMidiPrintGM2();

#define MIDI_CONTROLLER_COUNT 128
#define CONTROLLER_RANGE 16384
#define C_RANGE_MIN_1 (CONTROLLER_RANGE - 1)

/*
 * API Status codes
 */
#define BRISTOL_MIDI_OK 0
#define BRISTOL_MIDI_DEV -1
#define BRISTOL_MIDI_HANDLE -2
#define BRISTOL_MIDI_DEVICE -3
#define BRISTOL_MIDI_DRIVER -4
#define BRISTOL_MIDI_CHANNEL -5

/* Conns are masked with 0x0ff0 */
#define BRISTOL_CONN_UNIX		0x00000010
#define BRISTOL_CONN_TCP		0x00000020
#define BRISTOL_CONN_MIDI		0x00000040
#define BRISTOL_CONN_OSSMIDI	0x00000080 /* As opposed to ALSA */
#define BRISTOL_CONN_SEQ		0x00000100 /* ALSA Sequencer interface */
#define BRISTOL_CONN_JACK		0x00000200 /* Jack Midi interface */

#define BRISTOL_CONN_PASSIVE	0x00001000 /* listen socket. */
#define BRISTOL_CONN_FORCE		0x00002000

#define BRISTOL_CONN_NBLOCK		0x00004000
#define BRISTOL_CONN_SYSEX		0x00008000
#define BRISTOL_CONN_FORWARD	0x00010000

/*
 * API flags
 */
#define BRISTOL_RDONLY 1
#define BRISTOL_WRONLY 2
#define BRISTOL_DUPLEX 3

#define BRISTOL_ACCEPT_SOCKET	0x80000000
#define BRISTOL_CONTROL_SOCKET	0x40000000
#define _BRISTOL_MIDI_DEBUG		0x20000000

/*
 * Midi command codes
 */
#define BRISTOL_MIDI_CHANNELS 16
#define BRISTOL_NOTE_ON (1 << (MIDI_NOTE_ON >> 4))
#define BRISTOL_NOTE_OFF (1 << (MIDI_NOTE_OFF >> 4))

#define MIDI_COMMAND_MASK 0x0f0
#define MIDI_CHAN_MASK 0x0f

#define MIDI_BANK_SELECT	0x00

#define MIDI_STATUS_MASK	0x080
#define MIDI_NOTE_ON		0x090
#define MIDI_NOTE_OFF		0x080
#define MIDI_POLY_PRESS		0x0a0
#define MIDI_CONTROL		0x0b0
#define MIDI_PROGRAM		0x0c0
#define MIDI_CHAN_PRESS		0x0d0
#define MIDI_PITCHWHEEL		0x0e0

#define MIDI_ALL_SOUNDS_OFF	120
#define MIDI_ALL_CONTS_OFF	121
#define MIDI_LOCAL_KBD		122
#define MIDI_ALL_NOTES_OFF	123
#define MIDI_OMNI_ON		124
#define MIDI_OMNI_OFF		125
#define MIDI_POLY_ON		126
#define MIDI_POLY_OFF		127

#define MIDI_RP_PW			0x0
#define MIDI_RP_FINETUNE	0x1
#define MIDI_RP_COARSETUNE	0x2

/*
 * The first just reflect the registered numbers. Breaks if there are more than
 * 127 registered parameters..... Hm.
 */
#define MIDI_NRP_PW			(0x3f80 + MIDI_RP_PW)
#define MIDI_NRP_FINETUNE	(0x3f80 + MIDI_RP_FINETUNE)
#define MIDI_NRP_COARSETUNE	(0x3f80 + MIDI_RP_COARSETUNE)

/* Then there are mine, top page of NRP counting down. */
#define BRISTOL_NRP_NULL		16383 /* = NRP 0x7f-7f = 0x3fff */
#define BRISTOL_NRP_GAIN		16382 /* = NRP 0x7f-7e = 0x3ffe */
#define BRISTOL_NRP_DETUNE		16381 /* = NRP 0x7f-7d = 0x3ffd */
#define BRISTOL_NRP_VELOCITY	16380 /* = NRP 0x7f-7c = 0x3ffc */
#define BRISTOL_NRP_DEBUG		16379 /* = NRP 0x7f-7b = 0x3ffb */
#define BRISTOL_NRP_GLIDE		16378 /* = NRP 0x7f-7a = 0x3ffa */
#define BRISTOL_NRP_ENABLE_NRP	16377 /* = NRP 0x7f-79 = 0x3ff9 */
#define BRISTOL_NRP_LWF			16376 /* = NRP 0x7f-78 = 0x3ff8 */
#define BRISTOL_NRP_MNL_PREF	16375 /* = NRP 0x7f-77 = 0x3ff7 */
#define BRISTOL_NRP_MNL_TRIG	16374 /* = NRP 0x7f-76 = 0x3ff6 */
#define BRISTOL_NRP_MNL_VELOC	16373 /* = NRP 0x7f-75 = 0x3ff5 */
#define BRISTOL_NRP_FORWARD		16372 /* = NRP 0x7f-74 = 0x3ff4 */
#define BRISTOL_NRP_MIDI_GO		16371 /* = NRP 0x7f-73 = 0x3ff3 */
#define BRISTOL_NRP_SYSID_H		16370 /* = NRP 0x7f-72 = 0x3ff2 */
#define BRISTOL_NRP_SYSID_L		16369 /* = NRP 0x7f-71 = 0x3ff1 */
#define BRISTOL_NRP_REQ_SYSEX	16368 /* = NRP 0x7f-70 = 0x3ff0 */
#define BRISTOL_NRP_REQ_FORWARD	16367 /* = NRP 0x7f-6f = 0x3fef */
#define BRISTOL_NRP_REQ_DEBUG	16366 /* = NRP 0x7f-6e = 0x3fee */

#define MIDI_SYSTEM		0x0f0
#define MIDI_SYSEX		0x0f8
#define MIDI_EOS		0x0f7

#define BRISTOL_REQ_ON		0x001
#define BRISTOL_REQ_OFF		0x002
#define BRISTOL_REQ_PP		0x004
#define BRISTOL_REQ_CONTROL	0x008
#define BRISTOL_REQ_PROGRAM	0x010
#define BRISTOL_REQ_CHAN	0x020
#define BRISTOL_REQ_PITCH	0x040
#define BRISTOL_REQ_SYSEX	0x080

#define BRISTOL_REQ_ALL		0x0ff
#define BRISTOL_REQ_NSX		0x07f /* not sysex messages....... */

#define MIDI_GM_DATAENTRY	6
#define MIDI_GM_NRP			99
#define MIDI_GM_RP			101
#define MIDI_GM_DATAENTRY_F	38
#define MIDI_GM_NRP_F		98
#define MIDI_GM_RP_F		100

#define BRISTOL_KF_RAW		0
#define BRISTOL_KF_TCP		1
#define BRISTOL_KF_JACK		2

typedef struct KeyMsg {
	unsigned char key;
	unsigned char velocity;
	unsigned char flags;
} keyMsg;

typedef struct ChanPressMsg {
	unsigned char pressure;
} chanPressMsg;

typedef struct PressureMsg {
	unsigned char key;
	unsigned char pressure;
} pressureMsg;

typedef struct PitchMsg {
	unsigned char lsb;
	unsigned char msb;
} pitchMsg;

typedef struct SysexMsg {
	int length;
	unsigned char *data;
} sysexMsg;

typedef struct ControlMsg {
	unsigned char c_id;
	unsigned char c_val;
} controlMsg;

typedef struct ProgMsg {
	unsigned char p_id;
} programMsg;

typedef struct BristolMidiMsg {
	unsigned char midiHandle;
	unsigned char channel;
	unsigned char mychannel;
	unsigned char command;
	struct timeval timestamp;
	int offset;
	unsigned int sequence;
	union {
		keyMsg key;
		pressureMsg pressure;
		chanPressMsg channelpress;
		pitchMsg pitch;
		controlMsg controller;
		programMsg program;
		bristolMsg bristol;
		bristolMsgType2 bristolt2;
		sysexMsg *sysex;
	} params;
	/*
	 * For continuous controllers we have another table. These may change the
	 * controller value as some are coarse and fine controls. All values are
	 * now floats to respect the interface resolutions. It is up to the synth
	 * to decide which values to use, but anticipated users are the GUI CC
	 * linkage and the engine GM-2 defaults.
	 */
	struct {
		int c_id; /* The NRP will start by reflecting the controllers */
		int c_id_coarse, c_id_fine; /* the two 7 bit c_id from MIDI encoding. */
		float value; /* 0.0 1.0 */
		int intvalue; /* 14 bit value */
		int coarse, fine; /* the two 7 bit values from MIDI encoding. */
	} GM2;
} bristolMidiMsg;

#define DEF_TAB_SIZE 128

typedef struct TableEntry {
	float defnote;
	float rate;
	float gain;
} tableEntry;

/*
 * The remainder are actually specific to bristol, and could be moved to a
 * separate header file.
 */
extern tableEntry defaultTable[DEF_TAB_SIZE];
extern tableEntry gainTable[CONTROLLER_RANGE];

typedef int (*midiHandlerRoutine)();

typedef struct fTab {
	float step;
	float freq;
} fTab;

typedef struct MidiHandler {
	midiHandlerRoutine callback;
	int map[128];
	float floatmap[128];
} midiHandler;

typedef struct BristolMidiHandler {
	midiHandler bmr[8];
	fTab freq[128];
} bristolMidiHandler;

#endif /* _BRISTOL_MIDI_API_H */

