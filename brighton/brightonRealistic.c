
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
 */


#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int realisticInit();
static int realisticConfigure();
static int realisticCallback(brightonWindow *, int, int, float);
static int realisticMidiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define OPTS_PANEL 0
#define MOD_PANEL 1
#define KEY_PANEL 1

#define FIRST_DEV 0

#define MOD_COUNT 31

#define OPTS_START 0

#define ACTIVE_DEVS (MOD_COUNT - 3)
#define DEVICE_COUNT (MOD_COUNT)

extern int memCallback(brightonWindow * , int, int, float);
extern brightonLocations mem[];

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
 * This example is for a realisticBristol type synth interface.
 */

#define R0 100
#define R1 (R0 + 80)
#define R2 (R0 + 180)
#define R3 570
#define R4 (R3 + 50)
#define R5 (R4 + 50)

#define W1 20
#define W2 42
#define W3 170
#define W4 16
#define L1 280
#define L2 55
#define L3 150

#define D1 48
#define D2 12
#define C1 40
#define C2 (C1 + D1)
#define C3 (C2 + D1)
#define C4 (C3 + D1 + D1)
#define C5 (C4 + D1 + D1)
#define C6 (C5 + D1)
#define C7 (C6 + D1)
#define C8 (C7 + D1 + D1)
#define C9 (C8 + D1 + D1)
#define C10 (C9 + D1)
#define C11 (C10 + D1 + 30)
#define C12 (C11 + D1)
#define C13 (C12 + D1)
#define C14 (C13 + D1)
#define C15 (C14 + D1)

static brightonLocations locations[MOD_COUNT] = {
	/* Tune 0 */
	{"SynthTuning", 0, C1 - 10, R0 + 50, W3, W3, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, 
		BRIGHTON_NOTCH},
	{"PolyTuning", 0, C3 - 15, R0 + 50, W3, W3, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, 
		BRIGHTON_NOTCH},

	/* Mods 2 */
	{"Mod-Tone", 1, C1, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Mod-Filter", 1, C2, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Mod-Glide", 1, C3, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Mod-LFO-Rate", 1, C4, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Mod-Shape", 2, C4 - D2, R0, W2, L2, 0, 2, 0, 0, 0,
		BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"Mod-EnvTrig", 2, C4 - D2, R2 + 50, W2, L2, 0, 1, 0, "bitmaps/buttons/sw3.xpm",
		"bitmaps/buttons/sw5.xpm", BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},

	/* Osc-1 8 */
	{"Osc1-Sync", 2, C5, R1, W2, L2, 0, 1, 0,
		"bitmaps/buttons/sw3.xpm", "bitmaps/buttons/sw5.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"Osc1-Transpose", 2, C6 + 20, R0 + 40, W4, L3, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"Osc1-Waveform", 2, C7 + 15, R1, W2, L2, 0, 1, 0,
		"bitmaps/buttons/sw3.xpm", "bitmaps/buttons/sw5.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},

	/* Osc-2 11 */
	{"Osc2-Detune", 1, C5, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, BRIGHTON_NOTCH},
	{"Osc2-Transpose", 2, C6 + 20, R4 + 40, W4, L3, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"Osc2-Waveform", 2, C7 + 15, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/sw3.xpm", "bitmaps/buttons/sw5.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},

	/* Contour 14 */
	{"Env-AR/ASR", 2, C8 + 6, R0, W2, L2, 0, 2, 0,
		"bitmaps/buttons/sw3.xpm", "bitmaps/buttons/sw5.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"Env-Trigger", 2, C8 + 6, R2, W2, L2, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"Env-Attack", 1, C9, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Env-Release", 1, C10, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},

	/* Filter 18 */
	{"VCF-KeyTrack", 2, C8 + 6, R5, W2, L2, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"VCF-Cutoff", 1, C9 - 25, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"VCF-Emphasis", 1, C9 + 25, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"VCF-Env", 1, C10 + 20, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},

	/* Mixer 22 */
	{"Mix-Osc1", 1, C11, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Mix-Osc2", 1, C12, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Mix-Noise", 1, C13, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Mix-Bell", 1, C14, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},
	{"Mix-Poly", 1, C15, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/knob7.xpm", 0, 0},

	/* Volume 27 */
	{"MasterVolume", 0, C13 - 15, R0 + 50, W3, W3, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, 0},

	/* Midi Up/Down, save */
	{"", 2, C15, R0, W1, 100, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, C15, R2, W1, 100, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, C15, R2 + 120, W1, 100, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
};

#define S1 200

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp realisticApp = {
	"realistic",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH,
	realisticInit,
	realisticConfigure, /* 3 callbacks */
	realisticMidiCallback,
	0,
	/* This looks like 32 voices however the synth is single voice in preops */
	{32, 100, 2, 2, 5, 520, 0, 0},
	600, 320, 0, 0,
	2, /* panel count */
	{
		{
			"Mods",
			"bitmaps/blueprints/realistic.xpm",
			0,
			BRIGHTON_STRETCH,
			0,
			0,
			realisticCallback,
			22, 0, 975, 500,
			MOD_COUNT,
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
			100, 570, 810, 430,
			KEY_COUNT_2OCTAVE2,
			keys2octave2
		},
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

static int
realisticMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
/* printf("realisticMidiSendMsg(%i, %i, %i)\n", c, o, v); */
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
static int
realisticCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*	printf("realisticCallback(%i, %i, %f): %x\n", panel, index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (realisticApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	switch (panel) {
		case OPTS_PANEL:
			index += OPTS_START;
			break;
	}

	synth->mem.param[index] = value;

	if ((!global.libtest) || (index >= ACTIVE_DEVS))
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#define DEBUG
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
realisticOscTransp(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
printf("transp %i %i\n", cont, value);
	if (cont == 0) {
		switch (value) {
			case 0:
				value = 2;
				break;
			case 1:
				value = 1;
				break;
			case 2:
				value = 0;
				break;
		}
		bristolMidiSendMsg(global.controlfd, synth->sid, cont, 1, value);
	} else {
		switch (value) {
			case 0:
				value = 3;
				break;
			case 1:
				value = 2;
				break;
			case 2:
				value = 1;
				break;
		}
		bristolMidiSendMsg(global.controlfd, synth->sid, cont, 1, value);
	}
}

static void
realisticGate(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
printf("gate %i %i\n", cont, value);
	/*
	 * Whatever we do we have to send the gate value to the engine
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, cont, op, value);

	/*
	 * Now, if the value is 0 we have been requested continuous notes. Need to
	 * decide what that means, but it could be interpreted as no note off
	 * events for the mono synth?
	 */
}

static void
realisticOscWave(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
printf("wave %i %i\n", cont, value);
	if (value == 0) {
		bristolMidiSendMsg(global.controlfd, synth->sid, cont, 5, 0);
		bristolMidiSendMsg(global.controlfd, synth->sid, cont, 6, 1);
	} else {
		bristolMidiSendMsg(global.controlfd, synth->sid, cont, 5, 1);
		bristolMidiSendMsg(global.controlfd, synth->sid, cont, 6, 0);
	}
}

static void
realisticDecay(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int iv = synth->mem.param[17] * C_RANGE_MIN_1;

	/* Always configure the DR values */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 1, iv);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 3, iv);

	if (synth->mem.param[14] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 16383);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 0);

return;
	/*
	 * Couple of oddities here. Firstly this is AR or ASR so we need to check
	 * the value of the sustain switch. Also, if we have autotrigger its just
	 * going to set sustain to 0.
	 */
	if (synth->mem.param[14] == 0) {
		/* Sustain is on, see if trigger allows it */
		if (synth->mem.param[7] != 0) {
printf("1: sustain on, trigger off\n");
			bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 16383);
		} else {
printf("2: sustain on, trigger on\n");
			bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 0);
		}
	} else {
		/* Sustain is off, turn it off */
printf("3: sustain off\n");
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 0);
	}

	/* And set the trigger if desired */
	if (cont == 126) {
		if (value == 1) {
			if (synth->mem.param[14] == 0) {
printf("4: sustain on, trigger off\n");
				bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 16383);
			} else {
printf("5: sustain off, trigger off\n");
				bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 0);
			}
		}

		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 9, value);
	}
}

static void
realisticFiltKey(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int v;

printf("filter key %i\n", value);

	switch (value) {
		default:
		case 0:
			v = 16383;
			break;
		case 1:
			v = 4096;
			break;
		case 2:
			v = 0;
			break;
	}
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, v);
}

static void
realisticMonoTuning(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 10, value);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 10, value);
}

static void
realisticMemory(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
printf("Memory: %i, %i\n", synth->location, value);

	if (synth->flags & SUPPRESS)
		return;

	saveMemory(synth, "realistic", 0, synth->location, 0);
}

static int
realisticMidiCallback(brightonWindow *win, int command, int value, float v)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", command, value);

	switch(command)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", command, value);
			synth->location = value;

			realisticMemory(synth, global.controlfd, synth->sid,
				synth->dispatch[OPTS_START].controller,
				synth->dispatch[OPTS_START].operator, 1.0f);

			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", command, value);
			synth->bank = value;
			break;
	}
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
realisticInit(brightonWindow *win)
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

	printf("Initialise the Realistic link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].routine = realisticMidiSendMsg;

	synth->dispatch[MOD_COUNT - 1].routine
			= (synthRoutine) realisticMemory;

	/* Mono tuning */
	dispatch[0].controller = 7;
	dispatch[0].operator = 10;
	dispatch[0].routine = (synthRoutine) realisticMonoTuning;
	/* Poly tuning */
	dispatch[1].controller = 7;
	dispatch[1].operator = 10;

	/* Mods */
	dispatch[2].controller = 126;
	dispatch[2].operator = 4;
	dispatch[3].controller = 126;
	dispatch[3].operator = 5;

	/* Glide */
	dispatch[4].controller = 126;
	dispatch[4].operator = 0;

	/* LFO rate */
	dispatch[5].controller = 2;
	dispatch[5].operator = 0;
	/* LFO waveform */
	dispatch[6].controller = 126;
	dispatch[6].operator = 6;
	/* LFO Trigger */
	dispatch[7].controller = 126;
	dispatch[7].operator = 9;

	/* Osc-1 sync, transpose, waveform */
	dispatch[8].controller = 1;
	dispatch[8].operator = 7;
	dispatch[9].controller = 0;
	dispatch[9].operator = 0;
	dispatch[9].routine = (synthRoutine) realisticOscTransp;
	dispatch[10].controller = 0;
	dispatch[10].operator = 0;
	dispatch[10].routine = (synthRoutine) realisticOscWave;

	/* Osc-2 detune, transpose, waveform */
	dispatch[11].controller = 1;
	dispatch[11].operator = 2;
	dispatch[12].controller = 1;
	dispatch[12].operator = 0;
	dispatch[12].routine = (synthRoutine) realisticOscTransp;
	dispatch[13].controller = 1;
	dispatch[13].operator = 0;
	dispatch[13].routine = (synthRoutine) realisticOscWave;

	/* Contour */
	dispatch[14].routine = (synthRoutine) realisticDecay;
	dispatch[15].controller = 126;
	dispatch[15].operator = 10;
	dispatch[15].routine = (synthRoutine) realisticGate;
	dispatch[16].controller = 4;
	dispatch[16].operator = 0;
	dispatch[17].routine = (synthRoutine) realisticDecay;

	/* Filter tracking, freq, emf, contour */
	dispatch[18].routine = (synthRoutine) realisticFiltKey;
	dispatch[19].controller = 3;
	dispatch[19].operator = 0;
	dispatch[20].controller = 3;
	dispatch[20].operator = 1;
	dispatch[21].controller = 126;
	dispatch[21].operator = 11;

	/* Osc-1 Mix */
	dispatch[22].controller = 126;
	dispatch[22].operator = 2;
	/* Osc-2 Mix */
	dispatch[23].controller = 126;
	dispatch[23].operator = 3;
	/* Noise Mix */
	dispatch[24].controller = 126;
	dispatch[24].operator = 7;
	/* Bell Mix */
	dispatch[25].controller = 9;
	dispatch[25].operator = 2;
	/* Poly Mix */
	dispatch[26].controller = 7;
	dispatch[26].operator = 3;

	/* Master Volume */
	dispatch[27].controller = 126;
	dispatch[27].operator = 8;

	/*
	 * We have a few parameters that need to be set here to control the 
	 * poly grooming envelope
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 0, 10);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, 10);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 3, 10);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 4, 16383);
	/* Gain of poly osc */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 0, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 6, 1);
	/* Global tuning */
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 1, 8192);
	/* Mono waveform */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 7, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, 8192);
	/* Ring mod */
	bristolMidiSendMsg(global.controlfd, synth->sid, 9, 0, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 9, 1, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 9, 2, 16383);
	/* Noise gain */
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0, 16383);
	/* Mono env */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 16383);
	/* LFO Sync */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, 1);
	/* Filter Type */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
realisticConfigure(brightonWindow *win)
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
	synth->keypanel = KEY_PANEL;
	synth->keypanel2 = -1;
	synth->transpose = 41;
/*	loadMemory(synth, "realistic", 0, synth->location, synth->mem.active, */
/*		FIRST_DEV, 0); */

	loadMemory(synth, "realistic", 0, synth->location, ACTIVE_DEVS, 0,
		BRISTOL_FORCE);

	brightonPut(win,
		"bitmaps/blueprints/realshade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	/*
	event.type = BRIGHTON_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, 0, 0, &event);
	*/
	configureGlobals(synth);

	return(0);
}

