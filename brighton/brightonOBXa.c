
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
#include "bristol.h"
#include "bristolmidi.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int initmem;

extern void *brightonmalloc();

static int splitPoint = -1, width = 0;

static int obxaInit();
static int obxaConfigure();
static int obxaCallback(brightonWindow *, int, int, float);
static int obxaModCallback(brightonWindow *, int, int, float);
static int obxaKeyCallback(brightonWindow *, int, int, float);
static void obxaCopyLayer(guiSynth *);
static int obxaMidiCallback(brightonWindow *, int, int, float);
static void obxaSetMode(guiSynth *);

static int seqLearn = 0, chordLearn = 0;

extern guimain global;
/*
 * These have to be integrated into the synth private structures, since we may
 * have multiple invocations of any given synth.
 */
static guimain manual;

#include "brightonKeys.h"

#define FIRST_DEV 0
#define DEVICE_COUNT 80
#define ACTIVE_DEVS 45
#define OBX_DEVS 50 /* Have to account for the manual options. */
#define MEM_START (OBX_DEVS)
#define MIDI_START (MEM_START + 18)
#define MOD_START (DEVICE_COUNT + ACTIVE_DEVS)
#define RADIOSET_1 MEM_START
#define RADIOSET_2 (MEM_START + 1)
#define RADIOSET_3 (MEM_START + 2)
/* Transpose radio set */
#define RADIOSET_4 (MOD_START + 10)

#define FUNCTION 71
#define HOLD_KEY 48
#define SEQ_MODE 72
#define SEQ_SEL (72 + 3)
#define OCTAVE_MODE (72 + 4)

#define LAYER_SWITCH 70

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
 * This example is for a obxaBristol type synth interface.
 */
#define S1 110
#define S2 17
#define S3 70

#define BO 10
#define B2 6

#define R1 200
#define R2 440
#define R2a 315
#define R3 625
#define R3b 600
#define R3c 470
#define R4 805

#define C1 30
#define C2 56
#define C3 100

#define C4 175

#define C5 242
#define C6 300
#define C7 354

#define C8 415
#define C9 475
#define C10 535

#define C11 590
#define C12 650
#define C13 710

#define C11b 750
#define C12b 800
#define C13b 850

#define C14 775
#define C15 825
#define C16 875
#define C17 925

/* memories */
#define MD 18
#define C20 (C10 + MD + MD/2)
#define C21 (C20 + MD)
#define C22 (C21 + MD)
#define C23 (C22 + MD)
#define C24 (C23 + MD)
#define C25 (C24 + MD)
#define C26 (C25 + MD)
#define C27 (C26 + MD)

#define C28 (C27 + MD + MD/2)
#define C29 (C28 + MD)
#define C30 (C29 + MD)
#define C31 (C30 + MD)
#define C32 (C31 + MD)
#define C33 (C32 + MD)
#define C34 (C33 + MD)
#define C35 (C34 + MD)

#define C36 (C35 + MD + MD/2)

#define C1a (29)
#define C2a (C1a + 29)
#define C3a (C2a + 29)
#define C4a (C3a + 29)

static brightonLocations locations[DEVICE_COUNT] = {
/* 0 Control */
	{"Glide", 0, C4, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Unison", 2, C4 + B2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc2-Detune", 0, C4, R3b, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
/* 3 Modulation */
	{"LFO-Rate", 0, C5, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-Sine", 2, C5 + B2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"LFO-Square", 2, C5 + B2, R3, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"LFO-S/H", 2, C5 + B2, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"Mod-FM-Gain", 0, C6, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Mod-FM-Osc1", 2, C6 + B2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Mod-FM-Osc2", 2, C6 + B2, R3, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Mod-FM-Filt", 2, C6 + B2, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"Mod-PWM-Gain", 0, C7, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Mod-PWM-Osc1", 2, C7 + B2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Mod-PWM-Osc2", 2, C7 + B2, R3, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* 14 Oscillators */
	{"Osc1-Transpose", 0, C8, R1, S1, S1, 0, 5, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Osc-PW-CLI!", 0, C9, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Osc2-Frequency", 0, C10, R1, S1, S1, 0, 47, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Osc1-Saw", 2, C8 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc1-Pulse", 2, C8 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc1-Env", 2, C9 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc1-2-Sync", 2, C9 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc2-Saw", 2, C10 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc2-Pulse", 2, C10 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* 23 Filter */
	{"VCF-12/24", 2, C13 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"VCF-Cutoff", 0, C11, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Resonance", 0, C12, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Mod", 0, C13, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Mix-Osc1", 2, C11 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"VCF-KBD", 2, C11 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"VCF-Mix-O1-1", 2, C12 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"VCF-Mix-O1-2", 2, C12 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"VCF-Mix-Noise", 2, C13 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* 32 Envelopes */
	{"VCF-Attack", 0, C14, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Decay", 0, C15, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Sustain", 0, C16, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Release", 0, C17, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"VCA-Attack", 0, C14, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCA-Decay", 0, C15, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCA-Sustain", 0, C16, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCA-Release", 0, C17, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
/* 40 Manual - vol, reset, balance */
	/* According to the original, these are not saved. Volume/balance will be. */
	{"MasterVolume", 0, C1, R1 - 100, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Reset", 2, C3 + BO, R2a, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	/* This is the Tremelo modifier */
	{"LFO-Mod-VCA", 2, C7 + B2, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Balance", 0, C3, R1 - 100, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* Master tune */
	{"Master-Tune", 0, C1, R3c, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
/* 45 Poly, Split, Layer */
	{"Mode-Poly", 2, C8 - 8, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Mode-Split", 2, C8 + 22, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Mode-Layer", 2, C9 - 8, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* 48 Hold/Auto - Up to now was 46 active devs */
	{"Hold", 2, C2 + BO * 3/2, R2a, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	/* AutoTune */
	{"", 2, C1, R2a, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
/* 50 Programmer */
	{"", 2, C10, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, C20, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C21, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C22, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C23, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C24, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C25, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C26, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C27, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* 59 */
	{"", 2, C28, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C29, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C30, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C31, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C32, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C33, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C34, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C35, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"", 2, C36, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, C17 - 10, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C17 + MD, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	{"LayerSelect", 2, C9 + 20, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Sequence and stuff - Fn, U, D, Hold - 71 */
	{"", 2, C1a, R4 - 110, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C2a, R4 - 110, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C3a, R4 - 110, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C4a, R4 - 110, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* SEQ and Octaves */
	{"", 2, C1a, R4 + 00, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C2a, R4 + 00, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C3a, R4 + 00, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C4a, R4 + 00, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"", 0, C3, R3c, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
};

#define OB_MODCOUNT 14
static brightonLocations obxaMods[OB_MODCOUNT] = {
	{"", 0, 70, 100, 180, 180, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 2, 525, 100, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 380, 100, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 0, 760, 100, 180, 180, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 1, 385, 280, 100, 450, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, BRIGHTON_CENTER},
	{"", 1, 520, 280, 100, 450, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	{"", 2, 25, 750, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 170, 750, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"", 2, 380, 750, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 525, 750, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"", 2, 715, 750, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 860, 750, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Multi LFO */
	{"", 2, 115, 465, 110, 130, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	/* Layer copy functions */
	{"", 2, 800, 465, 110, 130, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp obxaApp = {
	"obxa",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH, /*flags */
	obxaInit,
	obxaConfigure, /* 3 callbacks, unused? */
	obxaMidiCallback,
	destroySynth,
	{10, 100, 2, 2, 5, 520, 55, 0},
	770, 350, 0, 0,
	6, /* panel count */
	{
		{
			"OBXaPanel",
			"bitmaps/blueprints/obxa.xpm",
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			obxaCallback,
			15, 130, 970, 520,
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
			obxaKeyCallback,
			155, 690, 854, 290,
			KEY_COUNT,
			keys
		},
		{
			"OBXaMods",
			"bitmaps/blueprints/obxamod.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0,
			0,
			0,
			obxaModCallback,
			17, 690, 136, 290,
			OB_MODCOUNT,
			obxaMods
		},
		{
			"Logo",
			"bitmaps/blueprints/obxalogo.xpm",
			0,
			BRIGHTON_STRETCH, /*flags */
			0,
			0,
			0,
			15, 0, 970, 130,
			0,
			0
		},

		{
			"Wood",
			0,
			"bitmaps/textures/wood4.xpm",
			0, /* flags */
			0,
			0,
			0,
			0, 0, 15, 1000,
			0,
			0
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood4.xpm",
			0, /*flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		},
	}
};

static void
obxaPanelSwitch(guiSynth *synth, int dummy, int chan, int c, int o, int v)
{
	brightonEvent event;
	int i;

/*printf("obxaPanelSwitch(%x, %i, %i, %i, %i, %1.0f)\n", */
/*synth, fd, chan, c, o, global.synths->mem.param[LAYER_SWITCH]); */

	/*
	 * Prevent param changes when we are switching panels.
	 */
	synth->flags |= SUPPRESS;

	/*
	 * Go through all the active devices, and force the GUI to represent
	 * the new value.
	 */
	if (synth->mem.param[LAYER_SWITCH] == 0)
	{
		/*
		 * Go through all the params, and request an update to the GUI
		 */
		for (i = 0; i < ACTIVE_DEVS; i++)
		{
			event.type = BRIGHTON_FLOAT;
			event.value = synth->mem.param[i] + 1;
			brightonParamChange(synth->win, 0, i, &event);
			event.value = synth->mem.param[i];
			brightonParamChange(synth->win, 0, i, &event);
		}
	} else {
		for (i = 0; i < ACTIVE_DEVS; i++)
		{
			event.type = BRIGHTON_FLOAT;
			/*
			event.value = ((guiSynth *) synth->second)->mem.param[i] + 1;
			brightonParamChange(synth->win, 0, i, &event);
			event.value = ((guiSynth *) synth->second)->mem.param[i];
			brightonParamChange(synth->win, 0, i, &event);
			 */
			event.value = synth->mem.param[DEVICE_COUNT + i] + 1;
			brightonParamChange(synth->win, 0, i, &event);
			event.value = synth->mem.param[DEVICE_COUNT + i];
			brightonParamChange(synth->win, 0, i, &event);
		}
	}

	synth->flags &= ~SUPPRESS;
}

/*
 * We need our own load and save routines as these are non standard memories.
 * Am going to use the normal 'loadMemory' to set most of the parameters for 
 * the second layer, then explicit read/write the second layer.
 */
static void
obxaLoadMemory(guiSynth *synth, char *name, char *d,
int loc, int o, int x, int y)
{
	char path[256];
	int fd, i, pSel;
	brightonEvent event;
	float pw[4];

	/*
	 * Memory loading should be a few phases. Firstly see which layer is visible
	 * and if it is not the first then display the first panel and load the mem.
	 * After that display the upper panel and load the next set of memories.
	 * Finally return to the original panel.
	 */
	if ((pSel = synth->mem.param[LAYER_SWITCH]) != 0)
	{
		synth->mem.param[LAYER_SWITCH] = 0;
		/*
		 * We should consider actually sending the button change action
		 */
		obxaPanelSwitch(synth, global.controlfd, synth->midichannel, 0, 0, 0);
	}

	/*
	 * We cannot really use this due to the added complexity of the dual PW
	 * controller as it removes any relationship between the value of the dial
	 * and what it controls. We can do it with 'NOCALLS' though.
	 */
	loadMemory(synth, name, d, loc, o, x, BRISTOL_NOCALLS);

	/*
	 * Save our PW
	 */
	pw[0] = synth->mem.param[15];

	event.type = BRIGHTON_FLOAT;
	for (i = 0; i < synth->mem.active; i++)
	{
		event.value = synth->mem.param[i];

		brightonParamChange(synth->win, 0, i, &event);
	}

	/*
	 * Now we need to consider the second layer.
	 */
	if ((name != 0) && (name[0] == '/'))
	{
		sprintf(synth->mem.algo, "%s", name);
		sprintf(path, "%s", name);
	} else {
		sprintf(path, "%s/memory/%s/%s%i.mem",
			getBristolCache("midicontrollermap"), name, name, loc);
		sprintf(synth->mem.algo, "%s", name);
		if (name == NULL)
			sprintf(synth->mem.name, "no name");
		else
			sprintf(synth->mem.name, "%s", name);
	}

	if ((fd = open(path, O_RDONLY, 0770)) < 0)
	{
		sprintf(path, "%s/memory/%s/%s%i.mem",
			global.home, name, name, loc);

		if ((fd = open(path, O_RDONLY, 0770)) < 0)
		{
			synth->flags &= ~MEM_LOADING;
			return;
		}
	}

	lseek(fd, DEVICE_COUNT * sizeof(float), SEEK_SET);

	if (read(fd, &synth->mem.param[DEVICE_COUNT],
		DEVICE_COUNT * sizeof(float)) < 0)
		printf("read failed for 2nd layer\n");

	close(fd);

	pw[1] = synth->mem.param[MOD_START + OB_MODCOUNT + 3];
	pw[2] = synth->mem.param[DEVICE_COUNT + 15];
	pw[3] = synth->mem.param[MOD_START + OB_MODCOUNT + 4];
	/*
	 * The parameters for the second layer are now set, but we need to set up
	 * the upper layer.
	 */
printf("loading second layer: %s\n", path);

	synth->mem.param[LAYER_SWITCH] = 1.0f;
	obxaPanelSwitch(synth, global.controlfd, synth->midichannel, 0, 0, 0);

	/*
	 * We now have to call the GUI to configure all these values. The GUI
	 * will then call us back with the parameters to send to the synth.
	 */
	event.type = BRIGHTON_FLOAT;
	for (i = 0; i < synth->mem.active; i++)
	{
		event.value = synth->mem.param[DEVICE_COUNT + i];

		brightonParamChange(synth->win, 0, i, &event);
	}

	synth->flags &= ~MEM_LOADING;

	synth->mem.param[LAYER_SWITCH] = pSel;

	obxaPanelSwitch(synth, global.controlfd, synth->midichannel, 0, 0, 0);

	/*
	 * At this point we have to have a good look at the poly/split/layer 
	 * selection, and force it to the correct value.
	 * The fix is to send an event to set the button state.....
	 */
	event.value = 1.0;
	if (synth->mem.param[MOD_START + OB_MODCOUNT + 0])
		brightonParamChange(synth->win, synth->panel, 45, &event);
	else if (synth->mem.param[MOD_START + OB_MODCOUNT + 1]) {
		brightonParamChange(synth->win, synth->panel, 46, &event);
		splitPoint = synth->mem.param[MOD_START + OB_MODCOUNT + 5];
	} else if (synth->mem.param[MOD_START + OB_MODCOUNT + 2])
		brightonParamChange(synth->win, synth->panel, 47, &event);

	/*
printf("Load %f %f %f %f\n", pw[0], pw[1], pw[2], pw[3]);
	 * We now need to force the PW of the two oscillators, both layers.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0,
		(int) (pw[0]));
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0,
		(int) (pw[1]));
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0,
		(int) (pw[2]));
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 0,
		(int) (pw[3]));

	synth->flags |= MEM_LOADING;
	/*
	 * Finally we need to look at the mods. There are two we do not want to
	 * look at, the two wheels - 4 and 5, the rest need to be set.
	 */
	for (i = 0; i < OB_MODCOUNT; i++)
	{
		if ((i == 4) || (i == 5))
			continue;

		event.value = synth->mem.param[MOD_START + i];
		brightonParamChange(synth->win, 2, i, &event);
	}
	synth->flags &= ~MEM_LOADING;

	/*
	 * Finally load and push the sequence information for this memory.
	 */
	loadSequence(&synth->seq1, "obxa", synth->bank*10 + synth->location, 0);
	fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
}

static int
obxaMidiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			obxaLoadMemory(synth, "obxa", 0, synth->bank * 10 + synth->location,
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
obxaKeyCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

/*
printf("obxaKeycallback(%x, %i, %i, %f): %i %i - %i\n", (size_t) synth, panel,
index, value, synth->transpose, global.controlfd, index + synth->transpose);
*/

	if ((seqLearn == 1) && (value != 0))
		seqInsert((arpeggiatorMemory *) synth->seq1.param,
			(int) index, synth->transpose);

	if ((chordLearn == 1) && (value != 0))
		chordInsert((arpeggiatorMemory *) synth->seq1.param,
			(int) index, synth->transpose);

	if (splitPoint == -2)
	{
printf("setting split point: %i\n", index);
		splitPoint = index;
		obxaSetMode(synth);
		synth->mem.param[MOD_START + OB_MODCOUNT + 5] = index;
		return(0);
	}

	/* Don't send MIDI messages if we are testing */
	if (global.libtest)
		return(0);

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 * Look at the Mode options. If we are layer then send two events, one for
	 * each channel. If split then check the key we have selected, and if
	 * poly send to each voice in turn.
	 *
	 * Take the tranpose out of here.
	 */
	if (value)
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
	else
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);

	return(0);
}

/*
static void
obxadestroy(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);

	bristolMidiSendMsg(manual.controlfd, synth->sid2, 127, 0,
		BRISTOL_EXIT_ALGO);
	bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
		BRISTOL_EXIT_ALGO);
}
*/

static void
obxaSeqRate(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RATE, v);
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RATE, v);
}

static int
obxaModCallback(brightonWindow *cid, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, cid);
	int sendvalue;

	/* printf("obxaModCallback(%x, %i, %i, %f)\n",
		(size_t)synth, panel, index, value); */

	/*
	 * These may eventually be saved with the memories. Location is above the
	 * second voice memories.
	 */
	synth->mem.param[MOD_START + index] = value;

	/*
	 * If this is controller 0 it is the frequency control, otherwise a 
	 * generic controller 1.
	 */
	switch (index) {
		case 0: /* Rate pot - Second LFO */
			sendvalue = value * C_RANGE_MIN_1;
			/*
			 * This is a monolithic LFO, applies to both layers via preops
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 8, 0, sendvalue);
			bristolMidiSendMsg(global.controlfd, synth->sid2, 8, 0, sendvalue);
			break;
		case 1: /* Lower - Apply mods to given layer */
			if (value != 0) {
				if (synth->mem.param[MOD_START + 6] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 25, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 25, 0);

				if (synth->mem.param[MOD_START + 7] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 27, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 27, 0);

				if (synth->mem.param[MOD_START + 8] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 29, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 29, 0);
			} else {
				bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 25, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 27, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 29, 0);
			}
			break;
		case 2: /* Upper - Apply mods to given layer */
			if (value != 0.0) {
				if (synth->mem.param[MOD_START + 6] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 25, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 25, 0);

				if (synth->mem.param[MOD_START + 7] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 27, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 27, 0);

				if (synth->mem.param[MOD_START + 8] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 29, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 29, 0);
			} else {
				bristolMidiSendMsg(global.controlfd, synth->sid, 126, 25, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid, 126, 27, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid, 126, 29, 0);
			}
			break;
		case 3: /* Depth pot - Of LFO, affected by mod wheel */
			if (synth->mem.param[MOD_START + 9] == 0)
				sendvalue = value * C_RANGE_MIN_1;
			else
				sendvalue = C_RANGE_MIN_1 / 2 +
					(int) (((value - 0.5) / 3) * C_RANGE_MIN_1);

			/*
			 * This is a monolithic LFO, applies to both layers via preops
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid,
					126, 26, sendvalue);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
					126, 26, sendvalue);
			break;
		case 4: /* Freq wheel */
			if (synth->mem.param[MOD_START + 9] == 0)
				sendvalue = value * C_RANGE_MIN_1;
			else
				sendvalue = C_RANGE_MIN_1 / 2 +
					(int) (((value - 0.5) / 3) * C_RANGE_MIN_1);
			/* Check on Layer switch and Osc settings */
			if (synth->mem.param[MOD_START + 1] != 0)
				bristolMidiSendMsg(global.controlfd, synth->midichannel,
					BRISTOL_EVENT_PITCH, 0, sendvalue);
			if (synth->mem.param[MOD_START + 2] != 0)
				bristolMidiSendMsg(global.controlfd, synth->midichannel + 1,
					BRISTOL_EVENT_PITCH, 0, sendvalue);
			break;
		case 5: /* Mod wheel */
			if (synth->mem.param[MOD_START + 9] == 0)
				sendvalue = value * C_RANGE_MIN_1;
			else
				sendvalue = value * C_RANGE_MIN_1 / 4;

			/* Check on Layer switches */
			bristolMidiControl(global.controlfd, synth->midichannel,
				0, 1, ((int) (value * C_RANGE_MIN_1)) >> 7);
			bristolMidiControl(global.controlfd, synth->midichannel + 1,
				0, 1, ((int) (value * C_RANGE_MIN_1)) >> 7);
			break;
		case 6: /* Mod O1 - Apply Mod to Osc 1 */
			if (value != 0) {
				if (synth->mem.param[MOD_START + 1] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 25, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 25, 0);
			} else {
				bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 25, 0);
			}

			if (value != 0) {
				if (synth->mem.param[MOD_START + 2] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 25, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 25, 0);
			} else {
				bristolMidiSendMsg(global.controlfd, synth->sid, 126, 25, 0);
			}
			break;
		case 7: /* Mod O2 - Apply Mod to Osc 2 */
			if (value != 0) {
				if (synth->mem.param[MOD_START + 1] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 27, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 27, 0);
			} else {
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 27, 0);
			}

			if (value != 0) {
				if (synth->mem.param[MOD_START + 2] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 27, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 27, 0);
			} else {
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 27, 0);
			}
			break;
		case 8: /* MOD to PWM */
			if (value != 0) {
				if (synth->mem.param[MOD_START + 1] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 29, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 29, 0);
			} else {
					bristolMidiSendMsg(global.controlfd, synth->sid2,
						126, 29, 0);
			}

			if (value != 0) {
				if (synth->mem.param[MOD_START + 2] != 0.0)
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 29, 1);
				else
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 29, 0);
			} else {
					bristolMidiSendMsg(global.controlfd, synth->sid,
						126, 29, 0);
			}
			break;
		case 9: /* Ammount - Wide Narrow mods */
			/* Nothing to do - just a flag that affects swing of wheels. */
			/* That is not wise, it should really go to the engine */
			break;
		case 10: /* Transpose down - Radio buttons */
			/*
			 * These are 3 way radio buttons, which is a bit of a bummer. We
			 * should consider making it layer sensitive however there is
			 * already sufficient transpose in the oscillators.
			 */
			if (value == 0) {
				if (synth->mem.param[MOD_START + 11] == 0)
					/*
					 * Default transpose
					synth->transpose = 36;
					 */
					bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
						BRISTOL_TRANSPOSE);
					bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
						BRISTOL_TRANSPOSE);
			} else {
				if (synth->mem.param[MOD_START + 11] != 0)
				{
					brightonEvent event;

					event.type = BRIGHTON_FLOAT;
					event.value = 0;
					brightonParamChange(synth->win, 2, 11, &event);
				}
				bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
					BRISTOL_TRANSPOSE - 12);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
					BRISTOL_TRANSPOSE - 12);
			}
			break;
		case 11: /* Transpose up - Radio buttons */
			if (value == 0) {
				if (synth->mem.param[MOD_START + 10] == 0)
					/* Default transpose */
					bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
						BRISTOL_TRANSPOSE);
					bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
						BRISTOL_TRANSPOSE);
			} else {
				if (synth->mem.param[MOD_START + 10] != 0)
				{
					brightonEvent event;

					event.type = BRIGHTON_FLOAT;
					event.value = 0;
					brightonParamChange(synth->win, 2, 10, &event);
				}
				bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
					BRISTOL_TRANSPOSE + 12);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
					BRISTOL_TRANSPOSE + 12);
			}
			break;
		case 12: /* MULTI MOD LFO - gives MOD LFO per voice rather than global. */
			if (synth->mem.param[MOD_START + 12] != 0)
			{
				bristolMidiSendMsg(global.controlfd, synth->sid, 126, 22, 1);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 22, 1);
				/*
				 * Configure for sync to NOTE_ON, improves variance.
				 */
				bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, 1);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 8, 1, 1);
			} else {
				bristolMidiSendMsg(global.controlfd, synth->sid, 126, 22, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 22, 0);
				/*
				 * Configure for sync to never
				 */
				bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, 0);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 8, 1, 0);
			}
			break;
		case 13: /* Copy lower layer parameters to upper layer */
			if ((synth->flags & MEM_LOADING) == 0)
				obxaCopyLayer(synth);
			break;
	}
	return(0);
}

static void
obxaSetMode(guiSynth *synth)
{
	int selection = synth->mem.param[45];

	if (selection == 0)
	{
		if (synth->mem.param[46] == 0)
			selection = 3;
		else
			selection = 2;
	}

	switch (selection) {
		case 1:
			/* Poly */
			printf("configure poly\n");
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 30, 1);
			bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
				BRISTOL_HIGHKEY|127);
			bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
				BRISTOL_LOWKEY|127);
			break;
		case 2:
			/* Split */
			printf("configure split %i\n", splitPoint);
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 30, 0);
			if (splitPoint > 0) {
				bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
					BRISTOL_HIGHKEY|splitPoint);
				bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
					BRISTOL_LOWKEY|(splitPoint + 1));
			}
			break;
		case 3:
			/* Layer */
			printf("configure layer\n");
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 30, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
				BRISTOL_HIGHKEY|127);
			bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
				BRISTOL_LOWKEY|0);
			break;
	}
}

static void
obxaMode(guiSynth *synth, int c, int o, int value)
{
	brightonEvent event;

	if (synth->flags & MEM_LOADING)
		return;

	synth->mem.param[c] = value;

	/*
	 * Poly, split and layer radio buttons.
	 */
	if (synth->dispatch[RADIOSET_3].other2)
	{
		synth->dispatch[RADIOSET_3].other2 = 0;
		return;
	}

	//printf("obxaMode(%i, %i, %i)\n", c, o, value);

	if (synth->dispatch[RADIOSET_3].other1 != -1)
	{
		event.type = BRIGHTON_FLOAT;

		synth->dispatch[RADIOSET_3].other2 = 1;

		if (synth->dispatch[RADIOSET_3].other1 != c)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[RADIOSET_3].other1, &event);
	}
	synth->dispatch[RADIOSET_3].other1 = c;

	/* Bury this here as it has to go somewhere */
	if (synth->win->width != width)
		brightonPut(synth->win, "bitmaps/blueprints/obxashade.xpm",
			0, 0, width = synth->win->width, synth->win->height);

	if (global.libtest)
		return;

	/*
	 * If Poly then set upper split to 127-127, lower to 0-127, raise voices
	 * on lower layer.
	 * Dual set both splits to 0-127, lower voices on lower layer.
	 * Layer then set upper and lower splits, lower voices on lower layer.
	 */
	if (value == 0)
		return;

	obxaSetMode(synth);

	return;
}

static int pwSelect = 0;
/*
 * We need to see if for any given oscillator none of the wave buttons are
 * selected. If this is the case then select triangle. Else disable tri and
 * configure the selected values.
 */
static void
obxaWaveform(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * See if this is the PW parameter.
	 */
	if (c == 15)
	{
		if (synth->flags & MEM_LOADING)
			return;
		if (pwSelect == 0) {
			if (synth->mem.param[LAYER_SWITCH] == 0)
				synth->mem.param[15] = v;
			else
				synth->mem.param[DEVICE_COUNT + 15] = v;
			bristolMidiSendMsg(fd, chan, 0, 0, v);
		} else {
			synth->mem.param[MOD_START + OB_MODCOUNT + 3
				+ (int) synth->mem.param[LAYER_SWITCH]] = v;
			bristolMidiSendMsg(fd, chan, 1, 0, v);
		}
		return;
	}

	/*
	 * See if we need to set PW selector
	 */
	if (o == 6)
	{
		/*
		 * If the sqr is being selected, then PW will modify this oscillator,
		 * and if going off then it will cotrol the other
		 */
		if (v != 0)
			pwSelect = c;
		else
			pwSelect = 1 - c;
	}

	/*
	 * If the value is '1' we can just set the desired wave and disable tri.
	 */
	if (v != 0)
	{
		bristolMidiSendMsg(fd, chan, c, o, v);
		bristolMidiSendMsg(fd, chan, c, 5, 0);
	} else {
		int layer = 0;
		
		if (synth->mem.param[LAYER_SWITCH] != 0)
			layer = DEVICE_COUNT;
		bristolMidiSendMsg(fd, chan, c, o, v);
		if (c == 0)
		{
			if ((synth->mem.param[layer + 17] == 0) &&
				(synth->mem.param[layer + 18] == 0))
				bristolMidiSendMsg(fd, chan, c, 5, 1);
		} else {
			if ((synth->mem.param[layer + 21] == 0) &&
				(synth->mem.param[layer + 22] == 0))
				bristolMidiSendMsg(fd, chan, c, 5, 1);
		}
	}
}

static int dreset = 0;
static int d1 = 0;
static int d2 = 0;

/*
 * If controller #41 has value zero then we configure the release rates, else
 * we use the shortest release rate.
 */
static void
obxaDecay(void *synth, int fd, int chan, int c, int o, int v)
{
	if (c == 41)
	{
		if (v == 0)
		{
			/*
			 * Set values to their current settings
			 */
			dreset = 0;
			bristolMidiSendMsg(fd, chan, 3, 3, d1);
			bristolMidiSendMsg(fd, chan, 5, 3, d2);
		} else {
			/*
			 * Set values to minimum release time
			 */
			dreset = 1;
			bristolMidiSendMsg(fd, chan, 3, 3, 20);
			bristolMidiSendMsg(fd, chan, 5, 3, 20);
		}
	} else {
		if (dreset == 0)
			bristolMidiSendMsg(fd, chan, c, o, v);

		if (c == 3)
			d1 = v;
		else
			d2 = v;
	}
}

static int
obxaMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
multiTune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int layer = global.synths->mem.param[LAYER_SWITCH];

/*	if (synth->flags & MEM_LOADING) */
/*		return; */

	/*
	 * Configures all the tune settings to zero (ie, 0.5).
	 */

	event.type = BRIGHTON_FLOAT;
	event.value = 0.5;

	global.synths->mem.param[LAYER_SWITCH] = 0;
	brightonParamChange(synth->win, synth->panel, FIRST_DEV + 2,
		&event);
	brightonParamChange(synth->win, synth->panel, FIRST_DEV + 44,
		&event);

	global.synths->mem.param[LAYER_SWITCH] = 1;
	brightonParamChange(synth->win, synth->panel, FIRST_DEV + 2,
		&event);
	brightonParamChange(synth->win, synth->panel, FIRST_DEV + 44,
		&event);

	global.synths->mem.param[LAYER_SWITCH] = layer;

	global.synths->mem.param[2] = 0.5;
	global.synths->mem.param[44] = 0.5;
	global.synths->mem.param[DEVICE_COUNT + 2] = 0.5;
	global.synths->mem.param[DEVICE_COUNT + 44] = 0.5;
}

/*
 * This should take the current layer and copy to the other.
 */
static void
obxaCopyLayer(guiSynth *synth)
{
	int i, pSel;
	brightonEvent event;
	float pw[2];

printf("Copy Layer\n");

	pw[0] = synth->mem.param[15];
	pw[1] = synth->mem.param[MOD_START + OB_MODCOUNT + 3];

	bcopy(synth->mem.param, &synth->mem.param[DEVICE_COUNT],
		ACTIVE_DEVS * sizeof(float));

	/*
	 * Memory loading should be a few phases. Firstly see which layer is visible
	 * and if it is not the first then display the first panel and load the mem.
	 * After that display the upper panel and load the next set of memories.
	 * Finally return to the original panel.
	 */
	if ((pSel = synth->mem.param[LAYER_SWITCH]) == 0)
	{
		synth->mem.param[LAYER_SWITCH] = 1;
		/*
		 * We should consider actually sending the button change action
		 */
		obxaPanelSwitch(synth, global.controlfd, synth->midichannel, 0, 0, 0);
	}

	/*
	 * We now have to call the GUI to configure all these values. The GUI
	 * will then call us back with the parameters to send to the synth.
	 */
	event.type = BRIGHTON_FLOAT;
	for (i = 0; i < synth->mem.active - 2; i++)
	{
		event.value = synth->mem.param[DEVICE_COUNT + i];

		brightonParamChange(synth->win, 0, i, &event);
	}

	synth->mem.param[DEVICE_COUNT + 15] = pw[0];
	synth->mem.param[MOD_START + OB_MODCOUNT + 4] = pw[1];

	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0,
		(int) pw[0] * C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 0,
		(int) pw[1] * C_RANGE_MIN_1);

	/*
	 * Memory loading should be a few phases. Firstly see which layer is visible
	 * and if it is not the first then display the first panel and load the mem.
	 * After that display the upper panel and load the next set of memories.
	 * Finally return to the original panel.
	 */
	if ((synth->mem.param[LAYER_SWITCH] = pSel) == 0)
		/*
		 * We should consider actually sending the button change action
		 */
		obxaPanelSwitch(synth, global.controlfd, synth->midichannel, 0, 0, 0);
}

static void
obxaSaveMemory(guiSynth *synth, char *name, char *d, int loc, int o)
{
	char path[256];
	int fd;

	/*
printf("%f %f %f %f\n",
synth->mem.param[15],
synth->mem.param[MOD_START + OB_MODCOUNT + 3],
synth->mem.param[DEVICE_COUNT + 15],
synth->mem.param[MOD_START + OB_MODCOUNT + 4]);
	 * We need to save the Poly/Split/Layer, but don't really want them to be
	 * part of the other general routines, so 'force' it here. Not nice but
	 * will do for now.
	 */
	saveMemory(synth, name, d, loc, o);

	/*
	 * Just save a single sequencer, not one per memory. Can that, save lots
	 */
	saveSequence(synth, "obxa", loc, 0);

	/*
	 * Now we need to consider the second layer.
	 */
	if ((name != 0) && (name[0] == '/'))
	{
		sprintf(synth->mem.algo, "%s", name);
		sprintf(path, "%s", name);
	} else {
		sprintf(path, "%s/memory/%s/%s%i.mem",
			getBristolCache("midicontrollermap"), name, name, loc);
		sprintf(synth->mem.algo, "%s", name);
		if (name == NULL)
			sprintf(synth->mem.name, "no name");
		else
			sprintf(synth->mem.name, "%s", name);
		//printf("saving second layer\n");
	}

	if ((fd = open(path, O_WRONLY, 0770)) < 0)
	{
		sprintf(path, "%s/memory/%s/%s%i.mem",
			global.home, name, name, loc);
		if ((fd = open(path, O_WRONLY, 0770)) < 0)
		{
			return;
		}
	}

	lseek(fd, DEVICE_COUNT * sizeof(float), SEEK_SET);

	if (write(fd, &synth->mem.param[DEVICE_COUNT],
			DEVICE_COUNT * sizeof(float)) < 0)
		printf("write failed 2\n");

	close(fd);
}

static void
obxaSequence(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	printf("obxaSequence\n");

	if (synth->seq1.param == NULL) {
		loadSequence(&synth->seq1, "obxa", synth->bank*10 + synth->location, 0);
		fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
	}

	if (synth->mem.param[FUNCTION] != 0)
	{
		if  (v != 0) {
			if ((synth->flags & OPERATIONAL) == 0)
				return;
			if (synth->flags & MEM_LOADING)
				return;

			printf("Sequence learn mode requested %x\n",  synth->flags);

			seqLearn = 1;

			synth->seq1.param[BRISTOL_AM_SMAX] = 0;

			/*
			 * Stop the arpeggiator and send a learn request
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 1);
		} else {
			/*
			 * Stop the learning process
			 */
			seqLearn = 0;
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
			event.value = 0;
			brightonParamChange(synth->win, synth->panel, FUNCTION, &event);
		}

		return;
	}

	if (seqLearn == 1) {
		/*
		 * This is a button press during the learning sequence, in which case
		 * it needs to be terminated.
		 */
		seqLearn = 0;

		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
	}

	/*
	 * If value is going to zero then stop the sequencer.
	 */
	if (v == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, 0);
		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);

		return;
	}

	/*
	 * Otherwise start it.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 1);
}

static void
obxaFunction(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	/*
	 * This is not really an active button so does not really require a
	 * callback - the memory location is polled by other callbacks.
	 */
	printf("obxaFunction\n");

	if (v == 0) {
		if (synth->mem.param[HOLD_KEY] != 0) {
			event.type = BRIGHTON_FLOAT;
			event.value = 0;
			brightonParamChange(synth->win, synth->panel, HOLD_KEY, &event);
		}

		if (synth->mem.param[SEQ_MODE] != 0) {
			event.type = BRIGHTON_FLOAT;
			event.value = 0;
			brightonParamChange(synth->win, synth->panel, SEQ_MODE, &event);
		}

		if (synth->mem.param[SEQ_MODE + 1] != 0) {
			event.type = BRIGHTON_FLOAT;
			event.value = 0;
			brightonParamChange(synth->win, synth->panel, SEQ_MODE+1, &event);
		}

		if (synth->mem.param[SEQ_MODE + 2] != 0) {
			event.type = BRIGHTON_FLOAT;
			event.value = 0;
			brightonParamChange(synth->win, synth->panel, SEQ_MODE+2, &event);
		}
	}
}

static void
obxaArpOctave(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

printf("obxaArpOctave: %i %i %i\n", c, o, v);
	/*
	 * Force exclusion
	 */
	if (synth->dispatch[OCTAVE_MODE].other2)
	{
		synth->dispatch[OCTAVE_MODE].other2 = 0;
		return;
	}

	event.type = BRIGHTON_FLOAT;

	if (v != 0) 
	{
		if (synth->dispatch[OCTAVE_MODE].other1 != -1)
		{
			synth->dispatch[OCTAVE_MODE].other2 = 1;

			if (synth->dispatch[OCTAVE_MODE].other1 != c)
				event.value = 0;
			else
				event.value = 1;

			brightonParamChange(synth->win, synth->panel,
				synth->dispatch[OCTAVE_MODE].other1, &event);
		}
		synth->dispatch[OCTAVE_MODE].other1 = c;

		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RANGE, o);
		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RANGE, o);

		return;
	} else {
		if ((synth->mem.param[OCTAVE_MODE] == 0)
			&& (synth->mem.param[OCTAVE_MODE + 1] == 0)
			&& (synth->mem.param[OCTAVE_MODE + 2] == 0))
		{
			event.value = 1;

			brightonParamChange(synth->win, synth->panel, c, &event);

			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RANGE, o);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RANGE, o);
		}
	}

}

static void
obxaFilter(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * Don't change the filter type, alter the feedforward from the other 
	 * poles
	 */
	if (v != 0)
		v = 16383;

	bristolMidiSendMsg(fd, chan, 4, 4, 4);
	bristolMidiSendMsg(fd, chan, 4, 7, v);
//	bristolMidiSendMsg(fd, chan, 4, 1, synth->mem.param[25] * C_RANGE_MIN_1);
}

static void
obxaArpeggiate(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

printf("obxaArpeggiate: %i %i\n", c, v);
	/*
	 * Force exclusion
	 */
	if (synth->dispatch[SEQ_MODE].other2)
	{
		synth->dispatch[SEQ_MODE].other2 = 0;
		return;
	}

	if (synth->seq1.param == NULL) {
		loadSequence(&synth->seq1, "obxa", synth->bank*10 + synth->location, 0);
		fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
	}

	printf("obxaArpeggiate(%i, %i)\n", c, v);

	event.type = BRIGHTON_FLOAT;

	if (v != 0) 
	{
		if (synth->dispatch[SEQ_MODE].other1 != -1)
		{
			synth->dispatch[SEQ_MODE].other2 = 1;

			if (synth->dispatch[SEQ_MODE].other1 != c)
				event.value = 0;
			else
				event.value = 1;

			brightonParamChange(synth->win, synth->panel,
				synth->dispatch[SEQ_MODE].other1, &event);
		}
		synth->dispatch[SEQ_MODE].other1 = c;

		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_DIR, o);
		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_DIR, o);
		/* And enable it */
		if (synth->mem.param[SEQ_SEL] == 0)
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, 1);
		else
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 1);

		return;
	}

	/*
	 * If we are going off then disable the arpeggiator, otherwise set the
	 * mode and start it.
	 */
	if (v == 0) {
		/*
		 * OK, the value is zero. If this is the same controller going off
		 * then disable the arpeggiator, otherwise do nothing.
		 */
		if (synth->dispatch[SEQ_MODE].other1 == c) {
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, 0);
		}
		return;
	}
}

static void
obxaChord(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	printf("Chord request: %i\n", v);

	if (synth->seq1.param == NULL) {
		loadSequence(&synth->seq1, "obxa", synth->bank*10 + synth->location, 0);
		fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
	}

	event.type = BRIGHTON_FLOAT;

	if (synth->mem.param[FUNCTION] != 0)
	{
		if  (v != 0)
		{
			if ((synth->flags & OPERATIONAL) == 0)
				return;
			if (synth->flags & MEM_LOADING)
				return;

			chordLearn = 1;

			printf("Chord learn requested %x\n",  synth->flags);

			synth->seq1.param[BRISTOL_AM_CCOUNT] = 0;

			/*
			 * This is to start a re-seq of the chord.
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 1);
		} else {
			chordLearn = 0;
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);
			event.value = 0;
			brightonParamChange(synth->win, synth->panel, FUNCTION, &event);
		}

		return;
	}

	if (chordLearn == 1) {
		/*
		 * This is a button press during the learning sequence, in which case
		 * it needs to be terminated.
		 */
		chordLearn = 0;
		bristolMidiSendMsg(global.controlfd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);
	}

	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, v);
}

static void
obxaMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	switch (c) {
		case 16:
			/* Load */
			obxaLoadMemory(synth, "obxa", 0, synth->bank * 10 + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			break;
		case 17:
			/* Save */
			obxaSaveMemory(synth, "obxa", 0,
				synth->bank * 10 + synth->location, 0);
			break;
		case 0:
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

				if (synth->dispatch[RADIOSET_1].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_1].other1, &event);
			}
			synth->dispatch[RADIOSET_1].other1 = o;
			synth->bank = c + 1;
			break;
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			if (synth->dispatch[RADIOSET_2].other2)
			{
				synth->dispatch[RADIOSET_2].other2 = 0;
				return;
			}
			if (synth->dispatch[RADIOSET_2].other1 != -1)
			{
				synth->dispatch[RADIOSET_2].other2 = 1;

				if (synth->dispatch[RADIOSET_2].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_2].other1, &event);
			}
			synth->dispatch[RADIOSET_2].other1 = o;
			/*
			 * Having the minus seven looks odd, but the controllers count from
			 * zero but I want the actual memories to be from 1 to 64, this 
			 * reflects the panel blueprint c - 8 + 1 = c - 7;
			 */
			synth->location = c - 7;
			break;
	}

/*	printf("	obxaMemory(B: %i L %i: %i)\n", */
/*		synth->bank, synth->location, o); */
}

static void
obxaMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	/*
	 * For this synth we are going to try and put everything on a single 
	 * midi channel - split poly is all voices, split will tell the engine
	 * which ranges apply to which voices on this synth, layer will apply 
	 * two notes on the channel, and unison all of them. Unison should really
	 * be an engine feature to setup every voice for a given baudio structure.
	 *
	 * This works badly for the Prophet10 though, so may go for two midi 
	 * channels and decide how to intepret them.
	 */
	if (c == 2) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			return;
		}
	} else {
		if ((newchan = synth->midichannel + 1) > 15)
		{
			synth->midichannel = 15;
			return;
		}
	}

	printf("midichannel %i %i\n", c, newchan);

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid2,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
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
obxaCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue, sid = 0;

//printf("obxaCallback(%i, %i, %f): %x, %1.0f\n", panel, index, value, synth,
//global.synths->mem.param[LAYER_SWITCH]);

	if (synth->flags & SUPPRESS)
		return(0);

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (obxaApp.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	if (index == LAYER_SWITCH)
	{
		synth->mem.param[LAYER_SWITCH] = value;

		obxaPanelSwitch(synth,
			global.controlfd,
			sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

		return(0);
	}

	if (synth->mem.param[LAYER_SWITCH] == 0)
	{
		/*
		 * This is actually broken for certain cases with the dual function of
		 * the pulsewidth button.
		 */
		if (index != 15)
			synth->mem.param[index] = value;
		sid = synth->sid;
	} else {
		((guiSynth *) synth->second)->mem.param[index] = value;
		if (index != 15)
			synth->mem.param[DEVICE_COUNT + index] = value;
		sid = synth->sid2;
	}

	/* Mode switches */
	if ((index == 45) || (index == 46) || (index == 47))
	{
		if (index == 46)
		{
			if (value != 0)
				splitPoint = -2;
			else
				splitPoint = -1;
		}

		obxaMode(synth, synth->dispatch[index].controller,
			synth->dispatch[index].operator, value);

		/* Looks odd, but need to hide the parameters in spare space. */
		synth->mem.param[MOD_START + OB_MODCOUNT + index - 45] = value;
		return(0);
	}

	if ((!global.libtest) || (index >= ACTIVE_DEVS))
		synth->dispatch[index].routine(synth,
			global.controlfd,
			sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

#ifdef DEBUG
	else
		printf("dispatch[%x,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd,
			synth->sid,
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
obxaInit(brightonWindow *win)
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

	printf("Initialise the %s link to bristol: %p %p\n",
		synth->resources->name, synth, synth->win);

	synth->mem.param = (float *)
		brightonmalloc(DEVICE_COUNT * 2 * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
	synth->dispatch = (dispatcher *)
		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher));
	dispatch = synth->dispatch;

	synth->second = brightonmalloc(sizeof(guiSynth));
	bcopy(synth, ((guiSynth *) synth->second), sizeof(guiSynth));
	((guiSynth *) synth->second)->mem.param =
		(float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	((guiSynth *) synth->second)->mem.count = DEVICE_COUNT;
	((guiSynth *) synth->second)->mem.active = ACTIVE_DEVS;
	((guiSynth *) synth->second)->dispatch = synth->dispatch;

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets.
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		int v;

		if (synth->voices == BRISTOL_VOICECOUNT) {
			synth->voices = 10;
			((guiSynth *) synth->second)->voices = 5;
		} else
			((guiSynth *) synth->second)->voices = synth->voices >> 1;

		v = synth->voices;

		synth->synthtype = BRISTOL_OBX;
		bcopy(&global, &manual, sizeof(guimain));
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

		manual.flags |= BRISTOL_CONN_FORCE|BRIGHTON_NOENGINE;
		manual.port = global.port;
		manual.host = global.host;
		synth->voices = ((guiSynth *) synth->second)->voices;
		/*
		 * Just 5 voices on each layer. This should be a parameter since they
		 * had 3, 4 and 5 voices per layer originally.
		 */
		if ((synth->sid2 = initConnection(&manual, synth)) < 0)
			return(-1);

		global.manualfd = manual.controlfd;
		global.manual = &manual;
		manual.manual = &global;

		synth->voices = v;
	} else {
		synth->sid = 5;
		synth->sid2 = 10;
	}

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = obxaMidiSendMsg;

	/* Glide and Unison, done */
	dispatch[FIRST_DEV].controller = 126;
	dispatch[FIRST_DEV].operator = 0;
	dispatch[FIRST_DEV + 1].controller = 126;
	dispatch[FIRST_DEV + 1].operator = 1;
	dispatch[FIRST_DEV + 2].controller = 1;
	dispatch[FIRST_DEV + 2].operator = 10;

	/* LFO - operator 2 parameters - Done */
	dispatch[FIRST_DEV + 3].controller = 2;
	dispatch[FIRST_DEV + 3].operator = 0;
	dispatch[FIRST_DEV + 4].controller = 126;
	dispatch[FIRST_DEV + 4].operator = 19;
	dispatch[FIRST_DEV + 5].controller = 126;
	dispatch[FIRST_DEV + 5].operator = 20;
	/* S/H - this should be a routing switch. */
	dispatch[FIRST_DEV + 6].controller = 126;
	dispatch[FIRST_DEV + 6].operator = 21;

	/* Depth Osc/Osc/Filt - routing control/switches */
	dispatch[FIRST_DEV + 7].controller = 126;
	dispatch[FIRST_DEV + 7].operator = 4;
	dispatch[FIRST_DEV + 8].controller = 126;
	dispatch[FIRST_DEV + 8].operator = 5;
	dispatch[FIRST_DEV + 9].controller = 126;
	dispatch[FIRST_DEV + 9].operator = 6;
	dispatch[FIRST_DEV + 10].controller = 126;
	dispatch[FIRST_DEV + 10].operator = 7;

	/* PWM Osc/Osc - routing control/switches */
	dispatch[FIRST_DEV + 11].controller = 126;
	dispatch[FIRST_DEV + 11].operator = 8;
	dispatch[FIRST_DEV + 12].controller = 126;
	dispatch[FIRST_DEV + 12].operator = 9;
	dispatch[FIRST_DEV + 13].controller = 126;
	dispatch[FIRST_DEV + 13].operator = 10;

	/* Osc - Some routing switches. */
	dispatch[FIRST_DEV + 14].controller = 0;
	dispatch[FIRST_DEV + 14].operator = 1;
	/* PWM - needs to adjust both oscillators, so need a dispatcher. */
	dispatch[FIRST_DEV + 15].controller = 15;
	dispatch[FIRST_DEV + 15].operator = 1;
	dispatch[FIRST_DEV + 16].controller = 1;
	dispatch[FIRST_DEV + 16].operator = 9;
	/* Saw/Square. May need dispatch for tri when both off */
	dispatch[FIRST_DEV + 17].controller = 0;
	dispatch[FIRST_DEV + 17].operator = 4;
	dispatch[FIRST_DEV + 18].controller = 0;
	dispatch[FIRST_DEV + 18].operator = 6;
	/* Crossmod and sync - routing switches */
	dispatch[FIRST_DEV + 19].controller = 126;
	dispatch[FIRST_DEV + 19].operator = 23;
	dispatch[FIRST_DEV + 20].controller = 1;
	dispatch[FIRST_DEV + 20].operator = 7;
	/* Saw/Square. May need dispatch for tri when both off */
	dispatch[FIRST_DEV + 21].controller = 1;
	dispatch[FIRST_DEV + 21].operator = 4;
	dispatch[FIRST_DEV + 22].controller = 1;
	dispatch[FIRST_DEV + 22].operator = 6;
	/* Osc waveforms need a dispatcher to enable tri with both buttons off. */
	dispatch[FIRST_DEV + 15].routine =
		dispatch[FIRST_DEV + 17].routine =
		dispatch[FIRST_DEV + 18].routine =
		dispatch[FIRST_DEV + 21].routine =
		dispatch[FIRST_DEV + 22].routine =
			(synthRoutine) obxaWaveform;

	/* Filter - some routing switches */
	dispatch[FIRST_DEV + 23].controller = 4;
	dispatch[FIRST_DEV + 23].operator = 4;
	dispatch[FIRST_DEV + 23].routine = (synthRoutine) obxaFilter;
	dispatch[FIRST_DEV + 24].controller = 4;
	dispatch[FIRST_DEV + 24].operator = 0;
	dispatch[FIRST_DEV + 25].controller = 4;
	dispatch[FIRST_DEV + 25].operator = 1;
	dispatch[FIRST_DEV + 26].controller = 4;
	dispatch[FIRST_DEV + 26].operator = 2;
	/* Routing switches. Need to check on KBD tracking though */
	dispatch[FIRST_DEV + 27].controller = 126;
	dispatch[FIRST_DEV + 27].operator = 12;
	dispatch[FIRST_DEV + 28].controller = 4;
	dispatch[FIRST_DEV + 28].operator = 3;
	dispatch[FIRST_DEV + 29].controller = 126;
	dispatch[FIRST_DEV + 29].operator = 14;
	dispatch[FIRST_DEV + 30].controller = 126;
	dispatch[FIRST_DEV + 30].operator = 15;
	dispatch[FIRST_DEV + 31].controller = 126;
	dispatch[FIRST_DEV + 31].operator = 17;

	/* Envelopes */
	dispatch[FIRST_DEV + 32].controller = 3;
	dispatch[FIRST_DEV + 32].operator = 0;
	dispatch[FIRST_DEV + 33].controller = 3;
	dispatch[FIRST_DEV + 33].operator = 1;
	dispatch[FIRST_DEV + 34].controller = 3;
	dispatch[FIRST_DEV + 34].operator = 2;
	dispatch[FIRST_DEV + 35].controller = 3;
	dispatch[FIRST_DEV + 35].operator = 3;
	dispatch[FIRST_DEV + 36].controller = 5;
	dispatch[FIRST_DEV + 36].operator = 0;
	dispatch[FIRST_DEV + 37].controller = 5;
	dispatch[FIRST_DEV + 37].operator = 1;
	dispatch[FIRST_DEV + 38].controller = 5;
	dispatch[FIRST_DEV + 38].operator = 2;
	dispatch[FIRST_DEV + 39].controller = 5;
	dispatch[FIRST_DEV + 39].operator = 3;
	/* We need a separate dispatcher for the decay, since the reset button */
	/* will affect the value. */
	dispatch[FIRST_DEV + 35].routine = dispatch[FIRST_DEV + 39].routine =
			(synthRoutine) obxaDecay;

	/* Master volume - gain of envelope? */
	dispatch[FIRST_DEV + 40].controller = 5;
	dispatch[FIRST_DEV + 40].operator = 4;
	/* Reset or 'release' DONE */
	dispatch[FIRST_DEV + 41].controller = 41;
	dispatch[FIRST_DEV + 41].operator = 0;
	dispatch[FIRST_DEV + 41].routine = (synthRoutine) obxaDecay;
	/* Mod to tremelo */
	dispatch[FIRST_DEV + 42].controller = 126;
	dispatch[FIRST_DEV + 42].operator = 18;
	/* Balance. Should work as a dispatcher along with master volume? */
	dispatch[FIRST_DEV + 43].controller = 126;
	dispatch[FIRST_DEV + 43].operator = 28;
	/* Master tune */
	dispatch[FIRST_DEV + 44].controller = 126;
	dispatch[FIRST_DEV + 44].operator = 2;

	/* Mode management */
	dispatch[FIRST_DEV + 45].controller = 45;
	dispatch[FIRST_DEV + 45].operator = 0;
	dispatch[FIRST_DEV + 46].controller = 46;
	dispatch[FIRST_DEV + 46].operator = 1;
	dispatch[FIRST_DEV + 47].controller = 47;
	dispatch[FIRST_DEV + 47].operator = 2;
	dispatch[FIRST_DEV + 45].routine =
		dispatch[FIRST_DEV + 46].routine =
		dispatch[FIRST_DEV + 47].routine = (synthRoutine) obxaMode;

	/* Hold - DONE (stole from Juno.....). Remapped to chording */
	dispatch[FIRST_DEV + 48].routine = (synthRoutine) obxaChord;
	/* Master tune - Done. */
	/*dispatch[FIRST_DEV + 48].controller = 126; */
	/*dispatch[FIRST_DEV + 48].operator = 2; */
	/* Autotune */
	dispatch[FIRST_DEV + 49].routine = (synthRoutine) multiTune;

	dispatch[FIRST_DEV + 79].routine = (synthRoutine) obxaSeqRate;

/* Memories. */
	dispatch[MEM_START + 0].controller = 16;
	dispatch[MEM_START + 1].controller = 0;
	dispatch[MEM_START + 2].controller = 1;
	dispatch[MEM_START + 3].controller = 2;
	dispatch[MEM_START + 4].controller = 3;
	dispatch[MEM_START + 5].controller = 4;
	dispatch[MEM_START + 6].controller = 5;
	dispatch[MEM_START + 7].controller = 6;
	dispatch[MEM_START + 8].controller = 7;
	dispatch[MEM_START + 9].controller = 8;
	dispatch[MEM_START + 10].controller = 9;
	dispatch[MEM_START + 11].controller = 10;
	dispatch[MEM_START + 12].controller = 11;
	dispatch[MEM_START + 13].controller = 12;
	dispatch[MEM_START + 14].controller = 13;
	dispatch[MEM_START + 15].controller = 14;
	dispatch[MEM_START + 16].controller = 15;
	dispatch[MEM_START + 17].controller = 17;

	dispatch[MEM_START + 0].operator = MEM_START + 0;
	dispatch[MEM_START + 1].operator = MEM_START + 1;
	dispatch[MEM_START + 2].operator = MEM_START + 2;
	dispatch[MEM_START + 3].operator = MEM_START + 3;
	dispatch[MEM_START + 4].operator = MEM_START + 4;
	dispatch[MEM_START + 5].operator = MEM_START + 5;
	dispatch[MEM_START + 6].operator = MEM_START + 6;
	dispatch[MEM_START + 7].operator = MEM_START + 7;
	dispatch[MEM_START + 8].operator = MEM_START + 8;
	dispatch[MEM_START + 9].operator = MEM_START + 9;
	dispatch[MEM_START + 10].operator = MEM_START + 10;
	dispatch[MEM_START + 11].operator = MEM_START + 11;
	dispatch[MEM_START + 12].operator = MEM_START + 12;
	dispatch[MEM_START + 13].operator = MEM_START + 13;
	dispatch[MEM_START + 14].operator = MEM_START + 14;
	dispatch[MEM_START + 15].operator = MEM_START + 15;
	dispatch[MEM_START + 16].operator = MEM_START + 16;
	dispatch[MEM_START + 17].operator = MEM_START + 17;

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
		dispatch[MEM_START + 17].routine =
			(synthRoutine) obxaMemory;

	dispatch[FIRST_DEV + 68].routine = dispatch[FIRST_DEV + 69].routine =
		(synthRoutine) obxaMidi;
	dispatch[FIRST_DEV + 68].controller = 1;
	dispatch[FIRST_DEV + 69].controller = 2;

	dispatch[FIRST_DEV + 71].routine = (synthRoutine) obxaFunction;
	dispatch[FIRST_DEV + 72].controller = 72;
	dispatch[FIRST_DEV + 72].operator = 0;
	dispatch[FIRST_DEV + 72].routine = (synthRoutine) obxaArpeggiate;
	dispatch[FIRST_DEV + 73].controller = 73;
	dispatch[FIRST_DEV + 73].operator = 1;
	dispatch[FIRST_DEV + 73].routine = (synthRoutine) obxaArpeggiate;
	dispatch[FIRST_DEV + 74].controller = 74;
	dispatch[FIRST_DEV + 74].operator = 2;
	dispatch[FIRST_DEV + 74].routine = (synthRoutine) obxaArpeggiate;

	dispatch[FIRST_DEV + 75].routine = (synthRoutine) obxaSequence;
	dispatch[FIRST_DEV + 76].controller = 76;
	dispatch[FIRST_DEV + 76].operator = 0;
	dispatch[FIRST_DEV + 76].routine = (synthRoutine) obxaArpOctave;
	dispatch[FIRST_DEV + 77].controller = 77;
	dispatch[FIRST_DEV + 77].operator = 1;
	dispatch[FIRST_DEV + 77].routine = (synthRoutine) obxaArpOctave;
	dispatch[FIRST_DEV + 78].controller = 78;
	dispatch[FIRST_DEV + 78].operator = 2;
	dispatch[FIRST_DEV + 78].routine = (synthRoutine) obxaArpOctave;

	dispatch[RADIOSET_1].other1 = -1;
	dispatch[RADIOSET_2].other1 = -1;
	dispatch[RADIOSET_3].other1 = -1;
	dispatch[SEQ_MODE].other1 = -1;
	dispatch[OCTAVE_MODE].other1 = -1;

	dispatch[FIRST_DEV + 70].routine = (synthRoutine) obxaPanelSwitch;

	dispatch[RADIOSET_1].other1 = -1;

	/* Tune/Gain osc-1/2 main layer */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 3, C_RANGE_MIN_1);
	/* Gain on filter contour */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, C_RANGE_MIN_1);
	/* Select alt filter */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 0);

	/* Tune/Gain osc-1/2 second layer */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 3, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 3, C_RANGE_MIN_1);
	/* Gain on filter contour */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 4, C_RANGE_MIN_1);
	/* Select alt filter and pole remixing */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 4, 4, 4);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 4, 7, 4096);

	/* This cause the voices to see REKEY events for arpeg and seq steps */
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_TRIGGER, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RANGE, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_TRIGGER, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RANGE, 2);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
obxaConfigure(brightonWindow *win)
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

	printf("going operational: %p, %p\n", synth, win);

	synth->flags |= OPERATIONAL;
	synth->keypanel = 1;
	synth->keypanel2 = -1;
	synth->transpose = 36;

	/* Don't load a memory, send a bank, location and load request. */
	event.intvalue = 1;
	brightonParamChange(synth->win, synth->panel,
		MEM_START + 1, &event);
	brightonParamChange(synth->win, synth->panel,
		MEM_START + 9, &event);

	multiTune(synth, 0,0,0,0,0);

	synth->mem.param[LAYER_SWITCH] = 0;

	synth->bank = 1;

	/*
	 * For the OBXa, most of the following has to be moved into the memory
	 * load routines. It is a new paradigm - one panel, two synths.
	 */
	obxaLoadMemory(synth, "obxa", 0, initmem,
		synth->mem.active, FIRST_DEV, 0);

	synth->mem.param[LAYER_SWITCH] = 0;

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);

	/* Set arpeggiator to 2 octaves and a middle rate */
	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;
	brightonParamChange(synth->win, 0, OCTAVE_MODE + 1, &event);
	event.value = 0.4;
	brightonParamChange(synth->win, 0, 79, &event);

	synth->loadMemory = (loadRoutine) obxaLoadMemory;
	synth->saveMemory = (saveRoutine) obxaSaveMemory;

	configureGlobals(synth);
	return(0);
}

