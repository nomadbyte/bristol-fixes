
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
 * Some structures for device management. These are for the library internal,
 * and not for anybody using the API. If you want to use the midi library, use
 * bristolmdidiapi.h
 *
 * This controls messages on any given midi channel, handles that have been
 * opened for any channel, and global control structures.
 */

#ifndef _BRISTOL_MIDI_H
#define _BRISTOL_MIDI_H

#include "bristol.h"
#include "bristolaudio.h"

#if (BRISTOL_HAS_ALSA == 1)
#include <alsa/asoundlib.h>
#include <alsa/seq.h>
#endif /* BRISTOL_HAS_ALSA */

#include "bristolmidiapi.h"

#define BRISTOL_SOCKNAME "/tmp/.bristol"

#define BRISTOL_PERMMASK 0x00f
#define BRISTOL_CONNMASK 0xff0

/*
 * Relogging flags
 */
#define BRISTOL_LOG_TERMINATE -1
#define BRISTOL_LOG_BRISTOL 0
#define BRISTOL_LOG_BRIGHTON 1
#define BRISTOL_LOG_DAEMON 2 
#define BRISTOL_LOG_SYSLOG 3
#define BRISTOL_LOG_CONSOLE 4
#define BRISTOL_LOG_DISYNTHEGRATE 5

/*
 * Global limits
 */
#define BRISTOL_MIDI_DEVCOUNT 32
#define BRISTOL_MIDI_HANDLES 32
#define BRISTOL_MIDI_CHCOUNT 64
#define BRISTOL_MIDI_BUFSIZE 64

/*
 * Control flags
#define BRISTOL_ALSA_RAWMIDI 1
#define BRISTOL_OSS_RAWMIDI 2
#define BRISTOL_ALSA_SEQ 3
 */

#define BRISTOL_MIDI_TERMINATE	0x80000000
#define BRISTOL_MIDI_INITTED	0x40000000
#define BRISTOL_MIDI_FORWARD	0x20000000
#define BRISTOL_MIDI_FHOLD		0x10000000
#define BRISTOL_MIDI_GO			0x08000000
#define BRISTOL_BMIDI_DEBUG		0x04000000

/*
 * Channel state flags
 */
#define BRISTOL_CHANSTATE_WAIT 0
#define BRISTOL_CHANSTATE_WAIT_1 1
#define BRISTOL_CHANSTATE_WAIT_2 2
#define BRISTOL_CHANSTATE_WAIT_3 3

typedef struct BristolMidiChannel {
	int channel;
	int handle;
	int command;
	int state;
	int count; /* Of current number of bytes for this command */
} bristolMidiChannel;

typedef struct BristolMidiHandle {
	int handle;
	int state;
	int channel;
	int dev;
	unsigned int flags;
	int messagemask;
	int (*callback)();
	void *param;
} bristolMidiHandle;

#if (BRISTOL_HAS_ALSA == 1)
typedef struct BristolALSADev {
	snd_rawmidi_t *handle; /* ALSA driver handle */
//	snd_seq_t *seq_handle;
} bristolALSADev;

typedef struct BristolSeqDev {
	snd_seq_t *handle; /* ALSA driver handle */
} bristolSeqDev;
#endif /* BRISTOL_HAS_ALSA */

typedef struct BristolMidiDev {
	char name[64];
	int state;
	unsigned int flags;
	int fd;
	int lastchan;
	int lastcommand;
	int lastcommstate;
	unsigned int sequence;
	int handleCount; /* numberof handles using this dev */
	struct {
		int count;
		bristolMsg *bm;
	} sysex;
	union {
#if (BRISTOL_HAS_ALSA == 1)
		bristolALSADev alsa; /* ALSA driver handle */
		bristolSeqDev seq; /* ALSA sequencer driver handle */
#endif
		/*
		 * And descriptor types for any other dev libraries.
		 */
	} driver;
	unsigned char buffer[BRISTOL_MIDI_BUFSIZE * 2];
	int bufcount;
	int bufindex;
	bristolMidiChannel I_channel[BRISTOL_MIDI_CHANNELS];
	bristolMidiChannel O_channel[BRISTOL_MIDI_CHANNELS];
	bristolMidiMsg msg;
} bristolMidiDev;

typedef struct BristolMidiMain {
	unsigned int flags;
	unsigned int SysID;
	bristolMidiDev dev[BRISTOL_MIDI_DEVCOUNT];
	bristolMidiHandle handle[BRISTOL_MIDI_HANDLES];
	int (*msgforwarder)();
/*	int GM2values[128][16]; // Values all controllers by channel */
/*	int mapping[128][16]; // default midi conntroller mapping table. */
} bristolMidiMain;

extern int bristolGetMidiFD(int);
extern int bristolMidiDevRead(int, bristolMidiMsg *);
extern int bristolMidiTCPRead(bristolMidiMsg *);
extern void bristolMidiPost(bristolMidiMsg *);
extern int bristolMidiSendNRP(int, int, int, int);
extern int bristolMidiSendRP(int, int, int, int);
extern int bristolMidiSendMsg(int, int, int, int, int);
extern int bristolMidiSendKeyMsg(int, int, int, int, int);

extern int bristolMidiControl(int, int, int, int, int);

extern int bristolMidiOption(int, int, int);
extern void bristolMidiRegisterForwarder(int (*)());


extern char *getBristolCache(char *);
extern void resetBristolCache();
#define NO_INTERPOLATE 0x01
extern int bristolGetMap(char *, char *, float *, int, int);
extern int bristolGetFreqMap(char *, char *, fTab *, int, int, int);

extern int bristolParseScala(char *, float *);

extern pthread_t bristolOpenStdio(int);

extern void bristolMidiValueMappingTable(u_char [128][128], int [128], char *);
extern void bristolMidiToGM2(int [128], int [128], u_char [128][128], bristolMidiMsg *);
extern int bristolMidiRawToMsg(unsigned char *, int, int, int, bristolMidiMsg *);
extern int midiMsgHandler(bristolMidiMsg *, audioMain *);

extern int buildCurrentTable(Baudio *, float);

#endif /* _BRISTOL_MIDI_H */

