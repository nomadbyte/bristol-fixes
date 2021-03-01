
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

#ifndef BRIGHTONMINI_H
#define BRIGHTONMINI_H

#include "brighton.h"
#include "brightoninternals.h"
#include "bristol.h"

#define BRIGHTON_POLYPHONIC 0
#define BRIGHTON_LNP 1
#define BRIGHTON_HNP 2

extern int configureGlobals();
extern int initConnection();

extern void cleanupBristol();
extern void cleanupBristolQuietly();

extern int bristolMidiSendControlMsg();
extern int bristolMidiSendNRP();
extern int bristolMidiSendRP();
extern int bristolMidiSendMsg();
extern int destroySynth(brightonWindow *);

typedef int (*synthRoutine)(void *, int, int, int, int, int);
typedef int (*saveRoutine)(void *, char *, char *, int, int);
typedef int (*loadRoutine)(void *, char *, char *, int, int, int, int);

typedef struct miniDispatch {
	int controller;
	int operator;
	int other1;
	int other2;
	synthRoutine routine;
} dispatcher;

typedef struct Memory {
	char algo[32];
	char name[32];
	short count;
	short vers;
	short active;
	short pad;
	float *param;
} memory;

#define BRISTOL_NOCALLS		0x01
#define BRISTOL_STAT		0x02
#define BRISTOL_FORCE		0x04
#define BRISTOL_SID2		0x08
#define BRISTOL_SEQLOAD		0x10
#define BRISTOL_SEQFORCE	0x20

#define OPERATIONAL			0x00000001
#define BANK_SELECT			0x00000002
#define MEM_LOADING			0x00000004
#define SUPPRESS			0x00000008
#define NO_KEYTRACK			0x00000010
#define REQ_MIDI_DEBUG		0x00000020
#define MIDI_NRP			0x00000040
#define REQ_MIDI_DEBUG2		0x00000080
#define REQ_EXIT			0x00000100
#define REQ_FORWARD			0x00000200
#define REQ_LOCAL_FORWARD	0x00000400
#define REQ_REMOTE_FORWARD	0x00000800
#define REQ_DEBUG_MASK		0x0000f000
#define REQ_DEBUG_1			0x00001000
#define REQ_DEBUG_2			0x00002000
#define REQ_DEBUG_3			0x00004000
#define REQ_DEBUG_4			0x00008000
#define LADI_ENABLE			0x00010000
#define GUI_NRP				0x00020000
#define NO_LATCHING_KEYS	0x00040000

typedef struct GuiSynth {
	struct GuiSynth *next, *last;
	unsigned int flags;
	char name[32];
	int sid;
	int sid2; /* for GUIs with dual manual connections. */
	int midichannel;
	int velocity;
	int synthtype;
	int voices;
	brightonWindow *win;
	int mbi;
	int bank;
	int location;
	int panel;
	int transpose;
	memory mem;
	memory seq1;
	memory seq2;
	brightonApp *resources;
	dispatcher *dispatch;
	struct guiSynth *second; /* Dual manual keyboards */
	struct guimain *manual; /* Dual manual keyboards */
	int gain;
	int detune;
	int pwd;
	int keypanel;
	int keypanel2;
	int lowkey;
	int highkey;
	int glide;
	int lwf;
	int notepref;
	int notetrig;
	int legatovelocity;
	saveRoutine saveMemory;
	loadRoutine loadMemory;
	int firstDev;
	int cmem;
	int lmem;
	/* LADI support */
	int ladimode;
	int ladimem;
	char *ladiStateFile;
} guiSynth;

#define BRIGHTON_NOENGINE 0x80000000
#define BRIGHTON_NRP	 0x40000000

typedef struct guiMain {
	unsigned int flags;
	char *home;
	int controlfd;
	int enginePID;
	int libtest;
	int voices;
	guiSynth *synths;
	char *host;
	int port;
	int manualfd;
	struct guiMain *manual;
} guimain;

extern guiSynth *findSynth(guiSynth *, brightonWindow *);
extern int bristolMemoryImport(int, char *, char *);
extern int bristolMemoryExport(int, char *, char *);
extern int loadMemory(guiSynth *, char *, char *,  int, int, int, int);
extern int loadSequence(memory *, char *, int, int);
extern int chordInsert(arpeggiatorMemory *, int, int);
extern int seqInsert(arpeggiatorMemory *, int, int);
extern int fillSequencer(guiSynth *, arpeggiatorMemory *, int);
extern int saveSequence(guiSynth *, char *, int, int);
extern int saveMemory(guiSynth *, char *, char *, int, int);
extern int displayText(guiSynth *, char *, int, int);
extern void displayPanel(guiSynth *, char *, int, int, int);
extern int displayPanelText(guiSynth *, char *, int, int, int);

extern void brightonReadConfiguration(brightonWindow *, brightonApp *, int, char *, char *);
extern void brightonWriteConfiguration(brightonWindow *, char *, int, char *);

extern int doAlarm();

#endif /* BRIGHTONMINI_H */

