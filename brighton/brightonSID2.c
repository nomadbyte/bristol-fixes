
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

#include "brightonSID2.h"

static int sid2Init();
static int sid2Configure();
static int sid2Callback(brightonWindow *, int, int, float);
static int sid2ModCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);
static int sid2KeyCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define SYNTH_NAME synth->resources->name

#define DEVICE_COUNT 254
#define ACTIVE_DEVS 229
#define MEM_START (ACTIVE_DEVS + 8) /* 8 are for globals */

#define KEY_PANEL 1
#define MODS_PANEL 2
#define DISPLAY_DEV (DEVICE_COUNT - 1)

static int dc, mbh = 0;

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
 * This example is for a sid2Bristol type synth interface.
 */

/*
 * GUI needs the following:
 *
 * 	Modes:
 *
 * 		Mono
 * 		Poly-H
 * 		Poly-V
 * 		Voice ALL/1/2/3/4/5
 *
 * 	Voice-3 PolyMod section to include LFO option, touch, destination.
 *
 * 		PolyMod needs:
 *
 * 			On/off		button (No)
 * 			LFO			button
 * 			Tracking	button (LFO frequency)
 * 			Tracking	button (ENV gain)
 * 			Tracking	button (Voice gain)
 * 			Legato		button
 * 			PPM			button
 *
 * 			OSC Routing - 6 bus volumes, touch
 * 			ENV Routing - 6 bus volumes, touch
 * 			Prod Routing- 6 bus volumes, touch
 *
 * 	LFO-1
 *
 * 		Remove noise
 * 		Stack waveforms
 * 		Legato Env
 *
 * 	LFO-2 MonoMods -> S&H. Product.
 *
 * 	No Arpeggiator
 *
 * 	Add in analogue parameters:
 * 		S/N, Detune, DC/Bias, Leakage, tune/trans, volume.
 * 		perhaps leave out DC Bias?
 *
 * 	Filter: cutoff, res, pole mix, volume+touch, pan.
 *
 * Want to have 6 mod busses. The mono mods will be routed to them, the poly
 * mods also, then the busses can be directed to destinations with routing
 * buttons. Internally these should be mod routes per chip, presentation may
 * be distinct - PPM switch:
 * 	PPM off - MMods to all, PolyMods to self
 * 	PPM on - MMods to any, PolyMods to any
 *
 * Add stereo panning for voices. Add global volume, global tuning. There are
 * also some voice controls I need to add.
 *
 * FIX:
 * 	Velocity opion per voice - could be in PolyMods?
 * 	Keyboard ranging per voice (and arpeggiate?)
 * 	Pan - per voice or Mono/All use slightly different positioning +/-
 *
 * To send so many voice and routing messages will probably need to either
 * notify engine of which voice is under control or use different controller
 * id from 100 for example.
 */

/* SID2-1 */
#define V1R1 50
#define V1R2 125
#define V1R3 250

#define VD1 36
#define VD2 22
#define VD3 2

#define VBW1 28
#define VBW2 10
#define VBW3 45

#define VBH1 30
#define VBH2 70
#define VBH3 235
#define VBH4 200

#define V1C1 25
#define V1C2 (V1C1 + VD1)
#define V1C3 (V1C2 + VD1)
#define V1C4 (V1C3 + VD1)
#define V1C1b (V1C1 + VD3)
#define V1C2b (V1C2 + VD3)
#define V1C3b (V1C3 + VD3)
#define V1C4b (V1C4 + VD3)
#define V1C5 (V1C4 + VD1 + 15)
#define V1C6 (V1C5 + VD2)
#define V1C7 (V1C6 + VD2)
#define V1C8 (V1C7 + VD2)

#define V2C1 (V1C8 + VD2 + VD1)
#define V2C2 (V2C1 + VD1)
#define V2C3 (V2C2 + VD1)
#define V2C4 (V2C3 + VD1)
#define V2C1b (V2C1 + VD3)
#define V2C2b (V2C2 + VD3)
#define V2C3b (V2C3 + VD3)
#define V2C4b (V2C4 + VD3)
#define V2C5 (V2C4 + VD1 + 15)
#define V2C6 (V2C5 + VD2)
#define V2C7 (V2C6 + VD2)
#define V2C8 (V2C7 + VD2)

#define V3C1 (V2C8 + VD2 + VD1)
#define V3C2 (V3C1 + VD1)
#define V3C3 (V3C2 + VD1)
#define V3C4 (V3C3 + VD1)
#define V3C1b (V3C1 + VD3)
#define V3C2b (V3C2 + VD3)
#define V3C3b (V3C3 + VD3)
#define V3C4b (V3C4 + VD3)
#define V3C5 (V3C4 + VD1 + 15)
#define V3C6 (V3C5 + VD2)
#define V3C7 (V3C6 + VD2)
#define V3C8 (V3C7 + VD2)

#define FC1 (V3C8 + VD2 + VD1)
#define FC2 (FC1 + VD1)
#define FC3 (FC2 + VD1)
#define FC1b (FC1 + VD3)
#define FC2b (FC2 + VD3)
#define FC3b (FC3 + VD3)

/* SID2-2 Mods */
#define S2R0 460
#define S2R1 (S2R0 + 30) //630
#define S2R2 (S2R1 + 30) //675
#define S2R3 (S2R2 + 50) //750
#define S2R4 (S2R3 + 50) //825

/* SID2-2 Mods */
#define S2R5 760
#define S2R6 (S2R5 + 30) //630
#define S2R7 (S2R6 + 30) //675
#define S2R8 (S2R7 + 50) //750
#define S2R9 (S2R8 + 50) //825

#define S2C0 700
#define S2C1 20
#define S2C1b (S2C1 + VD1/2)
#define S2C2 (S2C1 + VD1)
#define S2C2b (S2C1 + VD1 + VD1/2)
#define S2C3 (S2C2 + VD1)
#define S2C3b (S2C3 + VD1/2)
#define S2C4 (S2C3 + VD1)
#define S2C5 (S2C4 + VD1 + VD2)
#define S2C6 (S2C5 + VD1)

#define S2C7 (S2C5 + VD1)
#define S2C8 (S2C7 + VD1)
#define S2C8b (S2C8 + VD1)
#define S2C9 (S2C8b + VD2)
#define S2C10 (S2C9 + VD2)
#define S2C11 (S2C10 + VD2)
#define S2C12 (S2C11 + VD2)

#define S2C13 (S2C12 + VD1 + 10)
#define S2C14 (S2C13 + VD1/2)
#define S2C15 (S2C13 + VD1)

#define S2C16 730 //(S2C15 + VD1 + VD2)
#define S2C17 (S2C16 + VD1)
#define S2C18 (S2C17 + VD1)
#define S2C19 (S2C18 + VD1)
#define S2C20 (S2C19 + VD1)
#define S2C21 (S2C20 + VD1)
#define S2C22 (S2C21 + VD1)
#define S2C23 (S2C22 + VD1 + 10)

#define S3C0 730
#define S3C1 (S3C0 + 40)
#define S3C2 (S3C1 + 45)
#define S3C3 (S3C2 + 45)
#define S3C4 (S3C3 + 40)
#define S3C5 (S3C4 + 50)

#define SRD1 34

#define SRC0 450
#define SRC1 (SRC0 + SRD1)
#define SRC2 (SRC1 + SRD1)
#define SRC3 (SRC2 + SRD1)
#define SRC4 (SRC3 + SRD1)
#define SRC5 (SRC4 + SRD1)
#define SRC6 (SRC5 + SRD1)

#define SRR0 (S2R5-35)
#define SRR1 (S2R5)
#define SRR2 (S2R5+35)
#define SRR3 (S2R5+70)
#define SRR4 (S2R5+105)
#define SRR5 (S2R5+140)
#define SRR6 (S2R5+175)

static
brightonLocations modwheel[5] = {
	{"", BRIGHTON_MODWHEEL, 0, 0, 390, 1000, 0, 1, 0,
		"bitmaps/knobs/modwheel.xpm", 0,
		BRIGHTON_HSCALE|BRIGHTON_NOSHADOW|BRIGHTON_NOTCH},
	{"", 2, 500, 50, 500, 130, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, 500, 306, 500, 130, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, 500, 562, 500, 130, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, 500, 820, 500, 130, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
};

static
brightonLocations locations[DEVICE_COUNT] = {
	/*
	 * Three voices with same parameterisation, roughly
	 * Tri/Ramp/Square/Noise Buttons
	 * RM/SYNC/Mute/Routing(Multi global) Buttons
	 * PW/Tune/transpose(/Glide?) - Pots
	 * Att/Dec/Sust/Rel - Sliders
	 */
	{"", 2, V1C1, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V1C2, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V1C3, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V1C4, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 0, V1C1b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"", 0, V1C2b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 
		BRIGHTON_NOTCH},
	{"", 0, V1C3b, V1R2, VBW3, VBH2, 0, 24,0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"", 0, V1C4b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"", 2, V1C1, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V1C2, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V1C3, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V1C4, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 1, V1C5, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V1C6, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V1C7, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V1C8, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},

	/* Voice 2 - 16 */
	{"", 2, V2C1, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V2C2, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V2C3, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V2C4, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 0, V2C1b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0,0},
	{"", 0, V2C2b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 
		BRIGHTON_NOTCH},
	{"", 0, V2C3b, V1R2, VBW3, VBH2, 0, 24, 0, "bitmaps/knobs/knobred.xpm",0,0},
	{"", 0, V2C4b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0,0},
	{"", 2, V2C1, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V2C2, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V2C3, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V2C4, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 1, V2C5, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V2C6, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V2C7, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V2C8, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},

	/* Voice 3 - 32 */
	{"", 2, V3C1, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V3C2, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V3C3, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V3C4, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 0, V3C1b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0,0},
	{"", 0, V3C2b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 
		BRIGHTON_NOTCH},
	{"", 0, V3C3b, V1R2, VBW3, VBH2, 0, 24, 0, "bitmaps/knobs/knobred.xpm",0,0},
	{"", 0, V3C4b, V1R2, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0,0},
	{"", 2, V3C1, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V3C2, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V3C3, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, V3C4, V1R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 1, V3C5, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V3C6, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V3C7, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, V3C8, V1R1, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},

	/* Filter - 48 */
	{"", 2, FC1, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, FC2, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, FC3, V1R1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 0, FC1b, V1R2-24, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm",
		0, 0},
	{"", 0, FC2b, V1R2-24, VBW3, VBH2, 0, 15,0, "bitmaps/knobs/knobred.xpm",
		0, 0},
	{"", 0, FC3b, V1R2-24, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm",
		0, 0},
	{"", 2, FC3, V1R3-50, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, FC3, V1R3+4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* Pan and VoiceVolume */
	{"", 0,  FC1b, V1R3 - 45, VBW3, VBH2, 0, 15, 0,
		"bitmaps/knobs/knobyellow.xpm", 0, 0},
	{"", 0,  FC2b, V1R3 - 45, VBW3, VBH2, 0, 15, 0,
		"bitmaps/knobs/knobyellow.xpm", 0, 0},

	/* 58 - Polymods */
	{"", 2, SRC0, S2R0-80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC1, S2R0-80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC2, S2R0-80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC3, S2R0-80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC4, S2R0-80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC5, S2R0-80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC6, S2R0-80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 65 */
	{"", 0, SRC0, S2R0-0, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC1, S2R0-0, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC2, S2R0-0, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC3, S2R0-0, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC4, S2R0-0, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC5, S2R0-0, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 2, SRC6, S2R0+20, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 72 */
	{"", 0, SRC0, S2R2+23, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC1, S2R2+23, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC2, S2R2+23, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC3, S2R2+23, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC4, S2R2+23, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC5, S2R2+23, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 2, SRC6, S2R2+43, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 79 */
	{"", 0, SRC0, S2R4, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC1, S2R4, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC2, S2R4, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC3, S2R4, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC4, S2R4, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, SRC5, S2R4, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 2, SRC6, S2R4+20, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* 86 - mod routing change to 6 lots of 7 destinations */
	{"", 2, SRC0, SRR1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC0, SRR2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC0, SRR3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC0, SRR4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC0, SRR5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC0, SRR6, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, SRC1, SRR1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC1, SRR2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC1, SRR3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC1, SRR4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC1, SRR5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC1, SRR6, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, SRC2, SRR1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC2, SRR2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC2, SRR3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC2, SRR4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC2, SRR5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC2, SRR6, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, SRC3, SRR1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC3, SRR2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC3, SRR3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC3, SRR4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC3, SRR5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC3, SRR6, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, SRC4, SRR1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC4, SRR2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC4, SRR3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC4, SRR4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC4, SRR5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC4, SRR6, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, SRC5, SRR1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC5, SRR2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC5, SRR3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC5, SRR4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC5, SRR5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC5, SRR6, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, SRC6, SRR1, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC6, SRR2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC6, SRR3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC6, SRR4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC6, SRR5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, SRC6, SRR6, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/*
	 * MODS: 135
	 *
	 */
	/*
	 * Key Mode:
	 * 	First three radio: Mono, Poly-H, Poly-V
	 * 	6 voice selectors: All/1/2/3/4/5
	 */
	{"", 2, S3C0, S2R0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S3C0, S2R0+40, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S3C0, S2R0+80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, S3C0, S2R0+160, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	{"", 2, S3C1, S2R0+0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S3C1, S2R0+40, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S3C1, S2R0+80, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S3C1, S2R0+120, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S3C1, S2R0+160, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* Mods - 144 */
	{"", 0, S2C1, S2R1, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, S2C3, S2R1, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 2, S2C1, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C2, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C3, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C4, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 150 - bus selections */
	{"", 2, S2C5, S2R0+0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R0+35, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R0+70, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R0+105, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R0+140, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R0+175, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 156 - Mod Touch control */
	{"", 2, S2C2, S2R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C4, S2R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 158 - Env mods */
	{"", 2, S2C7, S2R0+0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R0+35, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R0+70, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R0+105, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R0+140, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R0+175, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 166 - Mod Env */
	{"", 1, S2C8b, S2R0, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, S2C9, S2R0, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, S2C10, S2R0, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, S2C11, S2R0, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, S2C12, S2R0, VBW2, VBH4, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 2, S2C8, S2R0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C8, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 173 - 2nd Mods */
	{"", 0, S2C1, S2R6, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 0, S2C3, S2R6, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"", 2, S2C1, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C2, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C3, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C4, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 179 */
	{"", 2, S2C5, S2R5+00, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R5+35, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R5+70, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R5+105, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R5+140, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C5, S2R5+175, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 185 - Mod Touch control */
	{"", 2, S2C2, S2R5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C2, S2R7, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C4, S2R5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C4, S2R7, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 189 - Env mods */
	{"", 2, S2C7, S2R5+00, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R5+35, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R5+70, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R5+105, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R5+140, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C7, S2R5+175, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 195 - Mod Env */
	{"", 1, S2C8b, S2R5, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, S2C9, S2R5, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, S2C10, S2R5, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, S2C11, S2R5, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 1, S2C12, S2R5, VBW2, VBH4, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"", 2, S2C8, S2R5, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 2, S2C8, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 202 - Dummies for voice /modes, etc */
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 2, 0, 0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/*
	 * This will shadow the version of the memory. Existing memories will have
	 * a zero in here (0.40) ones saved by this release (0.40.2) will have a
	 * 1 in there, this allows the GUI to adjust the keymode parameter.
	 */
#warning this index has changed
#define M_VERS 223
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/* ??? - These shadow the mod panel */
#warning this index has changed too
#define M_SHAD 224
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/* 229 - Globals - not saved in memories */
	{"", 0, S3C2, S2R0, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm",
		0, 0},
	{"", 0, S3C2, S2R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm",
		0, 0},
	{"", 0, S3C3, S2R0, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm",
		0, 0},
	{"", 0, S3C3, S2R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm",
		0, 0},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/* 236 - Master Vol */
	{"", 0, S3C4, S2R1+20, VBW3, VBH2+30, 0, 1, 0,
		"bitmaps/knobs/knob.xpm", 0, 0},

	/* Memories 237 - first 8 digits */
	{"", 2, S2C17, S2R8, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C18, S2R8, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C19, S2R8, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C20, S2R8, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},

	{"", 2, S2C17, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C18, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C19, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C20, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},

	/* Then load/save */
	{"", 2, S2C16, S2R8, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C16, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},

	/* Then Bank, down, up, find */
	{"", 2, S2C21, S2R8, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C22, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C22, S2R8, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C21, S2R9, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},

	/* Midi */
	{"", 2, S3C5, S2R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S3C5, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},

	/* Done */

	{"", 3, S2C17, S2R6, 175, 45, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}

};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 * Hm, the bit-1 was black, and the bit 99 was in various builds including a 
 * white one, but also a black one and a black one with wood panels. I would
 * like the black one with wood panels, so that will have to be the bit-1, the
 * bit-99 will be white with thin metal panels.
 */
brightonApp sid2App = {
	"melbourne",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal1.xpm",
	//"bitmaps/textures/p8b.xpm",
	BRIGHTON_STRETCH,
	sid2Init,
	sid2Configure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	1000, 450, 0, 0,
	3, /* panels */
	{
		{
			"Sid800",
			"bitmaps/blueprints/sid2.xpm",
			"bitmaps/textures/metal1.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			sid2Callback,
			25, 16, 950, 720,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/nkbg.xpm",
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			sid2KeyCallback,
			90, 760, 893, 230,
			KEY_COUNT_5OCTAVE,
			keysprofile2
		},
		{
			"mods",
			"bitmaps/buttons/blue.xpm",
			"bitmaps/textures/metal1.xpm",
			0,
			0,
			0,
			sid2ModCallback,
			32, 780, 50, 170,
			5,
			modwheel
		},
	}
};

/*
 * We really want to just use one midi channel and let the midi library decide
 * that we have multiple synths on the channel with their own split points.
 * The lower layer should define the midi channel, split point and transpose 
 * of upper layer.
 */
static int
sid2KeyCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (global.libtest)
		return(0);

	/*
	 * So we have a single key event and two MIDI channels. Just send the
	 * event on both channels, no need to be difficult about it since if this
	 * was a split configuration the library filters out the events.
	 */
	if (value) {
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
	} else {
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);
	}

	return(0);
}

/*
 * At this point we have loaded a memory so we need to send those actual new
 * parameters to the engine. This is an issue for MIDI program load, perhaps
 * we should consid2er dual load above memory 74 as per the original?
 *
 * The path of least resistance here is to scan through the the memory table
 * incrementing the input selector and delivering the memory value to the
 * data entry pot.....
 */
static void
loadMemoryShim(guiSynth *synth)
{
	brightonEvent event;

	/* We need to force some exclusion on LFO mods and Mode */
	synth->dispatch[173].other1 =
		synth->mem.param[173] != 0? 0:
			synth->mem.param[174] != 0? 1:
				synth->mem.param[175] != 0? 2:
						synth->mem.param[176] != 0? 3: 0;

	/* We need to force some exclusion on LFO mods and Mode */
	synth->dispatch[146].other1 =
		synth->mem.param[146] != 0? 0:
			synth->mem.param[147] != 0? 1:
				synth->mem.param[148] != 0? 2:
						synth->mem.param[149] != 0? 3: 0;

		synth->dispatch[135].other1 =
			synth->mem.param[135] != 0? 0:
				synth->mem.param[136] != 0? 1:
					synth->mem.param[137] != 0? 2: 0;

	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 5,
		synth->dispatch[135].other1);

	/* Set up the trigger options */

	/* Then we can look at the mod panel options */
	event.type = BRISTOL_FLOAT;
	event.value = synth->mem.param[111];
	brightonParamChange(synth->win, MODS_PANEL, 1, &event);
	event.value = synth->mem.param[112];
	brightonParamChange(synth->win, MODS_PANEL, 2, &event);
	event.value = synth->mem.param[113];
	brightonParamChange(synth->win, MODS_PANEL, 3, &event);
	event.value = synth->mem.param[114];
	brightonParamChange(synth->win, MODS_PANEL, 4, &event);

	/* And we should enforce mod waveform */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 66,
		synth->mem.param[146] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 67,
		synth->mem.param[147] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 68,
		synth->mem.param[149] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 69,
		synth->mem.param[149] == 0?0:1);
	if (synth->mem.param[146] != 0)
		bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 66, 1);

	/* And we should enforce mod waveform */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 66,
		synth->mem.param[173] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 67,
		synth->mem.param[174] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 68,
		synth->mem.param[175] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 69,
		synth->mem.param[176] == 0?0:1);
	if (synth->mem.param[173] != 0)
		bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 66, 1);

	/* See if we have to force memory variables */
	if (synth->mem.param[70] != 0)
	{
		int i;

		for (i = 0; i < 20; i++)
		{
			event.value = synth->mem.param[i];
			brightonParamChange(synth->win, 0, i, &event);
			brightonParamChange(synth->win, 0, i + 20, &event);
			brightonParamChange(synth->win, 0, i + 40, &event);
		}
	}
}

static void
loadMemoryMidiShim(guiSynth *synth, int from)
{
	loadMemory(synth, SYNTH_NAME, 0, synth->bank * 10 + from,
		synth->mem.active, 0, BRISTOL_FORCE);
	loadMemoryShim(synth);
}

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	switch(controller)
	{
		case MIDI_PROGRAM:
			if (synth->mem.param[87] == 0)
				return(0);
			/*
			 * We should accept 0..74 as lower layer and above that as dual
			 * loading requests.
			 */
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemoryMidiShim(synth, synth->location);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
sid2MidiNull(void *synth, int fd, int chan, int c, int o, int v)
{
	if (global.libtest)
		printf("This is a null callback on %i id %i\n", c, o);

	return(0);
}

static int vexclude = 0;

/*
 * Typically this should just do what sid2MidiShim does, send the parameter to
 * the target however in mode Poly-1 all voices need to have the same set of
 * parameters. This really means the GUI needs to replicate the parameter
 * changes all voice params. There are many ways to do this, the coolest is
 * actually to redistribute the requests, ganging the controls.
 */
static void
sid2VoiceShim(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int off1 = 20, off2 = 40;

	if (synth->mem.param[70] != 0)
	{
		if (vexclude)
			return;

		event.value = synth->mem.param[o - 10];

		vexclude = 1;

		/* Find out what parameters we need to use */
		if (o >= 50)
		{
			off1 = -40;
			off2 = -20;
		} else if (o >= 30) {
			off1 = -20;
			off2 = 20;
		}

		/* Update the displays */
		brightonParamChange(synth->win, synth->panel, o - 10 + off1, &event);
		brightonParamChange(synth->win, synth->panel, o - 10 + off2, &event);

		vexclude = 0;

		/* Send the messages for the other voices */
		bristolMidiSendMsg(fd, synth->sid2, c, o + off1, v);
		bristolMidiSendMsg(fd, synth->sid2, c, o + off2, v);
	}
	bristolMidiSendMsg(fd, synth->sid2, c, o, v);
}

static void
sid2MidiShim(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid2, c, o, v);
}

static void
sid2LFOWave(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	event.value = 1.0;
	event.type = BRISTOL_FLOAT;

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[146].other2)
	{
		synth->dispatch[146].other2 = 0;
		return;
	}

	/*
	 * We want to make these into memory buttons. To do so we need to
	 * know what the last active button was, and deactivate its 
	 * display, then send any message which represents the most
	 * recently configured value. Since this is a memory button we do
	 * not have much issue with the message, but we are concerned with
	 * the display.
	 */
	if (synth->dispatch[146].other1 != -1)
	{
		synth->dispatch[146].other2 = 1;

		if (synth->dispatch[146].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[146].other1 + 146, &event);
		bristolMidiSendMsg(fd, synth->sid2, 126,
			66 + synth->dispatch[146].other1, 0);
	}
	synth->dispatch[146].other1 = o;

	bristolMidiSendMsg(fd, synth->sid2, 126, 66 + o, 1);
}

static int mode = -1;

static void
sid2Mode(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int i, sv;

	event.value = 1.0;
	event.type = BRISTOL_FLOAT;

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[135].other2)
	{
		synth->dispatch[135].other2 = 0;
		return;
	}

	/*
	 * We want to make these into memory buttons. To do so we need to
	 * know what the last active button was, and deactivate its 
	 * display, then send any message which represents the most
	 * recently configured value. Since this is a memory button we do
	 * not have much issue with the message, but we are concerned with
	 * the display.
	 */
	if (synth->dispatch[135].other1 != -1)
	{
		synth->dispatch[135].other2 = 1;

		if (synth->dispatch[135].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[135].other1 + 135, &event);
	}
	synth->dispatch[135].other1 = o;

	bristolMidiSendMsg(fd, synth->sid2, 126, 5, o);

	/*
	 * This code would sync all the voice parameters as we enter Poly-1 however
	 * that is always desirable. We should either sync the engine parameters
	 * to voice-3 entering Poly-1 and reset them to GUI parameters on exit,
	 * or do nothing.
	 */
	if (synth->mem.param[70] != 0)
	{
		/*
		 * Sync voices in engine
		 */
		if (mode != 1)
			for (i = 0; i < 20; i++)
			{
				if (sid2App.resources[0].devlocn[i].to == 1)
					sv = synth->mem.param[i+40] * C_RANGE_MIN_1;
				else
					sv = synth->mem.param[i+40];

				bristolMidiSendMsg(fd, synth->sid2, 126, i + 10, sv);
				bristolMidiSendMsg(fd, synth->sid2, 126, i + 30, sv);
			}
	} else {
		if (mode == 1)
			for (i = 0; i < 60; i++)
			{
				event.value = synth->mem.param[i];
				brightonParamChange(synth->win, synth->panel, i, &event);
			}
	}

	mode = o;
}

static void
sid2Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
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

					if (loadMemory(synth, SYNTH_NAME, 0,
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
				if (loadMemory(synth, SYNTH_NAME, 0,
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
			if (loadMemory(synth, SYNTH_NAME, 0,
				synth->bank * 10 + synth->location,
				synth->mem.active, 0, BRISTOL_FORCE) < 0)
				displayText(synth, "FREE MEM",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
			else {
				loadMemoryShim(synth);
				displayText(synth, "PROGRAM",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
			}
			synth->flags &= ~BANK_SELECT;

			/* Doubleclick on load will toggle debugging */
			if (brightonDoubleClick(dc) != 0)
				bristolMidiSendMsg(fd, synth->sid2, 126, 4, 1);

			break;
		case 2:
			if (synth->bank < 1)
				synth->bank = 1;
			if (synth->location == 0)
				synth->location = 1;
			if (brightonDoubleClick(dc) != 0)
			{
				synth->mem.param[M_VERS] = 1.0;
				saveMemory(synth, SYNTH_NAME, 0,
					synth->bank * 10 + synth->location, 0);
				displayText(synth, "PROGRAM",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
				synth->flags &= ~BANK_SELECT;
			}
			break;
		case 3:
			if (synth->flags & BANK_SELECT) {
				synth->flags &= ~BANK_SELECT;
				if (loadMemory(synth, SYNTH_NAME, 0,
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
			while (loadMemory(synth, SYNTH_NAME, 0, bank * 10 + location,
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
			loadMemory(synth, SYNTH_NAME, 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_FORCE);
			loadMemoryShim(synth);
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			break;
		case 5:
			if (++location > 8) {
				location = 1;
				if (++bank > 88)
					bank = 1;
			}
			while (loadMemory(synth, SYNTH_NAME, 0, bank * 10 + location,
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
			loadMemory(synth, SYNTH_NAME, 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_FORCE);
			loadMemoryShim(synth);
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

			while (loadMemory(synth, SYNTH_NAME, 0, bank * 10 + location,
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

			if (loadMemory(synth, SYNTH_NAME, 0, bank * 10 + location,
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
}

static int
sid2ModCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	event.type = BRISTOL_FLOAT;
	synth->mem.param[M_SHAD + index] = value;

	switch (index) {
		case 0:
			/* Wheel - we don't send pitch, just mod */
			bristolMidiControl(global.controlfd, synth->midichannel,
				0, 1, ((int) (value * (C_RANGE_MIN_1 - 1))) >> 7);
			break;
		case 1:
			/* Pitch select - these should actually just flag the engine */
			bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 100,
				value == 0?0:1);
			break;
		case 2:
			/* LFO Select - these should actually just flag the engine */
			bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 98,
				value == 0?0:1);
			if ((synth->mem.param[M_SHAD + index] = value) == 0)
				return(0);

			bristolMidiControl(global.controlfd, synth->midichannel, 0, 1,
				((int) (synth->mem.param[M_SHAD] * (C_RANGE_MIN_1 - 1))) >> 7);
			break;
		case 3:
			/* ENV Select - these should actually just flag the engine */
			bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 99,
				value == 0?0:1);
			if ((synth->mem.param[M_SHAD + index] = value) == 0)
				return(0);

			bristolMidiControl(global.controlfd, synth->midichannel, 0, 1,
				((int) (synth->mem.param[M_SHAD] * (C_RANGE_MIN_1 - 1))) >> 7);
			break;
		case 4:
			/* Center spring */
			if (value == 0)
				win->app->resources[panel].devlocn[0].flags &= ~BRIGHTON_CENTER;
			else {
				win->app->resources[panel].devlocn[0].flags |= BRIGHTON_CENTER;
				/* And centre the control */
				event.value = 0.5;
				brightonParamChange(synth->win, panel, 0, &event);
			}
			break;
	};

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
sid2Callback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (global.libtest)
		printf("sid2Callback(%i, %i, %f)\n", panel, index, value);

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if ((index >= 111) && (index <= 114))
		return(sid2ModCallback(win, MODS_PANEL, index - M_SHAD, value));

	if (sid2App.resources[0].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	synth->mem.param[index] = value;

//	if (index < 100)
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid2,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

	return(0);
}

static void
sid2MidiChannel(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if ((c == 0) && (++synth->midichannel > 15))
		synth->midichannel = 15;
	else if ((c == 1) && (--synth->midichannel < 0))
		synth->midichannel = 0;

	displayText(synth, "MIDI CHAN", synth->midichannel + 1, DISPLAY_DEV);

	if (global.libtest)
		return;

	bristolMidiSendMsg(global.controlfd, synth->sid2,
		127, 0, BRISTOL_MIDICHANNEL|synth->midichannel);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
sid2Init(brightonWindow *win)
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

	printf("Initialise the sid2 link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
	synth->dispatch = (dispatcher *)
		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher));
	dispatch = synth->dispatch;

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets (actually implements TCP unfortunately).
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		if ((synth->sid2 = initConnection(&global, synth)) < 0)
			return(-1);
	}

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		synth->dispatch[i].routine = (synthRoutine) sid2MidiNull;
		synth->dispatch[i].controller = i;
		synth->dispatch[i].operator = i;
	}

	/* Voice 1 */
	dispatch[0].controller = 126;
	dispatch[0].operator = 10;
	dispatch[0].routine = (synthRoutine) sid2VoiceShim;
	dispatch[1].controller = 126;
	dispatch[1].operator = 11;
	dispatch[1].routine = (synthRoutine) sid2VoiceShim;
	dispatch[2].controller = 126;
	dispatch[2].operator = 12;
	dispatch[2].routine = (synthRoutine) sid2VoiceShim;
	dispatch[3].controller = 126;
	dispatch[3].operator = 13;
	dispatch[3].routine = (synthRoutine) sid2VoiceShim;
	/* PW, tune, transpose, glide */
	dispatch[4].controller = 126;
	dispatch[4].operator = 14;
	dispatch[4].routine = (synthRoutine) sid2VoiceShim;
	dispatch[5].controller = 126;
	dispatch[5].operator = 15;
	dispatch[5].routine = (synthRoutine) sid2VoiceShim;
	dispatch[6].controller = 126;
	dispatch[6].operator = 16;
	dispatch[6].routine = (synthRoutine) sid2VoiceShim;
	dispatch[7].controller = 126;
	dispatch[7].operator = 17;
	dispatch[7].routine = (synthRoutine) sid2VoiceShim;
	/* RM/sync/mute/route */
	dispatch[8].controller = 126;
	dispatch[8].operator = 18;
	dispatch[8].routine = (synthRoutine) sid2VoiceShim;
	dispatch[9].controller = 126;
	dispatch[9].operator = 19;
	dispatch[9].routine = (synthRoutine) sid2VoiceShim;
	dispatch[10].controller = 126;
	dispatch[10].operator = 20;
	dispatch[10].routine = (synthRoutine) sid2VoiceShim;
	dispatch[11].controller = 126;
	dispatch[11].operator = 21;
	dispatch[11].routine = (synthRoutine) sid2VoiceShim;
	/* Env */
	dispatch[12].controller = 126;
	dispatch[12].operator = 22;
	dispatch[12].routine = (synthRoutine) sid2VoiceShim;
	dispatch[13].controller = 126;
	dispatch[13].operator = 23;
	dispatch[13].routine = (synthRoutine) sid2VoiceShim;
	dispatch[14].controller = 126;
	dispatch[14].operator = 24;
	dispatch[14].routine = (synthRoutine) sid2VoiceShim;
	dispatch[15].controller = 126;
	dispatch[15].operator = 25;
	dispatch[15].routine = (synthRoutine) sid2VoiceShim;

	/* Voice 2 */
	dispatch[16].controller = 126;
	dispatch[16].operator = 30;
	dispatch[16].routine = (synthRoutine) sid2VoiceShim;
	dispatch[17].controller = 126;
	dispatch[17].operator = 31;
	dispatch[17].routine = (synthRoutine) sid2VoiceShim;
	dispatch[18].controller = 126;
	dispatch[18].operator = 32;
	dispatch[18].routine = (synthRoutine) sid2VoiceShim;
	dispatch[19].controller = 126;
	dispatch[19].operator = 33;
	dispatch[19].routine = (synthRoutine) sid2VoiceShim;
	/* PW, tune, transpose, glide */
	dispatch[20].controller = 126;
	dispatch[20].operator = 34;
	dispatch[20].routine = (synthRoutine) sid2VoiceShim;
	dispatch[21].controller = 126;
	dispatch[21].operator = 35;
	dispatch[21].routine = (synthRoutine) sid2VoiceShim;
	dispatch[22].controller = 126;
	dispatch[22].operator = 36;
	dispatch[22].routine = (synthRoutine) sid2VoiceShim;
	dispatch[23].controller = 126;
	dispatch[23].operator = 37;
	dispatch[23].routine = (synthRoutine) sid2VoiceShim;
	/* RM/sync/mute/route */
	dispatch[24].controller = 126;
	dispatch[24].operator = 38;
	dispatch[24].routine = (synthRoutine) sid2VoiceShim;
	dispatch[25].controller = 126;
	dispatch[25].operator = 39;
	dispatch[25].routine = (synthRoutine) sid2VoiceShim;
	dispatch[26].controller = 126;
	dispatch[26].operator = 40;
	dispatch[26].routine = (synthRoutine) sid2VoiceShim;
	dispatch[27].controller = 126;
	dispatch[27].operator = 41;
	dispatch[27].routine = (synthRoutine) sid2VoiceShim;
	/* Env */
	dispatch[28].controller = 126;
	dispatch[28].operator = 42;
	dispatch[28].routine = (synthRoutine) sid2VoiceShim;
	dispatch[29].controller = 126;
	dispatch[29].operator = 43;
	dispatch[29].routine = (synthRoutine) sid2VoiceShim;
	dispatch[30].controller = 126;
	dispatch[30].operator = 44;
	dispatch[30].routine = (synthRoutine) sid2VoiceShim;
	dispatch[31].controller = 126;
	dispatch[31].operator = 45;
	dispatch[31].routine = (synthRoutine) sid2VoiceShim;

	/* Voice 3 */
	dispatch[32].controller = 126;
	dispatch[32].operator = 50;
	dispatch[32].routine = (synthRoutine) sid2VoiceShim;
	dispatch[33].controller = 126;
	dispatch[33].operator = 51;
	dispatch[33].routine = (synthRoutine) sid2VoiceShim;
	dispatch[34].controller = 126;
	dispatch[34].operator = 52;
	dispatch[34].routine = (synthRoutine) sid2VoiceShim;
	dispatch[35].controller = 126;
	dispatch[35].operator = 53;
	dispatch[35].routine = (synthRoutine) sid2VoiceShim;
	/* PW, tune, transpose, glide */
	dispatch[36].controller = 126;
	dispatch[36].operator = 54;
	dispatch[36].routine = (synthRoutine) sid2VoiceShim;
	dispatch[37].controller = 126;
	dispatch[37].operator = 55;
	dispatch[37].routine = (synthRoutine) sid2VoiceShim;
	dispatch[38].controller = 126;
	dispatch[38].operator = 56;
	dispatch[38].routine = (synthRoutine) sid2VoiceShim;
	dispatch[49].controller = 126;
	dispatch[39].operator = 57;
	dispatch[39].routine = (synthRoutine) sid2VoiceShim;
	/* RM/sync/mute/route */
	dispatch[40].controller = 126;
	dispatch[40].operator = 58;
	dispatch[40].routine = (synthRoutine) sid2VoiceShim;
	dispatch[41].controller = 126;
	dispatch[41].operator = 59;
	dispatch[41].routine = (synthRoutine) sid2VoiceShim;
	dispatch[42].controller = 126;
	dispatch[42].operator = 60;
	dispatch[42].routine = (synthRoutine) sid2VoiceShim;
	dispatch[43].controller = 126;
	dispatch[43].operator = 61;
	dispatch[43].routine = (synthRoutine) sid2VoiceShim;
	/* Env */
	dispatch[44].controller = 126;
	dispatch[44].operator = 62;
	dispatch[44].routine = (synthRoutine) sid2VoiceShim;
	dispatch[45].controller = 126;
	dispatch[45].operator = 63;
	dispatch[45].routine = (synthRoutine) sid2VoiceShim;
	dispatch[46].controller = 126;
	dispatch[46].operator = 64;
	dispatch[46].routine = (synthRoutine) sid2VoiceShim;
	dispatch[47].controller = 126;
	dispatch[47].operator = 65;
	dispatch[47].routine = (synthRoutine) sid2VoiceShim;

	/* Filter */
	dispatch[48].controller = 126;
	dispatch[48].operator = 70;
	dispatch[48].routine = (synthRoutine) sid2MidiShim;
	dispatch[49].controller = 126;
	dispatch[49].operator = 71;
	dispatch[49].routine = (synthRoutine) sid2MidiShim;
	dispatch[50].controller = 126;
	dispatch[50].operator = 72;
	dispatch[50].routine = (synthRoutine) sid2MidiShim;
	/* Cutoff/res/OBmix */
	dispatch[51].controller = 126;
	dispatch[51].operator = 73;
	dispatch[51].routine = (synthRoutine) sid2MidiShim;
	dispatch[52].controller = 126;
	dispatch[52].operator = 74;
	dispatch[52].routine = (synthRoutine) sid2MidiShim;
	dispatch[53].controller = 126;
	dispatch[53].operator = 75;
	dispatch[53].routine = (synthRoutine) sid2MidiShim;
	dispatch[54].controller = 126;
	dispatch[54].operator = 76;
	dispatch[54].routine = (synthRoutine) sid2MidiShim;

	/* Multi and master vol */
	dispatch[67].controller = 126;
	dispatch[67].operator = 2;
	dispatch[67].routine = (synthRoutine) sid2MidiShim;
	dispatch[68].controller = 126;
	dispatch[68].operator = 3;
	dispatch[68].routine = (synthRoutine) sid2MidiShim;

	dispatch[135].operator = 0;
	dispatch[136].operator = 1;
	dispatch[137].operator = 2;
	dispatch[135].routine = dispatch[136].routine = dispatch[137].routine
		= (synthRoutine) sid2Mode;

	/* Mod freq, gain and waveform */
	dispatch[74].controller = 126;
	dispatch[74].operator = 77;
	dispatch[74].routine = (synthRoutine) sid2MidiShim;
	dispatch[75].controller = 126;
	dispatch[75].operator = 78;
	dispatch[75].routine = (synthRoutine) sid2MidiShim;
	dispatch[146].operator = 0;
	dispatch[147].operator = 1;
	dispatch[148].operator = 2;
	dispatch[149].operator = 3;
	dispatch[146].routine = dispatch[147].routine = dispatch[148].routine =
		dispatch[149].routine = (synthRoutine) sid2LFOWave;

	/* Then we have the routing buttons, TBD */
	dispatch[80].controller = 126;
	dispatch[80].operator = 79;
	dispatch[80].routine = (synthRoutine) sid2MidiShim;
	dispatch[81].controller = 126;
	dispatch[81].operator = 80;
	dispatch[81].routine = (synthRoutine) sid2MidiShim;
	dispatch[82].controller = 126;
	dispatch[82].operator = 81;
	dispatch[82].routine = (synthRoutine) sid2MidiShim;
	dispatch[83].controller = 126;
	dispatch[83].operator = 82;
	dispatch[83].routine = (synthRoutine) sid2MidiShim;
	dispatch[84].controller = 126;
	dispatch[84].operator = 83;
	dispatch[84].routine = (synthRoutine) sid2MidiShim;
	dispatch[85].controller = 126;
	dispatch[85].operator = 84;
	dispatch[85].routine = (synthRoutine) sid2MidiShim;
	dispatch[86].controller = 126;
	dispatch[86].operator = 85;
	dispatch[86].routine = (synthRoutine) sid2MidiShim;
	dispatch[87].controller = 126;
	dispatch[87].operator = 101;
	dispatch[87].routine = (synthRoutine) sid2MidiShim;
	dispatch[88].controller = 126;
	dispatch[88].operator = 102;
	dispatch[88].routine = (synthRoutine) sid2MidiShim;

	dispatch[95].controller = 126;
	dispatch[95].operator = 89;
	dispatch[95].routine = (synthRoutine) sid2MidiShim;
	dispatch[96].controller = 126;
	dispatch[96].operator = 90;
	dispatch[96].routine = (synthRoutine) sid2MidiShim;
	dispatch[97].controller = 126;
	dispatch[97].operator = 91;
	dispatch[97].routine = (synthRoutine) sid2MidiShim;
	dispatch[98].controller = 126;
	dispatch[98].operator = 92;
	dispatch[98].routine = (synthRoutine) sid2MidiShim;
	dispatch[99].controller = 126;
	dispatch[99].operator = 86;
	dispatch[99].routine = (synthRoutine) sid2MidiShim;
	dispatch[100].controller = 126;
	dispatch[100].operator = 87;
	dispatch[100].routine = (synthRoutine) sid2MidiShim;
	dispatch[101].controller = 126;
	dispatch[101].operator = 88;
	dispatch[101].routine = (synthRoutine) sid2MidiShim;

	/* Env mod */
	dispatch[102].controller = 126;
	dispatch[102].operator = 93;
	dispatch[102].routine = (synthRoutine) sid2MidiShim;
	dispatch[103].controller = 126;
	dispatch[103].operator = 94;
	dispatch[103].routine = (synthRoutine) sid2MidiShim;
	dispatch[104].controller = 126;
	dispatch[104].operator = 95;
	dispatch[104].routine = (synthRoutine) sid2MidiShim;
	dispatch[105].controller = 126;
	dispatch[105].operator = 96;
	dispatch[105].routine = (synthRoutine) sid2MidiShim;
	dispatch[106].controller = 126;
	dispatch[106].operator = 97;
	dispatch[106].routine = (synthRoutine) sid2MidiShim;
	dispatch[107].controller = 126;
	dispatch[107].operator = 103;
	dispatch[107].routine = (synthRoutine) sid2MidiShim;

	/* Arpeggiator */
	dispatch[115].controller = 126;
	dispatch[115].operator = 6;
	dispatch[115].routine = (synthRoutine) sid2MidiShim;
	dispatch[116].controller = 126;
	dispatch[116].operator = 7;
	dispatch[116].routine = (synthRoutine) sid2MidiShim;
	dispatch[117].controller = 126;
	dispatch[117].operator = 8;
	dispatch[117].routine = (synthRoutine) sid2MidiShim;

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
		dispatch[MEM_START + 13].routine = (synthRoutine) sid2Memory;

	dispatch[132].controller = 0;
	dispatch[133].controller = 1;
	dispatch[132].routine = dispatch[133].routine
		= (synthRoutine) sid2MidiChannel;

	/* Osc-1 settings */

	/* Env settings */

	/* Filter */

	dispatch[MEM_START].other1 = 1;
	dispatch[135].other1 = 0;
	dispatch[146].other1 = 0;
	dispatch[173].other1 = 0;

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
sid2Configure(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	if (synth == 0)
	{
		printf("problems going operational\n");
		return(-1);
	}

	if (synth->location == 0) {
		synth->bank = 1;
		synth->location = 1;
	} else if (synth->location > 0) {
		mbh = (synth->location / 100) * 100;
		if ((synth->bank = synth->location / 10) > 10)
			synth->bank = synth->bank % 10;
		if (((synth->location = synth->location % 10) == 0)
			|| (synth->location > 8))
			synth->location = 1;
	}

	if (synth->flags & OPERATIONAL)
		return(0);

	printf("going operational\n");

	synth->flags |= OPERATIONAL;
	synth->keypanel = 1;
	synth->keypanel2 = -1;
	synth->transpose = 24;

	brightonPut(win,
		"bitmaps/blueprints/melbourneshade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);

	/* Push some mod panel defaults */
	event.type = BRISTOL_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, MODS_PANEL, 1, &event);
	brightonParamChange(synth->win, MODS_PANEL, 2, &event);
	brightonParamChange(synth->win, MODS_PANEL, 4, &event);
	configureGlobals(synth);

	/*
	 * First memory location
	 */
	event.value = 1.0;
	brightonParamChange(synth->win, 0, MEM_START + synth->location-1, &event);
	brightonParamChange(synth->win, 0, 135, &event);
	brightonParamChange(synth->win, 0, 146, &event);
	brightonParamChange(synth->win, 0, 173, &event);

	loadMemory(synth, SYNTH_NAME, 0, synth->bank * 10 + synth->location,
		synth->mem.active, 0, BRISTOL_FORCE);
	loadMemoryShim(synth);

	event.value = 0.505;
	brightonParamChange(synth->win, MODS_PANEL, 0, &event);

	synth->dispatch[MEM_START].other1 = synth->location;

	dc = brightonGetDCTimer(1000000);

	return(0);
}

