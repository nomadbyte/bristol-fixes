
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
 * Notes from the Voyager manual:
 *
 * Two filter modes:
 *
 * 	1. Dual parallel LP, stereo separation, both resonant
 * 	2. Serial HPF/LPF, mono, HPF is non resonant.
 *
 * Currently the filter switch is for key envelope velocity. Drop velocity on
 * this env and use the correct option to select the filter type.
 *
 * The mod busses are passed through:
 *
 * 	1. Wheel Mod
 * 	2. On/Off Pedal
 *
 * Release is a scaler of the actual release time. Affects both envelopes.
 *
 * Key modes, from control panel (mono only):
 *
 * 	1. Lowest
 * 	2. Highest
 * 	3. Last key
 * 	4. First key
 *
 * Trigger modes, from control panel (mono only):
 *
 * 	1. Single
 * 	2. Multi
 */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

int explorerInit();
int explorerConfigure();
int explorerCallback(brightonWindow *, int, int, float);
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
 * This example is for a explorerBristol type synth interface.
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
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp explorerApp = {
	"explorer",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/wood2.xpm",
	0, /* or BRIGHTON_STRETCH, default is tesselate */
	explorerInit,
	explorerConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	817, 472, 0, 0,
	7, /* one panel only */
	{
		{
			"Explorer",
			"bitmaps/blueprints/explorer.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
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
			"bitmaps/blueprints/mods.xpm",
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			modCallback,
			15, 675, 125, 300,
			2,
			mods
		},
		{
			"SP0",
			0,
			"bitmaps/textures/wood2.xpm",
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
			synth->location = value + synth->bank * 10;
			if (loadMemory(synth, "explorer", 0, synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY1);
			else
				displayText(synth, "PRG", synth->location, DISPLAY1);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			synth->location = (synth->location % 10) + value * 10;
			if (loadMemory(synth, "explorer", 0, synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY1);
			else
				displayText(synth, "PRG", synth->location, DISPLAY1);
			break;
	}
	return(0);
}

static int
explorerMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
explorerMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
/*	printf("explorerMemory(%i %i %i %i %i)\n", fd, chan, c, o, v); */

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
			if (synth->dispatch[MEM_START].other1 != -1)
			{
				brightonEvent event;
				synth->dispatch[MEM_START].other2 = 1;

				if (synth->dispatch[MEM_START].other1 != c)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[MEM_START].other1, &event);
			}
			synth->dispatch[MEM_START].other1 = c;

			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;

			if (loadMemory(synth, "explorer", 0, synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY1);
			else
				displayText(synth, "PRG", synth->location, DISPLAY1);
			break;
		case 1:
			if (loadMemory(synth, "explorer", 0, synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY2);
			else
				displayText(synth, "PRG", synth->location, DISPLAY2);
/*			synth->location = 0; */
			break;
		case 2:
			saveMemory(synth, "explorer", 0, synth->location, FIRST_DEV);
			displayText(synth, "PRG", synth->location, DISPLAY2);
/*			synth->location = 0; */
			break;
		case 3:
			while (loadMemory(synth, "explorer", 0, ++synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location > 999)
					synth->location = -1;
			}
			displayText(synth, "PRG", synth->location, DISPLAY2);
			break;
		case 4:
			while (loadMemory(synth, "explorer", 0, --synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location < 0)
					synth->location = 999;
			}
			displayText(synth, "PRG", synth->location, DISPLAY2);
			break;
	}
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
int
explorerCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/* printf("explorerCallback(%i, %i, %f)\n", panel, index, value); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);
	if (index >= DEVICE_COUNT)
		return(0);

	if (explorerApp.resources[0].devlocn[index].to == 1.0)
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

static void
explorerMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	printf("explorerMidi(%i %i %i %i %i)\n", fd, chan, c, o, v);

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[MEM_START + 14].other1 == MEM_START + 14)
	{
		int newchan;

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
			bristolMidiSendMsg(global.controlfd, synth->sid,
				127, 0, BRISTOL_MIDICHANNEL|newchan);

		synth->midichannel = newchan;

		displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY1);
	} else {
		if (c == 1) {
			while (locations[--synth->dispatch[MEM_START + 15].other2].name[0]
				== '\0')
			{
				if (synth->dispatch[MEM_START + 15].other2 < 0)
					synth->dispatch[MEM_START + 15].other2 = 49;
			}
		} else { 
			while (locations[++synth->dispatch[MEM_START + 15].other2].name[0]
				== '\0')
			{
				if (synth->dispatch[MEM_START + 15].other2 >= 49)
					synth->dispatch[MEM_START + 15].other2 = 0;
			}
		}
		displayText(synth,
			locations[synth->dispatch[MEM_START + 15].other2].name,
			synth->dispatch[MEM_START + 15].other2,
			DISPLAY2);
		if (synth->dispatch[MEM_START + 15].other1 == 1)
		{
/*printf("PANEL X SELECTOR %i %i = %i %i\n", */
/*synth->dispatch[MEM_START + 15].other1, */
/*synth->dispatch[MEM_START + 15].other2, */
/*synth->dispatch[synth->dispatch[MEM_START + 15].other2].controller, */
/*synth->dispatch[synth->dispatch[MEM_START + 15].other2].operator); */
			synth->dispatch[54].controller =
				synth->dispatch[
					synth->dispatch[MEM_START + 15].other2].controller;
			synth->dispatch[54].operator =
				synth->dispatch[
					synth->dispatch[MEM_START + 15].other2].operator;
		} else {
/*printf("PANEL Y SELECTOR %i %i = %i %i\n", */
/*synth->dispatch[MEM_START + 15].other1, */
/*synth->dispatch[MEM_START + 15].other2, */
/*synth->dispatch[synth->dispatch[MEM_START + 15].other2].controller, */
/*synth->dispatch[synth->dispatch[MEM_START + 15].other2].operator); */
			synth->dispatch[55].controller =
				synth->dispatch[
					synth->dispatch[MEM_START + 15].other2].controller;
			synth->dispatch[55].operator =
				synth->dispatch[
					synth->dispatch[MEM_START + 15].other2].operator;
		}
	}
}

static void
explorerPanelSelect(guiSynth *synth, int fd, int chan, int c, int o, int v)
{

/*	printf("explorerPanelSelect(%x, %i, %i, %i, %i, %i\n", */
/*		synth, fd, chan, c, o, v); */
/* dispatch[MEM_START + 14] = dispatch[MEM_START + 15] */

	if (synth->dispatch[MEM_START + 14].other2)
	{
		synth->dispatch[MEM_START + 14].other2 = 0;
		return;
	}

	if (synth->dispatch[MEM_START + 14].other1 != -1)
	{
		brightonEvent event;
		synth->dispatch[MEM_START + 14].other2 = 1;

		if (synth->dispatch[MEM_START + 14].other1 != c)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[MEM_START + 14].other1, &event);
	}
	displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY1);

	if (c == MEM_START + 15)
	{
		if (synth->dispatch[MEM_START + 15].other1 > 1)
			synth->dispatch[MEM_START + 15].other1 = 0;

		switch (synth->dispatch[MEM_START + 15].other1) {
			case 0:
			default:
				displayText(synth, "PAN-X",
					synth->dispatch[MEM_START + 15].other2, DISPLAY1);
				break;
			case 1:
				displayText(synth, "PAN-Y",
					synth->dispatch[MEM_START + 15].other2, DISPLAY1);
				break;
		}
		synth->dispatch[MEM_START + 15].other1++;
	}
	synth->dispatch[MEM_START + 14].other1 = c;
}

/*
static void
explorerOsc3Transpose(guiSynth *synth)
{
	if (synth->mem.param[23] != 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1,
			(int) (synth->mem.param[18] / 4));
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1,
			(int) synth->mem.param[18]);
}
*/

static void
explorerFilterMode(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	/*
	 * If value is zero we need to configure two H/LPF and clear the MODE
	 * flag (12/12). Otherwise set the flag and configure C/HPF, H/LPF.
	 */
	if (v == 0) {
		/*
		printf("LPF/LPF\n");
		 * FIlter 1 must change to type 4, Houvilainen LPF.
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 4);
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 6, 0);
	} else {
		/*
		printf("HPF/LPF\n");
		 * FIlter 1 must change to type 0, Chamberlain and HPF
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 0);
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 6, 2);
	}

	/* And the main routing flag for filter stereo panning */
	bristolMidiSendMsg(global.controlfd, synth->sid, 12, 12, v);
}

static void
explorerFilter(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	int value;

	/* See if this is the key track or mod */
	if (c == 3)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, c, v);
		bristolMidiSendMsg(global.controlfd, synth->sid, 9, c, v);

		return;
	}

	if (c == 1)
	{
		/* Res goes to both LPF but not to the HPF. */
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, c, v);
		bristolMidiSendMsg(global.controlfd, synth->sid, 9, c, v);

		return;
	}

	/*
	 * We have mod and res that just goes to each filter.
	 *
	 * Cutoff and spare are a function of the eachother.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 9, 0,
		(int) (synth->mem.param[34] * C_RANGE_MIN_1));

	value = (synth->mem.param[34] + (synth->mem.param[35] - 0.5f))
		* C_RANGE_MIN_1;

	if (value > C_RANGE_MIN_1)
		value = C_RANGE_MIN_1;
	else if (value < 0)
		value = 0;

	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 0, value);
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 0, */
/*		(int) (synth->mem.param[35] * C_RANGE_MIN_1)); */
}

static void
explorerRelease(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	if ((c == 3) || (c == 5)) {
		/* Filter/APM env release */
		if (synth->mem.param[53] != 0)
			bristolMidiSendMsg(global.controlfd, synth->sid, c, 3, v);
		else
			bristolMidiSendMsg(global.controlfd, synth->sid, c, 3, v / 10);
	} else {
		/* Selection switch */
		if (synth->mem.param[53] != 0)
		{
			bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3,
				(int) (synth->mem.param[42] * C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 5, 3,
				(int) (synth->mem.param[47] * C_RANGE_MIN_1));
		} else {
			bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3,
				(int) (synth->mem.param[42] * C_RANGE_MIN_1 / 10));
			bristolMidiSendMsg(global.controlfd, synth->sid, 5, 3,
				(int) (synth->mem.param[47] * C_RANGE_MIN_1 / 10));
		}
	}
}

static void
explorerGlide(guiSynth *synth)
{
	if (synth->mem.param[52] != 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 11, 0,
			(int) (synth->mem.param[3] * C_RANGE_MIN_1));
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 11, 0, 0);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
int
explorerInit(brightonWindow *win)
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

	synth->win = win;

	printf("Initialise the explorer link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].routine = explorerMidiSendMsg;

	/* tune/glide */
	dispatch[0].controller = 8;
	dispatch[0].operator = 0;
	dispatch[1].controller = 8;
	dispatch[1].operator = 1;
	dispatch[2].controller = 10;
	dispatch[2].operator = 0;
	dispatch[3].routine = (synthRoutine) explorerGlide;

	/* Bus mods */
	dispatch[4].controller = 15;
	dispatch[4].operator = 0;
	dispatch[5].controller = 15;
	dispatch[5].operator = 1;
	dispatch[6].controller = 15;
	dispatch[6].operator = 2;
	dispatch[7].controller = 15;
	dispatch[7].operator = 3;
	dispatch[8].controller = 16;
	dispatch[8].operator = 0;
	dispatch[9].controller = 16;
	dispatch[9].operator = 1;
	dispatch[10].controller = 16;
	dispatch[10].operator = 2;
	dispatch[11].controller = 16;
	dispatch[11].operator = 3;

	/* Oscillators */
	dispatch[12].controller = 0;
	dispatch[12].operator = 1;
	dispatch[13].controller = 0;
	dispatch[13].operator = 0;
	dispatch[14].controller = 1;
	dispatch[14].operator = 2;
	dispatch[15].controller = 1;
	dispatch[15].operator = 1;
	dispatch[16].controller = 1;
	dispatch[16].operator = 0;
	dispatch[17].controller = 2;
	dispatch[17].operator = 2;
	dispatch[18].controller = 2;
	dispatch[18].operator = 1;
	dispatch[19].controller = 2;
	dispatch[19].operator = 0;
	/* Osc options */
	dispatch[20].controller = 12;
	dispatch[20].operator = 9;
	dispatch[21].controller = 12;
	dispatch[21].operator = 10;
	dispatch[22].controller = 12;
	dispatch[22].operator = 0;
	dispatch[23].controller = 12;
	dispatch[23].operator = 8;

	/* MIXER */
	dispatch[24].controller = 14;
	dispatch[24].operator = 0;
	dispatch[25].controller = 0;
	dispatch[25].operator = 3;
	dispatch[26].controller = 1;
	dispatch[26].operator = 3;
	dispatch[27].controller = 2;
	dispatch[27].operator = 3;
	dispatch[28].controller = 12;
	dispatch[28].operator = 13;

	/* mixer flags */
	dispatch[29].controller = 12;
	dispatch[29].operator = 5;
	dispatch[30].controller = 12;
	dispatch[30].operator = 2;
	dispatch[31].controller = 12;
	dispatch[31].operator = 3;
	dispatch[32].controller = 12;
	dispatch[32].operator = 4;
	dispatch[33].controller = 12;
	dispatch[33].operator = 6;

	/* Filter */
	dispatch[34].controller = 4;
	dispatch[34].operator = 0;
	dispatch[35].controller = 12;
	dispatch[35].operator = 12;
	dispatch[36].controller = 4;
	dispatch[36].operator = 1;
	dispatch[37].controller = 4;
	dispatch[37].operator = 3;

	/* Filter spacing is now in the emulater, not the filter. We also need a */
	/* filter param dispatcher to configure the two filters together. */
	/* Dispatcher */
	dispatch[34].routine =
		dispatch[35].routine =
		dispatch[36].routine =
		dispatch[37].routine = (synthRoutine) explorerFilter;

	dispatch[38].controller = 12;
	dispatch[38].operator = 12;
	dispatch[38].routine = (synthRoutine) explorerFilterMode;

	/* Filter ADSR */
	dispatch[39].controller = 3;
	dispatch[39].operator = 0;
	dispatch[40].controller = 3;
	dispatch[40].operator = 1;
	dispatch[41].controller = 3;
	dispatch[41].operator = 2;
	dispatch[42].controller = 3;
	dispatch[42].operator = 3;
	dispatch[42].routine = (synthRoutine) explorerRelease;

	/* velocity from filter control */
	dispatch[43].controller = 12;
	dispatch[43].operator = 11;

	/* Amp ADSR */
	dispatch[44].controller = 5;
	dispatch[44].operator = 0;
	dispatch[45].controller = 5;
	dispatch[45].operator = 1;
	dispatch[46].controller = 5;
	dispatch[46].operator = 2;
	dispatch[47].controller = 5;
	dispatch[47].operator = 3;
	dispatch[47].routine = (synthRoutine) explorerRelease;
	dispatch[48].controller = 5;
	dispatch[48].operator = 5;

	/* volume, on/off */
	dispatch[49].controller = 5;
	dispatch[49].operator = 4;
	dispatch[50].controller = 12;
	dispatch[50].operator = 1;
	dispatch[51].controller = 12;
	dispatch[51].operator = 7;
	dispatch[52].routine = (synthRoutine) explorerGlide;
	dispatch[53].controller = 6;
	dispatch[53].routine = (synthRoutine) explorerRelease;

	/* Touch panel */
	dispatch[54].controller = 8;
	dispatch[54].operator = 0;
	dispatch[55].controller = 4;
	dispatch[55].operator = 0;

	/* memory */
	dispatch[MEM_START].operator = 0;
	dispatch[MEM_START].controller = MEM_START;
	dispatch[MEM_START + 1].operator = 1;
	dispatch[MEM_START + 1].controller = MEM_START + 1;
	dispatch[MEM_START + 2].operator = 2;
	dispatch[MEM_START + 2].controller = MEM_START + 2;
	dispatch[MEM_START + 3].operator = 3;
	dispatch[MEM_START + 3].controller = MEM_START + 3;
	dispatch[MEM_START + 4].operator = 4;
	dispatch[MEM_START + 4].controller = MEM_START + 4;
	dispatch[MEM_START + 5].operator = 5;
	dispatch[MEM_START + 5].controller = MEM_START + 5;
	dispatch[MEM_START + 6].operator = 6;
	dispatch[MEM_START + 6].controller = MEM_START + 6;
	dispatch[MEM_START + 7].operator = 7;
	dispatch[MEM_START + 7].controller = MEM_START + 7;
	dispatch[MEM_START + 8].operator = 8;
	dispatch[MEM_START + 8].controller = MEM_START + 8;
	dispatch[MEM_START + 9].operator = 9;
	dispatch[MEM_START + 9].controller = MEM_START + 9;

	dispatch[MEM_START + 10].controller = 1;
	dispatch[MEM_START + 11].controller = 2;
	dispatch[MEM_START + 16].controller = 3;
	dispatch[MEM_START + 17].controller = 4;

	dispatch[MEM_START + 0].routine = dispatch[MEM_START + 1].routine =
	dispatch[MEM_START + 2].routine = dispatch[MEM_START + 3].routine =
	dispatch[MEM_START + 4].routine = dispatch[MEM_START + 5].routine =
	dispatch[MEM_START + 6].routine = dispatch[MEM_START + 7].routine =
	dispatch[MEM_START + 8].routine = dispatch[MEM_START + 9].routine =
	dispatch[MEM_START + 10].routine = dispatch[MEM_START + 11].routine =
	dispatch[MEM_START + 16].routine = dispatch[MEM_START + 17].routine =
		(synthRoutine) explorerMemory;

	/* midi */
	dispatch[MEM_START + 12].controller = 2;
	dispatch[MEM_START + 13].controller = 1;
	dispatch[MEM_START + 12].routine = dispatch[MEM_START + 13].routine =
		(synthRoutine) explorerMidi;

	/* midi/panel selectors */
	dispatch[MEM_START + 14].controller = MEM_START + 14;
	dispatch[MEM_START + 15].controller = MEM_START + 15;
	dispatch[MEM_START + 14].routine = dispatch[MEM_START + 15].routine =
		(synthRoutine) explorerPanelSelect;

	/* No velocity on filter contour, fixed gain */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 0, 16000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 5, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 16383);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
int
explorerConfigure(brightonWindow *win)
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

	displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY1);

	loadMemory(synth, "explorer", 0, synth->location, synth->mem.active,
		FIRST_DEV, 0);
	displayText(synth, "PRG", synth->location, DISPLAY2);

	brightonPut(win,
		"bitmaps/blueprints/explorershade.xpm", 0, 0, win->width, win->height);

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

