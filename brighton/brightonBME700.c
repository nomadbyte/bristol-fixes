
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
 * Mods:
 *
 * 	LFO sync, Multi each 4 - 4 switches
 * 	Osc detune and ramp 2, multithread option - 2 knob 1 switch
 * 	Noise pink/multi - switch knob
 * 	Envelope velocity, retrig, gain both 6 - 4 switches 2 knob
 * 	Resonance res and mix high/low 4 (algo select for each?) 4 knob
 * 	Filter mix, kbd tracking? 2 knob
 *
 *	 	More signal into the filters, less out DONE
 * 		Multiplying mods
 * 		LFO/Env gain gives droning, mono mode
 * 		Oscillator is dull SOME - multiple strands
 * 		Load opts only once, or when double click load button DONE
 *
 * 	Remixed oscillators are also pretty dull, the only net effect appears to be
 * 	a volume gain. Rework the code for add/subtract of different poles and/or
 * 	add/sub different remix poles also.
 */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int BME700Init();
static int BME700Configure();
static int BME700Callback(brightonWindow *, int, int, float);
static int BME700ModCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);
static int BME700KeyCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define SYNTH_NAME synth->resources->name

#define OPTS_START 43
#define OPTS_COUNT 32
#define ACTIVE_DEVS (OPTS_START + OPTS_COUNT)
#define MEM_MGT ACTIVE_DEVS
#define MIDI_MGT (MEM_MGT + 12)

#define MODS_COUNT 12

#define DEVICE_COUNT (ACTIVE_DEVS + 2)

#define KEY_PANEL 1
#define MODS_PANEL 2
#define DISPLAY_DEV (OPTS_COUNT - 6)

static int dc;
static int shade_id;

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
 * This example is for a BME700Bristol type synth interface.
 */

#define SPW 50
#define SPH 110

#define R1 110
#define R1b (R1 + SPH / 4)
#define R2 330
#define R2b (R2 + SPH / 4)
#define R3 575
#define R3b (R3 + SPH / 4)
#define R4 795
#define R4b (R4 + SPH / 4)

#define D1 65

#define C1 36
#define C1b (C1 + SPW/3)
#define C2 (C1 + D1)
#define C2b (C2 + D1 / 2)
#define C3 (C2 + D1)
#define C3b (C3 + SPW/3)
#define C4 (C3 + D1)
#define C4b (C4 + SPW/3)

#define C10 401
#define C10b (C10 + SPW/3)

#define C20 570
#define C20b (C20 + SPW/3)
#define C21 (C20 + D1)
#define C21b (C21 + SPW/3)
#define C22 (C21 + D1)

#define C30 810
#define C30b (C30 + SPW/3)
#define C31 (C30 + D1)
#define C32 (C31 + D1)

static
brightonLocations locations[DEVICE_COUNT] = {
	/* 0 */
	{"Mod1 Tri/Square", 2, C1b, R1b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Mod1 Rate", 0, C2, R1, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Mod2 Rate", 0, C3, R1, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Mod2 Tri/Square", 2, C4b, R1b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	/* 4 */
	{"Mod1/Env", 2, C1b + 35, R2b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"ModMix", 0, C2b, R2, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Single/Double", 2, C4, R2b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Tri/Square", 2, C4b + 12, R2b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	/* 8 */
	{"VCO Glide", 0, C1, R3, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"VCO PW", 0, C2, R3, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"VCO Vibrato", 0, C3, R3, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"VCO Shape", 0, C3, R4, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Glide On/Off", 2, C1b, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"PW Man/Auto", 2, C2 - 1, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"PW Env/Mod", 2, C2b, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO 8'", 2, C4, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO 16'/4'", 2, C4b + 12, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	/* 17 */
	{"Mix Noise/VCO", 0, C10, R1, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},

	{"Res #/b", 2, C10b, R3b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	/* 19 */
	{"Res 1", 2, C10b - 60, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Res 2", 2, C10b - 30, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Res 4", 2, C10b, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Res 8", 2, C10b + 30, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Res 16", 2, C10b + 60, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	/* 24 */
	{"Env1 ASR/AR", 2, C20b, R1b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Attack", 0, C21, R1, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Decay", 0, C22, R1, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Env2 ASR/AR", 2, C30b, R1b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"Attack", 0, C31, R1, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Decay", 0, C32, R1, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},

	/* 30 */
	{"VCF Env Mix", 0, C21, R2, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"VCA Env Mix", 0, C31, R2, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},

	/* 32 */
	{"Filt Resonace", 0, C20, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Filt Cutoff", 0, C21, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Filt Env", 0, C22, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"Filt ModMix", 0, C22, R4, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"F-Mod Tri/Sqr", 2, C20b, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"F-Mod1/Mod2", 2, C21 - 7, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"F-KBD/Mod", 2, C21b + 12, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	/* 39 */
	{"Mix VCF/Res", 0, C30, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"VCA ModMix", 0, C32, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"VCA Mod Mod1/Mod2", 2, C30b + 36, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"VCA Mod Tri/Sqr", 2, C30b + 101, R4b, 10, 60, 0, 1.01, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	/* 43 - 32 dummies masking opts */
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* 59 Tuning, moved out of active devs */
	{"Tuning", 0, C4, R3, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 
		BRIGHTON_NOTCH},
	/* 60 Global volume, moved out of active devs */
	{"Volume", 0, C31, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	/* Done */
};

/*
 * Memory panel, keep it simple - 8 memories, load save, up, down. Uses cs80 backlit buttons
 */
static
brightonLocations bme700mods[MODS_COUNT] = {
	/* LOAD */
	{"", 2, 190, 700, 100, 100, 0, 1, 0, "bitmaps/buttons/touchnlo.xpm",
		"bitmaps/buttons/touchnlO.xpm", BRIGHTON_CHECKBUTTON},

	/* MEMORY SELECT */
	{"", 2, 190, 200, 100, 100, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, 390, 200, 100, 100, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, 590, 200, 100, 100, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, 790, 200, 100, 100, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},

	{"", 2, 190, 450, 100, 100, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, 390, 450, 100, 100, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, 590, 450, 100, 100, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, 790, 450, 100, 100, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},

	/* SAVE */
	{"", 2, 390, 700, 100, 100, 0, 1, 0, "bitmaps/buttons/touchnlr.xpm",
		"bitmaps/buttons/touchnlR.xpm", BRIGHTON_CHECKBUTTON},

	/*
	 * UP/DOWN was memory but could be midi channel. Withdrawn for now since I want to keep
	 * this panel uncluttered
	 */
	{"", 2, 590, 700, 100, 100, 0, 1, 0, "bitmaps/buttons/touchnlg.xpm",
		"bitmaps/buttons/touchnlG.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 790, 700, 100, 100, 0, 1, 0, "bitmaps/buttons/touchnlo.xpm",
		"bitmaps/buttons/touchnlO.xpm", 0},
};

static
brightonLocations bme700opts[OPTS_COUNT] = {
	{"", 2, C1b, R1b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C1b + 50, R1b, 10, 60, 0, 1.0, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	{"", 2, C4b - 50, R1b, 10, 60, 0, 1.0, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C4b, R1b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	{"", 0, 0, 0, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 
		BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C2, R3, SPW, SPH, 0, 1, 0, "bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"", 2, C1b, R3b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	/* Noise Pink + filter */
	{"", 2, C10-5, R1b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"", 0, C10+40, R1, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},

	{"", 0, C10-40, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"", 0, C10+40, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"", 0, C10-40, R4, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"", 0, C10+40, R4, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},

	{"", 2, C20b, R1b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C21+12, R1b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"", 0, C22, R1, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},

	{"", 2, C30b, R1b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C31+12, R1b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},
	{"", 0, C32, R1, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},

	{"", 0, C20, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},
	{"", 0, C21, R3, SPW, SPH, 0, 1, 0,"bitmaps/knobs/yellowknob.xpm", 0, 0},

	/* Noise Multi */
	{"", 2, C10-45, R1b, 10, 60, 0, 1.00, 0,
		"bitmaps/buttons/klunk4.xpm", 0, BRIGHTON_VERTICAL},

	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp bme700App = {
	"BME700",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH,
	BME700Init,
	BME700Configure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	800, 417, 0, 0,
	7,
	{
		{
			"Bme700",
			"bitmaps/blueprints/BME700.xpm",
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			BME700Callback,
			22, 0, 956, 600,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/tkbg.xpm",
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			BME700KeyCallback,
			172, 600, 800, 350,
			KEY_COUNT_3_OCTAVE,
			keys3_octave
		},
		{
			"Bme700Mods",
			"bitmaps/blueprints/BME700mods.xpm",
			"bitmaps/textures/bme700bg.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			BME700ModCallback,
			22, 600, 150, 350,
			MODS_COUNT,
			bme700mods
		},
		{
			"Bme700Opts",
			"bitmaps/blueprints/BME700opts.xpm",
			"bitmaps/textures/metal5.xpm",
			BRIGHTON_WITHDRAWN, /* flags */
			0,
			0,
			BME700Callback,
			22, 0, 956, 600,
			OPTS_COUNT,
			bme700opts
		},
		{
			"SP2",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			0,
			23, 955, 954, 44,
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
			0, 0, 22, 1000,
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
			978, 0, 22, 1000,
			0,
			0
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
BME700KeyCallback(brightonWindow *win, int panel, int index, float value)
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

static void
optsShim(guiSynth *synth)
{
	int i;
	brightonEvent event;

	event.type = BRISTOL_FLOAT;

	for (i = 0; i < OPTS_COUNT; i++)
	{
		event.value = synth->mem.param[i + OPTS_START];
		brightonParamChange(synth->win, 3, i, &event);
	}
}

static void
panelSwitch(brightonWindow *win, guiSynth *synth, float value)
{
	brightonEvent event;

	/* 
	 * If the sendvalue is zero, then withdraw the opts window, draw the
	 * slider window, and vice versa.
	 */
	if (value == 0)
	{
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(synth->win, 3, -1, &event);
		event.intvalue = 1;
		brightonParamChange(synth->win, 0, -1, &event);

		shade_id = brightonPut(synth->win, "bitmaps/blueprints/BME700shade.xpm",
			0, 0, synth->win->width, synth->win->height);
	} else {
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(synth->win, 0, -1, &event);
		event.intvalue = 1;
		brightonParamChange(synth->win, 3, -1, &event);

		brightonRemove(synth->win, shade_id);
	}
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
			loadMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location,
				OPTS_START, 0, BRISTOL_FORCE);
			optsShim(synth);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			synth->location = value;
			loadMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location,
				OPTS_START, 0, BRISTOL_FORCE);
			break;
	}
	return(0);
}

static void
BME700MidiShim(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid, c, o, v);
}

static int
BME700ModCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;
	int start = synth->location;

	if (global.libtest)
		printf("BME700ModCallback(%i, %f)\n", index, value);

	if (synth->dispatch[ACTIVE_DEVS].other2)
	{
		synth->dispatch[ACTIVE_DEVS].other2 = 0;
		return(0);
	}

	switch (index) {
		case 0:
			/* Load */
			if (brightonDoubleClick(dc) != 0) {
				loadMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location,
					synth->mem.active, 0, BRISTOL_FORCE);
				optsShim(synth);
			} else
printf("%i %i\n", synth->bank, synth->location);
				loadMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location,
					OPTS_START, 0, BRISTOL_FORCE);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			/*
			 * Radiobutton memory selectors, doubleclick should load
			 */
			if (synth->dispatch[ACTIVE_DEVS].other1 != -1)
			{
				synth->dispatch[ACTIVE_DEVS].other2 = 1;

				if (synth->dispatch[ACTIVE_DEVS].other1 != index)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, panel,
					synth->dispatch[ACTIVE_DEVS].other1, &event);
			}
			synth->dispatch[ACTIVE_DEVS].other1 = index;

			synth->location = index;

			if (brightonDoubleClick(dc) != 0) {
				loadMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location,
					OPTS_START, 0, BRISTOL_FORCE);
				/* optsShim(synth); */
			}
			break;
		case 9:
			/*
			 * Save on doubleclick
			 */
			if (brightonDoubleClick(dc) != 0)
				saveMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location, 0);
			break;
		case 10:
			/*
			 * Up
			 */
			while (++synth->location != start)
			{
				if (synth->location > 8)
					synth->location = 1;

				if (loadMemory(synth, synth->resources->name, 0,
					synth->bank + synth->location, synth->mem.active,
						0, BRISTOL_STAT) >= 0)
					break;
			}

			loadMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location,
				synth->mem.active, 0, BRISTOL_FORCE);
			/* optsShim(synth); */
			event.type = BRISTOL_FLOAT;
			event.value = 1;
			brightonParamChange(synth->win, MODS_PANEL, synth->location,
				&event);
			break;
		case 12:
			/*
			 * Down
			 */
			while (--synth->location != start)
			{
				if (synth->location < 1)
					synth->location = 8;

				if (loadMemory(synth, synth->resources->name, 0,
					synth->bank + synth->location, synth->mem.active,
						0, BRISTOL_STAT) >= 0)
					break;
			}

			loadMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location,
				OPTS_START, 0, BRISTOL_FORCE);
			/* optsShim(synth); */
			event.type = BRISTOL_FLOAT;
			event.value = 1;
			brightonParamChange(synth->win, MODS_PANEL, synth->location,
				&event);
			break;
		case 11:
			/*
			 * Show Opts panel
			 */
			panelSwitch(win, synth, value);
			break;
	}

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
BME700Callback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (global.libtest)
		printf("BME700Callback(%i, %f)\n", index, value);

	if (synth == 0)
		return(0);

	if ((index >= OPTS_START) && (index < ACTIVE_DEVS))
		return(0);

	if (panel == 3)
		index += OPTS_START;

//	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
	if (index >= DEVICE_COUNT)
		return(0);

	if (bme700App.resources[0].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	synth->mem.param[index] = value;

//	if ((!global.libtest) || (index >= ACTIVE_DEVS))
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].operator,
			synth->dispatch[index].controller,
			sendvalue);

	return(0);
}

static void
envelope(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	switch (o) {
		case 0:
			/* Mode selector - sustain level */
			bristolMidiSendMsg(fd, synth->sid, c, 2, v == 0? 0:C_RANGE_MIN_1);
			break;
		case 1:
			/* Attack */
			bristolMidiSendMsg(fd, synth->sid, c, 0, v);
			break;
		case 2:
			/* Decay/release */
			bristolMidiSendMsg(fd, synth->sid, c, 1, v);
			bristolMidiSendMsg(fd, synth->sid, c, 3, v);
			break;
	}
}

static void
pwm(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float pw = 0.5;
	int source = 0;
	int man = 0;

	/*
	 * Params are
	 * 	9 = PW rotary
	 * 	13 = auto/man
	 * 	14 = env/mod
	 */
	if (synth->mem.param[13] != 0)
	{
		pw = synth->mem.param[9] * 0.5;
		man = 1;
	}

	if (synth->mem.param[14] != 0)
		source = 1;

	bristolMidiSendMsg(fd, synth->sid, 2, 13, (int) (pw * C_RANGE_MIN_1));
	bristolMidiSendMsg(fd, synth->sid, 126, 29,
		(int) (synth->mem.param[9] * 0.9 * C_RANGE_MIN_1));
	bristolMidiSendMsg(fd, synth->sid, 126, 16, man);
	bristolMidiSendMsg(fd, synth->sid, 126, 17, source);
}

static void
lfofreq(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid, c, o, C_RANGE_MIN_1 - v);
}

static void
filterTracking(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid, 4, 3,
		v == 0? 0: (int) (synth->mem.param[62] * C_RANGE_MIN_1));
	bristolMidiSendMsg(fd, synth->sid, 126, 22, v);
}

static void
footage(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (synth->mem.param[49] != 0)
	{
		int f16 = 0, f8 = C_RANGE_MIN_1, f4 = 0, detune = 0;

		if (synth->mem.param[15] == 0) {
			detune =
				(synth->mem.param[48] * synth->mem.param[48] * C_RANGE_MIN_1);
			if (synth->mem.param[16] == 0)
				/* 4' */
				f4 = f8;
			else
				/* 16' */
				f16 = f8;
		}

		/* Mix in separate strands */
		bristolMidiSendMsg(fd, synth->sid, 2, 2, 12);
		bristolMidiSendMsg(fd, synth->sid, 2, 3, detune);
		bristolMidiSendMsg(fd, synth->sid, 2, 4, f16);
		bristolMidiSendMsg(fd, synth->sid, 2, 5, f8);
		bristolMidiSendMsg(fd, synth->sid, 2, 6, f4);
	} else {
		/* Transpose the oscillator */
		int transp = 12;

		if (synth->mem.param[15] == 0) {
			if (synth->mem.param[16] == 0)
				/* 4' */
				transp = 24;
			else
				/* 16' */
				transp = 0;
		}

		/* 8' only */
		bristolMidiSendMsg(fd, synth->sid, 2, 2, transp);
		bristolMidiSendMsg(fd, synth->sid, 2, 3, 0);
		bristolMidiSendMsg(fd, synth->sid, 2, 4, 0);
		bristolMidiSendMsg(fd, synth->sid, 2, 5, C_RANGE_MIN_1);
		bristolMidiSendMsg(fd, synth->sid, 2, 6, 0);
	}
}

static void
resFilter(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float cutoff, lower = 0.10, upper = 0.90;
	int algorithm = 4; /* This was 1, 12db/oct, now just 24dB with remix */
	float res, mix;

	if (synth->mem.param[18] != 0)
	{
		algorithm = 4;
		res = synth->mem.param[52];
		mix = synth->mem.param[53];
	} else {
		res = synth->mem.param[54];
		mix = synth->mem.param[55];
	}

	bristolMidiSendMsg(fd, synth->sid, 3, 4, algorithm);

	/*
	 * Otherwise we have 5 switches covering frequence. I will assume we are
	 * going to go full range. This is perhaps a bad idea, we should have a 
	 * lower cutoff, a higher cutoff, and then the switches give 32 steps
	 * between the two.
	if ((cutoff = 0.0312500
		+ (synth->mem.param[19] == 0? 0: 0.0312500)
		+ (synth->mem.param[20] == 0? 0: 0.062500)
		+ (synth->mem.param[21] == 0? 0: 0.12500)
		+ (synth->mem.param[22] == 0? 0: 0.2500)
		+ (synth->mem.param[23] == 0? 0: 0.500)) > 1.0)
		cutoff = 1.0;
	 */
	cutoff = (synth->mem.param[19] == 0? 0: 0.0312500)
		+ (synth->mem.param[20] == 0? 0: 0.062500)
		+ (synth->mem.param[21] == 0? 0: 0.12500)
		+ (synth->mem.param[22] == 0? 0: 0.2500)
		+ (synth->mem.param[23] == 0? 0: 0.500);

	cutoff = lower + (upper - lower) * cutoff;

	bristolMidiSendMsg(fd, synth->sid, 3, 0, (int) (cutoff * C_RANGE_MIN_1));
	bristolMidiSendMsg(fd, synth->sid, 3, 1, (int) (res * C_RANGE_MIN_1));
	bristolMidiSendMsg(fd, synth->sid, 3, 7, (int) (mix * C_RANGE_MIN_1));
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
BME700Init(brightonWindow *win)
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

	printf("Initialise the BME700 link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].operator = 126;
		synth->dispatch[i].controller = 101;
		synth->dispatch[i].routine = (synthRoutine) BME700MidiShim;
	}

	/* LFO */
	synth->dispatch[0].operator = 126;
	synth->dispatch[0].controller = 11;
	synth->dispatch[1].operator = 0;
	synth->dispatch[1].controller = 0;
	synth->dispatch[2].operator = 1;
	synth->dispatch[2].controller = 0;
	synth->dispatch[1].routine = synth->dispatch[2].routine
		= (synthRoutine) lfofreq;
	synth->dispatch[3].operator = 126;
	synth->dispatch[3].controller = 12;

	/* Vib Env */
	synth->dispatch[4].operator = 126;
	synth->dispatch[4].controller = 10;
	synth->dispatch[5].operator = 126;
	synth->dispatch[5].controller = 27;

	/* PWM double, wave */
	synth->dispatch[6].operator = 126;
	synth->dispatch[6].controller = 14;
	synth->dispatch[7].operator = 126;
	synth->dispatch[7].controller = 13;

	/* Glide */
	synth->dispatch[8].operator = 126;
	synth->dispatch[8].controller = 0;
	synth->dispatch[12].operator = 126;
	synth->dispatch[12].controller = 15;

	/* Signal */
	synth->dispatch[11].operator = 2;
	synth->dispatch[11].controller = 14;

	/* Vibrato */
	synth->dispatch[10].operator = 126;
	synth->dispatch[10].controller = 28;

	/* PWM Source */
	synth->dispatch[9].operator = 126;
	synth->dispatch[9].controller = 101;
	synth->dispatch[13].operator = 126;
	synth->dispatch[13].controller = 102;
	synth->dispatch[14].operator = 126;
	synth->dispatch[14].controller = 103;
	synth->dispatch[9].routine = synth->dispatch[13].routine
		= synth->dispatch[14].routine = (synthRoutine) pwm;

	/* Footage */
	synth->dispatch[15].operator = 126;
	synth->dispatch[15].controller = 18;
	synth->dispatch[16].operator = 126;
	synth->dispatch[16].controller = 19;
	synth->dispatch[15].routine = synth->dispatch[16].routine
		= (synthRoutine) footage;

	/* Noise/VCO mix */
	synth->dispatch[17].operator = 126;
	synth->dispatch[17].controller = 3;

	/* ResFilter - type, then frequency */
	synth->dispatch[18].operator = 3;
	synth->dispatch[18].controller = 0;
	synth->dispatch[19].operator = 3;
	synth->dispatch[19].controller = 1;
	synth->dispatch[20].operator = 3;
	synth->dispatch[20].controller = 2;
	synth->dispatch[21].operator = 3;
	synth->dispatch[21].controller = 3;
	synth->dispatch[22].operator = 3;
	synth->dispatch[22].controller = 4;
	synth->dispatch[23].operator = 3;
	synth->dispatch[23].controller = 5;
	synth->dispatch[18].routine = synth->dispatch[19].routine
		= synth->dispatch[20].routine = synth->dispatch[21].routine
		= synth->dispatch[22].routine = synth->dispatch[23].routine
			= (synthRoutine) resFilter;

	/* Env-1 */
	synth->dispatch[24].operator = 6;
	synth->dispatch[24].controller = 0;
	synth->dispatch[25].operator = 6;
	synth->dispatch[25].controller = 1;
	synth->dispatch[26].operator = 6;
	synth->dispatch[26].controller = 2;
	/* Env-2 */
	synth->dispatch[27].operator = 7;
	synth->dispatch[27].controller = 0;
	synth->dispatch[28].operator = 7;
	synth->dispatch[28].controller = 1;
	synth->dispatch[29].operator = 7;
	synth->dispatch[29].controller = 2;
	synth->dispatch[24].routine = synth->dispatch[25].routine
		= synth->dispatch[26].routine = synth->dispatch[27].routine
		= synth->dispatch[28].routine = synth->dispatch[29].routine
			= (synthRoutine) envelope;

	/* Env Mix */
	synth->dispatch[30].operator = 126;
	synth->dispatch[30].controller = 5;
	synth->dispatch[31].operator = 126;
	synth->dispatch[31].controller = 6;

	/* VCF */
	synth->dispatch[32].operator = 4;
	synth->dispatch[32].controller = 1;
	synth->dispatch[33].operator = 4;
	synth->dispatch[33].controller = 0;
	synth->dispatch[34].operator = 4;
	synth->dispatch[34].controller = 2;
	synth->dispatch[35].operator = 126;
	synth->dispatch[35].controller = 25;

	/* Filter mod */
	synth->dispatch[36].operator = 126;
	synth->dispatch[36].controller = 20;
	synth->dispatch[37].operator = 126;
	synth->dispatch[37].controller = 21;
	synth->dispatch[38].operator = 126;
	synth->dispatch[38].controller = 22;
	synth->dispatch[38].routine = (synthRoutine) filterTracking;

	/* filter mix */
	synth->dispatch[39].operator = 126;
	synth->dispatch[39].controller = 4;

	/* Amp mod */
	synth->dispatch[40].operator = 126;
	synth->dispatch[40].controller = 26;
	synth->dispatch[41].operator = 126;
	synth->dispatch[41].controller = 23;
	synth->dispatch[42].operator = 126;
	synth->dispatch[42].controller = 24;

	/* Options LFO */
	synth->dispatch[43].operator = 126;
	synth->dispatch[43].controller = 7;
	synth->dispatch[44].operator = 0;
	synth->dispatch[44].controller = 1;
	synth->dispatch[45].operator = 126;
	synth->dispatch[45].controller = 8;
	synth->dispatch[46].operator = 1;
	synth->dispatch[46].controller = 1;

	/* Test the LFO limits */
	synth->dispatch[47].operator = 0;
	synth->dispatch[47].controller = 4;
	synth->dispatch[48].operator = 0;
	synth->dispatch[48].controller = 5;

	/* Footage */
	synth->dispatch[49].operator = 126;
	synth->dispatch[49].controller = 101;
	synth->dispatch[49].routine = synth->dispatch[48].routine
		= (synthRoutine) footage;

	/* Noise */
	synth->dispatch[50].operator = 5;
	synth->dispatch[50].controller = 1;
	synth->dispatch[51].operator = 5;
	synth->dispatch[51].controller = 2;
	synth->dispatch[64].operator = 126;
	synth->dispatch[64].controller = 9;

	/* Resonator */
	synth->dispatch[52].operator = 3;
	synth->dispatch[52].controller = 1;
	synth->dispatch[53].operator = 3;
	synth->dispatch[53].controller = 7;
	synth->dispatch[54].operator = 3;
	synth->dispatch[54].controller = 1;
	synth->dispatch[55].operator = 3;
	synth->dispatch[55].controller = 7;
	synth->dispatch[52].routine = synth->dispatch[53].routine
		= synth->dispatch[54].routine = synth->dispatch[55].routine
			= (synthRoutine) resFilter;

	/* Env */
	synth->dispatch[56].operator = 6;
	synth->dispatch[56].controller = 5;
	synth->dispatch[57].operator = 6;
	synth->dispatch[57].controller = 6;
	synth->dispatch[58].operator = 6;
	synth->dispatch[58].controller = 4;
	synth->dispatch[59].operator = 7;
	synth->dispatch[59].controller = 5;
	synth->dispatch[60].operator = 7;
	synth->dispatch[60].controller = 6;
	synth->dispatch[61].operator = 7;
	synth->dispatch[61].controller = 4;

	/* VCF */
	synth->dispatch[62].operator = 4;
	synth->dispatch[62].controller = 3;
	synth->dispatch[62].routine = (synthRoutine) filterTracking;
	synth->dispatch[63].operator = 4;
	synth->dispatch[63].controller = 7;

	/* Global tuning, volume */
	synth->dispatch[75].operator = 126;
	synth->dispatch[75].controller = 1;
	synth->dispatch[76].operator = 126;
	synth->dispatch[76].controller = 2;

	/* Both LFO upper and lower limits */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 4, C_RANGE_MIN_1 / 20);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 5, C_RANGE_MIN_1 / 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 4, C_RANGE_MIN_1 / 20);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 5, C_RANGE_MIN_1 / 2);

	/* Osc settings */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 0, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 15, 1);
	/* Mix square 8' levels only */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 3, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 4, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 5, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 6, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 9, C_RANGE_MIN_1);

	/* Env settings: Gain */
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 4, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 4, C_RANGE_MIN_1);

	/* Filter settings */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 7, 12000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 5, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 7, 12000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 3);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 5, 16383);

	dispatch[ACTIVE_DEVS].other1 = -1;

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
BME700Configure(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	if (synth == 0)
	{
		printf("problems going operational\n");
		return(-1);
	}

	else if (synth->location == 0) {
		synth->bank = 0;
		synth->location = 1;
	} else if (synth->location > 0) {
		synth->bank = ((synth->location / 10) * 10) % 100;
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
	synth->transpose = 36;

	shade_id = brightonPut(win,
		"bitmaps/blueprints/BME700shade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	brightonParamChange(synth->win, MODS_PANEL, synth->location, &event);

	loadMemory(synth, SYNTH_NAME, 0, synth->bank + synth->location,
		synth->mem.active, 0, BRISTOL_FORCE);
	optsShim(synth);

	/*
	 * Volume, tuning, bend
	event.value = 0.822646;
	brightonParamChange(synth->win, MODS_PANEL, 10, &event);
	 */
	event.type = BRIGHTON_FLOAT;
	event.value = 0.5;
	brightonParamChange(synth->win, 0, 75, &event);
	event.value = 0.7;
	brightonParamChange(synth->win, 0, 76, &event);

	dc = brightonGetDCTimer(synth->win->dcTimeout);

	return(0);
}

