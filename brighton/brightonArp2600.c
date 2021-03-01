
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

/* ARP Arp2600 */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int arp2600Init();
static int arp2600Configure();
static int arp2600Callback(brightonWindow *, int, int, float);
/*static int keyCallback(void *, int, int, float); */
static int arp2600MidiCallback(brightonWindow *, int, int, float);

extern guimain global;

/* #include "brightonKeys.h" */

#define DEVICE_COUNT 201
#define ARP_OUTPUTS 40
#define ARP_INPUTS 60
#define OUTPUT_START 82
#define ACTIVE_DEVS (OUTPUT_START + ARP_OUTPUTS)
#define INPUT_START ACTIVE_DEVS
#define MEM_START (DEVICE_COUNT - 19)
#define MIDI_START (MEM_START + 14)

#define DISPLAY_DEV (DEVICE_COUNT - 2)
#define DISPLAY_DEV2 (DEVICE_COUNT - 1)

#define KEY_PANEL 1

static int oselect;
static struct {
	int input;
	int id; /* From graphical interface. */
} links[ARP_OUTPUTS];

#define CDIFF 30
#define CDIFF2 12
#define CDIFF3 24
#define CDIFF4 23
#define CDIFF5 20

#define C0 25
#define C1 47
#define C2 67
#define C3 110
#define C4 (C3 + CDIFF)
#define C5 (C4 + CDIFF3)
#define C6 (C5 + CDIFF3)
#define C7 (C6 + CDIFF3)
#define C8 (C7 + CDIFF)
#define C9 (C8 + CDIFF3)
#define C10 (C9 + CDIFF3 - 1)
#define C11 (C10 + CDIFF3)
#define C12 (C11 + CDIFF3)
#define C13 (C12 + CDIFF + 1)
#define C14 (C13 + CDIFF3)
#define C15 (C14 + CDIFF3)
#define C16 (C15 + CDIFF3)
#define C17 (C16 + CDIFF3 - 1)
#define C18 (C17 + CDIFF)
#define C19 (C18 + CDIFF3)
#define C20 (C19 + CDIFF3)
#define C21 (C20 + CDIFF3)
#define C22 (C21 + CDIFF3)
#define C23 (C22 + CDIFF3 - 2)
#define C24 (C23 + CDIFF3 + 1)
#define C25 (C24 + CDIFF3)

#define C26 (C25 + CDIFF3 + 4)
#define C27 (C26 + CDIFF3)
#define C28 (C27 + CDIFF3 - 1)
#define C29 (C28 + CDIFF3)
#define C30 (C29 + CDIFF3 + 7)
#define C31 (C30 + CDIFF3 - 1)
#define C32 (C31 + CDIFF3)
#define C33 (C32 + CDIFF3)
#define C34 (C33 + CDIFF3 + 6)
#define C35 (C34 + CDIFF3)
#define C36 (C35 + CDIFF3 - 1)
#define C37 (C36 + CDIFF3)

#define Cb0 602
#define Cb1 (Cb0 + CDIFF5)
#define Cb2 (Cb1 + CDIFF5)
#define Cb3 (Cb2 + CDIFF5)
#define Cb4 (Cb3 + CDIFF5 + 8)
#define Cb5 (Cb4 + CDIFF5)
#define Cb6 (Cb5 + CDIFF5)
#define Cb7 (Cb6 + CDIFF5)
#define Cb8 (Cb7 + CDIFF5)

/* 244 */
#define Cb9 100
#define Cb10 (Cb9 + CDIFF4 + 1)

/* 360 */
#define Cb11 498
#define Cb12 (Cb11 + CDIFF4 + 2)

#define R0 365
#define R1 72
#define R2 217
#define R3 720
#define R5 185
#define R6 435

#define W0 11
#define W1 12
#define W2 13
#define L1 160

#define BDIFF (CDIFF + 2)
#define BDIFF2 (CDIFF2 + 2)

#define B0 170
#define B1 (B0 + BDIFF + BDIFF2)
#define B2 (B1 + BDIFF)
#define B3 (B2 + BDIFF)
#define B4 (B3 + BDIFF)
#define B5 (B4 + BDIFF)
#define B6 (B5 + BDIFF)
#define B7 (B6 + BDIFF)
#define B8 (B7 + BDIFF)
#define B9 (B8 + BDIFF + BDIFF2)
#define B10 (B9 + BDIFF)
#define B11 (B10 + BDIFF)
#define B12 (B11 + BDIFF)
#define B13 (B12 + BDIFF)
#define B14 (B13 + BDIFF)
#define B15 (B14 + BDIFF)
#define B16 (B15 + BDIFF)
#define B17 (B16 + BDIFF + BDIFF2)

#define B18 (BDIFF)
#define B19 (B18 + BDIFF + BDIFF)

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
 * This example is for a arp2600Bristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {
	{"EnvFollow Gain", 1, C0 - 2, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl2.xpm", 0, 0},
	{"RM in1 level", 1, C2 - 2, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl2.xpm", 0, 0},
	{"RM in2 level", 1, C3 - 1, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl2.xpm", 0, 0},

	/* Osc-1 - 3 */
	{"VCO1 Transpose", 1, C4 - 5, R5, 7, 80, 0, 4, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
/*		"bitmaps/knobs/sliderblackl2.xpm", 0, 0}, */
/*	{"", 1, C4 - 1, R0, W1, L1, 0, 1, 0, */
/*		"bitmaps/knobs/sliderblackl2.xpm", 0, 0}, */
	{"VCO1 Tracking", 2, C4 - 6, R6 - 8, 9, 45, 0, 2, 0,
		"bitmaps/buttons/klunk2.xpm", 0, BRIGHTON_THREEWAY},
	{"VCO1 FM Mod1 lvl", 1, C5, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl.xpm", 0, 0},
	{"VCO1 FM Mod2 lvl", 1, C6, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl.xpm", 0, 0},
	{"VCO1 FM Mod3 lvl", 1, C7, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl.xpm", 0, 0},
	{"VCO1 Sync 2->1", 2, C5 + 17, R5 - 15, 16, 14, 0, 1, 0,
		"bitmaps/buttons/klunk2.xpm", 0, 0},
	{"VCO1 Tune Coarse", 1, 138, 77, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpointL.xpm", 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOTCH},
	{"VCO1 Tune Fine", 1, 138, 120, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpointL.xpm", 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOTCH},

	/* Osc-2 - 11 */
	{"VCO2 Transpose", 1, C8 - 6, R5, 7, 80, 0, 4, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
/*		"bitmaps/knobs/sliderblackl.xpm", 0, 0}, */
/*	{"VCO2-Transpose", 1, C8, R0, W1, L1, 0, 1, 0, */
/*		"bitmaps/knobs/sliderblackl.xpm", 0, 0}, */
	{"VCO2 Tracking", 2, C8 - 7, R6 - 8, 9, 45, 0, 2, 0,
		"bitmaps/buttons/klunk2.xpm", 0, BRIGHTON_THREEWAY},
	{"VCO2 FM Mod1 lvl", 1, C9, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl.xpm", 0, 0},
	{"VCO2 FM Mod2 lvl", 1, C10, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCO2 FM Mod2 lvl", 1, C11, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCO2 PWM", 1, C12, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCO2 Tune Coarse", 1, 250, 77, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOTCH},
	{"VCO2 Tune Fine", 1, 250, 120, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOTCH},
	{"VCO2 PW", 1, 250, 165, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	
	/* Osc-3 - 20 */
	{"VCO3 Transpose", 1, C13 - 6, R5, 7, 80, 0, 4, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
/*		"bitmaps/knobs/sliderblack2.xpm", 0, 0}, */
/*	{"VCO3 Transpose", 1, C13, R0, W1, L1, 0, 1, 0, */
/*		"bitmaps/knobs/sliderblack2.xpm", 0, 0}, */
	{"VCO3 Tracking", 2, C13 - 7, R6 - 8, 9, 45, 0, 2, 0,
		"bitmaps/buttons/klunk2.xpm", 0, BRIGHTON_THREEWAY},
	{"VCO3 FM Mod1 lvl", 1, C14, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCO3 FM Mod2 lvl", 1, C15, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCO3 FM Mod3 lvl", 1, C16, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCO3 PWM", 1, C17, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCO3 Tune Coarse", 1, 376, 77, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpoint.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOTCH},
	{"VCO3 Tune Fine", 1, 376, 120, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpoint.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOTCH},
	{"VCO3 PW", 1, 376, 165, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpoint.xpm", 0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE},

	/* VCF - 29 */
/*	{"", 1, C23 + 11, R1 + 14, 8, 78, 0, 4, 0, */
/*		"bitmaps/knobs/sliderblack2.xpm", 0, 0}, */
	{"VCF Mix1 lvl", 1, C18, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCF Mix2 lvl", 1, C19, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCF Mix3 lvl", 1, C20, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCF Mix4 lvl", 1, C21, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCF Mix5 lvl", 1, C22, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCF Mod1 lvl", 1, C23, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCF Mod2 lvl", 1, C24, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCF Mod3 lvl", 1, C25, R0, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"VCF Cutoff", 1, 534, 77, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpoint.xpm", 0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	{"VCF Resonance", 1, 534, 120, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpoint.xpm", 0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
/*	{"", 1, 500, 165, 104, 30, 0, 1, 0, */
/*		"bitmaps/knobs/sliderpoint.xpm", 0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE}, */
/*	{"", 1, 500, 208, 104, 30, 0, 1, 0, */
/*		"bitmaps/knobs/sliderpoint.xpm", 0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE}, */

	/* ADSR/AR - 39 */
	{"Attack", 1, C26, R1, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"Decay", 1, C27, R1, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"Sustain", 1, C28, R1, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"Release", 1, C29, R1, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"AR Attack", 1, C26, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"AR Release", 1, C29, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"AR Gate", 2, C27 - 4, 474, 8, 25, 0, 1, 0,
		"bitmaps/buttons/klunk2.xpm", 0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE},

	/* ?? - 46 */
	{"VCA Mix1 lvl", 1, C30, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"VCA Mix2 lvl", 1, C31, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"VCA Mod1 lvl", 1, C32, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"VCA Mod2 lvl", 1, C33, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr2.xpm", 0, 0},
	/* Mixer */
	{"Mix lvl1", 1, C34 + 1, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr2.xpm", 0, 0},
	{"Mix lvl2", 1, C35 + 1, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr2.xpm", 0, 0},

	/* Effects return - 52 */
	{"FX Return Left", 1, C36 + 2, R2, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr2.xpm", 0, 0},
	{"FX Return Right", 1, C37 + 2, R2, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr2.xpm", 0, 0},

	/* - 54 */
	{"Chorus D1", 1, Cb0, R3, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"Chorus D2", 1, Cb1, R3, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"Chorus D3", 1, Cb2, R3, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"Chorus D4", 1, Cb3, R3, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},

	/* - 58 */
	{"Reverb D1", 1, Cb4, R3, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"Reverb D2", 1, Cb5, R3, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr.xpm", 0, 0},
	{"Reverb D3", 1, Cb6, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr2.xpm", 0, 0},
	{"Reverb D4", 1, Cb7, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackr2.xpm", 0, 0},
	/* Manual start */
	{"", 2, 712, 382, 12, 15, 0, 1, 0, "bitmaps/buttons/patchlow.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_NOSHADOW},

	/* - 63 */
	{"Noise White/Pink", 1, Cb9, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl2.xpm", 0, 0},
	{"Noise Level", 1, Cb10, R3, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblackl2.xpm", 0, 0},

	/*; - 65 */
	{"LFO Level", 1, Cb11, R3, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"LFO Rate", 1, Cb12, R3, W0, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},

	/* - 67 */
	{"LFO Sync", 2, Cb12 + 5, 920, 16, 14, 0, 1, 0,
		"bitmaps/buttons/klunk2.xpm", 0, 0},

	/* Global (now program) Volume - 68 */
	{"Volume Program", 1, 786, 120, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpointR.xpm", 0,BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	{"Volume Pan", 1, 888, 120, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpointR.xpm", 0,BRIGHTON_VERTICAL|BRIGHTON_REVERSE|
		BRIGHTON_NOTCH},

	/* Input gain and glide */
	{"Input Gain", 0, 47, 78, 50, 50, 0, 1, 0,
		"bitmaps/knobs/knobgreynew.xpm",
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Glide", 0, 101, 78, 50, 50, 0, 1, 0,
		"bitmaps/knobs/knobgreynew.xpm",
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* Voltage processors */
	{"Mix1 Gain1", 1, 243, 711, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	{"Mix1 Gain2", 1, 243, 767, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	{"Mix2 Gain2", 1, 243, 821, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	{"Mix3 LagRate", 1, 243, 881, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE},

	/* MARK used one dummy for VCA initial volume */
	{"VCA Initial Vol", 1, 786, 163, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpointR.xpm", 0,BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	/* DUMMIES */
	{"", 1, 0, 0, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, 104, 30, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN},

	/*
	 * These are the patch buttons. Positions are unfortunately arbitrary.
	 * First the outputs, then the inputs.
	 */
	/* 82 */
	{"Output Preamp", 2, 110, 213, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 110, 280, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 46, 439, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* VCO1 */
	{"", 2, 212, 249, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 212, 316, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* VCO2 */
	{"", 2, 313, 249, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 313, 316, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 337, 249, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 337, 316, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* VCO3 */
	{"", 2, 438, 249, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 438, 316, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 462, 249, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 462, 316, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* VCF */
	{"", 2, 660, 287, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* ADSR */
	{"", 2, 760, 287, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 735, 382, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 864, 287, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* Mix out */
	{"", 2, 902, 235, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* Noise/SH/Switch */
	{"", 2, 146, 797, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* SH/Switch - 324 */
	{"", 2, 463, 797, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 560, 666, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 560, 906, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 891, 339, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 915, 339, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* Voltage Processor Outputs */
	{"", 2, 411, 734, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 411, 792, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 411, 832, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 411, 886, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* Reverb R out */
	{"", 2, 961, 505, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	/* CV/Audio inputs */
	{"", 2, 30, 716, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 30, 772, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 30, 826, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	{"", 2, 30, 886, 12, 15, 0, 1, 0,
		"bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", 0},
	/* DUMMIES */
	{"", 2, 0, 0, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	/*
	 * Inputs. From the envelope follower onwards, these offset start from '1'
	 * rather than from zero, this corrected later in the code.
	 */
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"Input EnvFollow", 2, 24, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 67, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 110, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* VCO1 - the KDB CV input is disabled - FFS in engine. */
	{"", 2, 140, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 164, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 188, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 212, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* VCO2 */
	{"", 2, 242, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 266, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 289, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 313, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 337, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* VCO3 12 */
	{"", 2, 367, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 391, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 415, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 438, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 462, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* VCF */
	{"", 2, 493, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 516, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 540, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 564, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 588, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 611, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 635, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 659, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* Envelopes, etc. */
	{"", 2, 687, 287, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 710, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 734, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 758, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	/* AMP, etc. */
	{"", 2, 789, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 813, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 837, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 861, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* The rest */
	{"", 2, 891, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 915, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 938, 552, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* Lin, Rin */
	{"", 2, 891, 180, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 961, 180, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* Electro inputs */
	{"", 2, 560, 754, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 560, 797, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* Noise into SH */
	{"", 2, 463, 666, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* Ext and switch clock */
	{"", 2, 324, 886, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 544, 864, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* Pan */
	{"", 2, 915, 180, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* Voltage Processor inputs */
	{"", 2, 197, 716, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 197, 772, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 197, 826, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 197, 886, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 362, 690, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 362, 746, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 362, 802, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* CV/Audio inputs */
	{"", 2, 950, 716, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 950, 772, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 950, 826, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 950, 886, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON},
	/* DUMMIES */
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},

#define XS 808
#define XD 30
#define YS 730
#define YD 35

#define XSZ 19
#define YSZ 26

	/* L-S-0 */
	{"", 2, XS, YS+3*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/L.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+2*XD, YS+3*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/S.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+XD, YS+3*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/0.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},

	/* 1-2-3 */
	{"", 2, XS, YS, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/1.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+XD, YS, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/2.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+2*XD, YS, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/3.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},

	/* 4-5-6 */
	{"", 2, XS, YS+1*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/4.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+XD, YS+1*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/5.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+2*XD, YS+1*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/6.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},

	/* 7-8-9 */
	{"", 2, XS, YS+2*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/7.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+XD, YS+2*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/8.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+2*XD, YS+2*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/9.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},

	/* U-P */
	{"", 2, XS, YS+4*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/Down.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+2*XD, YS+4*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/Up.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, XS, YS+5*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/Down.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, XS+2*XD, YS+5*YD, XSZ, YSZ, 0, 1, 0, "bitmaps/digits/Up.xpm",
		"bitmaps/buttons/jellyon.xpm", BRIGHTON_CHECKBUTTON},

	/* MARK Stuff in another control for Global Volume */
	{"", 1, 786, 77, 104, 30, 0, 1, 0,
		"bitmaps/knobs/sliderpointR.xpm", 0,BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	/* Display */
	{"", 3, 797, 670, 100, YSZ, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}, /*display 60 */
	{"", 3, 797, 695, 100, YSZ, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay2.xpm", 0}, /*display 60 */
/*		0, 0}, //display 60 */
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp arp2600App = {
	"arp2600",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal4.xpm",
	0, /* BRIGHTON_STRETCH, //flags */
	arp2600Init,
	arp2600Configure, /* 3 callbacks, unused? */
	arp2600MidiCallback,
	destroySynth,
	{1, 100, 3, 2, 5, 520, 0, 0},
	886, 500, 0, 0,
	1, /* Panels */
	{
		{
			"Arp2600",
			"bitmaps/blueprints/arp2600.xpm",
			/*"bitmaps/textures/metal4.xpm", */
			"bitmaps/textures/metal2.xpm",
			/*"bitmaps/textures/bluemeanie.xpm", */
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			arp2600Callback,
			0, 0, 1000, 1000,
			DEVICE_COUNT,
			locations
		}
	}
};

static int
arp2600MidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

/*
 * This is a shim, when loading a memory we have to clear out all existing 
 * patches and then re-request them afterwards. This is a slow memory operation
 * since we have to unpatch everything before the memory can be allocated.
 *
 * This could be improved.
 */
static int
arp2600LoadMemory(guiSynth *synth, char *n, char *a, int b, int c, int d, int e)
{
	brightonEvent event;
	int i, result;

	if (e == 0)
	{
		for (i = 0; i < ARP_OUTPUTS; i++)
		{
			if (links[i].input > 0)
			{
/*				printf("Mem disconnecting %i from %i", i, links[i].input); */
				event.type = BRIGHTON_UNLINK;
				event.intvalue = 16382;
				brightonParamChange(synth->win, 0, OUTPUT_START+ i, &event);
				for (i = 0; i < ARP_OUTPUTS; i++)
				{
					links[i].input = -1;
					links[i].id = -1;
				}
				break;
			}
		}
	
		/*
		 * This is used to ensure we could not have left any patches hanging in 
		 * the engine. We could use a similar 'clear all' operation for the GUI
		 * so speed things up?
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 102, 0, 0);
	}

	result = loadMemory(synth, n, a, b, c, d, e);

	/*
	 * See if we need to patch things back in.
	 */
	if (e == 0)
	{
		for (i = 0; i < ARP_OUTPUTS; i++)
		{
			links[i].input = -1;
			links[i].id = -1;

			if (synth->mem.param[OUTPUT_START + i] > 0)
			{
				int connect = synth->mem.param[OUTPUT_START + i];

/*				printf("Mem connecting %i to %1.0f\n", i, */
/*					synth->mem.param[OUTPUT_START + i]); */

				/*
				 * Select the input, request a link for the GUI and then the
				 * link request to the engine.
				 */
				event.type = BRIGHTON_PARAMCHANGE;
				event.value = 1;
				brightonParamChange(synth->win, 0, OUTPUT_START + i, &event);

				event.type = BRIGHTON_LINK;
				event.intvalue = INPUT_START + connect;
				links[oselect].input = connect;
				links[oselect].id =
					brightonParamChange(synth->win, 0, OUTPUT_START + i,
						&event);

				/*
				 * This call could be put into another early cycle through the
				 * table. This would cause the engine to be patched almost
				 * immediately, and the GUI would follow shortly afterwards.
				 *
				 * Subtraction of 1 is due to offset differences
				 */
				bristolMidiSendMsg(global.controlfd, synth->sid, 100, i,
					connect - 1);

				synth->mem.param[OUTPUT_START + i] = connect;
			}
		}
		oselect = -1;

		/* PW Osc-1 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 16383);
	}

	return(result);
}

static int
arp2600MidiCallback(brightonWindow *win, int command, int value, float v)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", command, value);

	switch(command)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", command, value);
			synth->location = value;

			arp2600LoadMemory(synth, "arp2600", 0,
				synth->location, synth->mem.active, 0, 0);

			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", command, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
arp2600Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
/*	printf("	arp2600Memory(B: %i L %i: %i, %i)\n", */
/*		synth->bank, synth->location, c, o); */

	switch (c) {
		default:
		case 0:
			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;
			if (loadMemory(synth, "arp2600", 0, synth->location,
				synth->mem.active, 0, BRISTOL_STAT) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY_DEV);
			else
				displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
		case 1:
			arp2600LoadMemory(synth, "arp2600", 0, synth->location,
				synth->mem.active, 0, 0);
			displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
		case 2:
			/*
			 * Before we can save a memory we have to merge our links into 
			 * the mem buffer
			 */
			saveMemory(synth, "arp2600", 0, synth->location, 0);
			displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
		case 3:
			/*
			 * Go through the memories, but do not load any of them. This will
			 * break when we find a used one.
			 */
			while (arp2600LoadMemory(synth, "arp2600", 0, --synth->location,
				synth->mem.active, 0, 1) < 0)
			{
				if (synth->location < 0)
				{
					synth->location = 1000;
				}
			}
			/* Then load it */
			arp2600LoadMemory(synth, "arp2600", 0, synth->location,
				synth->mem.active, 0, 0);
			displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
		case 4:
			/*
			 * Go through the memories, but do not load any of them. This will
			 * break when we find a used one.
			 */
			while (arp2600LoadMemory(synth, "arp2600", 0, ++synth->location,
				synth->mem.active, 0, 1) < 0)
			{
				if (synth->location > 999)
				{
					synth->location = -1;
				}
			}
			/* Then load it */
			arp2600LoadMemory(synth, "arp2600", 0, synth->location,
				synth->mem.active, 0, 0);
			displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
	}
	return(0);
}

static int
arp2600Midi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			newchan = synth->midichannel = 0;
			displayText(synth, "MIDI", newchan + 1, DISPLAY_DEV2);
			return(0);
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 16)
		{
			newchan = synth->midichannel = 15;
			displayText(synth, "MIDI", newchan + 1, DISPLAY_DEV2);
			return(0);
		}
	}

	displayText(synth, "MIDI", newchan + 1, DISPLAY_DEV2);

	if (global.libtest == 0)
	{
		/*
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
arp2600Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	/* printf("arp2600Callback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (arp2600App.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * (CONTROLLER_RANGE - 1);
	else
		sendvalue = value;

	synth->mem.param[index] = value;

	/*if ((!global.libtest) || (index >= ACTIVE_DEVS)) */
	if ((!global.libtest) || (index >= 71))
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

static int
arp2600Volume(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, c, o,
		 (int) (synth->mem.param[68] * synth->mem.param[198] * C_RANGE_MIN_1));

	return(0);
}

static int
arp2600ManualStart(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (v == 0)
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, 40);
	else
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, 40);
	return(0);
}

static int
arp2600IOSelect(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

/*	printf("arp2600IOSelect(%i, %i, %i)", c, o, v); */

	event.command = BRIGHTON_PARAMCHANGE;

	/*
	 * We have quite a lot of work here, we need to mark any output that is
	 * selected, if an input is selected then we should mark active the previous
	 * output to link them up and draw the desired layer item. We should keep
	 * all this in some structure such that it can be deconfigured, ie, patches
	 * can be removed.
	 *
	 * The structure we need should have an input and output list, this should
	 * give the true co-ords for the start and endpoint since it will be used
	 * to evaluate the transforms for the patch source to the on-screen dest
	 * bitmaps.
	 *
	 * There should also be a sanity check that clears any wayward output 
	 * selections, required before saveMemory() can be called.
	 *
	 * We will probably need a selection of bitmaps so that we can simplify
	 * the stretch algorithm. If we only have one bitmaps then we need a more
	 * complex transform since the end points should not be stretched, only 
	 * the intermittant cabling:
	 *	cable source is 146bits, 3 for each end.
	 * 	Target length is 725 for example
	 *	the 3 start and end pixels get copied,
	 *	the middle 140 bits must be scaled to 719.
	 *	The result should then be rotated into position.
	 */
	if (c == 1)
	{
		if (v > 0)
			oselect = o;
		else
			oselect = -1;

		/*
		 * If we select an output that is assigned then clear it.
		 */
		if (links[o].input > 0)
		{
			bristolMidiSendMsg(global.controlfd, synth->sid,
				101, o, links[o].input - 1);
			links[o].input = 0;
			event.type = BRIGHTON_UNLINK;
			event.intvalue = links[o].id;
			brightonParamChange(synth->win, 0, OUTPUT_START + o, &event);
		}
	} else if (c == 2) {
		int i;

		if (oselect >= 0) {
			for (i = 0; i < ARP_OUTPUTS; i++)
			{
				if ((links[i].input > 0)
					&& (links[i].input == o))
				{
					int hold;

/*					event.intvalue = links[i].id; */
/*					event.type = BRIGHTON_UNLINK; */
/*					brightonParamChange(synth->win, 0, OUTPUT_START+ i, &event); */
					hold = oselect;
					event.type = BRIGHTON_PARAMCHANGE;
					event.value = 0;
					brightonParamChange(synth->win, 0, OUTPUT_START+ i, &event);
					oselect = hold;
				}
			}

			synth->mem.param[OUTPUT_START + oselect] = o;
			links[oselect].input = o;
			/*
			 * Need to request the drawing of a link. We need to send a message
			 * that the library will interpret as a connection request.
			 * All we have is brighton param change. We should consider sending
			 * one message with each key click on an output, and another on
			 * each input - this is the only way we can get two panel numbers
			 * across, something we will eventually require.
				brightonParamChange(synth->win, 1, INPUT_START + oselect,
					&event);
			 * Subtraction of 1 is due to offset differences
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 100, oselect, o-1);
			event.type = BRIGHTON_LINK;
			event.intvalue = INPUT_START + o;
			links[oselect].id =
				brightonParamChange(synth->win, 0, OUTPUT_START + oselect,
					&event);
		} else {
			/*
			 * See if this input is connected, if so clear it.
			 */
			for (i = 0; i < ARP_OUTPUTS; i++)
			{
				if ((links[i].input > 0)
					&& (links[i].input == o))
				{
					/*
					 * What I need to do is send an event to the output device
					 * turning it off.
					 */
					event.type = BRIGHTON_PARAMCHANGE;
					event.value = 0;
					brightonParamChange(synth->win, 0, OUTPUT_START+i, &event);
				}
			}
		}

		oselect = -1;
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
arp2600Init(brightonWindow *win)
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

	printf("Initialise the arp2600 link to bristol: %p\n", synth->win);

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
	{
		/*
		 * This sets up the input and output code.
		 */
		if (i >= MEM_START) {
			synth->dispatch[i].controller = 0;
		} else if (i >= INPUT_START) {
			/* Inputs */
			synth->dispatch[i].controller = 2;
			synth->dispatch[i].operator = i - ACTIVE_DEVS;
			synth->dispatch[i].routine = (synthRoutine) arp2600IOSelect;
		} else if (i >= OUTPUT_START) {
			/* Outputs */
			synth->dispatch[i].controller = 1;
			synth->dispatch[i].operator = i - ACTIVE_DEVS + ARP_OUTPUTS;
			synth->dispatch[i].routine = (synthRoutine) arp2600IOSelect;
		} else
			synth->dispatch[i].routine = arp2600MidiSendMsg;
	}

	/* Env and Ringmod */
	dispatch[0].controller = 11;
	dispatch[0].operator = 0;
	dispatch[1].controller = 8;
	dispatch[1].operator = 0;
	dispatch[2].controller = 8;
	dispatch[2].operator = 1;
	/* VCO1 */
	dispatch[3].controller = 0;
	dispatch[3].operator = 1;
	/* 4 is KBD control */
	dispatch[4].controller = 126;
	dispatch[4].operator = 3;
	dispatch[5].controller = 103;
	dispatch[5].operator = 4;
	dispatch[6].controller = 103;
	dispatch[6].operator = 5;
	dispatch[7].controller = 103;
	dispatch[7].operator = 6;
	dispatch[8].controller = 1; /* Sync NEXT osc to this on */
	dispatch[8].operator = 7;
	dispatch[9].controller = 0;
	dispatch[9].operator = 2;
	dispatch[10].controller = 0;
	dispatch[10].operator = 10;
	/* VCO2 */
	dispatch[11].controller = 1; /* Transpose */
	dispatch[11].operator = 1;
	/* 12 is KDB control */
	dispatch[12].controller = 126;
	dispatch[12].operator = 4;
	dispatch[13].controller = 103;
	dispatch[13].operator = 8;
	dispatch[14].controller = 103;
	dispatch[14].operator = 9;
	dispatch[15].controller = 103;
	dispatch[15].operator = 10;
	dispatch[16].controller = 103; /* Gain PWM of square wave. */
	dispatch[16].operator = 11;
	dispatch[17].controller = 1;
	dispatch[17].operator = 2;
	dispatch[18].controller = 1;
	dispatch[18].operator = 10;
	dispatch[19].controller = 1;
	dispatch[19].operator = 0;
	/* VCO3 */
	dispatch[20].controller = 2;
	dispatch[20].operator = 1;
	/* 21 is KDB control */
	dispatch[21].controller = 126;
	dispatch[21].operator = 5;
	dispatch[22].controller = 103;
	dispatch[22].operator = 13;
	dispatch[23].controller = 103;
	dispatch[23].operator = 14;
	dispatch[24].controller = 103;
	dispatch[24].operator = 15;
	dispatch[25].controller = 103;
	dispatch[25].operator = 16;
	dispatch[26].controller = 2;
	dispatch[26].operator = 2;
	dispatch[27].controller = 2;
	dispatch[27].operator = 10;
	dispatch[28].controller = 2;
	dispatch[28].operator = 0;
	/* Mixer into VCF */
	dispatch[29].controller = 103;
	dispatch[29].operator = 17;
	dispatch[30].controller = 103;
	dispatch[30].operator = 18;
	dispatch[31].controller = 103;
	dispatch[31].operator = 19;
	dispatch[32].controller = 103;
	dispatch[32].operator = 20;
	dispatch[33].controller = 103;
	dispatch[33].operator = 21;
	/*
	dispatch[29].routine = dispatch[30].routine = dispatch[31].routine
		= dispatch[32].routine = dispatch[33].routine
			= (synthRoutine) arp2600FilterMix;
	*/

	/* VCF mods */
	dispatch[34].controller = 103;
	dispatch[34].operator = 22;
	dispatch[35].controller = 103;
	dispatch[35].operator = 23;
	dispatch[36].controller = 103;
	dispatch[36].operator = 24;
	/* VCF Params */
	dispatch[37].controller = 3;
	dispatch[37].operator = 0;
	dispatch[38].controller = 3;
	dispatch[38].operator = 1;
	/* Env Params */
	dispatch[39].controller = 4;
	dispatch[39].operator = 0;
	dispatch[40].controller = 4;
	dispatch[40].operator = 1;
	dispatch[41].controller = 4;
	dispatch[41].operator = 2;
	dispatch[42].controller = 4;
	dispatch[42].operator = 3;
	/* AR */
	dispatch[43].controller = 7;
	dispatch[43].operator = 0;
	dispatch[44].controller = 7;
	dispatch[44].operator = 3;
	/* Trigger. */
	dispatch[45].controller = 126;
	dispatch[45].operator = 7;
	/* AMP Params */
	dispatch[46].controller = 103;
	dispatch[46].operator = 29;
	dispatch[47].controller = 103;
	dispatch[47].operator = 30;
	dispatch[48].controller = 103;
	dispatch[48].operator = 31;
	dispatch[49].controller = 103;
	dispatch[49].operator = 32;
	/* Mixer */
	dispatch[50].controller = 103;
	dispatch[50].operator = 33;
	dispatch[51].controller = 103;
	dispatch[51].operator = 34;

	/* FX levels */
	dispatch[52].controller = 99;
	dispatch[52].operator = 4;
	dispatch[53].controller = 99;
	dispatch[53].operator = 5;

	/* Four dimension D controls - 13 */
	dispatch[54].controller = 13;
	dispatch[54].operator = 0;
	dispatch[55].controller = 13;
	dispatch[55].operator = 1;
	dispatch[56].controller = 13;
	dispatch[56].operator = 2;
	dispatch[57].controller = 13;
	dispatch[57].operator = 3;
	/* Reverb controls */
	dispatch[58].controller = 99;
	dispatch[58].operator = 0;
	dispatch[59].controller = 99;
	dispatch[59].operator = 1;
	dispatch[60].controller = 99;
	dispatch[60].operator = 2;
	dispatch[61].controller = 99;
	dispatch[61].operator = 3;
	dispatch[62].controller = 99;
	dispatch[62].operator = 60;
	dispatch[62].routine = (synthRoutine) arp2600ManualStart;

	/* Noise */
	dispatch[63].controller = 6;
	dispatch[63].operator = 2;
	dispatch[64].controller = 6;
	dispatch[64].operator = 0;
	/* LFO stuff */
	dispatch[65].controller = 126;
	dispatch[65].operator = 6;
	dispatch[66].controller = 9;
	dispatch[66].operator = 0;
	dispatch[67].controller = 9;
	dispatch[67].operator = 1;

	/* Master Volume and Pan - both should become GM2 option MARK */
	dispatch[68].controller = 126;
	dispatch[68].operator = 11;
	dispatch[68].routine = (synthRoutine) arp2600Volume;
	dispatch[69].controller = 126;
	dispatch[69].operator = 2;

	/* Preamplifier */
	dispatch[70].controller = 126;
	dispatch[70].operator = 10;

	/* Glide */
	dispatch[71].controller = 126;
	dispatch[71].operator = 0;

	/* Voltage processing */
	dispatch[72].controller = 103;
	dispatch[72].operator = 44;
	dispatch[73].controller = 103;
	dispatch[73].operator = 45;
	dispatch[74].controller = 103;
	dispatch[74].operator = 46;
	dispatch[75].controller = 12;
	dispatch[75].operator = 0;

	dispatch[76].controller = 126;
	dispatch[76].operator = 12;

	dispatch[MEM_START +0].routine = dispatch[MEM_START +1].routine =
		dispatch[MEM_START +2].routine = dispatch[MEM_START +3].routine =
		dispatch[MEM_START +4].routine = dispatch[MEM_START +5].routine =
		dispatch[MEM_START +6].routine = dispatch[MEM_START +7].routine =
		dispatch[MEM_START +8].routine = dispatch[MEM_START +9].routine =
		dispatch[MEM_START +10].routine = dispatch[MEM_START +11].routine =
		dispatch[MEM_START +12].routine = dispatch[MEM_START +13].routine
		= (synthRoutine) arp2600Memory;

	dispatch[MEM_START + 0].controller = 1;
	dispatch[MEM_START + 1].controller = 2;
	dispatch[MEM_START + 2].operator = 0;
	dispatch[MEM_START + 3].operator = 1;
	dispatch[MEM_START + 4].operator = 2;
	dispatch[MEM_START + 5].operator = 3;
	dispatch[MEM_START + 6].operator = 4;
	dispatch[MEM_START + 7].operator = 5;
	dispatch[MEM_START + 8].operator = 6;
	dispatch[MEM_START + 9].operator = 7;
	dispatch[MEM_START + 10].operator = 8;
	dispatch[MEM_START + 11].operator = 9;
	dispatch[MEM_START + 12].controller = 3;
	dispatch[MEM_START + 13].controller = 4;

	dispatch[MIDI_START].controller = 1;
	dispatch[MIDI_START + 1].controller = 2;
	dispatch[MIDI_START].routine = dispatch[MIDI_START + 1].routine
		= (synthRoutine) arp2600Midi;

	dispatch[198].controller = 126;
	dispatch[198].operator = 11;
	dispatch[198].routine = (synthRoutine) arp2600Volume;

	/*
	 * Need to specify env gain fixed, filter mod on, osc waveform.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 102, 0, 0); /* clear patch */
	/* No filter kbd tracking - we emulate it */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 7, 4096);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 8, 100);
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1, 1);
	/* AR Env params */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 1, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 4, 16383);
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 16383); */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 16383); */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 4, 16383); */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 6, 16383); */
	/* Request rooney filter for lag operator. */
	bristolMidiSendMsg(global.controlfd, synth->sid, 12, 4, 3);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
arp2600Configure(brightonWindow* win)
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
	synth->keypanel = synth->keypanel2 = -1;
	synth->transpose = 36;

	synth->bank = 0;
	event.value = 1;

	arp2600LoadMemory(synth, "arp2600", 0,
		synth->location, synth->mem.active, 0, 0);
	/*
	 * This needs to be fixed. If we load a memory it can override the active
	 * device count. Perhaps for eventual release of the software this is not
	 * an issue though - the active count and memories are then set in stone.
	 * If they do not match then we should back out, but it is a bit on the
	 * late side now. Should be put into the loadMem routines.
	 */
	synth->mem.active = ACTIVE_DEVS;

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_FLOAT;
	event.value = 0.8;
	brightonParamChange(synth->win, 0, 198, &event);

	/*
	 * Touch a key on/off
	 */
/*	bristolMidiSendMsg(global.controlfd, synth->midichannel, */
/*		BRISTOL_EVENT_KEYON, 0, 10 + synth->transpose); */
/*	bristolMidiSendMsg(global.controlfd, synth->midichannel, */
/*		BRISTOL_EVENT_KEYOFF, 0, 10 + synth->transpose); */
	configureGlobals(synth);

	synth->loadMemory = (loadRoutine) arp2600LoadMemory;

	return(0);
}

