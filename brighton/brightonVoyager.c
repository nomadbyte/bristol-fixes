
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
 * This is the emulator stub for the Moog Voyager Ice Blue. It has a separate
 * layout but then uses all the same code from the Explorer interface. It could
 * have been buried in the same file.
 */
#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

/*
static int voyagerInit();
static int voyagerCallback(brightonWindow *, int, int, float);
*/
static int voyagerConfigure();

extern int explorerInit();
extern int explorerConfigure();
extern int explorerCallback(brightonWindow *, int, int, float);

static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

extern int empty;

#include "brightonKeys.h"

#define KEY_PANEL 1

#define FIRST_DEV 0
#define DEVICE_COUNT 77
#define ACTIVE_DEVS 56
#define MEM_START (ACTIVE_DEVS + 1)

#define DISPLAY1 (DEVICE_COUNT - 2)
#define DISPLAY2 (DEVICE_COUNT - 1)

#define S1 85

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
 * This example is for a voyagerBristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {

/* LFO and control */
	{"LFO-Freq", 0, C1 - 7, R14 - 20, S1 + 40, S1 + 40, 0, 1, 0, 0, 0, 0},
	{"LFO-Sync", 2, C1 - 5, R24 + 25, B3, B4, 0, 3, 0,
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"GLOBAL TUNE", 0, C1, R34, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_NOTCH},
	{"Glide", 0, C1, R44, S1, S1, 0, 1, 0, 0, 0, 0},
/* Routing */
	{"Mod1-Source", 0, C2, R14, S1, S1, 0, 4, 0, 0, 0, 0},
	{"Mod1-Shape", 0, C2, R24, S1, S1, 0, 3, 0, 0, 0, 0},
	{"Mod1-Dest", 0, C2, R34, S1, S1, 0, 5, 0, 0, 0, 0},
	{"Mod1-Gain", 0, C2, R44, S1, S1, 0, 1, 0, 0, 0, 0},
	{"Mod2-Source", 0, C3, R14, S1, S1, 0, 4, 0, 0, 0, 0},
	{"Mod2-Shape", 0, C3, R24, S1, S1, 0, 3, 0, 0, 0, 0},
	{"Mod2-Dest", 0, C3, R34, S1, S1, 0, 5, 0, 0, 0, 0},
	{"Mod2-Gain", 0, C3, R44, S1, S1, 0, 1, 0, 0, 0, 0},
/* Oscillators */
	{"Osc1-Transpose", 0, C4, R24, S1, S1, 0, 5, 0, 0, 0, 0},
	{"Osc1-Waveform", 0, C4, R34, S1, S1, 0, 1, 0, 0, 0, 0},
	{"Osc2-Tuning", 0, C5 - 7, R14 - 20, S1 + 40, S1 + 40, 0, 1, 0, 0, 0, BRIGHTON_NOTCH},
	{"Osc2-Transpose", 0, C5, R24, S1, S1, 0, 5, 0, 0, 0, 0},
	{"Osc2-Waveform", 0, C5, R34, S1, S1, 0, 1, 0, 0, 0, 0},
	{"Osc3-Tuning", 0, C6 - 7, R14 - 20, S1 + 40, S1 + 40, 0, 1, 0, 0, 0, BRIGHTON_NOTCH},
	{"Osc3-Transpose", 0, C6, R24, S1, S1, 0, 5, 0, 0, 0, 0},
	{"Osc3-Waveform", 0, C6, R34, S1, S1, 0, 1, 0, 0, 0, 0},
/* Osc mod switches */
	{"Sync-1/2", 2, C4, R44 + 10, B1, B2, 0, 1, 0, 0, 0, BRIGHTON_VERTICAL},
	{"FM-1/3", 2, C4 + 40, R44 + 10, B1, B2, 0, 1, 0, 0, 0, BRIGHTON_VERTICAL},
	{"KBD-3", 2, C4 + 95, R44 + 10, B1, B2, 0, 1, 0, 0, 0, BRIGHTON_VERTICAL},
	{"Freq-3", 2, C4 + 135, R44 + 10, B1, B2, 0, 1, 0, 0, 0, BRIGHTON_VERTICAL},
/* Mixer */
	{"Mix-EXT", 0, C7, R15, S1, S1, 0, 1, 0, 0, 0, 0},
	{"Mix-O1", 0, C7, R25, S1, S1, 0, 1, 0, 0, 0, 0},
	{"Mix-O2", 0, C7, R35, S1, S1, 0, 1, 0, 0, 0, 0},
	{"Mix-O3", 0, C7, R45, S1, S1, 0, 1, 0, 0, 0, 0},
	{"Mix-NSE", 0, C7, R55, S1, S1, 0, 1, 0, 0, 0, 0},
	{"Mix-Ext-off", 2, C8, R15 + 25, B3, B4, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"Mix-O1-off", 2, C8, R25 + 25, B3, B4, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"Mix-O2-off", 2, C8, R35 + 25, B3, B4, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"Mix-O3-off", 2, C8, R45 + 25, B3, B4, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"Mix-Nse-off", 2, C8, R55 + 25, B3, B4, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
/* Filter */
	{"VCF-Cutoff", 0, C9 - 7, R15 - 25, S1 + 40, S1 + 40, 0, 1, 0, 0, 0, 0},
	{"VCF-Space", 0, C9, R25, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCF-Res", 0, C9, R35, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCF-KBD", 0, C9, R45, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCF-Mode", 2, C9, R55 + 25, B3, B4, 0, 1, 0, 0, 0, 0},
/* Filter envelope */
	{"VCF-Attack", 0, C10, R15, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCF-Decay", 0, C10, R25, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCF-Sustain", 0, C10, R35, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCF-Release", 0, C10, R45, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCF-EnvLevel", 0, C10, R55, S1, S1, 0, 1, 0, 0, 0, 0},
/* Amp Envelope */
	{"VCA-Attack", 0, C11, R15, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCA-Decay", 0, C11, R25, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCA-Sustain", 0, C11, R35, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCA-Release", 0, C11, R45, S1, S1, 0, 1, 0, 0, 0, 0},
	{"VCA-Veloc", 2, C11, R55 + 25, B3, B4, 0, 1, 0, 0, 0, 0},
/* Vol, on/off, glide and release */
	{"Volume", 0, C12 - 7, R15 - 25 , S1 + 40, S1 + 40, 0, 1, 0, 0, 0, 0},
	{"On/Off", 2, C12 - 4, R25 + 25, B3, B4, 0, 1, 0, 0, 0, 0},
	{"LFO-Multi", 2, C12 - 4, R35 + 25, B3, B4, 0, 1, 0, 
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	{"Glide-On", 2, C12 - 4, R45 + 25, B3, B4, 0, 1, 0, 
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	{"Release-On", 2, C12 - 4, R55 + 25, B3, B4, 0, 1, 0, 
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	{"Reserved", 5, 405, 410, 180, 518, 0, 1, 0, "bitmaps/buttons/pointer.xpm", 0, 0},
	{"Reserved", 5, 405, 410, 180, 518, 0, 1, 0, "bitmaps/buttons/pointer.xpm", 0, 
		BRIGHTON_WITHDRAWN},
/* logo */
	{"", 4, 762, 1037, 240, 140, 0, 1, 0, "bitmaps/images/explorer.xpm", 0, 0},

/* memories */
	{"", 2, 418, 225, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 436, 225, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 454, 225, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 472, 225, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 490, 225, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 418, 308, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 436, 308, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 454, 308, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 472, 308, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 490, 308, 17, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 510, 228, 15, 70, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 510, 309, 15, 70, 0, 1.01, 0,
		"bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 540, 70, 16, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 540, 145, 16, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 540, 227, 16, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 540, 308, 16, 78, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	/* mem up down */
	{"", 2, 402, 228, 15, 70, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 402, 311, 15, 70, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* displays */
	{"", 3, 403, 90, 128, 63, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0},
	{"", 3, 403, 150, 128, 63, 0, 1, 0, 0, 0, 0}
};

/*
 * These are new mod controls for the Pro-1. Used here for the Explorer.
 */
static brightonLocations newmods[2] = {
	{"", BRIGHTON_MODWHEEL, 260, 200, 190, 565, 0, 1, 0,
		"bitmaps/knobs/modwheelblue.xpm", 0,
		BRIGHTON_HSCALE|BRIGHTON_NOSHADOW|BRIGHTON_CENTER|BRIGHTON_NOTCH},
	{"", BRIGHTON_MODWHEEL, 610, 200, 190, 565, 0, 1, 0,
		"bitmaps/knobs/modwheelblue.xpm", 0,
		BRIGHTON_HSCALE|BRIGHTON_NOSHADOW},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp voyagerApp = {
	"voyager",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/voyagerpaint.xpm",
	0, /* or BRIGHTON_STRETCH, default is tesselate */
	explorerInit,
	voyagerConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	817, 472, 0, 0,
	7, /* one panel only */
	{
		{
			"Voyager",
			"bitmaps/blueprints/voyager.xpm",
			"bitmaps/textures/black.xpm",
			0, /* flags */
			0,
			explorerConfigure,
			explorerCallback,
			25, 25, 950, 532,
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
			keyCallback,
			140, 675, 840, 300,
			KEY_COUNT_3OCTAVE,
			keys3octave
		},
		{
			"Mods",
			0,
			"bitmaps/textures/voyagerpaint.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			modCallback,
			15, 675, 125, 300,
			2,
			newmods
		},
		{
			"SP0",
			0,
			"bitmaps/textures/voyagerpaint.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			930, 675, 60, 300,
			0,
			0
		},
		{
			"SP1",
			0,
			"bitmaps/textures/voyagerpaint.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 15, 1000,
			0,
			0
		},
		{
			"SP2",
			0,
			"bitmaps/textures/voyagerpaint.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		},
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

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
			loadMemory(synth, "explorer", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
voyagerConfigure(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	memset(&event, 0, sizeof(brightonEvent));

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

	displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY1);

	loadMemory(synth, "explorer", 0, synth->location, synth->mem.active,
		FIRST_DEV, 0);
	displayText(synth, "PRG", synth->location, DISPLAY2);

	brightonPut(win,
		"bitmaps/blueprints/voyagershade.xpm", 0, 0, win->width, win->height);

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

