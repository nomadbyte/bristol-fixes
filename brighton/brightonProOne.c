
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


#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int initmem;

static int pro1Init();
static int pro1Configure();
static int pro1Callback(brightonWindow *, int, int, float);
/*static int keyCallback(void *, int, int, float); */
static int proOneKeyCallback(brightonWindow *, int, int, float);
/*static int modCallback(void *, int, int, float); */
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;
static int seqLearn = 0;

#include "brightonKeys.h"

#define DEVICE_COUNT 72
#define ACTIVE_DEVS 52
#define DISPLAY_DEV (DEVICE_COUNT - 1)
#define MEM_START (ACTIVE_DEVS + 2) // 54

#define KEY_PANEL 1

#define R1 150
#define R1S (R1 + 20)
#define R2 (R1 + 245)
#define R2S (R2 + 20)
#define R3 (R2 + 245)
#define R3S (R3 + 20)
#define R4 (R3 + 275)

#define RA (R1 - 38)
#define RB (RA + 144)
#define RC (RB + 142)
#define RD (RC + 145)
#define RE (RD + 144)

#define CD1 63

#define C1 31
#define C2 (C1 + 60)
#define C3 (C2 + 56)

#define C4 (C1 + 155)
#define C4a (C4 + 60)
#define C4b (C4a + 31)
#define C4c (C4b + 31)
#define C5 (C4 + CD1 + 3)
#define C6 (C5 + 58)
#define C7 (C6 + 31)
#define C7S (C7 + 31)
#define C8 (C5 + 124)
#define C8a (C8 + 29)
#define C8b (C8a + 59)
#define C8c (C8b + 32)

#define C7a (C7 + 58)

#define C9 (C8 + 59)

#define C10 (C8 + 102)
#define C11 (C10 + CD1)
#define C11a (C11 + 15)
#define C11b (C11a + 60)
#define C11c (C11 + 3)
#define C11d (C11 + 44)
#define C11e (C11 + 83)
#define C12 (C11 + CD1)

#define C13 (C12 + 75)
#define C14 (C13 + CD1)
#define C15 (C14 + CD1)
#define C16 (C15 + CD1)

#define C17 (C16 + 70)

#define S1 100
#define S2 12
#define S3 70
#define S4 11
#define S5 90
#define S6 15
#define S7 50

/*
 * This structure is for device definition. The structure is defined in 
 * include/brighton.h, further definitions in brighton/brightonDevtable.h and
 * include/brightoninternals.h
 *
 *	typedef int (*brightonCallback)(int, float);
 *	typedef struct BrightonLocations {
 *		int device; 0=rotary, 1=scale, etc.
 *		float relx, rely; relative position with regards to 1000 by 1000 window
 *		float relw, relh; relative height.
 *		int from, to;
 *		brightonCallback callback; specific to this dev
 *		char *image; bitmap. If zero take a device default.
 *		int flags;
 *	} brightonLocations;
 *
 * This example is for a pro1Bristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {
/* mod */
	{"ModFiltEnv", 0, C1, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"ModFiltEnv", 2, C2, R1S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"ModOscBMod", 0, C1, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"ModOscBRt", 2, C2, R2S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"ModLfo", 0, C1, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"ModLfoRt", 2, C2, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	{"ModOSCAFr", 1, C3, RA, S4, S5, 0, 2, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
	{"ModOSCAPWM", 1, C3, RB, S4, S5, 0, 2, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
	{"ModOSCBFr", 1, C3, RC, S4, S5, 0, 2, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
	{"ModOSCBPWM", 1, C3, RD, S4, S5, 0, 2, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
	{"ModFilter", 1, C3, RE, S4, S5, 0, 2, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
/* Osc-A */
	{"OscAFine", 0, C4, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_NOTCH},
	{"OscAOct", 0, C5, R1, S1, S1, 0, 3, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_STEPPED},
	{"OscASaw", 2, C6, R1S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"OscAPuls", 2, C7, R1S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"OscAPW", 0, C8, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"OscASync", 2, C9, R1S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
/* Osc-B */
	{"OscBFine", 0, C4, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_NOTCH},
	{"OscBOct", 0, C5, R2, S1, S1, 0, 3, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_STEPPED},
	{"OscBSaw", 2, C6, R2S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"OscBTri", 2, C7, R2S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"OscBPuls", 2, C7S, R2S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"OscBPW", 0, C8a, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"OscBLFO", 2, C8b, R2S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"OscBKEY", 2, C8c, R2S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
/* LFO */
	{"LFORATE", 0, C4, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"LFOSAW", 2, C4a, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"LFOTRI", 2, C4b, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"LFOPuls", 2, C4c, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
/* Seq */
	{"SEQ 1/2", 2, C8 - 6, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Rec/Play", 2, C8a + 5, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
/* Arpeg */
	{"Up/Down", 1, C10 - 4, R3S - 5, S4, S5, 0, 2, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
/* Mode */
	{"Metro", 2, C11c, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Repeat", 2, C11d, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Drone", 2, C11e, R3S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
/* Mixer */
	{"OSCAMix", 0, C10, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"OSCBMix", 0, C11, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"Noise Mix", 0, C12, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
/* Glide */
	{"Glide", 0, C11a, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"Auto", 2, C11b, R2S, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
/* Filter */
	{"Cutoff", 0, C13, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"Emphasis", 0, C14, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"EnvAmt", 0, C15, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"KeyAmt", 0, C16, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
/* ENV */
	{"Filt Attack", 0, C13, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"Filt Decay", 0, C14, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"Filt Sustain", 0, C15, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
	{"Filt Release", 0, C16, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW},
/* ENV */
	{"Attack", 0, C13, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_HALFSHADOW},
	{"Decay", 0, C14, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_HALFSHADOW},
	{"Sustain", 0, C15, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_HALFSHADOW},
	{"Release", 0, C16, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_HALFSHADOW},
/* Tune Vol */
	{"VolumeTuning", 0, C17, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_NOTCH|BRIGHTON_REDRAW},
	{"MasterVolume", 0, C17, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob9.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_HALFSHADOW},
/* memories */
	{"", 2, C10 + 10, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C10 + 30, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C10 + 50, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C10 + 70, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C10 + 90, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C10 + 110, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C10 + 130, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C10 + 150, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C4, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C4 + 30, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C4 + 60, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* Mem search buttons */
	{"", 2, C10 + 180, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C10 + 210, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C10 + 240, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

	{"", 4, 798, 870, 125, 100, 0, 1, 0, "bitmaps/images/pro1.xpm", 0, 0},

/* Midi, perhaps eventually file import/export buttons */
	{"", 2, C1, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C2 + 5, R4, S6, S7, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

	{"", 3, C5 + 23, R4, 200, S7 - 10, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}
};

/*
 * These are new mod controls for the Pro-1. May be used elsewhere
 */
static brightonLocations promods[2] = {
	{"", BRIGHTON_MODWHEEL, 300, 220, 150, 350, 0, 1, 0,
		"bitmaps/knobs/modwheel.xpm", 0,
		BRIGHTON_HSCALE|BRIGHTON_NOSHADOW|BRIGHTON_CENTER|BRIGHTON_NOTCH},
	{"", BRIGHTON_MODWHEEL, 640, 220, 150, 350, 0, 1, 0,
		"bitmaps/knobs/modwheel.xpm", 0,
		BRIGHTON_HSCALE|BRIGHTON_NOSHADOW},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp pro1App = {
	"pro1",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal4.xpm",//"bitmaps/textures/metal7.xpm",
	BRIGHTON_STRETCH,//|BRIGHTON_REVERSE|BRIGHTON_VERTICAL, //flags
	pro1Init,
	pro1Configure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	895, 500, 0, 0,
	5,
	{
		{
			"Pro1",
			"bitmaps/blueprints/pro1.xpm",
			0, // "bitmaps/textures/metal5.xpm",
			0, /*BRIGHTON_STRETCH, // flags */
			0,
			0,
			pro1Callback,
			25, 0, 950, 650,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/ekbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			proOneKeyCallback,
			140, 680, 845, 300,
			KEY_COUNT_3OCTAVE,
			keys3octave
		},
		{
			"Mods",
			0, //"bitmaps/blueprints/pro1mod.xpm",
			"bitmaps/buttons/blue.xpm", /* flags */
			0,
			0,
			0,
			modCallback,
			25, 680, 110, 320,
			2,
			promods
		},
		{
			"Metal side",
			0,
			"bitmaps/textures/wood4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 25, 1000,
			0,
			0
		},
		{
			"Metal side",
			0,
			"bitmaps/textures/wood4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			976, 0, 25, 1000,
			0,
			0
		},
	}
};

static void
pro1SeqInsert(guiSynth *synth, int note, int layer)
{
	arpeggiatorMemory *seq = (arpeggiatorMemory *) synth->seq1.param;

	if (seq->s_max == 0)
		seq->s_dif = note + synth->transpose;

	seq->sequence[(int) (seq->s_max)] =
//		(float) (note + synth->transpose - seq->s_dif);
		(float) (note + synth->transpose);

//	if (pro10Debug(synth, 0))
		printf("Seq put %i into %i\n", (int) seq->sequence[(int) (seq->s_max)],
			(int) (seq->s_max));

	if ((seq->s_max += 1) >= BRISTOL_SEQ_MAX)
		seq->s_max = BRISTOL_SEQ_MAX;
}

static int
proOneKeyCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (global.libtest)
		return(0);

//printf("proOneKeyCallback(%i, %i, %f) %i\n", panel, index, value, seqLearn);

	if ((seqLearn) && (value != 0))
		pro1SeqInsert(synth, index, panel);

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 */
	if (value)
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
	else
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);

	return(0);
}

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->bank = value - (value % 8);
			synth->location = value % 8;
			loadMemory(synth, "pro1", 0, synth->bank * 10 + synth->location,
				synth->mem.active, 0, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

/*static dispatcher dispatch[DEVICE_COUNT]; */

static int
pro1MidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
pro1Sequence(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (synth->seq1.param == NULL)
	{
		/* This does not actually reload a sequence..... FFS, see Pro10 code */
		loadSequence(&synth->seq1, "pro1", 0, 0);
		fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
	}
	switch (o) {
		case 0:
			/* On/Off */
			if (v == 0) {
				seqLearn = 0;
				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			} else {
				seqLearn = 0;
				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_DIR, BRISTOL_ARPEG_UP);
				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_TRIGGER, 1);
				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RANGE, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 1);
			}
			break;
		case 1:
			/* Record/Play */
			if (v == 0) {
				seqLearn = 0;
				/* Turn off sequence learning */
				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
				/* If we are configured to play then start else stop */
				if (synth->mem.param[29] != 0) {
					bristolMidiSendMsg(global.controlfd, synth->sid,
						BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_TRIGGER, 1);
					bristolMidiSendMsg(global.controlfd, synth->sid,
						BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_DIR, BRISTOL_ARPEG_UP);
					bristolMidiSendMsg(global.controlfd, synth->sid,
						BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RANGE, 0);
					bristolMidiSendMsg(global.controlfd, synth->sid,
						BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 1);
				} else
					bristolMidiSendMsg(global.controlfd, synth->sid,
						BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			} else {
				arpeggiatorMemory *seq = (arpeggiatorMemory *) synth->seq1.param;
				seqLearn = 1;
				seq->s_max = 0;

				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid,
					BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 1);
			}
			break;
	}
}

static void
pro1Arpeggiate(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * See if this is rate. Send the request to both the LFO and the arpeg
	 */
	if (c != 126)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RATE, v);
		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RATE, v);
		bristolMidiSendMsg(global.controlfd, synth->sid,
			2, 0, v);
		return;
	}

	switch (v) {
		case 2:
			/* Arpeggiation up */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_DIR, BRISTOL_ARPEG_UP);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, 1);
			break;
		case 1:
			/* Arpeggiation off */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_TRIGGER, 1);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RANGE, 2);
			break;
		case 0:
			/* Arpeggiation up/down */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_DIR, BRISTOL_ARPEG_UD);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, 1);
			break;
	}
}

static void
pro1Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int bank = synth->bank;
	int location = synth->location;

	event.value = 1.0;
	event.type = BRISTOL_FLOAT;

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[MEM_START].other2)
	{
		synth->dispatch[MEM_START].other2 = 0;
		return;
	}

	switch (c) {
		default:
		case 0:
			/*
			 * We want to make these into memory buttons. To do so we need to
			 * know what the last active button was, and deactivate its 
			 * display, then send any message which represents the most
			 * recently configured value. Since this is a memory button we do
			 * not have much issue with the message, but we are concerned with
			 * the display.
			 */
			if (synth->dispatch[MEM_START].other1 != -1)
			{
				synth->dispatch[MEM_START].other2 = 1;

				if (synth->dispatch[MEM_START].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[MEM_START].other1 + MEM_START - 1, &event);
			}
			synth->dispatch[MEM_START].other1 = o;

			if (synth->flags & BANK_SELECT) {
				if ((synth->bank * 10 + o) >= 100)
				{
					synth->location = o;
					synth->flags &= ~BANK_SELECT;

					if (loadMemory(synth, "pro1", 0,
						synth->bank * 10 + synth->location, synth->mem.active,
							0, BRISTOL_STAT) < 0)
						displayText(synth, "FREE MEM",
							synth->bank * 10 + synth->location, DISPLAY_DEV);
					else
						displayText(synth, "PROGRAM",
							synth->bank * 10 + synth->location, DISPLAY_DEV);
				} else {
					synth->bank = synth->bank * 10 + o;
					displayText(synth, "BANK",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
				}
			} else {
				if (synth->bank < 1)
					synth->bank = 1;
				synth->location = o;
				if (loadMemory(synth, "pro1", 0,
					synth->bank * 10 + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FREE MEM",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
				else
					displayText(synth, "PROGRAM",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
			}
			break;
		case 1:
			if (synth->bank < 1)
				synth->bank = 1;
			if (synth->location == 0)
				synth->location = 1;
			if (loadMemory(synth, "pro1", 0, synth->bank * 10 + synth->location,
				synth->mem.active, 0, BRISTOL_FORCE) < 0)
				displayText(synth, "FREE MEM",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
			else {
				displayText(synth, "PROGRAM",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
				loadSequence(&synth->seq1,
					"pro1", synth->bank * 10 + synth->location, 0);
				fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
			}
			synth->flags &= ~BANK_SELECT;
			break;
		case 2:
			if (synth->bank < 1)
				synth->bank = 1;
			if (synth->location == 0)
				synth->location = 1;
			saveMemory(synth, "pro1", 0, synth->bank * 10 + synth->location,
				0);
			saveSequence(synth, "pro1", synth->bank * 10 + synth->location, 0);
			displayText(synth, "PROGRAM", synth->bank * 10 + synth->location,
				DISPLAY_DEV);
			synth->flags &= ~BANK_SELECT;
			break;
		case 3:
			if (synth->flags & BANK_SELECT) {
				synth->flags &= ~BANK_SELECT;
				if (loadMemory(synth, "pro1", 0,
					synth->bank * 10 + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FREE MEM",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
				else {
					displayText(synth, "PROGRAM",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
					loadSequence(&synth->seq1,
						"pro1", synth->bank * 10 + synth->location, 0);
					fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
				}
			} else {
				synth->bank = 0;
				displayText(synth, "BANK", synth->bank, DISPLAY_DEV);
				synth->flags |= BANK_SELECT;
			}
			break;
		case 4:
			if (--location < 1) {
				location = 8;
				if (--bank < 1)
					bank = 88;
			}
			while (loadMemory(synth, "pro1", 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_STAT) < 0)
			{
				if (--location < 1) {
					location = 8;
					if (--bank < 1)
						bank = 88;
				}
				if ((bank * 10 + location)
					== (synth->bank * 10 + synth->location))
					break;
			}
			displayText(synth, "PROGRAM", bank * 10 + location, DISPLAY_DEV);
			synth->bank = bank;
			synth->location = location;
			loadMemory(synth, "pro1", 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_FORCE);
			loadSequence(&synth->seq1, "pro1", bank * 10 + location, 0);
			fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			break;
		case 5:
			if (++location > 8) {
				location = 1;
				if (++bank > 88)
					bank = 1;
			}
			while (loadMemory(synth, "pro1", 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_STAT) < 0)
			{
				if (++location > 8) {
					location = 1;
					if (++bank > 88)
						bank = 1;
				}
				if ((bank * 10 + location)
					== (synth->bank * 10 + synth->location))
					break;
			}
			displayText(synth, "PROGRAM", bank * 10 + location, DISPLAY_DEV);
			synth->bank = bank;
			synth->location = location;
			loadMemory(synth, "pro1", 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_FORCE);
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			loadSequence(&synth->seq1, "pro1", bank * 10 + location, 0);
			fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
			break;
		case 6:
			/* Find the next free mem */
			if (++location > 8) {
				location = 1;
				if (++bank > 88)
					bank = 1;
			}

			while (loadMemory(synth, "pro1", 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_STAT) >= 0)
			{
				if (++location > 8) {
					location = 1;
					if (++bank > 88)
						bank = 1;
				}
				if ((bank * 10 + location)
					== (synth->bank * 10 + synth->location))
					break;
			}

			if (loadMemory(synth, "pro1", 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_STAT) >= 0)
				displayText(synth, "PROGRAM",
					bank * 10 + location, DISPLAY_DEV);
			else
				displayText(synth, "FREE MEM",
					bank * 10 + location, DISPLAY_DEV);

			synth->bank = bank;
			synth->location = location;
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			break;
	}
/*	printf("	pro1Memory(B: %i L %i: %i)\n", */
/*		synth->bank, synth->location, o); */
}

static int
pro1Midi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			return(0);
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 16)
		{
			synth->midichannel = 15;
			return(0);
		}
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

/*	printf("P: going to display: %x, %x\n", synth, synth->win); */
	displayText(synth, "MIDI CH", synth->midichannel + 1, DISPLAY_DEV);

	return(0);
}

static void
pro1ShowParam(guiSynth *synth, int index, float value)
{
	char showthis[64];

	if (index >= pro1App.resources[0].ndevices)
		return;

	if (pro1App.resources[0].devlocn[index].name[0] == '\0') {
		if (pro1App.resources[0].devlocn[index].to == 1.0)
			sprintf(showthis, "%i: %1.3f", index, value);
		else
			sprintf(showthis, "%i: %1.1f", index, value);
	} else {
		if (pro1App.resources[0].devlocn[index].to == 1.0)
			sprintf(showthis, "%s: %1.3f",
				pro1App.resources[0].devlocn[index].name,
				value);
		else
			sprintf(showthis, "%s: %1.1f",
				pro1App.resources[0].devlocn[index].name,
				value);
	}

	displayPanel(synth, showthis, index, 0, DISPLAY_DEV);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
pro1Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/* printf("pro1Callback(%i, %f): %x\n", index, value, synth); */

	if ((synth->flags & MEM_LOADING) == 0)
		pro1ShowParam(synth, index, value);

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (pro1App.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * (CONTROLLER_RANGE - 1);
	else
		sendvalue = value;

	synth->mem.param[index] = value;

	if ((!global.libtest) || (index >= ACTIVE_DEVS))
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

#ifdef DEBUG
	else
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#endif

	return(0);
}

static void
pro1Hold(guiSynth *synth)
{
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->mem.param[34])
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|1);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|0);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
pro1Init(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	dispatcher *dispatch;
	int i;

	if (synth == 0)
	{
		synth = findSynth(global.synths, (int) 0);
		if (synth == 0)
		{
			printf("cannot init\n");
			return(0);
		}
	}

	if ((initmem = synth->location) == 0)
		initmem = 11;

	synth->win = win;

	printf("Initialise the pro1 link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
	synth->dispatch = (dispatcher *)
		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher));
	dispatch = synth->dispatch;

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets.
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);
	}

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = pro1MidiSendMsg;

	/* Mods source */
	dispatch[0].controller = 3;
	dispatch[0].operator = 4;
	dispatch[1].controller = 126;
	dispatch[1].operator = 3;
	dispatch[2].controller = 126;
	dispatch[2].operator = 4;
	dispatch[3].controller = 126;
	dispatch[3].operator = 5;
	dispatch[4].controller = 126;
	dispatch[4].operator = 6;
	dispatch[5].controller = 126;
	dispatch[5].operator = 7;

	/* Mods dest */
	dispatch[6].controller = 126;
	dispatch[6].operator = 10;
	dispatch[7].controller = 126;
	dispatch[7].operator = 11;
	dispatch[8].controller = 126;
	dispatch[8].operator = 12;
	dispatch[9].controller = 126;
	dispatch[9].operator = 13;
	dispatch[10].controller = 126;
	dispatch[10].operator = 14;

	/* Osc A */
	dispatch[11].controller = 0;
	dispatch[11].operator = 2;
	dispatch[12].controller = 0;
	dispatch[12].operator = 1;
	dispatch[13].controller = 0;
	dispatch[13].operator = 4;
	dispatch[14].controller = 0;
	dispatch[14].operator = 6;
	dispatch[15].controller = 0;
	dispatch[15].operator = 0;
	dispatch[16].controller = 0;
	dispatch[16].operator = 7;

	/* Osc B */
	dispatch[17].controller = 1;
	dispatch[17].operator = 2;
	dispatch[18].controller = 1;
	dispatch[18].operator = 1;
	dispatch[19].controller = 1;
	dispatch[19].operator = 4;
	dispatch[20].controller = 1;
	dispatch[20].operator = 5;
	dispatch[21].controller = 1;
	dispatch[21].operator = 6;
	dispatch[22].controller = 1;
	dispatch[22].operator = 0;
	dispatch[23].controller = 126;
	dispatch[23].operator = 18;
	dispatch[24].controller = 126;
	dispatch[24].operator = 19;

	/* LFO */
	dispatch[25].controller = 2;
	dispatch[25].operator = 0;
	dispatch[25].routine = (synthRoutine) pro1Arpeggiate;
	dispatch[26].controller = 126;
	dispatch[26].operator = 24;
	dispatch[27].controller = 126;
	dispatch[27].operator = 25;
	dispatch[28].controller = 126;
	dispatch[28].operator = 26;

	/* Seqx2/Arpeg/modex3 */
	dispatch[29].controller = 126;
	dispatch[29].operator = 0;
	dispatch[29].routine = (synthRoutine) pro1Sequence;
	dispatch[30].controller = 126;
	dispatch[30].operator = 1;
	dispatch[30].routine = (synthRoutine) pro1Sequence;
	dispatch[31].controller = 126;
	dispatch[31].operator = 101;
	dispatch[31].routine = (synthRoutine) pro1Arpeggiate;
	/* Modes, need to read up on function */
	dispatch[32].controller = 126; /* multitrig envelope note */
	dispatch[32].operator = 27;
	dispatch[33].controller = 126; /* Track LFO as trigger */
	dispatch[33].operator = 9;
	dispatch[34].controller = 126; /* Force gate open */
	dispatch[34].operator = 28;
	dispatch[34].routine = (synthRoutine) pro1Hold; /* Drone */

	/* Mixer */
	dispatch[35].controller = 126;
	dispatch[35].operator = 20;
	dispatch[36].controller = 126;
	dispatch[36].operator = 21;
	dispatch[37].controller = 126;
	dispatch[37].operator = 22;

	/* Glide */
	dispatch[38].controller = 126;
	dispatch[38].operator = 0;
	dispatch[39].controller = 126;
	dispatch[39].operator = 8;

	/* Filter and Env */
	dispatch[40].controller = 4;
	dispatch[40].operator = 0;
	dispatch[41].controller = 4;
	dispatch[41].operator = 1;
	dispatch[42].controller = 126;
	dispatch[42].operator = 23;
	dispatch[43].controller = 4;
	dispatch[43].operator = 3;
	dispatch[44].controller = 3;
	dispatch[44].operator = 0;
	dispatch[45].controller = 3;
	dispatch[45].operator = 1;
	dispatch[46].controller = 3;
	dispatch[46].operator = 2;
	dispatch[47].controller = 3;
	dispatch[47].operator = 3;

	/* Env */
	dispatch[48].controller = 5;
	dispatch[48].operator = 0;
	dispatch[49].controller = 5;
	dispatch[49].operator = 1;
	dispatch[50].controller = 5;
	dispatch[50].operator = 2;
	dispatch[51].controller = 5;
	dispatch[51].operator = 3;

	/* Tune and Master */
	dispatch[52].controller = 126;
	dispatch[52].operator = 2;
	dispatch[53].controller = 5;
	dispatch[53].operator = 4;

	/* Memory and MIDI */
	dispatch[MEM_START + 0].operator = 1;
	dispatch[MEM_START + 1].operator = 2;
	dispatch[MEM_START + 2].operator = 3;
	dispatch[MEM_START + 3].operator = 4;
	dispatch[MEM_START + 4].operator = 5;
	dispatch[MEM_START + 5].operator = 6;
	dispatch[MEM_START + 6].operator = 7;
	dispatch[MEM_START + 7].operator = 8;

	dispatch[MEM_START + 8].controller = 1;
	dispatch[MEM_START + 9].controller = 2;
	dispatch[MEM_START + 10].controller = 3;
	dispatch[MEM_START + 11].controller = 4;
	dispatch[MEM_START + 12].controller = 5;
	dispatch[MEM_START + 13].controller = 6;

	dispatch[MEM_START + 0].routine =
		dispatch[MEM_START + 1].routine =
		dispatch[MEM_START + 2].routine =
		dispatch[MEM_START + 3].routine =
		dispatch[MEM_START + 4].routine =
		dispatch[MEM_START + 5].routine =
		dispatch[MEM_START + 6].routine =
		dispatch[MEM_START + 7].routine =
		dispatch[MEM_START + 8].routine =
		dispatch[MEM_START + 9].routine =
		dispatch[MEM_START + 10].routine =
		dispatch[MEM_START + 11].routine =
		dispatch[MEM_START + 12].routine =
		dispatch[MEM_START + 13].routine = (synthRoutine) pro1Memory;

	dispatch[DEVICE_COUNT - 3].routine = dispatch[DEVICE_COUNT - 2].routine =
		(synthRoutine) pro1Midi;
	dispatch[DEVICE_COUNT - 3].controller = 1;
	dispatch[DEVICE_COUNT - 2].controller = 2;

	dispatch[MEM_START].other1 = -1;

	/* No velocity on filter contour */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 5, 0);
	/* LFO retrigger on keypress */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 4);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
pro1Configure(brightonWindow* win)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	if (synth == 0)
	{
		printf("problems going operational\n");
		return(-1);
	}

	if (synth->flags & OPERATIONAL)
		return(0);

printf("going operational\n");

	synth->flags |= OPERATIONAL;
	synth->keypanel = 1;
	synth->keypanel2 = -1;
	synth->transpose = 36;

	if (synth->location == 0)
	{
		synth->bank = 1;
		synth->location = 1;
	} else {
		synth->bank = synth->location / 10;
		synth->location %= 10;
	}
	loadMemory(synth, "pro1", 0, initmem, synth->mem.active, 0, 0);
	loadSequence(&synth->seq1, "pro1", initmem, 0);
	fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);

	brightonPut(win,
		"bitmaps/blueprints/pro1shade.xpm", 0, 0, win->width, win->height);

	event.type = BRIGHTON_FLOAT;
	/* Tune */
	event.value = 0.5;
	brightonParamChange(synth->win, 0, ACTIVE_DEVS + 0, &event);
	/* Volume */
	event.value = 0.9;
	brightonParamChange(synth->win, 0, ACTIVE_DEVS + 1, &event);
	/* First memory location */
	event.value = 1.0;
	brightonParamChange(synth->win, 0, MEM_START, &event);
	/* Some small amount of mod wheel */
	event.value = 0.2;
	brightonParamChange(synth->win, 2, 1, &event);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);
	return(0);
}

