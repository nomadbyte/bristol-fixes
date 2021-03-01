
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

/* Korg MonoPoly */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int initmem, width;

static int polyInit();
static int polyConfigure();
static int polyCallback(brightonWindow *, int, int, float);
/*static int keyCallback(void *, int, int, float); */
static int polyModCallback(brightonWindow *, int, int, float);
static int pmidiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define FIRST_DEV 0
#define DEVICE_COUNT 74
#define ACTIVE_DEVS 56
#define MEM_START ACTIVE_DEVS
#define RADIOSET_1 MEM_START
#define RADIOSET_2 (MEM_START + 1)
#define RADIOSET_3 53

#define KEY_PANEL 1
#define KEY_HOLD 52

#define CDIFF 56
#define CDIFF2 12

#define C0 25
#define C1 (C0 + CDIFF)
#define C2 (C1 + CDIFF + CDIFF2)
#define C3 (C2 + CDIFF + CDIFF2)
#define C4 (C3 + CDIFF)
#define C5 (C4 + CDIFF)
#define C6 (C5 + CDIFF)
#define C7 (C6 + CDIFF + CDIFF2)
#define C8 (C7 + CDIFF + CDIFF2)
#define C9 (C8 + CDIFF + CDIFF2)
#define C10 (C9 + CDIFF)
#define C11 (C10 + CDIFF)
#define C12 (C11 + CDIFF + CDIFF2)
#define C13 (C12 + CDIFF)
#define C14 (C13 + CDIFF)
#define C15 (C14 + CDIFF)

#define R0 115
#define R1 (R0 + 205)
#define R2 (R1 + 205)
#define R3 (R2 + 205)

#define R3u (R2 + 200)
#define R3d (R2 + 310)

#define S1 60
#define S2 100
#define S3 90
#define B1 15
#define B2 60

#define BOFF 11

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
 * This example is for a polyBristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {
	/* Osc-1 - 0 */
	{"Osc-1 Wave", 0, C9, R0, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Osc-1 Transpose", 0, C10, R0, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Osc-1 Gain", 0, C11, R0, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Osc-2 - 3 */
	{"Osc-2 Tune", 0, C8, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, BRIGHTON_NOTCH},
	{"Osc-2 Wave", 0, C9, R1, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Osc-2 Transpose", 0, C10, R1, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Osc-2 Gain", 0, C11, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Osc-3 - 7 */
	{"Osc-3 Tune", 0, C8, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, BRIGHTON_NOTCH},
	{"Osc-3 Wave", 0, C9, R2, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Osc-3 Transpose", 0, C10, R2, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Osc-3 Gain", 0, C11, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Osc-4 - 11 */
	{"Osc-3 Wave", 0, C8, R3, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, BRIGHTON_NOTCH},
	{"Osc-4 Tune", 0, C9, R3, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Osc-4 Transpose", 0, C10, R3, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"Osc-4 Gain", 0, C11, R3, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Filter - 15 */
	{"VCF-Cutoff", 0, C12, R0, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Resonance", 0, C13, R0, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Env", 0, C14, R0, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-KBD", 0, C15, R0, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Filter Env - 19 */
	{"VCF-Attack", 0, C12, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Decay", 0, C13, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Sustain", 0, C14, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Release", 0, C15, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Env - 23 */
	{"VCA-Attack", 0, C12, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCA-Decay", 0, C13, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCA-Sustain", 0, C14, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCA-Release", 0, C15, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Volume - 27 env gain and TO BE REASSIGNED */
	{"Volume", 0, C0, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Arpeg Mode", 2, C1, R1, B1, S2, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	/* Three LFO MG modes - 29 */
	{"MG-1 Gain", 0, C2, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"MG-1 Wave", 0, C2, R2, S3, S3, 0, 3, 0, "bitmaps/knobs/knob6.xpm", 0, 
		BRIGHTON_STEPPED},
	{"MG-2 Gain", 0, C3, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* 3 PW and PWM - 32 */
	{"PW-Source", 2, C4, R1, B1, S2, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"PWM", 0, C5, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"PW", 0, C6, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* 4, actually 5 mods. - 35 */
	{"Mod-X-Mod", 0, C3, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Mod-Freq", 0, C4, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Mod-Source", 2, C5 - 20, R2 + 20, B1, S1, 0, 1, 0,
		"bitmaps/buttons/sw1.xpm", "bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"Mod-Mode", 2, C5 + BOFF * 2, R2, B1, S2, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"Mod-Sng/Dbl", 2, C5 + BOFF * 5 + 10, R2 + 20, B1, S1, 0, 1, 0,
		"bitmaps/buttons/sw1.xpm", "bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	/* Glide, detune - 40 */
	{"Glide", 0, C7, R0, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Detune", 0, C7, R1, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Transpose - 42 */
	{"Transpose", 2, C7 - BOFF, R2, B1, S2, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	/* Noise - 43 */
	{"Noise", 0, C12, R3, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Trig, Damp - 44 */
	{"Trigger", 2, C13, R3 + 20, B1, S1, 0, 1, 0,
		"bitmaps/buttons/sw1.xpm", "bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"Damp", 2, C14, R3 + 20, B1, S1, 0, 1, 0,
		"bitmaps/buttons/sw1.xpm", "bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},

	/* Effects - 46 */
	{"FX on/off", 2, C4 + BOFF, R3 + 20, B1, S1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", 0},

	/* Master Tune - 47 */
	{"MasterTuning", 0, C8, R0, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, BRIGHTON_NOTCH},
	/* Wheel gain */
	{"Bend-Depth", 0, C0, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Bend-Dest", 2, C0, R3, B1, S2, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"ModWheel-Depth", 0, C1, R2, S3, S3, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"ModWheel-Dest", 2, C1, R3, B1, S2, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},

	/* Modes */
	{"Mode-Hold", 2, C2, R3, B1, S1, 0, 1, 0,
		"bitmaps/buttons/pressoffw.xpm", "bitmaps/buttons/pressonw.xpm", 0},
	{"Mode-Mono", 2, C2 + BOFF * 2 + 9, R3, B1, S1, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", 0},
	{"Mode-Poly", 2, C3 - 9, R3, B1, S1, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", 0},
	{"Mode-Share", 2, C3 + BOFF * 2, R3, B1, S1, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", 0},

	/* Memories - Save */
	{"", 2, C5, R3u, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C5 + BOFF* 2, R3u, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 4, R3u, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 6, R3u, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 8, R3u, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 10, R3u, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 12, R3u, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 14, R3u, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"", 2, C5 + BOFF* 2, R3d, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 4, R3d, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 6, R3d, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 8, R3d, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 10, R3d, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 12, R3d, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + BOFF* 14, R3d, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	/* Load */
	{"", 2, C5, R3d, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},

	/* Midi */
	{"", 2, C15, R3, B1, S1, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C15 + BOFF * 2, R3, B1, S1, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp polyApp = {
	"monopoly",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/wood4.xpm",
	0, /* BRIGHTON_STRETCH, //flags */
	polyInit,
	polyConfigure, /* 3 callbacks, unused? */
	pmidiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	750, 400, 0, 0,
	7,
	{
		{
			"Poly",
			"bitmaps/blueprints/poly.xpm",
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			polyCallback,
			15, 0, 970, 620,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/tkbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			150, 620, 845, 350,
			KEY_COUNT_3OCTAVE,
			keys3octave2
		},
		{
			"Mods",
			"bitmaps/blueprints/polymods.xpm",
			"bitmaps/textures/metal4.xpm", /* flags */
			BRIGHTON_REVERSE,
			0,
			0,
			polyModCallback,
			15, 620, 135, 350,
			2,
			mods
		},
		{
			"Side wood",
			0,
			"bitmaps/textures/wood4.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 15, 1000,
			0,
			0
		},
		{
			"Side wood",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			2, 5, 12, 985,
			0,
			0
		},
		{
			"Side wood",
			0,
			"bitmaps/textures/wood4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /*flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		},
		{
			"Side wood",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			986, 5, 14, 985,
			0,
			0
		}
	}
};

static void
fixModes(guiSynth *synth)
{
	brightonEvent event;
	int mode = 53;

	event.value = 1;
	/*
	 * Operating modes are not configured when a memory is loaded, as that
	 * does not work correctly. After the memory is loaded, this routine is
	 * called to configure them.
	 */
	if (synth->mem.param[54] != 0)
		mode = 54;
	else if (synth->mem.param[55] != 0)
		mode = 55;

	brightonParamChange(synth->win, synth->panel, mode, &event);
}

int
ploadMemory(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	loadMemory(synth, "mono", 0, location, synth->mem.active, FIRST_DEV, 0);
	fixModes(synth);
	return(0);
}

static int
pmidiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, "mono", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			fixModes(synth);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
polyModCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

/*	printf("polyModCallback(%x, %i, %i, %f)\n", synth, panel, index, value); */

	/*
	 * If this is controller 0 it is the frequency control, otherwise a 
	 * generic controller 1.
	 */
	if (index == 0)
		bristolMidiControl(global.controlfd, synth->midichannel,
			0, 2, ((int) (value * C_RANGE_MIN_1)) >> 7);
	else {
		bristolMidiControl(global.controlfd, synth->midichannel,
			0, 1, ((int) (value * C_RANGE_MIN_1)) >> 7);
	}
	return(0);
}

static void
polyMG2(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	bristolMidiSendMsg(fd, chan, 126, 26, v);
}

static int
polyMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
polyTranspose(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	switch (v)
	{
		case 0:
			synth->transpose = 36;
			break;
		case 1:
			synth->transpose = 48;
			break;
		case 2:
			synth->transpose = 60;
			break;
	}
}

static void
polyWaveform(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int offset;

	/*
	 * 0 = tri, 1 = ramp, 2 and 3 are PW
	 */
	if (c == 34)
	{
		/*
		 * Pulse width of all osc.
		 */
		bristolMidiSendMsg(fd, chan, 0, 0, v);
		bristolMidiSendMsg(fd, chan, 1, 0, v);
		bristolMidiSendMsg(fd, chan, 2, 0, v);
		bristolMidiSendMsg(fd, chan, 8, 0, v);
		return;
	}

	if ((offset = 22 + c) > 24)
		offset = 25;

	switch(v) {
		case 0:
			/*
			 * Ramp off, square off, tri on
			 */
			bristolMidiSendMsg(fd, chan, c, 4, 0);
			bristolMidiSendMsg(fd, chan, c, 6, 0);
			bristolMidiSendMsg(fd, chan, c, 5, 1);

			bristolMidiSendMsg(fd, chan, 126, offset, 0);
			break;
		case 1:
			bristolMidiSendMsg(fd, chan, c, 5, 0);
			bristolMidiSendMsg(fd, chan, c, 6, 0);
			bristolMidiSendMsg(fd, chan, c, 4, 1);

			bristolMidiSendMsg(fd, chan, 126, offset, 0);
			break;
		case 2:
			bristolMidiSendMsg(fd, chan, c, 4, 0);
			bristolMidiSendMsg(fd, chan, c, 5, 0);
			bristolMidiSendMsg(fd, chan, c, 6, 1);

			bristolMidiSendMsg(fd, chan, 126, offset, 1);
			break;
		case 3:
			bristolMidiSendMsg(fd, chan, c, 4, 0);
			bristolMidiSendMsg(fd, chan, c, 5, 0);
			bristolMidiSendMsg(fd, chan, c, 6, 1);

			bristolMidiSendMsg(fd, chan, 126, offset, 0);
			break;
	}
}

static int
polyMode(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (c == 52)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, o, v);
		return(0);
	}

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

	bristolMidiSendMsg(global.controlfd, synth->sid, 126, o, v);

	return(0);
}

static void
polyMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

/*	printf("	polyMemory(B: %i L %i: %i, %i)\n", */
/*		synth->bank, synth->location, c, o); */

	switch(c) {
		case 0:
			saveMemory(synth, "mono", 0, synth->bank + synth->location,
				FIRST_DEV);
			return;
			break;
		case 15:
			loadMemory(synth, "mono", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			fixModes(synth);
			return;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
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
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
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
			synth->location = c - 7;
			break;
	}
}

static void
polyEffect(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	/*
	 * If we are turning this on the the original would also force MONO mode
	 * so we should do the same.
	 */
	if (v != 0)
	{
		event.value = 1;
		brightonParamChange(synth->win, synth->panel, 53, &event);
	}
	bristolMidiSendMsg(fd, chan, c, o, v);
}

static void
polyMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

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
		/*
#warning if we do not check for ack then socket might hang on exit
		 * To overcome that we should consider checking a sequence number in
		 * the message library? That is non trivial since it requires that
		 * our midi messges have a 'ack' flag included - we cannot check for
		 * ack here (actually, we could, and in the app is probably the right
		 * place to do it rather than the lib however both would have to be
		 * changed to suppor this - nc).
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
polyCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*printf("polyCallback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (polyApp.resources[0].devlocn[index].to == 1.0)
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
		printf("dispatch[%x,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#endif

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
polyInit(brightonWindow *win)
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

	if ((initmem = synth->location) == 0)
		initmem = 11;

	printf("Initialise the poly link to bristol: %p\n", synth->win);

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
		synth->voices = 1;
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);
	}

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		synth->dispatch[i].routine = polyMidiSendMsg;
	}

	/* Osc-1 Waveform is a compound operator */
	dispatch[0].controller = 0;
	dispatch[0].operator = 0;
	dispatch[0].routine = (synthRoutine) polyWaveform;
	dispatch[1].controller = 0;
	dispatch[1].operator = 1;
	dispatch[2].controller = 126;
	dispatch[2].operator = 30;
	/* Osc-2 Waveform is a compound operator */
	dispatch[3].controller = 1;
	dispatch[3].operator = 10;
	dispatch[4].controller = 1;
	dispatch[4].operator = 0;
	dispatch[4].routine = (synthRoutine) polyWaveform;
	dispatch[5].controller = 1;
	dispatch[5].operator = 1;
	dispatch[6].controller = 126;
	dispatch[6].operator = 31;
	/* Osc-3 Waveform is a compound operator */
	dispatch[7].controller = 2;
	dispatch[7].operator = 10;
	dispatch[8].controller = 2;
	dispatch[8].operator = 0;
	dispatch[8].routine = (synthRoutine) polyWaveform;
	dispatch[9].controller = 2;
	dispatch[9].operator = 1;
	dispatch[10].controller = 126;
	dispatch[10].operator = 32;
	/* Osc-4 Waveform is a compound operator and different operator.... */
	dispatch[11].controller = 8;
	dispatch[11].operator = 10;
	dispatch[12].controller = 8;
	dispatch[12].operator = 0;
	dispatch[12].routine = (synthRoutine) polyWaveform;
	dispatch[13].controller = 8;
	dispatch[13].operator = 1;
	dispatch[14].controller = 126;
	dispatch[14].operator = 33;

	/* Filter */
	dispatch[15].controller = 4;
	dispatch[15].operator = 0;
	dispatch[16].controller = 4;
	dispatch[16].operator = 1;
	dispatch[17].controller = 4;
	dispatch[17].operator = 2;
	dispatch[18].controller = 4;
	dispatch[18].operator = 3;
	/* Filter Env */
	dispatch[19].controller = 3;
	dispatch[19].operator = 0;
	dispatch[20].controller = 3;
	dispatch[20].operator = 1;
	dispatch[21].controller = 3;
	dispatch[21].operator = 2;
	dispatch[22].controller = 3;
	dispatch[22].operator = 3;
	/* Env */
	dispatch[23].controller = 5;
	dispatch[23].operator = 0;
	dispatch[24].controller = 5;
	dispatch[24].operator = 1;
	dispatch[25].controller = 5;
	dispatch[25].operator = 2;
	dispatch[26].controller = 5;
	dispatch[26].operator = 3;
	/* Env gain */
	dispatch[27].controller = 5;
	dispatch[27].operator = 4;
	/* This switch reassigned to arpeggiator direction, was for headphone output */
	dispatch[28].controller = 126;
	dispatch[28].operator = 28;
	/* LFO frequencies */
	dispatch[29].controller = 9;
	dispatch[29].operator = 0;
	dispatch[30].controller = 126;
	dispatch[30].operator = 3;
	dispatch[31].controller = 10;
	dispatch[31].operator = 0;
	/* Needs a dispatcher since it has dual operation - arpeg and LFO2 */
	dispatch[31].routine = (synthRoutine) polyMG2;
	/* PWM */
	dispatch[32].controller = 126;
	dispatch[32].operator = 4;
	dispatch[33].controller = 126;
	dispatch[33].operator = 5;
	/* PW */
	dispatch[34].controller = 34;
	dispatch[34].operator = 34;
	dispatch[34].routine = (synthRoutine) polyWaveform;

	/* Modulators 35 though 39 NOT DONE */
	dispatch[35].controller = 126;
	dispatch[35].operator = 6;
	dispatch[36].controller = 126;
	dispatch[36].operator = 7;
	dispatch[37].controller = 126;
	dispatch[37].operator = 8;
	dispatch[38].controller = 126;
	dispatch[38].operator = 9;
	dispatch[39].controller = 126;
	dispatch[39].operator = 10;
/*	dispatch[35].routine = dispatch[36].routine = dispatch[37].routine = */
/*	dispatch[38].routine = dispatch[39].routine = (synthRoutine) polyEffect; */

	/* Glide */
	dispatch[40].controller = 126;
	dispatch[40].operator = 0;

	/* Detune. */
	dispatch[41].controller = 126;
	dispatch[41].operator = 11;

	/* Transpose */
	dispatch[42].controller = 0;
	dispatch[42].operator = 0;
	dispatch[42].routine = (synthRoutine) polyTranspose;

	/* Noise level */
	dispatch[43].controller = 7;
	dispatch[43].operator = 0;

	/* Trigger */
	dispatch[44].controller = 126;
	dispatch[44].operator = 12;
	/* Damp */
	dispatch[45].controller = 126;
	dispatch[45].operator = 13;

	/* Effects NOT DONE */
	dispatch[46].controller = 126;
	dispatch[46].operator = 14;
	dispatch[46].routine = (synthRoutine) polyEffect;

	/* Global tune */
	dispatch[47].controller = 126;
	dispatch[47].operator = 2;

	/* MG1 */
	dispatch[48].controller = 126;
	dispatch[48].operator = 15;
	dispatch[49].controller = 126;
	dispatch[49].operator = 16;
	/* MG2 */
	dispatch[50].controller = 126;
	dispatch[50].operator = 17;
	dispatch[51].controller = 126;
	dispatch[51].operator = 18;

	/*
	 * Hold
	 *
	 * Mono - two button
	 *
	 * Poly - two buttons
	 *
     * Propose
	 *	Mono - fat synth = UNISON
	 *	Poly - 4 voice thin synth
	 *	Share:
	 *		One key all voices (fat synth)
	 *		Key keys two notes each (2VCO synth)
	 *		Three or more, one note each (thin synth).
	 */
	dispatch[52].controller = 52;
	dispatch[52].operator = 27;
	dispatch[53].controller = 53;
	dispatch[53].operator = 19;
	dispatch[54].controller = 54; /* Does not do anything = Unison off */
	dispatch[54].operator = 20;
	dispatch[55].controller = 55;
	dispatch[55].operator = 21;
	dispatch[52].routine =
		dispatch[53].routine =
		dispatch[54].routine =
		dispatch[55].routine = (synthRoutine) polyMode;

	dispatch[56].controller = 0;
	dispatch[57].controller = 1;
	dispatch[58].controller = 2;
	dispatch[59].controller = 3;
	dispatch[60].controller = 4;
	dispatch[61].controller = 5;
	dispatch[62].controller = 6;
	dispatch[63].controller = 7;
	dispatch[64].controller = 8;
	dispatch[65].controller = 9;
	dispatch[66].controller = 10;
	dispatch[67].controller = 11;
	dispatch[68].controller = 12;
	dispatch[69].controller = 13;
	dispatch[70].controller = 14;
	dispatch[71].controller = 15;

	dispatch[56].routine =
		dispatch[57].routine =
		dispatch[58].routine =
		dispatch[59].routine =
		dispatch[60].routine =
		dispatch[61].routine =
		dispatch[62].routine =
		dispatch[63].routine =
		dispatch[64].routine =
		dispatch[65].routine =
		dispatch[66].routine =
		dispatch[67].routine =
		dispatch[68].routine =
		dispatch[69].routine =
		dispatch[70].routine =
		dispatch[71].routine
			= (synthRoutine) polyMemory;

	dispatch[RADIOSET_1].other1 = -1;
	dispatch[RADIOSET_2].other1 = -1;
	dispatch[RADIOSET_3].other1 = -1;

	dispatch[72].controller = 1;
	dispatch[73].controller = 2;
	dispatch[72].routine = dispatch[73].routine =
		(synthRoutine) polyMidi;

	/* Tune osc-1
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 10, 8192);
	*/
	/*
	 * We put all the osc on SYNC/MAXgain, but manipulate the sync buffer in the
	 * engine.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 7, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 7, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 7, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 7, 8192);

	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 4);

	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 3, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 3, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 3, C_RANGE_MIN_1);

	/* Gain on filter contour */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, C_RANGE_MIN_1);
	/* LFO Options */
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 2, 8192);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
polyConfigure(brightonWindow* win)
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

	event.value = 1;
	brightonParamChange(synth->win, synth->panel, MEM_START + 1, &event);
	brightonParamChange(synth->win, synth->panel, MEM_START + 8, &event);

	loadMemory(synth, "mono", 0, initmem, synth->mem.active, FIRST_DEV, 0);
	fixModes(synth);

	brightonPut(win,
		"bitmaps/blueprints/monoshade.xpm", 0, 0, win->width, win->height);
	width = win->width;

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);

	/*
	 * Touch a key on/off
	 */
	bristolMidiSendMsg(global.controlfd, synth->midichannel,
		BRISTOL_EVENT_KEYON, 0, 10 + synth->transpose);
	bristolMidiSendMsg(global.controlfd, synth->midichannel,
		BRISTOL_EVENT_KEYOFF, 0, 10 + synth->transpose);
	configureGlobals(synth);

	synth->loadMemory = (loadRoutine) ploadMemory;

	return(0);
}

