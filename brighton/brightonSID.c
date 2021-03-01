
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

static int sidInit();
static int sidConfigure();
static int sidCallback(brightonWindow *, int, int, float);
static int sidModCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);
static int sidKeyCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define SYNTH_NAME synth->resources->name

#define DEVICE_COUNT 135
#define ACTIVE_DEVS 118
#define MEM_START ACTIVE_DEVS

#define ENTRY_POT 18


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
 * This example is for a sidBristol type synth interface.
 */

/*
 * The GUI should give three voices with all the OSC, ENV, RM, SYNC options
 * and then separately the test, multi and detune.
 *
 * Tri/Ramp/Square/Noise
 * PW/Tune/transpose(/Glide?)
 * RM/SYNC/Mute/Routing
 * Att/Dec/Sust/Rel
 *
 * osc3 -> PWM osc 1/2
 *
 * Filter:
 * freq, res, pole
 * HP/BP/LP inc 24dB
 *
 * should add in option * of osc3 and env3 to filter cutoff.
 * Multi
 * 
 * Must have S/N, leakage, global tuning. DC bias? Reset?.
 *
 * Voice options to include:
 *
 * 	mono (with detune then between voices in emulator)
 * 	three voice poly-1: all play same sound
 * 	three voice poly-2: all have their own sound
 * 	two voice poly, voice 3 arpeggio - argeggiate all other held notes
 *
 * Arpeggiator to have rate, retrig and wavescanning, rates down to X samples.
 *
 * We should consider using two SID, the second one could be used just for
 * an extra Env and Osc(LF) to modulate the first one, later look at more 
 * voices or other layer options?
 *
 * The second SID will have voices 1 + 2 in TEST state, no output will be taken
 * just clocked, no filter selections made to all reduce overhead - we only
 * want env3 and osc3 outputs from the registers anyway.
 *
 * MODS:
 *
 * Env will have adsr plus routing
 * LFO will have waveform, rate (0.1-10Hz) and routing
 *
 * Destinations can be
 * 	vibrato, PW -> v1/v2/v3.
 *	wah
 *
 * Notes:
 *
 * 	Lost multi, vol - replaced them. FIXED
 * 	Lost detune in multi FIXED CHECK
 *
 *	S/N ratio is far too high (value levels needs reducing) FIXED
 *	Filter res is not working FIXED
 *	Mixed 12/24 filters not working FIXED
 *	Mods are mixed up re lfo/env to what parameters FIXED
 *	Mods are overloaded FIXED
 *	Mods default to noise FIXED
 *	Noise MOD does not work - no signal output from SID-2. FIXED
 *	Tune/Transpose are not proactive Fixed
 *	Glide not implemented. FIXED
 *	Pitch bend implemented. FIXED, but leaves notes shifted when disabled.
 *	RM damaged FIXED
 *	Memories using S&H do not get LFO Mod loaded correctly. FIXED
 *
 *	Key assign modes finished. FIXED
 *		Need to clear out keys when changing modes in engine. FIXED
 *	Arpeggio finished, needs more testing, esp trig FIXED
 *
 * Implemented two new arpeggiation modes, Arpeg-1 and Arpeg-2. Both will split
 * the keyboard at note 52 then Arpeg-1 will apply on the top half, the bottom
 * half can play duophonic baselines. Arpeg-2 will apply to the bottom half
 * and the upper half can play duophonic leads.
 *
 * Review volume levels direct/filter and Multi.
 *
 * Linux audio drivers are broken, stutter with window changes.
 *
 * May need more Poly modes:
 *	lowest key, highest key, arpeggiate rest.
 *
 * Future release
 *
 *	need Mod env retrig option - legato?
 *	need Aud env retrig option - legato?
 *	access to analogue parameters from GUI
 *	filter tracking keyboard - whick key?
 *	velocity to mod, filter, pw
 *	arpeg direction
 */

/* SID-1 */
#define V1R1 50
#define V1R2 120
#define V1R3 240
#define V1R4 430

#define VD1 36
#define VD2 22
#define VD3 2

#define VBW1 28
#define VBW2 10
#define VBW3 45

#define VBH1 45
#define VBH2 110
#define VBH3 370
#define VBH4 290

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

/* SID-2 Mods */
#define S2R0 650
#define S2R1 (S2R0 + 30) //630
#define S2R2 (S2R1 + 45) //675
#define S2R3 (S2R2 + 75) //750
#define S2R4 (S2R3 + 75) //825

#define S2C0 20
#define S2C1 (S2C0 + VD1 + VD1 + 5)
#define S2C1b (S2C1 + VD1/2)
#define S2C2 (S2C1 + VD1)
#define S2C2b (S2C1 + VD1 + VD1/2)
#define S2C3 (S2C2 + VD1)
#define S2C3b (S2C3 + VD1/2)
#define S2C4 (S2C3 + VD1)
#define S2C5 (S2C4 + VD1 + VD2)
#define S2C6 (S2C5 + VD1)

#define S2C7 (S2C6 + VD1 + VD1)
#define S2C8 (S2C7 + VD1)
#define S2C8b (S2C8 + VD1 + VD2)
#define S2C9 (S2C8b + VD2)
#define S2C10 (S2C9 + VD2)
#define S2C11 (S2C10 + VD2)
#define S2C12 (S2C11 + VD2)

#define S2C13 (S2C12 + VD1 + 10)
#define S2C14 (S2C13 + VD1/2)
#define S2C15 (S2C13 + VD1)

#define S2C16 (S2C15 + VD1 + VD2)
#define S2C17 (S2C16 + VD1)
#define S2C18 (S2C17 + VD1)
#define S2C19 (S2C18 + VD1)
#define S2C20 (S2C19 + VD1)
#define S2C21 (S2C20 + VD1)
#define S2C22 (S2C21 + VD1)
#define S2C23 (S2C22 + VD1)

static
brightonLocations modwheel[5] = {
	{"", BRIGHTON_MODWHEEL, 0, 0, 380, 1000, 0, 1, 0,
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
	{"V1-Noise", 2, V1C1, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V1-Tri", 2, V1C2, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V1-Ramp", 2, V1C3, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V1-Square", 2, V1C4, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V1-PW", 0, V1C1b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"V1-Tune", 0, V1C2b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 
		BRIGHTON_NOTCH},
	{"V1-Transpose", 0, V1C3b, V1R3, VBW3, VBH2, 0, 24,0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"V1-Glide", 0, V1C4b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"V1-RM", 2, V1C1, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V1-3-1-Sync", 2, V1C2, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V1-Mute", 2, V1C3, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V1-Filt", 2, V1C4, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V1-Attack", 1, V1C5, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V1-Decay", 1, V1C6, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V1-Sustain", 1, V1C7, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V1-Release", 1, V1C8, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	/* Dummies */
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/* Voice 2 - 20 */
	{"V2-Noise", 2, V2C1, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V2-Tri", 2, V2C2, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V2-Ramp", 2, V2C3, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V2-Square", 2, V2C4, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V2-PW", 0, V2C1b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0,0},
	{"V2-Tune", 0, V2C2b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 
		BRIGHTON_NOTCH},
	{"V2-Transpose", 0, V2C3b, V1R3, VBW3, VBH2, 0, 24, 0, "bitmaps/knobs/knobred.xpm",0,0},
	{"V2-Glide", 0, V2C4b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0,0},
	{"V2-RM-1-2", 2, V2C1, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V2-Sync", 2, V2C2, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V2-Mute", 2, V2C3, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V2-Filt", 2, V2C4, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V2-Attack", 1, V2C5, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V2-Decay", 1, V2C6, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V2-Sustain", 1, V2C7, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V2-Release", 1, V2C8, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	/* Dummies */
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/* Voice 3 - 40 */
	{"V3-Noise", 2, V3C1, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V3-Tri", 2, V3C2, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V3-Ramp", 2, V3C3, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V3-Square", 2, V3C4, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V3-PW", 0, V3C1b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"V3-Tune", 0, V3C2b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 
		BRIGHTON_NOTCH},
	{"V3-Transpose", 0, V3C3b, V1R3, VBW3, VBH2, 0, 24, 0, "bitmaps/knobs/knobred.xpm", 0,0},
	{"V3-Glide", 0, V3C4b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"V3-RM-2-3", 2, V3C1, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V3-Sync", 2, V3C2, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V3-Mute", 2, V3C3, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V3-Filt", 2, V3C4, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"V3-Attack", 1, V3C5, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V3-Decay", 1, V3C6, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V3-Sustain", 1, V3C7, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"V3-Release", 1, V3C8, V1R2, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	/* Dummies */
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/* Filter - 60 */
	{"VCF-HP", 2, FC1, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"VCF-BP", 2, FC2, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"VCF-LP", 2, FC3, V1R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"VCF-Cutoff", 0, FC1b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"VCF-Resonance", 0, FC2b, V1R3, VBW3, VBH2, 0, 15,0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"VCF-Mix", 0, FC3b, V1R3, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"VCF-LPF24", 2, FC3, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* VCF Multi and MasterVol */
	{"VCF-Multi", 2, FC1, V1R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"MasterVolume", 0,  S2C23+8, S2R0 - 50, VBW3, VBH2, 0, 15, 0,
		"bitmaps/knobs/knobyellow.xpm", 0, 0},

	/*
	 * MODS: 69 (was 70)
	 *
	 * Env will have adsr plus routing
	 * LFO will have waveform, rate (0.1-10Hz) and routing
	 *
	 * Destinations can be
	 * 	vibrato, PW -> v1/v2/v3.
	 *	wah
	 */
	/* Key Mode */
	{"Mode Mono", 2, S2C0-3, S2R0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"Mode Poly1", 2, S2C0-3, S2R0+60, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"Mode Poly2", 2, S2C0-3, S2R0+120, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"Mode Arpeg1", 2, S2C0-3, S2R0+180, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"Mode Arpeg2", 2, S2C0-3, S2R0+240, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* Mods - 76 */
	{"LFO-Rate", 0, S2C1b+2, S2R1, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"LFO-Depth", 0, S2C3b+2, S2R1, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm",
		0, 0},
	{"???1", 2, S2C1, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"???2", 2, S2C2, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"???3", 2, S2C3, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"???4", 2, S2C4, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 80 */
	{"LFO-PW-V1", 2, S2C5, S2R0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"LFO-PW-V2", 2, S2C5, S2R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"LFO-PW-V3", 2, S2C5, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"LFO-FM-V1", 2, S2C6, S2R0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"LFO-FM-V2", 2, S2C6, S2R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"LFO-FM-V3", 2, S2C6, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"LFO-FM-Filt", 2, S2C6, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	/* Mod Touch control */
	{"LFO-F-Touch", 2, S2C1b, S2R0 - 70, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"LFO-G-Touch", 2, S2C3b, S2R0 - 70, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
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

	/* Env mods 95 */
	{"ModEnv-FM-V1", 2, S2C7, S2R0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"ModEnv-FM-V2", 2, S2C7, S2R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"ModEnv-FM-V3", 2, S2C7, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"ModEnv-FM-Filt", 2, S2C7, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"ModEnv-PW-V1", 2, S2C8, S2R0, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"ModEnv-PW-V2", 2, S2C8, S2R2, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"ModEnv-PW-V3", 2, S2C8, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* 102 - Mod Env */
	{"ModEnv-Attack", 1, S2C8b, S2R0, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"ModEnv-Sustain", 1, S2C9, S2R0, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"ModEnv-Decay", 1, S2C10, S2R0, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"ModEnv-Release", 1, S2C11, S2R0, VBW2, VBH4, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"ModEnv-Gain", 1, S2C12, S2R0, VBW2, VBH4, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, 0},
	{"ModEnv-Touch", 2, S2C11+13, S2R0-70, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/*
	 * This will shadow the version of the memory. Existing memories will have
	 * a zero in here (0.40) ones saved by this release (0.40.2) will have a
	 * 1 in there, this allows the GUI to adjust the keymode parameter.
	 */
	{"", 1, 0, 0, VBW2, VBH3, 0, 15, 0, "bitmaps/buttons/polywhiteV.xpm",
		0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},

	/* 110 - These shadow the mod panel */
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

	{"Arpeg-Rate", 0, S2C14, S2R1, VBW3, VBH2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm",0,0},
	{"Arpeg-Trig", 2, S2C13, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},
	{"Arpeg-Scan", 2, S2C15, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", 0},

	/* Memories 118 - first 8 digits */
	{"", 2, S2C17, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C18, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C19, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C20, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},

	{"", 2, S2C17, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C18, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C19, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},
	{"", 2, S2C20, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbongreen.xpm", 0},

	/* Then load/save */
	{"", 2, S2C16, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C16, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},

	/* Then Bank, down, up, find */
	{"", 2, S2C21, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C22, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C22, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C21, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, S2C23 + 6, S2R3, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, S2C23 + 6, S2R4, VBW1, VBH1, 0, 1, 0, "bitmaps/buttons/sidb.xpm",
		"bitmaps/buttons/sidbon.xpm", BRIGHTON_CHECKBUTTON},

	/* Done */

	{"", 3, S2C17, S2R1, 175, 60, 0, 1, 0, 0,
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
brightonApp sidApp = {
	"sidney",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal1.xpm",
	//"bitmaps/textures/p8b.xpm",
	BRIGHTON_STRETCH,
	sidInit,
	sidConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	1000, 330, 0, 0,
	3, /* panels */
	{
		{
			"Sid800",
			"bitmaps/blueprints/sid.xpm",
			"bitmaps/textures/metal1.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			sidCallback,
			25, 29, 950, 620,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
//			"bitmaps/keys/kbg.xpm",
			"bitmaps/newkeys/nkbg.xpm",
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			sidKeyCallback,
			90, 680, 893, 318,
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
			sidModCallback,
			32, 695, 50, 220,
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
sidKeyCallback(brightonWindow *win, int panel, int index, float value)
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
 * we should consider dual load above memory 74 as per the original?
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
	synth->dispatch[76].other1 =
		synth->mem.param[76] != 0? 0:
			synth->mem.param[77] != 0? 1:
				synth->mem.param[78] != 0? 2:
						synth->mem.param[79] != 0? 3: 0;

	/* Key mode changed between releases, this recovers them */
	if (synth->mem.param[109] == 0) {
		int on;

		synth->dispatch[69].other1 =
			synth->mem.param[70] != 0? 0:
				synth->mem.param[71] != 0? 1:
					synth->mem.param[72] != 0? 2:
						synth->mem.param[73] != 0? 3: 0;
		on = 69 + synth->dispatch[69].other1;
		event.type = BRISTOL_FLOAT;
		event.value = 0;
		brightonParamChange(synth->win, 0,
			70 + synth->dispatch[69].other1, &event);
		event.value = 1;
		brightonParamChange(synth->win, 0, on, &event);
	} else
		synth->dispatch[69].other1 =
			synth->mem.param[69] != 0? 0:
				synth->mem.param[70] != 0? 1:
					synth->mem.param[71] != 0? 2:
						synth->mem.param[72] != 0? 3:
							synth->mem.param[73] != 0? 4: 0;

	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 5,
		synth->dispatch[69].other1);

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
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 66,
		synth->mem.param[76] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 67,
		synth->mem.param[77] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 68,
		synth->mem.param[78] == 0?0:1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 69,
		synth->mem.param[79] == 0?0:1);
	if (synth->mem.param[76] != 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 66, 1);

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

int
loadMemoryKeyShim(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	loadMemory(synth, SYNTH_NAME, 0, location,
		synth->mem.active, 0, BRISTOL_FORCE);
	loadMemoryShim(synth);
	return(0);
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
sidMidiNull(void *synth, int fd, int chan, int c, int o, int v)
{
	if (global.libtest)
		printf("This is a null callback on %i id %i\n", c, o);

	return(0);
}

static int vexclude = 0;

/*
 * Typically this should just do what sidMidiShim does, send the parameter to
 * the target however in mode Poly-1 all voices need to have the same set of
 * parameters. This really means the GUI needs to replicate the parameter
 * changes all voice params. There are many ways to do this, the coolest is
 * actually to redistribute the requests, ganging the controls.
 */
static void
sidVoiceShim(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
		bristolMidiSendMsg(fd, synth->sid, c, o + off1, v);
		bristolMidiSendMsg(fd, synth->sid, c, o + off2, v);
	}
	bristolMidiSendMsg(fd, synth->sid, c, o, v);
}

static void
sidMidiShim(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid, c, o, v);
}

static void
sidLFOWave(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	event.value = 1.0;
	event.type = BRISTOL_FLOAT;

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[76].other2)
	{
		synth->dispatch[76].other2 = 0;
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
	if (synth->dispatch[76].other1 != -1)
	{
		synth->dispatch[76].other2 = 1;

		if (synth->dispatch[76].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[76].other1 + 76, &event);
		bristolMidiSendMsg(fd, synth->sid, 126,
			66 + synth->dispatch[76].other1, 0);
	}
	synth->dispatch[76].other1 = o;

	bristolMidiSendMsg(fd, synth->sid, 126, 66 + o, 1);
}

static int mode = -1;

static void
sidMode(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int i, sv;

	event.value = 1.0;
	event.type = BRISTOL_FLOAT;

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[69].other2)
	{
		synth->dispatch[69].other2 = 0;
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
	if (synth->dispatch[69].other1 != -1)
	{
		synth->dispatch[69].other2 = 1;

		if (synth->dispatch[69].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[69].other1 + 69, &event);
	}
	synth->dispatch[69].other1 = o;

	bristolMidiSendMsg(fd, synth->sid, 126, 5, o);

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
				if (sidApp.resources[0].devlocn[i].to == 1)
					sv = synth->mem.param[i+40] * C_RANGE_MIN_1;
				else
					sv = synth->mem.param[i+40];

				bristolMidiSendMsg(fd, synth->sid, 126, i + 10, sv);
				bristolMidiSendMsg(fd, synth->sid, 126, i + 30, sv);
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
sidMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
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

			/*
			 * Doubleclick on load will toggle debugging
			if (brightonDoubleClick(dc) != 0)
				bristolMidiSendMsg(fd, synth->sid, 126, 4, 1);
			 */

			break;
		case 2:
			if (synth->bank < 1)
				synth->bank = 1;
			if (synth->location == 0)
				synth->location = 1;
			if (brightonDoubleClick(dc) != 0)
			{
				synth->mem.param[109] = 1.0;
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
sidModCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	event.type = BRISTOL_FLOAT;
	synth->mem.param[110 + index] = value;

	switch (index) {
		case 0:
			/* Wheel - we don't send pitch, just mod */
			bristolMidiControl(global.controlfd, synth->midichannel,
				0, 1, ((int) (value * (C_RANGE_MIN_1 - 1))) >> 7);
			break;
		case 1:
			/* Pitch select - these should actually just flag the engine */
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 100,
				value == 0?0:1);
			break;
		case 2:
			/* LFO Select - these should actually just flag the engine */
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 98,
				value == 0?0:1);
			if ((synth->mem.param[110 + index] = value) == 0)
				return(0);

			bristolMidiControl(global.controlfd, synth->midichannel, 0, 1,
				((int) (synth->mem.param[110] * (C_RANGE_MIN_1 - 1))) >> 7);
			break;
		case 3:
			/* ENV Select - these should actually just flag the engine */
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 99,
				value == 0?0:1);
			if ((synth->mem.param[110 + index] = value) == 0)
				return(0);

			bristolMidiControl(global.controlfd, synth->midichannel, 0, 1,
				((int) (synth->mem.param[110] * (C_RANGE_MIN_1 - 1))) >> 7);
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
sidCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (global.libtest)
		printf("sidCallback(%i, %i, %f)\n", panel, index, value);

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if ((index >= 111) && (index <= 114))
		return(sidModCallback(win, MODS_PANEL, index - 110, value));

	if (sidApp.resources[0].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	synth->mem.param[index] = value;

//	if (index < 100)
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

	return(0);
}

static void
sidMidiChannel(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if ((c == 0) && (++synth->midichannel > 15))
		synth->midichannel = 15;
	else if ((c == 1) && (--synth->midichannel < 0))
		synth->midichannel = 0;

	displayText(synth, "MIDI CHAN", synth->midichannel + 1, DISPLAY_DEV);

	if (global.libtest)
		return;

	bristolMidiSendMsg(global.controlfd, synth->sid,
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
sidInit(brightonWindow *win)
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

	printf("Initialise the sid link to bristol: %p\n", synth->win);

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
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);
	}

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		synth->dispatch[i].routine = (synthRoutine) sidMidiNull;
		synth->dispatch[i].controller = i;
		synth->dispatch[i].operator = i;
	}

	/* Voice 1 */
	dispatch[0].controller = 126;
	dispatch[0].operator = 10;
	dispatch[0].routine = (synthRoutine) sidVoiceShim;
	dispatch[1].controller = 126;
	dispatch[1].operator = 11;
	dispatch[1].routine = (synthRoutine) sidVoiceShim;
	dispatch[2].controller = 126;
	dispatch[2].operator = 12;
	dispatch[2].routine = (synthRoutine) sidVoiceShim;
	dispatch[3].controller = 126;
	dispatch[3].operator = 13;
	dispatch[3].routine = (synthRoutine) sidVoiceShim;
	/* PW, tune, transpose, glide */
	dispatch[4].controller = 126;
	dispatch[4].operator = 14;
	dispatch[4].routine = (synthRoutine) sidVoiceShim;
	dispatch[5].controller = 126;
	dispatch[5].operator = 15;
	dispatch[5].routine = (synthRoutine) sidVoiceShim;
	dispatch[6].controller = 126;
	dispatch[6].operator = 16;
	dispatch[6].routine = (synthRoutine) sidVoiceShim;
	dispatch[7].controller = 126;
	dispatch[7].operator = 17;
	dispatch[7].routine = (synthRoutine) sidVoiceShim;
	/* RM/sync/mute/route */
	dispatch[8].controller = 126;
	dispatch[8].operator = 18;
	dispatch[8].routine = (synthRoutine) sidVoiceShim;
	dispatch[9].controller = 126;
	dispatch[9].operator = 19;
	dispatch[9].routine = (synthRoutine) sidVoiceShim;
	dispatch[10].controller = 126;
	dispatch[10].operator = 20;
	dispatch[10].routine = (synthRoutine) sidVoiceShim;
	dispatch[11].controller = 126;
	dispatch[11].operator = 21;
	dispatch[11].routine = (synthRoutine) sidVoiceShim;
	/* Env */
	dispatch[12].controller = 126;
	dispatch[12].operator = 22;
	dispatch[12].routine = (synthRoutine) sidVoiceShim;
	dispatch[13].controller = 126;
	dispatch[13].operator = 23;
	dispatch[13].routine = (synthRoutine) sidVoiceShim;
	dispatch[14].controller = 126;
	dispatch[14].operator = 24;
	dispatch[14].routine = (synthRoutine) sidVoiceShim;
	dispatch[15].controller = 126;
	dispatch[15].operator = 25;
	dispatch[15].routine = (synthRoutine) sidVoiceShim;

	/* Voice 2 */
	dispatch[20].controller = 126;
	dispatch[20].operator = 30;
	dispatch[20].routine = (synthRoutine) sidVoiceShim;
	dispatch[21].controller = 126;
	dispatch[21].operator = 31;
	dispatch[21].routine = (synthRoutine) sidVoiceShim;
	dispatch[22].controller = 126;
	dispatch[22].operator = 32;
	dispatch[22].routine = (synthRoutine) sidVoiceShim;
	dispatch[23].controller = 126;
	dispatch[23].operator = 33;
	dispatch[23].routine = (synthRoutine) sidVoiceShim;
	/* PW, tune, transpose, glide */
	dispatch[24].controller = 126;
	dispatch[24].operator = 34;
	dispatch[24].routine = (synthRoutine) sidVoiceShim;
	dispatch[25].controller = 126;
	dispatch[25].operator = 35;
	dispatch[25].routine = (synthRoutine) sidVoiceShim;
	dispatch[26].controller = 126;
	dispatch[26].operator = 36;
	dispatch[26].routine = (synthRoutine) sidVoiceShim;
	dispatch[27].controller = 126;
	dispatch[27].operator = 37;
	dispatch[27].routine = (synthRoutine) sidVoiceShim;
	/* RM/sync/mute/route */
	dispatch[28].controller = 126;
	dispatch[28].operator = 38;
	dispatch[28].routine = (synthRoutine) sidVoiceShim;
	dispatch[29].controller = 126;
	dispatch[29].operator = 39;
	dispatch[29].routine = (synthRoutine) sidVoiceShim;
	dispatch[30].controller = 126;
	dispatch[30].operator = 40;
	dispatch[30].routine = (synthRoutine) sidVoiceShim;
	dispatch[31].controller = 126;
	dispatch[31].operator = 41;
	dispatch[31].routine = (synthRoutine) sidVoiceShim;
	/* Env */
	dispatch[32].controller = 126;
	dispatch[32].operator = 42;
	dispatch[32].routine = (synthRoutine) sidVoiceShim;
	dispatch[33].controller = 126;
	dispatch[33].operator = 43;
	dispatch[33].routine = (synthRoutine) sidVoiceShim;
	dispatch[34].controller = 126;
	dispatch[34].operator = 44;
	dispatch[34].routine = (synthRoutine) sidVoiceShim;
	dispatch[35].controller = 126;
	dispatch[35].operator = 45;
	dispatch[35].routine = (synthRoutine) sidVoiceShim;

	/* Voice 3 */
	dispatch[40].controller = 126;
	dispatch[40].operator = 50;
	dispatch[40].routine = (synthRoutine) sidVoiceShim;
	dispatch[41].controller = 126;
	dispatch[41].operator = 51;
	dispatch[41].routine = (synthRoutine) sidVoiceShim;
	dispatch[42].controller = 126;
	dispatch[42].operator = 52;
	dispatch[42].routine = (synthRoutine) sidVoiceShim;
	dispatch[43].controller = 126;
	dispatch[43].operator = 53;
	dispatch[43].routine = (synthRoutine) sidVoiceShim;
	/* PW, tune, transpose, glide */
	dispatch[44].controller = 126;
	dispatch[44].operator = 54;
	dispatch[44].routine = (synthRoutine) sidVoiceShim;
	dispatch[45].controller = 126;
	dispatch[45].operator = 55;
	dispatch[45].routine = (synthRoutine) sidVoiceShim;
	dispatch[46].controller = 126;
	dispatch[46].operator = 56;
	dispatch[46].routine = (synthRoutine) sidVoiceShim;
	dispatch[47].controller = 126;
	dispatch[47].operator = 57;
	dispatch[47].routine = (synthRoutine) sidVoiceShim;
	/* RM/sync/mute/route */
	dispatch[48].controller = 126;
	dispatch[48].operator = 58;
	dispatch[48].routine = (synthRoutine) sidVoiceShim;
	dispatch[49].controller = 126;
	dispatch[49].operator = 59;
	dispatch[49].routine = (synthRoutine) sidVoiceShim;
	dispatch[50].controller = 126;
	dispatch[50].operator = 60;
	dispatch[50].routine = (synthRoutine) sidVoiceShim;
	dispatch[51].controller = 126;
	dispatch[51].operator = 61;
	dispatch[51].routine = (synthRoutine) sidVoiceShim;
	/* Env */
	dispatch[52].controller = 126;
	dispatch[52].operator = 62;
	dispatch[52].routine = (synthRoutine) sidVoiceShim;
	dispatch[53].controller = 126;
	dispatch[53].operator = 63;
	dispatch[53].routine = (synthRoutine) sidVoiceShim;
	dispatch[54].controller = 126;
	dispatch[54].operator = 64;
	dispatch[54].routine = (synthRoutine) sidVoiceShim;
	dispatch[55].controller = 126;
	dispatch[55].operator = 65;
	dispatch[55].routine = (synthRoutine) sidVoiceShim;

	/* Filter */
	dispatch[60].controller = 126;
	dispatch[60].operator = 70;
	dispatch[60].routine = (synthRoutine) sidMidiShim;
	dispatch[61].controller = 126;
	dispatch[61].operator = 71;
	dispatch[61].routine = (synthRoutine) sidMidiShim;
	dispatch[62].controller = 126;
	dispatch[62].operator = 72;
	dispatch[62].routine = (synthRoutine) sidMidiShim;
	/* Cutoff/res/OBmix */
	dispatch[63].controller = 126;
	dispatch[63].operator = 73;
	dispatch[63].routine = (synthRoutine) sidMidiShim;
	dispatch[64].controller = 126;
	dispatch[64].operator = 74;
	dispatch[64].routine = (synthRoutine) sidMidiShim;
	dispatch[65].controller = 126;
	dispatch[65].operator = 75;
	dispatch[65].routine = (synthRoutine) sidMidiShim;
	dispatch[66].controller = 126;
	dispatch[66].operator = 76;
	dispatch[66].routine = (synthRoutine) sidMidiShim;

	/* Multi and master vol */
	dispatch[67].controller = 126;
	dispatch[67].operator = 2;
	dispatch[67].routine = (synthRoutine) sidMidiShim;
	dispatch[68].controller = 126;
	dispatch[68].operator = 3;
	dispatch[68].routine = (synthRoutine) sidMidiShim;

	dispatch[69].operator = 0;
	dispatch[70].operator = 1;
	dispatch[71].operator = 2;
	dispatch[72].operator = 3;
	dispatch[73].operator = 4;
	dispatch[69].routine = dispatch[70].routine = dispatch[71].routine
		= dispatch[72].routine = dispatch[73].routine = (synthRoutine) sidMode;

	/* Mod freq, gain and waveform */
	dispatch[74].controller = 126;
	dispatch[74].operator = 77;
	dispatch[74].routine = (synthRoutine) sidMidiShim;
	dispatch[75].controller = 126;
	dispatch[75].operator = 78;
	dispatch[75].routine = (synthRoutine) sidMidiShim;
	dispatch[76].operator = 0;
	dispatch[77].operator = 1;
	dispatch[78].operator = 2;
	dispatch[79].operator = 3;
	dispatch[76].routine = dispatch[77].routine = dispatch[78].routine =
		dispatch[79].routine = (synthRoutine) sidLFOWave;

	/* Then we have the routing buttons, TBD */
	dispatch[80].controller = 126;
	dispatch[80].operator = 79;
	dispatch[80].routine = (synthRoutine) sidMidiShim;
	dispatch[81].controller = 126;
	dispatch[81].operator = 80;
	dispatch[81].routine = (synthRoutine) sidMidiShim;
	dispatch[82].controller = 126;
	dispatch[82].operator = 81;
	dispatch[82].routine = (synthRoutine) sidMidiShim;
	dispatch[83].controller = 126;
	dispatch[83].operator = 82;
	dispatch[83].routine = (synthRoutine) sidMidiShim;
	dispatch[84].controller = 126;
	dispatch[84].operator = 83;
	dispatch[84].routine = (synthRoutine) sidMidiShim;
	dispatch[85].controller = 126;
	dispatch[85].operator = 84;
	dispatch[85].routine = (synthRoutine) sidMidiShim;
	dispatch[86].controller = 126;
	dispatch[86].operator = 85;
	dispatch[86].routine = (synthRoutine) sidMidiShim;
	dispatch[87].controller = 126;
	dispatch[87].operator = 101;
	dispatch[87].routine = (synthRoutine) sidMidiShim;
	dispatch[88].controller = 126;
	dispatch[88].operator = 102;
	dispatch[88].routine = (synthRoutine) sidMidiShim;

	dispatch[95].controller = 126;
	dispatch[95].operator = 89;
	dispatch[95].routine = (synthRoutine) sidMidiShim;
	dispatch[96].controller = 126;
	dispatch[96].operator = 90;
	dispatch[96].routine = (synthRoutine) sidMidiShim;
	dispatch[97].controller = 126;
	dispatch[97].operator = 91;
	dispatch[97].routine = (synthRoutine) sidMidiShim;
	dispatch[98].controller = 126;
	dispatch[98].operator = 92;
	dispatch[98].routine = (synthRoutine) sidMidiShim;
	dispatch[99].controller = 126;
	dispatch[99].operator = 86;
	dispatch[99].routine = (synthRoutine) sidMidiShim;
	dispatch[100].controller = 126;
	dispatch[100].operator = 87;
	dispatch[100].routine = (synthRoutine) sidMidiShim;
	dispatch[101].controller = 126;
	dispatch[101].operator = 88;
	dispatch[101].routine = (synthRoutine) sidMidiShim;

	/* Env mod */
	dispatch[102].controller = 126;
	dispatch[102].operator = 93;
	dispatch[102].routine = (synthRoutine) sidMidiShim;
	dispatch[103].controller = 126;
	dispatch[103].operator = 94;
	dispatch[103].routine = (synthRoutine) sidMidiShim;
	dispatch[104].controller = 126;
	dispatch[104].operator = 95;
	dispatch[104].routine = (synthRoutine) sidMidiShim;
	dispatch[105].controller = 126;
	dispatch[105].operator = 96;
	dispatch[105].routine = (synthRoutine) sidMidiShim;
	dispatch[106].controller = 126;
	dispatch[106].operator = 97;
	dispatch[106].routine = (synthRoutine) sidMidiShim;
	dispatch[107].controller = 126;
	dispatch[107].operator = 103;
	dispatch[107].routine = (synthRoutine) sidMidiShim;

	/* Arpeggiator */
	dispatch[115].controller = 126;
	dispatch[115].operator = 6;
	dispatch[115].routine = (synthRoutine) sidMidiShim;
	dispatch[116].controller = 126;
	dispatch[116].operator = 7;
	dispatch[116].routine = (synthRoutine) sidMidiShim;
	dispatch[117].controller = 126;
	dispatch[117].operator = 8;
	dispatch[117].routine = (synthRoutine) sidMidiShim;

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
		dispatch[MEM_START + 13].routine = (synthRoutine) sidMemory;

	dispatch[132].controller = 0;
	dispatch[133].controller = 1;
	dispatch[132].routine = dispatch[133].routine
		= (synthRoutine) sidMidiChannel;

	/* Osc-1 settings */

	/* Env settings */

	/* Filter */

	dispatch[MEM_START].other1 = 1;
	dispatch[69].other1 = 0;
	dispatch[76].other1 = 0;

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
sidConfigure(brightonWindow *win)
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
		"bitmaps/blueprints/sidshade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	event.type = BRISTOL_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, MODS_PANEL, 1, &event);
	brightonParamChange(synth->win, MODS_PANEL, 2, &event);
	brightonParamChange(synth->win, MODS_PANEL, 4, &event);
	configureGlobals(synth);

	/* First memory location */
	event.value = 1.0;
	brightonParamChange(synth->win, 0, MEM_START + synth->location-1, &event);
	brightonParamChange(synth->win, 0, 69, &event);
	brightonParamChange(synth->win, 0, 76, &event);

	loadMemory(synth, SYNTH_NAME, 0, synth->bank * 10 + synth->location,
		synth->mem.active, 0, BRISTOL_FORCE);
	loadMemoryShim(synth);

	event.value = 0.505;
	brightonParamChange(synth->win, MODS_PANEL, 0, &event);

	synth->dispatch[MEM_START].other1 = synth->location;

	dc = brightonGetDCTimer(1000000);

	synth->loadMemory = (loadRoutine) loadMemoryKeyShim;

	return(0);
}

