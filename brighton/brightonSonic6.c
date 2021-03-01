
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
 * Highkey/Lowkey will govern glide only.
 * Integrate ADSR - GUI Done
 * Add mod to GenX/Y - GUI Done
 * Add PWM
 * Add options for LFO multi/uni
 * Add modwheel - GUI Done
 * Add LFO for LFO gain?
 * Add stereo reverb?
 *
 * Add global tuning.
 *
 * Octave is -/+2
 */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

int initmem = 0;

int sonic6Init();
int sonic6Configure();
int sonic6Callback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);
static int sonic6ModCallback(brightonWindow *, int, int, float);

extern guimain global;

extern int empty;

#include "brightonKeys.h"

#define KEY_PANEL 1

#define FIRST_DEV 0
#define DEVICE_COUNT 85
#define ACTIVE_DEVS 60
#define MEM_START (ACTIVE_DEVS + 8)

#define DISPLAY_DEV (DEVICE_COUNT - 1)

#define S1 80
#define S2 60
#define S3 13
#define S4 60
#define S5 25
#define S6 30
#define S7 70

#define B1 15
#define B2 110
#define B3 37
#define B4 42

#define R14 200
#define R24 400
#define R34 600
#define R44 800

#define R15 200
#define R25 350
#define R35 500
#define R45 650
#define R55 800

#define C1 25
#define C2 90
#define C3 155
#define C4 220
#define C5 285
#define C6 350
#define C6I 395

#define C7 615
#define C8 665
#define C9 742
#define C10 810
#define C11 875
#define C12 945

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
 * This example is for a sonic6Bristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {
/* LFO Master - 0*/
	{"LFO-MasterF", 1, 114, 490, 13, 180, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
/* LFO 1 */
	{"LFO-X-Wave", 0, 40, 253, S1, S1, 0, 3, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_STEPPED},
	{"LFO-X-Rate", 1, 90, 230, 13, 180, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"LFO-Dest", 2, 40, 370, S3, S4, 0, 2, 0,
		"sonicrocker", 0, BRIGHTON_THREEWAY},
/* LFO 2  - 4*/
	{"LFO-Y-Rate", 1, 135, 230, 13, 180, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"LFO-Wave", 0, 168, 253, S1, S1, 0, 3, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_STEPPED},
	{"LFO-Dest", 2, 188, 370, S3, S4, 0, 2, 0,
		"sonicrocker", 0, BRIGHTON_THREEWAY},
/* LFO Mix */
	{"LFO-MIX", 0, 106, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_NOTCH},

/* Osc 1 - 8 */
	{"OscA-Tuning", 0, 230, 260, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_NOTCH},
	{"OscA-PW", 0, 230, 360, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"OscA-Transpose", 2, 278, 280, S5, S6, 0, 2, 0,
		"sonicrocker", 0, BRIGHTON_VERTICAL|BRIGHTON_THREEWAY},
	/* This can replace the previous for a 5 octave span */
//	{"OscA-F", 0, 278, 260, S2, S2, 0, 4, 0, 
//		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_STEPPED},
	{"OscA-Waveform", 2, 278, 380, S5, S6, 0, 2, 0,
		"sonicrocker", 0, BRIGHTON_VERTICAL|BRIGHTON_THREEWAY},

	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},

	{"OscA-FM-XY", 0, 230, 518, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"OscA-FM-ADSR", 0, 285, 518, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"OscA-Mode", 2, 260, 640, S5, S6, 0, 2, 0,
		"sonicrocker", 0, BRIGHTON_VERTICAL|BRIGHTON_THREEWAY},
/* Osc 2 - 17 */
	{"OscB-Tuning", 0, 390, 260, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_NOTCH},
	{"OscB-PW", 0, 390, 360, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"OscB-Transpose", 2, 337, 280, S5, S6, 0, 2, 0,
		"sonicrocker", 0, BRIGHTON_VERTICAL|BRIGHTON_THREEWAY},
	/* This can replace the previous for a 5 octave span */
//	{"OscA-F", 0, 337, 260, S2, S2, 0, 4, 0, 
//		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_STEPPED},
	{"OscB-Waveform", 2, 337, 380, S5, S6, 0, 2, 0,
		"sonicrocker", 0, BRIGHTON_VERTICAL|BRIGHTON_THREEWAY},

	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},

	{"OscB-FM-XY", 0, 335, 518, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"OscB-FM-OscA", 0, 390, 518, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"OscB-PWM", 0, 362, 616, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
/* Osc Mix - 26 */
	{"Osc-A/B-Mix", 0, 309, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_NOTCH},

/* Ring Mod */
	{"RM-Src1", 2, 460, 274, S5, S6, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", BRIGHTON_VERTICAL},
	{"RM-Src2", 2, 460, 380, S5, S6, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", BRIGHTON_VERTICAL},

/* Noise */
	{"WhitePink", 2, 469, 580, S3, S4 + 10, 0, 1, 0,
		"bitmaps/buttons/sonicrocker3.xpm",
		"bitmaps/buttons/sonicrocker1.xpm", 0},

/* Mixer - 30 */
	{"Mix-OscA/B", 0, 548, 260, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Mix-RM", 0, 548, 370, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Mix-Ext", 0, 548, 480, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Mix-Noise", 0, 548, 590, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},

/* Bypass */
	{"Bypass", 2, 646, 225, S5, S6, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", BRIGHTON_VERTICAL},

/* Env - 35 */
	{"Attack", 0, 625, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Decay", 0, 677, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Sustain", 0, 729, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Release", 0, 781, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
/* Env Type */
	{"EnvType", 0, 647, 420, S2, S2, 0, 3, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_STEPPED},
	{"Velocity", 2, 648, 495, S5, S6, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", BRIGHTON_VERTICAL},

/* Filter - 41 */
	{"VCF-Cutoff", 1, 740, 260, 13, 270, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"VCF-Res", 1, 784, 260, 13, 270, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},

/* Direct output mixer */
	{"Direct-OscA", 0, 845, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Direct-OscB", 0, 890, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Direct-Ringmod", 0, 935, 88, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},

/* Triggers */
	{"TriggerKBD", 2, 625, 612, S3, S4 + 10, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", 0},
	{"TriggerLFO-X", 2, 655, 612, S3, S4 + 10, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", 0},
	{"TriggerLFO-Y", 2, 685, 612, S3, S4 + 10, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", 0},

/* Pitch control - 49 */
	{"Osc1-FM-ADSR", 0, 728, 616, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"Osc1-FM-KBD", 2, 762, 612, S3, S4 + 10, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", 0},
	{"Osc1-FM-X/Y", 0, 787, 616, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},

/* Memories dummies - 52 */
	/* Multi LFO-X */
	{"LFO-X-Multi", 2, 150, 884, S3, S4 + 4, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", 0},
	/* Multi LFO-X */
	{"LFO-Y-Multi", 2, 180, 884, S3, S4 + 4, 0, 1, 0,
		"bitmaps/buttons/sonicrocker1.xpm",
		"bitmaps/buttons/sonicrocker3.xpm", 0},
	{"is dummydummy", 0, 100, 880, S7, S7, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},

	{"FX-1", 0, 807, 880, S7, S7, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"FX-2", 0, 842, 880, S7, S7, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"FX-3", 0, 877, 880, S7, S7, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"FX-4", 0, 912, 880, S7, S7, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, 0},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},

/* Global controls (not in memories - 60 */
	{"Global", 0, 60, 880, S7, S7, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_NOTCH},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},
	{"dummy", 0, 0, 0, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob4.xpm", 0, BRIGHTON_WITHDRAWN},

/* REST IN HERE - memory, MIDI */
/* memories */
	{"", 2, 847, 300, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, 877, 300, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, 907, 300, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, 937, 300, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, 847, 380, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, 877, 380, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, 907, 380, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, 937, 380, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	/* Load Save, Bank */
	{"", 2, 847, 460, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 877, 460, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, 847, 540, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* Mem search buttons */
	{"", 2, 907, 540, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 907, 460, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 877, 540, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

/*
	{"", 2, 907, 460, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 847, 540, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 877, 540, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 907, 540, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
*/
/* Midi, perhaps eventually file import/export buttons */
	{"", 2, 937, 540, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 937, 460, 16, 50, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

	/* display */
	{"", 3, 835, 226, 130, 40, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0},
};

/*
 * These are the Sonic mods, glide, volume and now two wheels
 */
#define SM_COUNT 4
static brightonLocations sonicmods[SM_COUNT] = {
	{"Glide", 1, 235, 30, 100, 580, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"Volume", 1, 690, 30, 100, 580, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"Bend", 1, 100, 715, 920, 55, 0, 1, 0, 0, 0,
		BRIGHTON_NOSHADOW|BRIGHTON_CENTER|BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	{"Mod", 1, 100, 850, 920, 55, 0, 1, 0, 0, 0,
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp sonic6App = {
	"sonic6",
	0,
	"bitmaps/textures/leather.xpm",
	0,
	sonic6Init,
	sonic6Configure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	900, 520, 0, 0,
	3,
	{
		{
			"Sonic6",
			"bitmaps/blueprints/sonic6.xpm",
			0, //"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			sonic6Configure,
			sonic6Callback,
			0, 0, 1000, 670,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/keys/kbg.xpm",
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			160, 700, 810, 280,
			KEY_COUNT_4OCTAVE,
			keys4octave
		},
		{
			"Mods",
			"bitmaps/blueprints/sonicmods.xpm",
			"bitmaps/textures/metal5.xpm",
			0,
			0,
			0,
			sonic6ModCallback,
			27, 685, 115, 280,
			SM_COUNT,
			sonicmods
		},
	}
};

static int
sonic6ModCallback(brightonWindow *bwin, int c, int o, float value)
{
	guiSynth *synth = findSynth(global.synths, bwin);

	switch (o) {
		case 0:
			/* Glide */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				126, 0, (int) (value * C_RANGE_MIN_1));
			break;
		case 1:
			/* Volume */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				126, 1, (int) (value * C_RANGE_MIN_1));
			break;
		case 2:
			/* Bend */
			bristolMidiSendMsg(global.controlfd, synth->midichannel,
				BRISTOL_EVENT_PITCH, 0, (int) (value * C_RANGE_MIN_1));
			break;
		case 3:
			/* Mod */
			bristolMidiControl(global.controlfd, synth->midichannel,
				0, 1, ((int) (value * (C_RANGE_MIN_1 - 1))) >> 7);
			break;
	}

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
			synth->location = value;
			loadMemory(synth, "sonic6", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
sonic6MidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
int
sonic6Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/* printf("sonic6Callback(%i, %i, %f)\n", panel, index, value); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);
	if (index >= DEVICE_COUNT)
		return(0);

	if (sonic6App.resources[0].devlocn[index].to == 1.0)
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
	else
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
	return(0);
}

static int
sonic6Midi(guiSynth *synth, int fd, int chan, int c, int o, int value)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
			newchan = synth->midichannel = 0;
	} else {
		if ((newchan = synth->midichannel + 1) >= 16)
			newchan = synth->midichannel = 15;
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

	displayText(synth, "MIDI CH", synth->midichannel + 1, DISPLAY_DEV);

	return(0);

}

static void
sonic6LFOFreq(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	switch (o) {
		default:
			if (synth->mem.param[3] == 2)
				bristolMidiSendMsg(global.controlfd, synth->sid, 7, 0,
					(int) (synth->mem.param[0]
						* synth->mem.param[2] * C_RANGE_MIN_1));
			else
				bristolMidiSendMsg(global.controlfd, synth->sid, 7, 0,
					(int) (synth->mem.param[2] * C_RANGE_MIN_1));

			if (synth->mem.param[6] == 2)
				bristolMidiSendMsg(global.controlfd, synth->sid, 8, 0,
					(int) (synth->mem.param[0]
						* synth->mem.param[4] * C_RANGE_MIN_1));
			else
				bristolMidiSendMsg(global.controlfd, synth->sid, 8, 0,
					(int) (synth->mem.param[4] * C_RANGE_MIN_1));
			break;
		case 7:
			if (synth->mem.param[3] == 2)
				bristolMidiSendMsg(global.controlfd, synth->sid, 7, 0,
					(int) (synth->mem.param[0]
						* synth->mem.param[2] * C_RANGE_MIN_1));
			else
				bristolMidiSendMsg(global.controlfd, synth->sid, 7, 0,
					(int) (synth->mem.param[2] * C_RANGE_MIN_1));
			break;
		case 8:
			if (synth->mem.param[3] == 2)
				bristolMidiSendMsg(global.controlfd, synth->sid, 8, 0,
					(int) (synth->mem.param[0]
						* synth->mem.param[4] * C_RANGE_MIN_1));
			else
				bristolMidiSendMsg(global.controlfd, synth->sid, 8, 0,
					(int) (synth->mem.param[4] * C_RANGE_MIN_1));
			break;
	}
}

static void
sonic6Transpose(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, o, c, 3 - v);
}

static void
sonic6Waveform(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	switch (v) {
		case 2:
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 4, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 5, 1);
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 6, 0);
			break;
		case 1:
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 4, 1);
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 5, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 6, 0);
			break;
		case 0:
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 4, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 5, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid, o, 6, 1);
			break;
	}
}

static void
sonic6Envelope(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	/*
	 * Shim for the different envelope types
	 * param[39] as AR(0), ASR(1), ADSD(2), ADSR(3).
	 *
	 * AR/AD and ASR are the two types supported by the original device.
	 * ADSD is a MiniMoog type envelope, really just three parameters
	 * The last one is a full ADSR.
	 */
	switch (c) {
		case 1:
			/* * Decay  */
			switch ((int) synth->mem.param[39]) {
				case 0:
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, v);
					break;
				case 1:
					break;
				case 2:
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, v);
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, v);
					break;
				default:
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, v);
					break;
			}
			break;
		case 2:
			/* Sustain */
			switch ((int) synth->mem.param[39]) {
				case 0:
				case 1:
					break;
				case 2:
				default:
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, v);
					break;
			}
			break;
		case 3:
			/* Release */
			switch ((int) synth->mem.param[39]) {
				case 0:
					/* Type 0 uses decay, not release */
					break;
				case 1:
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, v);
					break;
				case 2:
					break;
				default:
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, v);
					break;
			}
			break;
		default:
			/*
			 * If we change the envelope type we may need to reconfigure a set
			 * of basic parameters to force the desired function
			 */
			switch ((int) synth->mem.param[39]) {
				case 0:
					/* AR - zero sustain, configure release=decay */
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 0);
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, 
						(int) (synth->mem.param[36] * C_RANGE_MIN_1));
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, 
						(int) (synth->mem.param[36] * C_RANGE_MIN_1));
					break;
				case 1:
					/* ASR - full sustain, configure decay=release */
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2,
						C_RANGE_MIN_1);
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, 
						(int) (synth->mem.param[38] * C_RANGE_MIN_1));
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, 
						(int) (synth->mem.param[38] * C_RANGE_MIN_1));
					break;
				case 2:
					/* ADSD - configured sustain, configure release=decay */
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2,
						(int) (synth->mem.param[37] * C_RANGE_MIN_1));
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3,
						(int) (synth->mem.param[36] * C_RANGE_MIN_1));
					break;
				default:
					/* ADSD - configured sustain, configure release=decay */
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1,
						(int) (synth->mem.param[36] * C_RANGE_MIN_1));
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2,
						(int) (synth->mem.param[37] * C_RANGE_MIN_1));
					bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3,
						(int) (synth->mem.param[38] * C_RANGE_MIN_1));
					break;
			}
			break;
	}
}

static void
sonic6Filter(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	if (v == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 3, v);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 3, 8192);
}

static void
sonic6Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
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

					if (loadMemory(synth, "sonic6", 0,
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
				if (loadMemory(synth, "sonic6", 0,
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
			if (loadMemory(synth, "sonic6", 0, synth->bank * 10 + synth->location,
				synth->mem.active, 0, BRISTOL_FORCE) < 0)
				displayText(synth, "FREE MEM",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
			else
				displayText(synth, "PROGRAM",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
			synth->flags &= ~BANK_SELECT;
			break;
		case 2:
			if (synth->bank < 1)
				synth->bank = 1;
			if (synth->location == 0)
				synth->location = 1;
			saveMemory(synth, "sonic6", 0, synth->bank * 10 + synth->location,
				0);
			displayText(synth, "PROGRAM", synth->bank * 10 + synth->location,
				DISPLAY_DEV);
			synth->flags &= ~BANK_SELECT;
			break;
		case 3:
			if (synth->flags & BANK_SELECT) {
				synth->flags &= ~BANK_SELECT;
				if (loadMemory(synth, "sonic6", 0,
					synth->bank * 10 + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FREE MEM",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
				else
					displayText(synth, "PROGRAM",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
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
			while (loadMemory(synth, "sonic6", 0, bank * 10 + location,
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
			loadMemory(synth, "sonic6", 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_FORCE);
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			break;
		case 5:
			if (++location > 8) {
				location = 1;
				if (++bank > 88)
					bank = 1;
			}
			while (loadMemory(synth, "sonic6", 0, bank * 10 + location,
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
			loadMemory(synth, "sonic6", 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_FORCE);
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			break;
		case 6:
			/* Find the next free mem */
			if (++location > 8) {
				location = 1;
				if (++bank > 88)
					bank = 1;
			}

			while (loadMemory(synth, "sonic6", 0, bank * 10 + location,
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

			if (loadMemory(synth, "sonic6", 0, bank * 10 + location,
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

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
int
sonic6Init(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	dispatcher *dispatch;
	int i;

	if (synth == 0)
	{
		synth = findSynth(global.synths, 0);
		if (synth == 0)
		{
			printf("cannot init\n");
			return(0);
		}
	}

	if ((initmem = synth->location) == 0)
		initmem = 11;

	synth->win = win;

	printf("Initialise the sonic6 link to bristol: %p\n", synth->win);

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
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

	for (i = 0; i < DEVICE_COUNT; i++) {
		synth->dispatch[i].controller = 126;
		synth->dispatch[i].operator = 101;
		synth->dispatch[i].routine = sonic6MidiSendMsg;
	}

	/* LFO */
	synth->dispatch[0].controller = 0;
	synth->dispatch[0].operator = 1;
	synth->dispatch[0].routine = (synthRoutine) sonic6LFOFreq;
	synth->dispatch[2].controller = 7;
	synth->dispatch[2].operator = 0;
	synth->dispatch[2].routine = (synthRoutine) sonic6LFOFreq;
	synth->dispatch[4].controller = 8;
	synth->dispatch[4].operator = 0;
	synth->dispatch[4].routine = (synthRoutine) sonic6LFOFreq;

	synth->dispatch[1].controller = 126;
	synth->dispatch[1].operator = 15;
	/* This should be shimmed to ensure the correct value is sent */
	synth->dispatch[3].controller = 126;
	synth->dispatch[3].operator = 16;

	synth->dispatch[5].controller = 126;
	synth->dispatch[5].operator = 17;
	/* This should be shimmed to ensure the correct value is sent */
	synth->dispatch[6].controller = 126;
	synth->dispatch[6].operator = 18;

	synth->dispatch[7].controller = 126;
	synth->dispatch[7].operator = 14;

	/* LFO Multi */
	synth->dispatch[52].controller = 126;
	synth->dispatch[52].operator = 19;
	synth->dispatch[53].controller = 126;
	synth->dispatch[53].operator = 20;

	/* VCO A */
	synth->dispatch[8].controller = 0;
	synth->dispatch[8].operator = 2;
	synth->dispatch[9].controller = 0;
	synth->dispatch[9].operator = 0;
	synth->dispatch[10].controller = 0;
	synth->dispatch[10].operator = 1;
	synth->dispatch[10].routine = (synthRoutine) sonic6Transpose;
	synth->dispatch[11].controller = 0;
	synth->dispatch[11].operator = 1;
	synth->dispatch[11].routine = (synthRoutine) sonic6Waveform;
	/* Two spares here */
	synth->dispatch[14].controller = 126;
	synth->dispatch[14].operator = 21;
	synth->dispatch[15].controller = 126;
	synth->dispatch[15].operator = 22;

	/* Osc-A mode LFO/KBD/HIGH */
	synth->dispatch[16].controller = 126;
	synth->dispatch[16].operator = 32;

	/* VCO B */
	synth->dispatch[17].controller = 1;
	synth->dispatch[17].operator = 2;
	synth->dispatch[18].controller = 1;
	synth->dispatch[18].operator = 0;
	synth->dispatch[19].controller = 1;
	synth->dispatch[19].operator = 1;
	synth->dispatch[19].routine = (synthRoutine) sonic6Transpose;
	synth->dispatch[20].controller = 1;
	synth->dispatch[20].operator = 1;
	synth->dispatch[20].routine = (synthRoutine) sonic6Waveform;
	/* Two spares here */
	synth->dispatch[23].controller = 126;
	synth->dispatch[23].operator = 23;
	synth->dispatch[24].controller = 126;
	synth->dispatch[24].operator = 24;
	synth->dispatch[25].controller = 126;
	synth->dispatch[25].operator = 25;

	/* Generate A/B */
	synth->dispatch[26].controller = 126;
	synth->dispatch[26].operator = 13;

	/* RM */
	synth->dispatch[27].controller = 126;
	synth->dispatch[27].operator = 26;
	synth->dispatch[28].controller = 126;
	synth->dispatch[28].operator = 27;

	/* Noise */
	synth->dispatch[29].controller = 6;
	synth->dispatch[29].operator = 1;

	/* Mixer */
	synth->dispatch[30].controller = 126;
	synth->dispatch[30].operator = 28;
	synth->dispatch[31].controller = 126;
	synth->dispatch[31].operator = 29;
	synth->dispatch[32].controller = 126;
	synth->dispatch[32].operator = 30;
	synth->dispatch[33].controller = 126;
	synth->dispatch[33].operator = 31;

	/* Bypass */
	synth->dispatch[34].controller = 126;
	synth->dispatch[34].operator = 35;

	/* ADSR */
	synth->dispatch[35].controller = 3;
	synth->dispatch[35].operator = 0;
	synth->dispatch[36].controller = 3;
	synth->dispatch[36].operator = 1;
	synth->dispatch[37].controller = 3;
	synth->dispatch[37].operator = 2;
	synth->dispatch[38].controller = 3;
	synth->dispatch[38].operator = 3;
	synth->dispatch[39].controller = 126;
	synth->dispatch[39].operator = 101;
	synth->dispatch[36].routine = synth->dispatch[37].routine
		= synth->dispatch[38].routine = synth->dispatch[39].routine
		= (synthRoutine) sonic6Envelope;

	synth->dispatch[40].controller = 3;
	synth->dispatch[40].operator = 5;

	/* Filter */
	synth->dispatch[41].controller = 4;
	synth->dispatch[41].operator = 0;
	synth->dispatch[42].controller = 4;
	synth->dispatch[42].operator = 1;

	/* Direct Outputs */
	synth->dispatch[43].controller = 126;
	synth->dispatch[43].operator = 10;
	synth->dispatch[44].controller = 126;
	synth->dispatch[44].operator = 11;
	synth->dispatch[45].controller = 126;
	synth->dispatch[45].operator = 12;

	/* Triggers */
	synth->dispatch[46].controller = 3;
	synth->dispatch[46].operator = 6;
	synth->dispatch[47].controller = 126;
	synth->dispatch[47].operator = 36;
	synth->dispatch[48].controller = 126;
	synth->dispatch[48].operator = 37;

	/* filter */
	synth->dispatch[49].controller = 126;
	synth->dispatch[49].operator = 33;
	synth->dispatch[50].controller = 4;
	synth->dispatch[50].operator = 3;
	synth->dispatch[50].routine = (synthRoutine) sonic6Filter;
	synth->dispatch[51].controller = 126;
	synth->dispatch[51].operator = 34;

	/* Reverb */
	synth->dispatch[55].controller = 101;
	synth->dispatch[55].operator = 0;
	synth->dispatch[56].controller = 101;
	synth->dispatch[56].operator = 1;
	synth->dispatch[57].controller = 101;
	synth->dispatch[57].operator = 2;
	synth->dispatch[58].controller = 101;
	synth->dispatch[58].operator = 3;

	/* Global Tuning */
	synth->dispatch[60].controller = 126;
	synth->dispatch[60].operator = 2;

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
		dispatch[MEM_START + 13].routine = (synthRoutine) sonic6Memory;

	dispatch[DEVICE_COUNT - 3].routine = dispatch[DEVICE_COUNT - 2].routine =
		(synthRoutine) sonic6Midi;
	dispatch[DEVICE_COUNT - 3].controller = 1;
	dispatch[DEVICE_COUNT - 2].controller = 2;

	dispatch[MEM_START].other1 = -1;

	/* VCO inits */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 1, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 5, 16383);

	/* VCO inits */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 4, 16383);

	/* LFO Sync to key_on */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 1, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, 16383);

	/* ADSR gain */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, 16383);

	/* Noise - gain and pink filter */
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0, 10024);
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 2, 4096);

	/* Ring Mod */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 0, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 2, 8192);

	/* filter mod and key tracking stuff */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 3, 4096);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 4);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 5, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 6, 0);

	/* Tune */
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 2, 8192);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
int
sonic6Configure(brightonWindow *win)
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
	}

	loadMemory(synth, "sonic6", 0, initmem, synth->mem.active, FIRST_DEV, 
		BRISTOL_FORCE);

	event.type = BRIGHTON_FLOAT;
	/* Tune */
	event.value = 0.5;
	brightonParamChange(synth->win, 0, 60, &event);
	/* Volume */
	event.value = 0.5;
	brightonParamChange(synth->win, 2, 1, &event);
	/* Glide */
	event.value = 0.1;
	brightonParamChange(synth->win, 2, 0, &event);
	/* Modwheel */
	event.value = 0.1;
	brightonParamChange(synth->win, 2, 3, &event);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	brightonPut(win,
		"bitmaps/blueprints/sonic6shade.xpm", 0, 0, win->width, win->height);

	return(0);
}

