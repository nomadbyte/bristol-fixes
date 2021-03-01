
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

/* Korg Poly6 UI */
/* Bend depth and modes need to be implemented. */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int initmem, width;

static int poly6Init();
static int poly6Configure();
static int poly6Callback(brightonWindow *, int, int, float);
static int poly6ModCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define DEVICE_COUNT (53)
#define ACTIVE_DEVS (DEVICE_COUNT - 20)

#define MEM_START ACTIVE_DEVS
#define MIDI_START (MEM_START + 18)

#define P6_BEND 2
#define KEY_HOLD 17
#define CHORUS (32)

#define RADIOSET_1 MEM_START
#define RADIOSET_2 (MEM_START + 1)
#define RADIOSET_3 17 /* MODE */

#define KEY_PANEL 1

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
 * This example is for a poly6Bristol type synth interface.
 */
#define R0 200
#define R1 465

#define D0 8
#define D1 41
#define D2 23

#define C0 25
#define C1 (C0 + D1 + D0)
#define C2 (C1 + D1 + D0)
#define C3 (C2 + D1)
#define C4 (C3 + D1)
#define C5 (C4 + D1)
#define C6 (C5 + D1)
#define C7 (C6 + D1)
#define C8 (C7 + D1)
#define C9 (C8 + D1)
#define C10 (C9 + D1)
#define C11 (C10 + D1 + D0)
#define C12 (C11 + D1)
#define C13 (C12 + D1)
#define C14 (C13 + D1)

#define C15 (C14 + D1 + D0)
#define C16 (C15 + D1)

#define C17 (C16 + D1)
#define C18 (C17 + D2)
#define C19 (C18 + D2)
#define C20 (C19 + D2)
#define C21 (C20 + D2)
#define C22 (C21 + D2)
#define C23 (C22 + D2)
#define C24 (C23 + D2)
#define C25 (C24 + D2)

#define C26 (C25 + D2 + 9)

#define S1 140
#define S2 12
#define S3 180
#define S4 10

static brightonLocations locations[DEVICE_COUNT] = {
	{"MasterVolume", 0, C0, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"MasterTuning", 0, C1, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, BRIGHTON_NOTCH},
	{"BendDepth", 0, C1, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* VCO 1 */
	{"OSC-Transpose", 0, C2, R0, S1, S1, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"OSC-WaveForm", 0, C3, R0, S1, S1, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"OSC-PWM", 0, C4, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Osc-Tuning", 0, C5, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Osc-Subwave", 2, C6 - 5, R0 + 10, S4, S3, 0, 2, 0, 0, 0,
		BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},

	/* Noise - Mods 8 */
	{"Noise", 0, C2, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"LFO-Freq", 0, C3, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"LFO-Delay", 0, C4, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"LFO-Level", 0, C5, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"LFO-Dest", 2, C6 - 5, R1 + 20, S4, S3, 0, 2, 0, 0, 0,
		BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},

	/* Filter 13 */
	{"VCF-Freq", 0, C7, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Res", 0, C8, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Env", 0, C9, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-KBD", 0, C10, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* Mode 17 */
	{"MODE Hold", 2, C8 + 8, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_RADIOBUTTON},
	{"MODE Mono", 2, C9 + 8, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_RADIOBUTTON},
	{"MODE Mono", 2, C10 + 8, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_RADIOBUTTON},

	/* ENV 20 */
	{"FiltEnv-Attack", 0, C11, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"FiltEnv-Decay", 0, C12, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"FiltEnv-Sustain", 0, C13, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"FiltEnv-Relase", 0, C14, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* ENV 24 */
	{"Env-Attack", 0, C11, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Env-Decay", 0, C12, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Env-Sustain", 0, C13, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Env-Release", 0, C14, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* Amp - Effects 28 */
	{"Amp-Env/Gate", 2, C15, R0 + 20, S4, S1 - 20, 0, 1, 0,
		"bitmaps/buttons/sw1.xpm", "bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"Amp-Gain", 0, C16, R0, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Chorus-Mode", 0, C15, R1, S1, S1, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Chorus-Depth", 0, C16, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Glide */
	{"Glide", 0, C0, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* Memory - MIDI 33 */
	{"", 2, C17, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, C18, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C19, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C20, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C21, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C22, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C24, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C25, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, C18, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C19, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C20, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C21, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C22, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C24, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C25, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	/* Load */
	{"", 2, C17, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, C26, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C26, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp poly6App = {
	"poly",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH,
	poly6Init,
	poly6Configure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{6, 100, 2, 2, 5, 520, 0, 0},
	800, 240, 0, 0,
	5, /* one panel only */
	{
		{
			"Poly6",
			"bitmaps/blueprints/poly6.xpm",
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			poly6Callback,
			12, 0, 976, 560,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			105, 550, 908, 435,
			KEY_COUNT,
			keys
		},
		{
			"Mods",
			"bitmaps/blueprints/polymods.xpm",
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			poly6ModCallback,
			15, 550, 90, 435,
			2,
			mods
		},
		{
			"Edge",
			0,
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 15, 995,
			0,
			0
		},
		{
			"Edge",
			0,
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			988, 0, 12, 995,
			0,
			0
		},
	}
};

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
			loadMemory(synth, "poly", 0,
				synth->bank + synth->location,
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
poly6ModCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	float bend = synth->mem.param[P6_BEND];

/*printf("poly6ModCallback(%x, %i, %i, %f)\n", synth, panel, index, value); */

	/*
	 * If this is controller 0 it is the frequency control, otherwise a 
	 * generic controller 1. The depth of the bend wheel is a parameter, so
	 * we need to scale the value here.
	 */
	if (index == 0)
	{
		/*
		 * Value is between 0 and 1 latching at 0.5. Have to scale the value
		 * and subtrace if from the mid point
		 */
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_PITCH, 0,
			(int) (((0.5 - bend / 2) + (value * bend)) * C_RANGE_MIN_1));
	} else {
		bristolMidiControl(global.controlfd, synth->midichannel,
			0, 1, ((int) (value * C_RANGE_MIN_1)) >> 7);
	}
	return(0);
}

static int
poly6MidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
/*printf("%i, %i, %i\n", c, o, v); */
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
poly6Hold(guiSynth *synth)
{
	if (((synth->flags & OPERATIONAL) == 0) || (global.libtest))
		return;

	if (synth->mem.param[KEY_HOLD])
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|1);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|0);
}

static void
fixModes(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * Look into the memory at 18 and 19 to find the current mode and send it
	 * to the engine.
	 */
	if (synth->mem.param[18] != 0)
		bristolMidiSendMsg(fd, chan, 126, 7, 1);
	else
		bristolMidiSendMsg(fd, chan, 126, 8, 1);

	poly6Hold(synth);
}

int
poly6loadMemory(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	brightonEvent event;

	loadMemory(synth, "poly", 0, location, synth->mem.active, 0, 0);
	fixModes(synth, synth->sid, synth->midichannel, 0, 0, 0);

	event.type = BRISTOL_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, 0, MEM_START + location / 10, &event);
	brightonParamChange(synth->win, 0, MEM_START+8+(location % 10), &event);

	return(0);
}

static void
poly6Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (synth->flags & MEM_LOADING)
		return;

	switch(c) {
		case 0:
			saveMemory(synth, "poly", 0,
				synth->bank + synth->location, 0);
			return;
			break;
		case 17:
			loadMemory(synth, "poly", 0,
				synth->bank + synth->location,
				synth->mem.active, 0, 0);
			fixModes(synth, fd, chan, c, o, v);
			return;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			if (synth->dispatch[RADIOSET_1].other2)
			{
				synth->dispatch[RADIOSET_1].other2 = 0;
				return;
			}
			if (synth->dispatch[RADIOSET_1].other1 != -1)
			{
				synth->dispatch[RADIOSET_1].other2 = 1;

				if (synth->dispatch[RADIOSET_1].other1 != c)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_1].other1 + MEM_START, &event);
			}
			synth->dispatch[RADIOSET_1].other1 = c;
			synth->bank = c * 10;
			break;
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			if (synth->dispatch[RADIOSET_2].other2)
			{
				synth->dispatch[RADIOSET_2].other2 = 0;
				return;
			}
			if (synth->dispatch[RADIOSET_2].other1 != -1)
			{
				synth->dispatch[RADIOSET_2].other2 = 1;

				if (synth->dispatch[RADIOSET_2].other1 != c)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_2].other1 + MEM_START, &event);
			}
			synth->dispatch[RADIOSET_2].other1 = c;
			/* Do radio buttons */
			synth->location = c - 8;
			break;
	}
}

static void
poly6PWM(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * This has two functions, it sets the PW of the osc and also the gain
	 * of PWM.
	 */
	bristolMidiSendMsg(fd, chan, 0, 0, v);
	bristolMidiSendMsg(fd, chan, 126, 4, v);
}

static void
poly6Waveform(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (c == 1)
	{
		/*
		 * Subosc 2 == off
		 */
		if (v == 2)
			bristolMidiSendMsg(fd, chan, 1, 6, 0);
		if (v == 1)
		{
			bristolMidiSendMsg(fd, chan, 1, 1, 1);
			bristolMidiSendMsg(fd, chan, 1, 6, 1);
		}
		if (v == 0)
		{
			bristolMidiSendMsg(fd, chan, 1, 1, 0);
			bristolMidiSendMsg(fd, chan, 1, 6, 1);
		}
		return;
	}

	switch(v) {
		case 0:
			/*
			 * Ramp off, square off, tri on
			 */
			bristolMidiSendMsg(fd, chan, 0, 4, 0);
			bristolMidiSendMsg(fd, chan, 0, 6, 0);
			bristolMidiSendMsg(fd, chan, 0, 5, 1);

			bristolMidiSendMsg(fd, chan, 126, 5, 0);
			break;
		case 1:
			bristolMidiSendMsg(fd, chan, 0, 5, 0);
			bristolMidiSendMsg(fd, chan, 0, 6, 0);
			bristolMidiSendMsg(fd, chan, 0, 4, 1);

			bristolMidiSendMsg(fd, chan, 126, 5, 0);
			break;
		case 2:
			bristolMidiSendMsg(fd, chan, 0, 4, 0);
			bristolMidiSendMsg(fd, chan, 0, 5, 0);
			bristolMidiSendMsg(fd, chan, 0, 6, 1);

			bristolMidiSendMsg(fd, chan, 126, 5, 0);
			break;
		case 3:
			bristolMidiSendMsg(fd, chan, 0, 4, 0);
			bristolMidiSendMsg(fd, chan, 0, 5, 0);
			bristolMidiSendMsg(fd, chan, 0, 6, 1);

			bristolMidiSendMsg(fd, chan, 126, 5, 1);
			break;
	}
}

static int
poly6Midi(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
		if ((newchan = synth->midichannel + 1) >= 15)
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
poly6Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	/* printf("poly6Callback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (poly6App.resources[0].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

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
poly6Gain(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	switch (c) 
	{
		case 6:
		case 101:
			/*
			 * Gain controls
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 4,
				((int) (synth->mem.param[0] * synth->mem.param[29]
					* C_RANGE_MIN_1)));
			break;
		case 100:
			/*
			 * This is the attentuator switch, 1 == gate.
			 */
			if (synth->mem.param[28])
			{
				/*
				 * Configure a gate.
				 */
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0, 512);
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1, 16383);
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 2, 16383);
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 3, 512);
			} else {
				/*
				 * Configure the env
				 */
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0,
					(int) (synth->mem.param[24] * C_RANGE_MIN_1));
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1,
					(int) (synth->mem.param[25] * C_RANGE_MIN_1));
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 2,
					(int) (synth->mem.param[26] * C_RANGE_MIN_1));
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 3,
					(int) (synth->mem.param[27] * C_RANGE_MIN_1));
			}
			break;
		case 99:
			if (synth->mem.param[28] == 0)
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, o, v);
			bristolMidiSendMsg(global.controlfd, synth->sid, 8, 3, v);
			break;
	}
}

static int
poly6Mode(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (synth->flags & MEM_LOADING)
		return(0);

	if (synth->dispatch[RADIOSET_3].other2)
		return(synth->dispatch[RADIOSET_3].other2 = 0);

	if (synth->dispatch[RADIOSET_3].other1 != -1)
	{
		synth->dispatch[RADIOSET_3].other2 = 1;

		if (synth->dispatch[RADIOSET_3].other1 != c)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[RADIOSET_3].other1, &event);
	}
	synth->dispatch[RADIOSET_3].other1 = c;

/*	bristolMidiSendMsg(global.controlfd, synth->sid, 126, o, v); */

	poly6Hold(synth);

	switch (c) {
		case 18:
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 7, 1);
			break;
		case 19:
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 8, 1);
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
poly6Init(brightonWindow *win)
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

	printf("Initialise the poly6 link to bristol: %p\n", synth->win);

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

	if (synth->voices == BRISTOL_VOICECOUNT)
		synth->voices = 6;
	if (!global.libtest)
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = poly6MidiSendMsg;

	/* Vol tune bend - NOT FINNISHED */
	dispatch[0].controller = 6;
	dispatch[0].operator = 4;
	dispatch[0].routine = (synthRoutine) poly6Gain;
	dispatch[1].controller = 126;
	dispatch[1].operator = 1;
	dispatch[2].controller = 126;
	dispatch[2].operator = 2;

	/* VCO */
	dispatch[3].controller = 0;
	dispatch[3].operator = 1;
	dispatch[4].controller = 0;
	dispatch[4].operator = 50;
	dispatch[4].routine = (synthRoutine) poly6Waveform;
	dispatch[5].controller = 0;
	dispatch[5].operator = 0;
	dispatch[5].routine = (synthRoutine) poly6PWM;
	dispatch[6].controller = 10;
	dispatch[6].operator = 0;
	/* Subosc */
	dispatch[7].controller = 1;
	dispatch[7].operator = 0;
	dispatch[7].routine = (synthRoutine) poly6Waveform;
	/* Noise */
	dispatch[8].controller = 7;
	dispatch[8].operator = 0;

	/* Mods */
	dispatch[9].controller = 2;
	dispatch[9].operator = 0;
	dispatch[10].controller = 8;
	dispatch[10].operator = 0;
	dispatch[11].controller = 8;
	dispatch[11].operator = 4;
	dispatch[12].controller = 126;
	dispatch[12].operator = 6;

	/* Filter */
	dispatch[13].controller = 3;
	dispatch[13].operator = 0;
	dispatch[14].controller = 3;
	dispatch[14].operator = 1;
	dispatch[15].controller = 3;
	dispatch[15].operator = 2;
	dispatch[16].controller = 3;
	dispatch[16].operator = 3;

	/* MODE NOT DONE */
	/*
	 * Hold - no key off events
	 * Mono - unison
	 * Poly - poly
	 */
	dispatch[17].controller = 17;
	dispatch[17].operator = 1;
	dispatch[18].controller = 18;
	dispatch[18].operator = 7;
	dispatch[19].controller = 19;
	dispatch[19].operator = 8;
	dispatch[17].routine = dispatch[18].routine = dispatch[19].routine
		= (synthRoutine) poly6Mode;

	/* Filter Env */
	dispatch[20].controller = 4;
	dispatch[20].operator = 0;
	dispatch[21].controller = 4;
	dispatch[21].operator = 1;
	dispatch[22].controller = 4;
	dispatch[22].operator = 2;
	dispatch[23].controller = 4;
	dispatch[23].operator = 3;
	/* Env */
	dispatch[24].controller = 99; /* Will be 6, but interpreted */
	dispatch[24].operator = 0;
	dispatch[25].controller = 99;
	dispatch[25].operator = 1;
	dispatch[26].controller = 99;
	dispatch[26].operator = 2;
	dispatch[27].controller = 99;
	dispatch[27].operator = 3;

	/* VCA NOT DONE */
	dispatch[28].controller = 100;
	dispatch[28].operator = 0;
	dispatch[29].controller = 101;
	dispatch[29].operator = 1;
	dispatch[28].routine = dispatch[29].routine
		= dispatch[24].routine = dispatch[25].routine
		= dispatch[26].routine = dispatch[27].routine
		= (synthRoutine) poly6Gain;
	/* Effect */
	dispatch[30].controller = 100;
	dispatch[30].operator = 0;
	dispatch[31].controller = 100;
	dispatch[31].operator = 1;

	dispatch[32].controller = 126;
	dispatch[32].operator = 0;

	dispatch[MEM_START + 0].controller = 0;
	dispatch[MEM_START + 1].controller = 1;
	dispatch[MEM_START + 2].controller = 2;
	dispatch[MEM_START + 3].controller = 3;
	dispatch[MEM_START + 4].controller = 4;
	dispatch[MEM_START + 5].controller = 5;
	dispatch[MEM_START + 6].controller = 6;
	dispatch[MEM_START + 7].controller = 7;
	dispatch[MEM_START + 8].controller = 8;
	dispatch[MEM_START + 9].controller = 9;
	dispatch[MEM_START + 10].controller = 10;
	dispatch[MEM_START + 11].controller = 11;
	dispatch[MEM_START + 12].controller = 12;
	dispatch[MEM_START + 13].controller = 13;
	dispatch[MEM_START + 14].controller = 14;
	dispatch[MEM_START + 15].controller = 15;
	dispatch[MEM_START + 16].controller = 16;
	dispatch[MEM_START + 17].controller = 17;

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
		dispatch[MEM_START + 13].routine =
		dispatch[MEM_START + 14].routine =
		dispatch[MEM_START + 15].routine =
		dispatch[MEM_START + 16].routine =
		dispatch[MEM_START + 17].routine
			= (synthRoutine) poly6Memory;

	dispatch[RADIOSET_1].other1 = -1;
	dispatch[RADIOSET_2].other1 = -1;
	dispatch[RADIOSET_3].other1 = -1;

	dispatch[MIDI_START].controller = 1;
	dispatch[MIDI_START + 1].controller = 2;
	dispatch[MIDI_START].routine = dispatch[MIDI_START + 1].routine =
		(synthRoutine) poly6Midi;

	/* Put in diverse defaults settings */
	/* Gain on filter env */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);
	/* Waveform subosc */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 6, C_RANGE_MIN_1);
	/* LFO Env parameters. */
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, 12000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 3, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 5, 0);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
poly6Configure(brightonWindow *win)
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

	brightonPut(win,
		"bitmaps/blueprints/poly6shade.xpm", 0, 0, win->width, win->height);
	width = win->width;

	if (synth->location == 0)
	{
		synth->bank = 10;
		synth->location = 1;
	} else {
		synth->bank = (synth->location % 100) / 10 * 10;
		synth->location = synth->location % 10;
	}

	event.type = BRISTOL_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, synth->panel, MEM_START + synth->bank / 10,
		&event);
	brightonParamChange(synth->win, synth->panel, MEM_START+8+synth->location,
		&event);
	/* Poly */
	brightonParamChange(synth->win, synth->panel, 19, &event);

	loadMemory(synth, "poly", 0, initmem, synth->mem.active, 0, 0);
	fixModes(synth, synth->sid, synth->midichannel, 0, 0, 0);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	event.type = BRIGHTON_FLOAT;
	event.value = 0.1;
	brightonParamChange(synth->win, 2, 1, &event);
	configureGlobals(synth);

	synth->loadMemory = (loadRoutine) poly6loadMemory;

	return(0);
}

