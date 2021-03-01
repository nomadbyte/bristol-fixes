
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
#include "bristolmidi.h"
#include "brightoninternals.h"

static int miniInit();
static int miniConfigure();
static int midiCallback(brightonWindow *, int, int, float);
static int miniCallback(brightonWindow * , int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define KEY_PANEL 1

#define FIRST_DEV 0
#define DEVICE_COUNT (61 + FIRST_DEV)
#define ACTIVE_DEVS (42 + FIRST_DEV)

#define R1 128
#define R5 698
#define R3 (R1 + (R5 - R1) / 2)
#define R2 (R1 + (R3 - R1) / 2)
#define R4 (R3 + (R5 - R3) / 2)
#define R6 782
#define R7 889
#define C1 168
#define C2 234
#define C3 318
#define C4 398
#define C5 472
#define C6 548
#define C7 673
#define C8 753
#define C9 827
#define C10 905
#define C11 622
#define C12 140

#define S1 120
#define S2 170
#define S3 180
#define S4 35
#define S5 60

#define B1 40
#define B2 16
#define B3 110

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
 * This example is for a miniBristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {
/* CONTROL */
	{"Tune", 0, 47, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm",
		0, BRIGHTON_NOTCH},
	{"Glide", 0, 18, R4, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Mod", 0, 80, R4, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0}, /*3 */
/* OSCILATORS */
	{"Osc1 Transpose", 0, C1, R1, S1, S1, 0, 5, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Osc2 Transpose", 0, C1, R3, S1, S1, 0, 5, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Osc3 Transpose", 0, C1, R5, S1, S1, 0, 5, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
/*	{"", 0, C2, 382, S2, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0, 0}, */
	{"Osc2 Tuning", 0, C2, 382, S2, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm",
		0, BRIGHTON_NOTCH},
	{"Osc3 Tuning", 0, C2, 660, S2, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, BRIGHTON_NOTCH},
	{"Osc1 Waveform", 0, C3, R1, S1, S1, 0, 5, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Osc2 Waveform", 0, C3, R3, S1, S1, 0, 5, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Osc3 Waveform", 0, C3, R5, S1, S1, 0, 5, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0}, /*11 */
/* MIXER */
	{"Osc1 Mix lvl", 0, C4, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Ext Mix lvl", 0, C6, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Osc2 Mix lvl", 0, C4, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Noise Mix lvl", 0, C6, R4, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Osc3 Mix lvl", 0, C4, R5, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0}, /*16 */
/* FILTER */
	{"Filter Freq", 0, C7, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Filter Emph", 0, C8, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Filter Contour", 0, C9, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0}, /*19 */
/* ADSR */
	{"Filter Attack", 0, C7, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Filter Decay", 0, C8, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Filter Release", 0, C9, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0}, /*22 */
/* another ADSR */
	{"Attack", 0, C7, R5, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Decay", 0, C8, R5, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"Release", 0, C9, R5, S1, S1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0}, /*25 */
/* MAIN OUT 25 */
	{"MasterVolume", 0, C10, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		0, 0},
	{"On/Off", 2, C10 + 7, R1 - 92, B1, B1, 0, 1, 0,
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"A-440", 2, C10 + 7, R2 + 75, B1, B1, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0}, /*28 */
/* MIX BUTTONS - 28 */
	{"Mix Osc1", 2, C5, R1 + 45, B1, B1, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"Mix Ext", 2, C5, R2 + 45, B1, B1, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"Mix Osc2", 2, C5, R3 + 45, B1, B1, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"Mix Noise", 2, C5, R4 + 45, B1, B1, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"Mix Osc3", 2, C5, R5 + 45, B1, B1, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, 0}, /*33 */
/* NOISE BUTTON - 33 */
	{"White/Pink", 2, C11 - 6, C6, B2, B3, 0, 1, 0, 
		"bitmaps/buttons/rockerblue.xpm", 0, BRIGHTON_VERTICAL}, /*34 */
/* CONTROL BUTTONS - 34 */
	{"Osc Mod 1", 2, C12 - 22, R2 - 13, B1, B1, 0, 1, 0, 0, 0, 0},
	{"Osc Mod 2", 2, C12 - 22, R2 + 97, B1, B1, 0, 1, 0, 0, 0, 0},
	{"Osc 3 LFO", 2, C12 - 9, R5 + 15, B2, B3, 0, 1, 0, 0, 0, BRIGHTON_VERTICAL},
	{"Release", 2, 78, 765, B1, B1, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	{"Multitrig", 2, 78, 825, B1, B1, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, 0}, /*39 */
/* FILTER BUTTONS - 39 */
	{"Filter Mod1", 2, C11 - 5, R1 + 30, B1, B1, 0, 1, 0, 0, 0, 0},
	{"Filter Velocity", 2, C11 - 5, R1 + 170, B1, B1, 0, 1, 0, 0, 0, 0},
	{"Filter KBD", 2, C11 - 5, R3, B1, B1, 0, 1, 0, 0, 0}, /*42 */
/* Memory tablet */
	{"", 2, R7, R6 - S5 * 3, S4, S5, 0, 1, 0,
		"bitmaps/digits/L.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4 * 2, R6 - S5 * 3, S4, S5, 0, 1, 0,
		"bitmaps/digits/S.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4, R6 - S5 * 3, S4, S5, 0, 1, 0,
		"bitmaps/digits/0.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7, R6 - S5 * 2, S4, S5, 0, 1, 0,
		"bitmaps/digits/1.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4, R6 - S5 * 2, S4, S5, 0, 1, 0,
		"bitmaps/digits/2.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4 * 2, R6 - S5 * 2, S4, S5, 0, 1, 0,
		"bitmaps/digits/3.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7, R6 - S5, S4, S5, 0, 1, 0,
		"bitmaps/digits/4.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4, R6 - S5, S4, S5, 0, 1, 0,
		"bitmaps/digits/5.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4 * 2, R6 - S5, S4, S5, 0, 1, 0,
		"bitmaps/digits/6.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7, R6, S4, S5, 0, 1, 0,
		"bitmaps/digits/7.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4, R6, S4, S5, 0, 1, 0,
		"bitmaps/digits/8.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4 * 2, R6, S4, S5, 0, 1, 0,
		"bitmaps/digits/9.xpm", 0, BRIGHTON_CHECKBUTTON},
	/* Up down midi */
	{"", 2, R7 + S4, 415, S4, S5, 0, 1, 0,
		"bitmaps/digits/Down.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4 * 2, 415, S4, S5, 0, 1, 0,
		"bitmaps/digits/Up.xpm", 0, BRIGHTON_CHECKBUTTON},
	/* Up Down memory */
	{"", 2, R7, R6 + S5, S4, S5, 0, 1, 0,
		"bitmaps/digits/Down.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 2, R7 + S4 * 2, R6 + S5, S4, S5, 0, 1, 0,
		"bitmaps/digits/Up.xpm", 0, BRIGHTON_CHECKBUTTON},
	{"", 3, R7, 430 + S5, 105, 54, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}, /*display 60 */
	{"", 3, R7, 425 + S5 + 55, 105, 54, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay2.xpm", 0}, /*display 60 */
	{"", 4, 820, 1015, 175, 140, 0, 1, 0,
		"bitmaps/images/mini.xpm", 0, 0},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp miniApp = {
	"mini",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/wood6.xpm",
	0, /*BRIGHTON_STRETCH, // default is tesselate */
	miniInit,
	miniConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth, /* 0, */
	{1, 100, 2, 2, 5, 520, 0, 0},
	700, 400, 0, 0, /*/367, */
	9,
	{
		{
			"Mini",
			"bitmaps/blueprints/mini.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			miniConfigure,
			miniCallback,
			15, 25, 970, 570,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/ekbg.xpm",
			0x020|BRIGHTON_STRETCH|BRIGHTON_KEY_PANEL,
			0,
			0,
			keyCallback,
			150, 700, 845, 280,
			KEY_COUNT_3OCTAVE,
			keys3octave
		},
		{
			"Mods",
			"bitmaps/blueprints/mods.xpm",
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			modCallback,
			15, 700, 145, 280,
			2,
			mods
		},
		{
			"SP2",
			0,
			"bitmaps/textures/wood4.xpm",
			0, /*BRIGHTON_STRETCH|BRIGHTON_VERTICAL, // flags */
			0,
			0,
			0,
			15, 0, 970, 25,
			0,
			0
		},
		{
			"SP1",
			0,
			"bitmaps/textures/wood2.xpm",
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
			"bitmaps/textures/wood2.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			985, 0, 17, 1000,
			0,
			0
		},
		{
			"SP2",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			2, 597, 11, 390,
			0,
			0
		},
		{
			"SP2",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			986, 597, 14, 390,
			0,
			0
		},
		{
			"SP2",
			0,
			"bitmaps/textures/wood2.xpm",
			0, /*BRIGHTON_STRETCH|BRIGHTON_VERTICAL, // flags */
			0,
			0,
			0,
			15, 982, 970, 18,
			0,
			0
		}
	}
};

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	//printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			//printf("midi program: %x, %i\n", controller, value);
			synth->location = value + synth->bank * 10;
			if (loadMemory(synth, synth->resources->name, 0,
				synth->location, synth->mem.active, FIRST_DEV, 0) < 0)
				displayText(synth, "FRE", synth->location, FIRST_DEV + 59);
			else
				displayText(synth, "PRG", synth->location, FIRST_DEV + 59);
			break;
		case MIDI_BANK_SELECT:
			//printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			synth->location = (synth->location % 10) + value * 10;
			if (loadMemory(synth, synth->resources->name, 0,
				synth->location, synth->mem.active, FIRST_DEV, 0) < 0)
				displayText(synth, "FRE", synth->location, FIRST_DEV + 59);
			else
				displayText(synth, "PRG", synth->location, FIRST_DEV + 59);
			break;
	}
	return(0);
}

/*static dispatcher dispatch[DEVICE_COUNT]; */

static int
miniMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
miniDualFilterDecay(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, 1, v);

	if (synth->mem.param[FIRST_DEV + 37])
		bristolMidiSendMsg(fd, chan, c, 3, v);
	else
		bristolMidiSendMsg(fd, chan, c, 3, 1024);
}

static void
miniDualDecay(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, 5, 1,
		(int) (synth->mem.param[FIRST_DEV + 23] * C_RANGE_MIN_1));

	if (synth->mem.param[FIRST_DEV + 37])
		bristolMidiSendMsg(fd, chan, 5, 3,
			(int) (synth->mem.param[FIRST_DEV + 23] * C_RANGE_MIN_1));
	else
		bristolMidiSendMsg(fd, chan, 5, 3, 1024);
}

static void
miniMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
/*	printf("miniMemory(%i %i %i %i %i)\n", fd, chan, c, o, v); */

	switch (c) {
		default:
		case 0:
			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;
			if (loadMemory(synth, "mini", 0, synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
				displayText(synth, "FRE", synth->location, FIRST_DEV + 59);
			else
				displayText(synth, "PRG", synth->location, FIRST_DEV + 59);
			break;
		case 1:
			loadMemory(synth, "mini", 0, synth->location, synth->mem.active,
				FIRST_DEV, 0);
			displayText(synth, "PRG", synth->location, FIRST_DEV + 59);
/*			synth->location = 0; */
			break;
		case 2:
			saveMemory(synth, "mini", 0, synth->location, FIRST_DEV);
			displayText(synth, "PRG", synth->location, FIRST_DEV + 59);
/*			synth->location = 0; */
			break;
		case 3:
			while (loadMemory(synth, "mini", 0, --synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location < 0)
				{
					synth->location = 1000;
					break;
				}
			}
			displayText(synth, "PRG", synth->location, FIRST_DEV + 59);
			break;
		case 4:
			while (loadMemory(synth, "mini", 0, ++synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location > 999)
				{
					synth->location = -1;
					break;
				}
			}
			displayText(synth, "PRG", synth->location, FIRST_DEV + 59);
			break;
	}

}

static void
miniMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

/*	printf("miniMidi(%x, %i %i %i %i %i)\n", synth, fd, chan, c, o, v); */

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			return;
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 16)
		{
			synth->midichannel = 15;
			return;
		}
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

	displayText(synth, "MIDI", newchan + 1, FIRST_DEV + 58);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
miniCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*	printf("miniCallback(%x(%x), %i, %i, %f): %i %x\n", */
/*		win, synth, panel, index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);
	if (synth->dispatch[index].controller >= DEVICE_COUNT)
		return(0);

	if (miniApp.resources[0].devlocn[index].to == 1.0)
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
/*
	else
		printf("dispatch[%x,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
*/

	return(0);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
miniInit(brightonWindow *win)
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

	synth->win = win;

	printf("Initialise the mini link to bristol: %p\n", synth->win);

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

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = miniMidiSendMsg;

	/* Tune, Glide, Modmix */
	dispatch[FIRST_DEV + 0].controller = 10;
	dispatch[FIRST_DEV + 0].operator = 0;
	dispatch[FIRST_DEV + 1].controller = 9;
	dispatch[FIRST_DEV + 1].operator = 0;
	dispatch[FIRST_DEV + 2].controller = 11;
	dispatch[FIRST_DEV + 2].operator = 1;

	/* Osc */
	dispatch[FIRST_DEV + 3].controller = 0;
	dispatch[FIRST_DEV + 3].operator = 1;
	dispatch[FIRST_DEV + 4].controller = 1;
	dispatch[FIRST_DEV + 4].operator = 1;
	dispatch[FIRST_DEV + 5].controller = 2;
	dispatch[FIRST_DEV + 5].operator = 1;
	dispatch[FIRST_DEV + 6].controller = 1;
	dispatch[FIRST_DEV + 6].operator = 2;
	dispatch[FIRST_DEV + 7].controller = 2;
	dispatch[FIRST_DEV + 7].operator = 2;
	dispatch[FIRST_DEV + 8].controller = 0;
	dispatch[FIRST_DEV + 8].operator = 0;
	dispatch[FIRST_DEV + 9].controller = 1;
	dispatch[FIRST_DEV + 9].operator = 0;
	dispatch[FIRST_DEV + 10].controller = 2;
	dispatch[FIRST_DEV + 10].operator = 0;

	/* MIX */
	dispatch[FIRST_DEV + 11].controller = 0;
	dispatch[FIRST_DEV + 11].operator = 3;
	dispatch[FIRST_DEV + 12].controller = 8;
	dispatch[FIRST_DEV + 12].operator = 0;
	dispatch[FIRST_DEV + 13].controller = 1;
	dispatch[FIRST_DEV + 13].operator = 3;
	dispatch[FIRST_DEV + 14].controller = 7;
	dispatch[FIRST_DEV + 14].operator = 0;
	dispatch[FIRST_DEV + 15].controller = 2;
	dispatch[FIRST_DEV + 15].operator = 3;

	dispatch[FIRST_DEV + 16].controller = 4;
	dispatch[FIRST_DEV + 16].operator = 0;
	dispatch[FIRST_DEV + 17].controller = 4;
	dispatch[FIRST_DEV + 17].operator = 1;
	dispatch[FIRST_DEV + 18].controller = 4;
	dispatch[FIRST_DEV + 18].operator = 2;

	dispatch[FIRST_DEV + 19].controller = 3;
	dispatch[FIRST_DEV + 19].operator = 0;
	dispatch[FIRST_DEV + 20].controller = 3;
	dispatch[FIRST_DEV + 20].operator = 1; /* special */
	dispatch[FIRST_DEV + 20].routine = (synthRoutine) miniDualFilterDecay;
	dispatch[FIRST_DEV + 21].controller = 3;
	dispatch[FIRST_DEV + 21].operator = 2;

	dispatch[FIRST_DEV + 22].controller = 5;
	dispatch[FIRST_DEV + 22].operator = 0;
	dispatch[FIRST_DEV + 23].controller = 5;
	dispatch[FIRST_DEV + 23].operator = 1; /* special */
	dispatch[FIRST_DEV + 23].routine = (synthRoutine) miniDualDecay;
	dispatch[FIRST_DEV + 24].controller = 5;
	dispatch[FIRST_DEV + 24].operator = 2;

	dispatch[FIRST_DEV + 25].controller = 5;
	dispatch[FIRST_DEV + 25].operator = 4;
	dispatch[FIRST_DEV + 26].controller = 12;
	dispatch[FIRST_DEV + 26].operator = 7;
	dispatch[FIRST_DEV + 27].controller = 12;
	dispatch[FIRST_DEV + 27].operator = 6;

	dispatch[FIRST_DEV + 28].controller = 12;
	dispatch[FIRST_DEV + 28].operator = 10;
	dispatch[FIRST_DEV + 29].controller = 12;
	dispatch[FIRST_DEV + 29].operator = 13;
	dispatch[FIRST_DEV + 30].controller = 12;
	dispatch[FIRST_DEV + 30].operator = 11;
	dispatch[FIRST_DEV + 31].controller = 12;
	dispatch[FIRST_DEV + 31].operator = 14;
	dispatch[FIRST_DEV + 32].controller = 12;
	dispatch[FIRST_DEV + 32].operator = 12;
	dispatch[FIRST_DEV + 33].controller = 7;
	dispatch[FIRST_DEV + 33].operator = 1;

	dispatch[FIRST_DEV + 34].controller = 12;
	dispatch[FIRST_DEV + 34].operator = 1;
	dispatch[FIRST_DEV + 35].controller = 12;
	dispatch[FIRST_DEV + 35].operator = 4;
	dispatch[FIRST_DEV + 36].controller = 12;
	dispatch[FIRST_DEV + 36].operator = 0;

	dispatch[FIRST_DEV + 37].routine = (synthRoutine) miniDualDecay;
	dispatch[FIRST_DEV + 38].controller = 12;
	dispatch[FIRST_DEV + 38].operator = 15;

	dispatch[FIRST_DEV + 39].controller = 12;
	dispatch[FIRST_DEV + 39].operator = 2;
	dispatch[FIRST_DEV + 40].controller = 4;
	dispatch[FIRST_DEV + 40].operator = 3;
	dispatch[FIRST_DEV + 41].controller = 5;
	dispatch[FIRST_DEV + 41].operator = 5;

	dispatch[ACTIVE_DEVS +0].routine = dispatch[ACTIVE_DEVS +1].routine =
		dispatch[ACTIVE_DEVS +2].routine = dispatch[ACTIVE_DEVS +3].routine =
		dispatch[ACTIVE_DEVS +4].routine = dispatch[ACTIVE_DEVS +5].routine =
		dispatch[ACTIVE_DEVS +6].routine = dispatch[ACTIVE_DEVS +7].routine =
		dispatch[ACTIVE_DEVS +8].routine = dispatch[ACTIVE_DEVS +9].routine =
		dispatch[ACTIVE_DEVS +10].routine = dispatch[ACTIVE_DEVS +11].routine =
		dispatch[ACTIVE_DEVS +14].routine = dispatch[ACTIVE_DEVS +15].routine
		= (synthRoutine) miniMemory;

	dispatch[ACTIVE_DEVS + 0].controller = 1;
	dispatch[ACTIVE_DEVS + 1].controller = 2;
	dispatch[ACTIVE_DEVS + 2].operator = 0;
	dispatch[ACTIVE_DEVS + 3].operator = 1;
	dispatch[ACTIVE_DEVS + 4].operator = 2;
	dispatch[ACTIVE_DEVS + 5].operator = 3;
	dispatch[ACTIVE_DEVS + 6].operator = 4;
	dispatch[ACTIVE_DEVS + 7].operator = 5;
	dispatch[ACTIVE_DEVS + 8].operator = 6;
	dispatch[ACTIVE_DEVS + 9].operator = 7;
	dispatch[ACTIVE_DEVS + 10].operator = 8;
	dispatch[ACTIVE_DEVS + 11].operator = 9;
	dispatch[ACTIVE_DEVS + 14].controller = 3;
	dispatch[ACTIVE_DEVS + 15].controller = 4;

	dispatch[ACTIVE_DEVS + 12].controller = 1;
	dispatch[ACTIVE_DEVS + 13].controller = 2;
	dispatch[ACTIVE_DEVS + 12].routine = dispatch[ACTIVE_DEVS + 13].routine
		= (synthRoutine) miniMidi;

	/* Tune osc-1 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	/* Gain on filter contour */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4,
		CONTROLLER_RANGE - 1);
	/*
	 * Filter type, this selects a Houvilainen filter
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 4);
	/*
	 * Pink filter configuration
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 2, 4096);

	return(0);
}

static void
configureKeylatch(guiSynth *synth)
{
	if (synth->keypanel < 0)
		return;

   	if (synth->flags & NO_LATCHING_KEYS)
   	{
   		int i, p = synth->keypanel;

   		for (i = 0; i < synth->win->app->resources[p].ndevices; i++)
   			synth->win->app->resources[p].devlocn[i].flags
   				|= BRIGHTON_NO_TOGGLE;

   		if (synth->keypanel2 < 0)
   			return;

	   	p = synth->keypanel2;

   		for (i = 0; i < synth->win->app->resources[p].ndevices; i++)
   			synth->win->app->resources[p].devlocn[i].flags
	   			|= BRIGHTON_NO_TOGGLE;
   	}
}


/*
 * These aught to be moved into a more common file, brightonRoutines.c or
 * similar.
 */
int
configureGlobals(guiSynth *synth)
{
	if (global.libtest)
	{
		int count = 10;

		printf("libtest flagged");

		while (global.libtest)
		{
			usleep(100000);
			if (--count == 0)
				break;
		}

		if (count)
			printf(", cleared\n");
		else
			printf("\n");
	}

	if (!global.libtest)
	{
		configureKeylatch(synth);

		if (synth->flags & REQ_MIDI_DEBUG)
		{
			bristolMidiSendNRP(global.controlfd, synth->midichannel,
				BRISTOL_NRP_DEBUG, 1);
			if (synth->sid2 != 0)
				bristolMidiSendNRP(global.manualfd, synth->midichannel+1,
					BRISTOL_NRP_DEBUG, 1);
		}
		if (synth->flags & REQ_MIDI_DEBUG2)
		{
			bristolMidiSendNRP(global.controlfd, synth->midichannel,
				BRISTOL_NRP_DEBUG, 2);
			if (synth->sid2 != 0)
				bristolMidiSendNRP(global.manualfd, synth->midichannel+1,
					BRISTOL_NRP_DEBUG, 2);
		}

		/* Send debug level request to engine and also setup local debugging */
		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_SYSTEM, 0,
				BRISTOL_REQ_DEBUG|((synth->flags&REQ_DEBUG_MASK)>>12));
		if (synth->sid2 != 0)
			bristolMidiSendMsg(global.manualfd, synth->sid2,
				BRISTOL_SYSTEM, 0,
					BRISTOL_REQ_DEBUG|((synth->flags&REQ_DEBUG_MASK)>>12));

		bristolMidiOption(global.controlfd, BRISTOL_NRP_DEBUG,
			(synth->flags&REQ_DEBUG_MASK)>>12);
		if (synth->sid2 != 0)
			bristolMidiOption(global.manualfd, BRISTOL_NRP_DEBUG,
				(synth->flags&REQ_DEBUG_MASK)>>12);

		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_GLIDE, synth->glide * C_RANGE_MIN_1 / 30);
		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_GAIN, synth->gain);
		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_DETUNE, synth->detune);
		bristolMidiSendRP(global.controlfd, synth->midichannel,
			MIDI_RP_PW, synth->pwd);
		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_VELOCITY, synth->velocity);
		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_LWF, synth->lwf);
		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_MNL_PREF, synth->notepref);
		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_MNL_TRIG, synth->notetrig);
		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_MNL_VELOC, synth->legatovelocity);

		/* Global message forwarding on/off */
		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_FORWARD, synth->flags & REQ_FORWARD? 1:0);
		bristolMidiOption(global.controlfd, BRISTOL_NRP_FORWARD,
			synth->flags & REQ_FORWARD? 1:0);

		if (synth->flags & MIDI_NRP)
			bristolMidiSendNRP(global.controlfd, synth->midichannel,
				BRISTOL_NRP_ENABLE_NRP, 1);
		else
			bristolMidiSendNRP(global.controlfd, synth->midichannel,
				BRISTOL_NRP_ENABLE_NRP, 0);

		bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
			BRISTOL_LOWKEY|synth->lowkey);
		bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
			BRISTOL_HIGHKEY|synth->highkey);

		if (synth->sid2 != 0) {
			bristolMidiSendNRP(global.controlfd, synth->midichannel+1,
				BRISTOL_NRP_GLIDE, synth->glide * C_RANGE_MIN_1 / 30);
			bristolMidiSendNRP(global.controlfd, synth->midichannel+1,
				BRISTOL_NRP_GAIN, synth->gain);
			bristolMidiSendNRP(global.controlfd, synth->midichannel+1,
				BRISTOL_NRP_DETUNE, synth->detune);
			bristolMidiSendRP(global.controlfd, synth->midichannel+1,
				MIDI_RP_PW, synth->pwd);
			bristolMidiSendNRP(global.controlfd, synth->midichannel+1,
				BRISTOL_NRP_VELOCITY, synth->velocity);
			bristolMidiSendNRP(global.controlfd, synth->midichannel+1,
				BRISTOL_NRP_LWF, synth->lwf);
			if (synth->flags & MIDI_NRP)
				bristolMidiSendNRP(global.controlfd, synth->midichannel+1,
					BRISTOL_NRP_ENABLE_NRP, 1);
			else
				bristolMidiSendNRP(global.controlfd, synth->midichannel+1,
					BRISTOL_NRP_ENABLE_NRP, 0);

			bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
				BRISTOL_LOWKEY|synth->lowkey);
			bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
				BRISTOL_HIGHKEY|synth->highkey);
		}

		if (synth->flags & REQ_LOCAL_FORWARD)
		{
			bristolMidiOption(global.controlfd, BRISTOL_NRP_FORWARD, 1);
			bristolMidiOption(global.controlfd, BRISTOL_NRP_MIDI_GO, 1);
			bristolMidiOption(global.controlfd, BRISTOL_NRP_REQ_FORWARD, 1);
		}

		/* Forwarding options for this emulator */
		if (synth->flags & REQ_REMOTE_FORWARD)
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_SYSTEM, 0, BRISTOL_REQ_FORWARD|1);

		/* Forwarding options disabled for second manual, local and remote */
		if (global.manualfd != 0)
		{
			bristolMidiOption(global.manualfd, BRISTOL_NRP_REQ_SYSEX, 1);
			bristolMidiSendMsg(global.manualfd, synth->sid2,
				BRISTOL_SYSTEM, 0, BRISTOL_REQ_FORWARD);
		}

		bristolMidiSendNRP(global.controlfd, synth->midichannel,
			BRISTOL_NRP_MIDI_GO, 1024);
	}
	/*
	 * We should consider seeding the synth with a short note blip
	if ((global.synths->notepref == BRIGHTON_HNP)
		|| (global.synths->notepref == BRIGHTON_LNP))
	{
		sleep(1);
		bristolMidiSendMsg(global.controlfd,
			synth->midichannel, BRISTOL_EVENT_KEYON, 0, 5);
		sleep(1);
		bristolMidiSendMsg(global.controlfd,
			synth->midichannel, BRISTOL_EVENT_KEYOFF, 0, 5);
	}
	 */
	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
miniConfigure(brightonWindow *win)
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

	printf("going operational: %p, %p\n", synth, win);

	synth->flags |= OPERATIONAL;
	synth->transpose = 36;
	synth->keypanel = KEY_PANEL;
	synth->keypanel2 = -1;

	/*
	 * We now want to set a couple of parameters provided by switches in the
	 * 0.9 release but configurable per synth. These are detune and gain to
	 * start with. Some of this code could be separated as it is likely to be
	 * common to each synth. These parameters will be sent to the engine using
	 * midi non-registered parameters. These will be sent as NRP-98/99 and Data
	 * Entry-6/38. To simplify the interface we should be able to make one call
	 * send a NRP including the number and its 14 bit value. It is up to the
	 * interface to decide how to send it depending on whether it is a repeat
	 * message or not.
	 */
	configureGlobals(synth);

	loadMemory(synth, "mini", 0, synth->location, synth->mem.active,
		FIRST_DEV, 0);

	brightonPut(win, "bitmaps/blueprints/minishade.xpm",
		0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);

	return(0);
}

