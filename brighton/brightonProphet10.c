
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
 * Master controls need to be implemented - vol/balance/tune. Done
 * Layer balance should be pan. Done
 * Master balance should be layer. Done
 *
 * Seq/Arp/Chord CHORDED
 * Mode selections Done.
 *
 * Confirm all the keyboard modes esp WRT layers.
 */


#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int initmem;
static int exclude = 0;

extern void *brightonmalloc();

static int pro10Init();
static int pro10Configure();
static int pro10Callback(brightonWindow * , int, int, float);
static int lmCallback(brightonWindow * , int, int, float);
static int ummodCallback(brightonWindow * , int, int, float);
static int lmmodCallback(brightonWindow * , int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

#define MODE_DUAL	0
#define MODE_LAYER	1
#define MODE_POLY	2
#define MODE_ALT	3 /* One voice then the other - not supported */

static int keymode = MODE_DUAL;
static int seqLearn = 0, chordLearn = 0;

extern guimain global;
/*
 * These have to be integrated into the synth private structures, since we may
 * have multiple invocations of any given synth.
 */
static guimain manual;

#include "brightonKeys.h"

#define DEVICE_COUNT 100
#define ACTIVE_DEVS 48
#define DISPLAY_DEV (DEVICE_COUNT - 2)
#define DISPLAY_DEV_2 (DEVICE_COUNT - 1)
#define RADIOSET_1 70
#define RADIOSET_2 81
#define RADIOSET_3 51
#define RADIOSET_4 (DEVICE_COUNT + 2)
#define RADIOSET_5 (DEVICE_COUNT + 5)
#define PANEL_SWITCH 95

#define KEY_PANEL 1
#define KEY_PANEL2 3

#define R1 75
#define RB1 95
#define R2 305
#define RB2 330
#define R3 535
#define RB3 560
#define RB4 785
#define RB5 880

#define C1 26
#define C2 76
#define CB1 66
#define CB2 95
#define C3 125
#define C4 155
#define C5 184

#define C6 235
#define CB6 275
#define CB7 305
#define C7 340
#define CB8 390

#define C9 280
#define CB9 330
#define CB10 360
#define CB11 390
#define C10 430
#define CB12 480
#define CB13 510

#define C11 442
#define C12 490
#define C13 538

#define C14 595
#define C15 642
#define C16 688
#define C17 735

#define C18 800
#define C19 850
#define C20 900
#define C21 950

#define C22 265
#define C23 470

#define S1 95

#define B1 14
#define B2 80
#define B3 65

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
 * This example is for a pro10Bristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {
/* poly mod */
	{"PolyMod-Filt", 0, C1, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"PolyMod-OscB", 0, C2, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"PolyMod-FreqA", 2, C3, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"PolyMod-PW-S", 2, C4, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"PolyMod-Filt", 2, C5, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* lfo */
	{"LFO-Freq", 0, C2, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"LFO-Ramp", 2, C3, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"LFO-Tri", 2, C4, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"LFO-Square", 2, C5, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* wheel mod */
	{"Wheel-Mix", 0, C1, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Wheel-FreqA", 2, CB1, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Wheel-FreqB", 2, CB2, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Wheel-PW-A", 2, C3, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Wheel-PW-B", 2, C4, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Wheel-Filt", 2, C5, RB3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* osc a */
	{"OscA-Transpose", 0, C6, R1, S1, S1, 0, 5, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"OscA-Ramp", 2, CB6, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscA-Pule", 2, CB7, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscA-PW", 0, C7, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"OscA-Sync", 2, CB8, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* osc b */
	{"OscB-Transpose", 0, C6, R2, S1, S1, 0, 5, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"OscB-Tune", 0, C9, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, BRIGHTON_NOTCH},
	{"OscB-Ramp", 2, CB9, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscB-Tri", 2, CB10, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscB-Pulse", 2, CB11, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscB-PW", 0, C10, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"OscB-LFO", 2, CB12, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"OscB-KBD", 2, CB13, RB2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
/* glide, etc */
	{"Glide", 0, C6, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Unison", 2, CB7, R3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* mixer */
	{"Mix-OscA", 0, C11, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Mix-OscB", 0, C12, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"Mix-Noise", 0, C13, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
/* filter + env */
	{"VCF-Cutoff", 0, C14, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Resonance", 0, C15, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Envelope", 0, C16, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-KBD", 2, C17 + 6, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCF-Attack", 0, C14, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Decay", 0, C15, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Sustain", 0, C16, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCF-Release", 0, C17, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
/* main env */
	{"VCA-Attack", 0, C14, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCA-Decay", 0, C15, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCA-Sustain", 0, C16, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"VCA-Release", 0, C17, R3, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},

/* tune + vol + pan */
	{"LayerTuning", 0, C20, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, BRIGHTON_NOTCH},
	{"LayerVolume", 0, C18, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 0},
	{"LayerPanning", 0, C19, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, BRIGHTON_NOTCH},

	{"Release", 2, C19 + 10, RB4, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
/* A440 */
	{"A-440", 2, C21 + 5, R2, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Retune", 2, C21 + 7, RB4, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
/* Dual/Layer/Poly */
	{"Mode-Dual", 2, CB12 - 20, R3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Mode-Layer", 2, CB12 + 15, R3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Mode-Split", 2, CB12 + 50, R3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* Master Globals */
	{"MasterVolume", 0, C17, RB4, S1, S1, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, 0},
	{"MasterPan", 0, C18, RB4, S1, S1, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, 
		BRIGHTON_NOTCH},
	{"MasterTune", 0, C20, RB4, S1, S1, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, 
		BRIGHTON_NOTCH},
/* Chording */
	{"", 2, C19 + 5, R3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C20 + 5, R3, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* dummies */
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C17, R3, S1, S1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
/* memories */
	{"", 2, C23 + 10, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 31, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 52, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 73, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 94, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 115, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 136, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 157, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C22, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C22 + 30, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm", "bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C22 + 60, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, C23 + 10, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 31, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 52, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 73, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 94, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 115, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 136, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23 + 157, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C22, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C22 + 30, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm", "bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C22 + 60, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

/* Midi, perhaps eventually file import/export buttons */
	{"", 2, C20 + 10, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C21 + 10, RB1, B1, B2, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	/* Synth select */
	{"Upper", 2, CB6 - 62, RB4, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", 0},
	{"Lower", 2, CB6 - 62, RB5, B1, B3, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", "bitmaps/buttons/pressong.xpm", 0},
	/* LOGO */
	{"", 4, 30, RB4, 143, 150, 0, 1, 0, "bitmaps/images/pro10.xpm", 0, 0},
	/* Display */
	{"", 3, C22 + 80, RB4, 130, B3, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0},
	{"", 3, C22 + 80, RB5, 130, B3, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}
};

/*
 * This is for the lower mod panel. It will control the MIDI operational mode
 * for poly/split/layer stuff and mod routing. It is not totally according to
 * the original that used the panel for argeggio. This may also control some
 * of the mod routing for each layer. It will not be saved in the memories.
 */
#define P10_MOD_COUNT 15
static brightonLocations p10Mods[P10_MOD_COUNT] = {
	/* lmod umod lpitch upitch */
	{"", 2, 60, 122, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 260, 122, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 460, 122, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 660, 122, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 860, 122, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},

	{"", 2, 60, 420, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 60, 625, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 60, 830, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},

	{"", 2, 660, 420, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 860, 420, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},

	{"", 2, 660, 625, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", 0},
	{"", 2, 860, 625, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 660, 830, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", 0},
	{"", 2, 860, 830, 90, 140, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},

	/* Rate */
	{"", 0, 300, 520, 220, 220, 0, 1, 0, "bitmaps/knobs/knob3.xpm", 0, 0},
};

#define PMOD_COUNT 6
brightonLocations pmods[PMOD_COUNT] = {
	{"", 1, 150, 170, 70, 620, 0, 1, 0, 0, 0,BRIGHTON_CENTER|BRIGHTON_NOSHADOW},
	{"", 1, 350, 170, 70, 620, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW},

	{"", 2, 580, 320, 90, 135, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 760, 320, 90, 135, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 580, 560, 90, 135, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, 760, 560, 90, 135, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
};

#define LM_MOD_INDEX DEVICE_COUNT
#define UM_MOD_INDEX (LM_MOD_INDEX + P10_MOD_COUNT)

static int
pro10destroy(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);

/*printf("pro10destroy(%i): %i\n", win, synth->midichannel); */
	/*
	 * Since we registered two synths, we now need to remove the upper
	 * manual.
	 */
	bristolMidiSendMsg(manual.controlfd, synth->sid2, 127, 0,
		BRISTOL_EXIT_ALGO);
	bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
		BRISTOL_EXIT_ALGO);
	return(0);
}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp pro10App = {
	"prophet10",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal5.xpm",
	0, /* BRIGHTON_STRETCH, //flags */
	pro10Init,
	pro10Configure, /* 3 callbacks, unused? */
	midiCallback,
	pro10destroy,
	{10, 100, 2, 2, 5, 520, 0, 0},
	745, 410, 0, 0,
	7,
	{
		{
			"Pro10",
			"bitmaps/blueprints/prophet10.xpm",
			"bitmaps/textures/metal5.xpm",
			0, /*BRIGHTON_STRETCH, // flags */
			0,
			0,
			pro10Callback,
			15, 5, 970, 500,
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
			lmCallback,
			148, 505, 860, 240,
			KEY_COUNT,
			keysprofile
		},
		{
			"Mods",
			"bitmaps/blueprints/prophet10mod.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0,
			0,
			0,
			ummodCallback,
			15, 505, 135, 240,
			PMOD_COUNT,
			pmods
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			lmCallback,
			153, 745, 850, 240,
			KEY_COUNT,
			keysprofile
		},
		{
			"Mods",
			"bitmaps/blueprints/prophetmod2.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0,
			0,
			0,
			lmmodCallback,
			18, 745, 136, 240,
			P10_MOD_COUNT,
			p10Mods
		},
		{
			"Pro10",
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
			"Pro10",
			0,
			"bitmaps/textures/wood2.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /*flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		}
	}
};

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %i, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, "prophet", 0, synth->bank + synth->location,
				synth->mem.active, 0, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

/*
 * Default dispatcher
 */
static int
pro10MidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
pro10DLP(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;

	if (synth->dispatch[RADIOSET_3].other2)
	{
		synth->dispatch[RADIOSET_3].other2 = 0;
		return;
	}

	if (synth->dispatch[RADIOSET_3].other1 != -1)
	{
		synth->dispatch[RADIOSET_3].other2 = 1;

		if (synth->dispatch[RADIOSET_3].other1 != c)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[RADIOSET_3].other1 + RADIOSET_3, &event);
	}
	synth->dispatch[RADIOSET_3].other1 = c;

/* printf("pro10DLP(%i, %i)\n", c, v); */

	/* We may have to consider Seq/Arpeg/Chord issues here */
	switch (c) {
		case 0:
			/*
			 * Dual keyboard:
			 *
			 * 	Separate the midi channels
			 * 	Half the key count for primary layer
			 * 	Max out the split for second layer
			 */
			keymode = MODE_DUAL;

			/* Separate the midi channels */
			if (synth->midichannel == 15) {
				synth->midichannel--;
				bristolMidiSendMsg(global.controlfd, synth->sid,
					127, 0, BRISTOL_MIDICHANNEL|synth->midichannel);
				bristolMidiSendMsg(global.controlfd, synth->sid2,
					127, 0, (BRISTOL_MIDICHANNEL|synth->midichannel) + 1);
			} else
				bristolMidiSendMsg(global.controlfd, synth->sid2,
					127, 0, (BRISTOL_MIDICHANNEL|synth->midichannel) + 1);

			((guiSynth *) synth->second)->midichannel = synth->midichannel + 1;

			/* Halve the voicecount */
			bristolMidiSendMsg(fd, chan, 126, 35, 1);

			/* Configure a whole keyboard */
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, BRISTOL_LOWKEY|0);

			break;
		case 1:
			/*
			 * Layered keyboards
			 *
			 * 	Join up the midi channels
			 * 	Half the key count for primary layer
			 * 	Max out the split for second layer
			 */
			keymode = MODE_LAYER;

			/* Join the midi channel */
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, (BRISTOL_MIDICHANNEL|synth->midichannel));
			/* Halve the voicecount */
			bristolMidiSendMsg(fd, chan, 126, 35, 1);
			/* Configure a whole keyboard */
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, BRISTOL_LOWKEY|0);

			((guiSynth *) synth->second)->midichannel = synth->midichannel;

			break;
		case 2:
			/*
			 * 10 Note Poly keyboards
			 *
			 * 	Join up the midi channels
			 * 	Double the key count for primary layer
			 * 	Zero out the split for second layer
			 */
			keymode = MODE_POLY;

			/* Join the midi channel although this is superfluous */
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, (BRISTOL_MIDICHANNEL|synth->midichannel));
			/* Whole the voicecount */
			bristolMidiSendMsg(fd, chan, 126, 35, 0);
			/* Configure no keyboard */
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, BRISTOL_LOWKEY|127);

			((guiSynth *) synth->second)->midichannel = synth->midichannel;
			break;
	};
}

static void
multiA440(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (v != 0) {
		bristolMidiSendMsg(fd, synth->sid, 126, 3, 1);
		bristolMidiSendMsg(fd, synth->sid2, 126, 3, 1);
	} else {
		bristolMidiSendMsg(fd, synth->sid, 126, 3, 0);
		bristolMidiSendMsg(fd, synth->sid2, 126, 3, 0);
	}
}

static void
pro10Tune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float t1, t2, tg;
	int value;

	/*
	 * We have three different tuning parameters, these are one per layer plus
	 * the global tuning. Here we are going to send the respective tuning values
	 * to both the target synths.
	 */
	t1 = synth->mem.param[45];
	t2 = ((guiSynth *) synth->second)->mem.param[45];
	tg = synth->mem.param[56] - 0.5;

	if ((value = (t1 + tg) * C_RANGE_MIN_1) < 0)
		value = 0;
	if (value > C_RANGE_MIN_1)
		value = C_RANGE_MIN_1;
	bristolMidiSendMsg(fd, synth->sid, 126, 2, value);

	if ((value = (t2 + tg) * C_RANGE_MIN_1) < 0)
		value = 0;
	if (value > C_RANGE_MIN_1)
		value = C_RANGE_MIN_1;
	bristolMidiSendMsg(fd, synth->sid2, 126, 2, value);
}

static void
multiTune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int hold = synth->mem.param[PANEL_SWITCH];

/*printf("multiTune()\n"); */
	/*
	 * Configures all the tune settings to zero (ie, 0.5). Do the Osc-A first,
	 * since it is not visible, and then request the GUI to configure Osc-B.
	 */
	synth->mem.param[PANEL_SWITCH] = 1;
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8191);
	event.type = BRIGHTON_FLOAT;
	event.value = 0.49;
	brightonParamChange(synth->win, 0, 45, &event);
	brightonParamChange(synth->win, 0, 21, &event);
	event.value = 0.50;
	brightonParamChange(synth->win, 0, 45, &event);
	brightonParamChange(synth->win, 0, 21, &event);
	/*
	 * This is global tuning outside of memories and should perhaps be left
	 * alone?
	brightonParamChange(synth->win, 0, 56, &event);
	 */

	synth->mem.param[PANEL_SWITCH] = 0;
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 2, 8191);
	event.value = 0.49;
	brightonParamChange(synth->win, 0, 45, &event);
	brightonParamChange(synth->win, 0, 21, &event);
	event.value = 0.50;
	brightonParamChange(synth->win, 0, 45, &event);
	brightonParamChange(synth->win, 0, 21, &event);

	synth->mem.param[PANEL_SWITCH] = hold;

	((guiSynth *) synth->second)->mem.param[49] = 0.5;
	synth->mem.param[49] = 0.5;
}

static void
midiRelease(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (!global.libtest)
	{
		/*
		 * Midi release is ALL notes, ALL synths. If this behaviour (in the
		 * engine) changes we may need to put in an all_notes_off on the
		 * second manual as well.
		 */
		bristolMidiSendMsg(fd, synth->sid, 127, 0,
			BRISTOL_ALL_NOTES_OFF);
	}
}

static int
ummodCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	synth->mem.param[UM_MOD_INDEX + index] = value;

	/*
	 * Selector buttonn debugs.
	printf("ummodCallback(%i, %i, %f) [%f %f %f %f]\n", panel, index, value,
		synth->mem.param[UM_MOD_INDEX + 2],
		synth->mem.param[UM_MOD_INDEX + 3],
		synth->mem.param[UM_MOD_INDEX + 4],
		synth->mem.param[UM_MOD_INDEX + 5]
		);
	 */

	/*
	 * If this is controller 0 it is the frequency control, otherwise a 
	 * generic controller 1.
	 */
	if (index == 0)
	{
		if (synth->mem.param[UM_MOD_INDEX + 4] != 0)
			bristolMidiSendMsg(global.controlfd, synth->midichannel + 1,
				BRISTOL_EVENT_PITCH, 0, (int) (value * C_RANGE_MIN_1));
		if (synth->mem.param[UM_MOD_INDEX + 2] != 0)
			bristolMidiSendMsg(global.controlfd, synth->midichannel,
				BRISTOL_EVENT_PITCH, 0, (int) (value * C_RANGE_MIN_1));
	} else if (index == 1) {
		if (synth->mem.param[UM_MOD_INDEX + 5] != 0)
			bristolMidiControl(global.controlfd, synth->midichannel + 1,
				0, 1, ((int) (value * C_RANGE_MIN_1)) >> 7);
		if (synth->mem.param[UM_MOD_INDEX + 3] != 0)
			bristolMidiControl(global.controlfd, synth->midichannel,
				0, 1, ((int) (value * C_RANGE_MIN_1)) >> 7);
	}

	return(0);
}

static int
lmmodCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;
	int ivalue = (int) (value * C_RANGE_MIN_1);

	if (synth->dispatch[RADIOSET_4].other2)
	{
		synth->dispatch[RADIOSET_4].other2 = 0;
		return(0);
	}

	if (synth->dispatch[RADIOSET_5].other2)
	{
		synth->dispatch[RADIOSET_5].other2 = 0;
		return(0);
	}

	if (exclude)
		return(0);

	event.type = BRIGHTON_FLOAT;

/*
printf("lmmodCallback(%i, %i, %f)\n", panel, index, value);
*/

	synth->mem.param[DEVICE_COUNT + index] = value;

	switch (index) {
		case 0:
			/* Lower Arpeg */
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, ivalue);
			break;
		case 1:
			/* Upper Arpeg */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, ivalue);
			break;
		case 2:
		case 3:
		case 4:
			/* Arpeg direction Up/UpDown/Down */
			if (synth->dispatch[RADIOSET_4].other1 != -1)
			{
				synth->dispatch[RADIOSET_4].other2 = 1;

				if (synth->dispatch[RADIOSET_4].other1 != index)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, 4,
					synth->dispatch[RADIOSET_4].other1, &event);
			}
			synth->dispatch[RADIOSET_4].other1 = index;

			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_DIR, index - 2);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_DIR, index - 2);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_DIR, index - 2);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_DIR, index - 2);
			break;
		case 5:
		case 6:
		case 7:
			/* Range 3 octaves */
			/* Range 2 octaves */
			/* Range 1 octaves */
			/*
			 * We first need to ensure exclusion, then send the octave value.
			 */
			if (synth->dispatch[RADIOSET_5].other1 != -1)
			{
				synth->dispatch[RADIOSET_5].other2 = 1;

				if (synth->dispatch[RADIOSET_5].other1 != index)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, 4,
					synth->dispatch[RADIOSET_5].other1, &event);
			}
			synth->dispatch[RADIOSET_5].other1 = index;

			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RANGE, 7 - index);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RANGE, 7 - index);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RANGE, 7 - index);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RANGE, 7 - index);
			break;
		case 8:
			/* Lower Env Trigger */
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_TRIGGER, ivalue);
			break;
		case 9:
			/* Upper Env Trigger */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_TRIGGER, ivalue);
			break;
		case 10:
			/* Upper manual Seq Rec */
			event.value = 0;
			exclude = 1;
			brightonParamChange(synth->win, 4, 11, &event);
			exclude = 0;
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, ivalue);
			seqLearn = ivalue;
			break;
		case 11:
			/* Upper manual Seq Play */
			event.value = 0;
			exclude = 1;
			brightonParamChange(synth->win, 4, 10, &event);
			exclude = 0;
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, ivalue);
			seqLearn = 0;
			break;
		case 12:
			/* Lower manual Seq Rec */
			event.value = 0;
			exclude = 1;
			brightonParamChange(synth->win, 4, 13, &event);
			exclude = 0;
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, ivalue);
			seqLearn = ivalue;
			break;
		case 13:
			/* Lower manual Seq Play */
			event.value = 0;
			exclude = 1;
			brightonParamChange(synth->win, 4, 12, &event);
			exclude = 0;
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, ivalue);
			seqLearn = 0;
			break;
		case 14:
			/* Seq/Arp rate */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RATE, ivalue);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RATE, ivalue);

			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RATE, ivalue);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RATE, ivalue);
			break;
	}
	return(0);
}

static void
pro10ChordInsert(guiSynth *synth, int note, int layer)
{
	arpeggiatorMemory *seq = (arpeggiatorMemory *) synth->seq1.param;

	if (layer != KEY_PANEL)
		seq = (arpeggiatorMemory *) synth->seq2.param;

	if (seq->c_count == 0)
		seq->c_dif = note + synth->transpose;

	seq->chord[(int) (seq->c_count)]
		= (float) (note + synth->transpose - seq->c_dif);

//	if (jupiterDebug(synth, 0))
		printf("Chord put %i into %i\n", (int) seq->chord[(int) (seq->c_count)],
			(int) (seq->c_count));

	if ((seq->c_count += 1) >= BRISTOL_CHORD_MAX)
		seq->c_count = BRISTOL_CHORD_MAX;
}

static void
pro10SeqInsert(guiSynth *synth, int note, int layer)
{
	arpeggiatorMemory *seq = (arpeggiatorMemory *) synth->seq1.param;

	if (layer != KEY_PANEL) {
printf("	Learning on second layer\n");
		seq = (arpeggiatorMemory *) synth->seq2.param;
	}

	if (seq->s_max == 0)
		seq->s_dif = note + synth->transpose;

	seq->sequence[(int) (seq->s_max)] =
//		(float) (note + synth->transpose - seq->s_dif);
		(float) (note + synth->transpose);

//	if (pro10Debug(synth, 0))
		printf("Seq put %i into %i\n", (int) seq->sequence[(int) (seq->s_max)],
			(int) (seq->s_max));

	if ((seq->s_max += 1) >= BRISTOL_SEQ_MAX)
		seq->s_max = BRISTOL_SEQ_MAX;
}

/*
 * This used to send key events on one or two channels depending on the config.
 * Nothing really wrong with that however it would be better to actually tell
 * the emmulator what midi channels to use so that other master keyboards work
 * as expected.
 *
 * For 10 vs 5 layered voices (layer versus poly) we also need to add some
 * tweaks to the voicecounts.
 *
 * The code for all this is in the Jupiter, Bit* and OBXa.
 */
static int
lmCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int mchan;

/*	printf("lmCallback(%i, %i, %f)\n", panel, index, value); */

	if (global.libtest)
		return(0);

	if (panel == KEY_PANEL)
		mchan = synth->midichannel;
	else
		mchan = ((guiSynth *) synth->second)->midichannel;

	if ((chordLearn) && (value != 0))
		pro10ChordInsert(synth, index, panel);

	if ((seqLearn) && (value != 0))
		pro10SeqInsert(synth, index, panel);

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 */
	if (value)
		bristolMidiSendMsg(global.controlfd, mchan,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
	else
		bristolMidiSendMsg(global.controlfd, mchan,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);

	return(0);
}

static void
pro10PanelSwitch2(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (synth->flags & SUPPRESS)
		return;

	if (v != 0) {
		brightonEvent event;

		event.type = BRIGHTON_FLOAT;
		event.value = 0.0;
		brightonParamChange(synth->win, 0, PANEL_SWITCH, &event);
	}
}

static void
pro10PanelSwitch(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int i;

/*	printf("pro10PanelSwitch(%x, %i, %i, %i, %i, %1.0f)\n", */
/*		synth, fd, chan, c, o, global.synths->mem.param[PANEL_SWITCH]); */

	/*
	 * Prevent param changes when we are switching panels.
	 */
	synth->flags |= SUPPRESS;

	/*
	 * Go through all the active devices, and force the GUI to represent
	 * the new value.
	 */
	if (synth->mem.param[PANEL_SWITCH] == 0)
	{
		/*
		 * Go through all the params, and request an update to the GUI
		 */
		for (i = 0; i < ACTIVE_DEVS; i++)
		{
			event.type = BRIGHTON_FLOAT;
			event.value = ((guiSynth *) synth->second)->mem.param[i] + 1;
			brightonParamChange(synth->win, 0, i, &event);
			event.value = ((guiSynth *) synth->second)->mem.param[i];
			brightonParamChange(synth->win, 0, i, &event);
		}
	} else {
		for (i = 0; i < ACTIVE_DEVS; i++)
		{
			event.type = BRIGHTON_FLOAT;
			event.value = synth->mem.param[i] + 1;
			brightonParamChange(synth->win, 0, i, &event);
			event.value = synth->mem.param[i];
			brightonParamChange(synth->win, 0, i, &event);
		}
	}

	event.type = BRIGHTON_FLOAT;
	event.value = 1.0 - synth->mem.param[PANEL_SWITCH];
	brightonParamChange(synth->win, 0, PANEL_SWITCH + 1, &event);

	synth->flags &= ~SUPPRESS;
}

/*
pro10MemoryShim(synth, "prophet", 0, synth->bank + synth->location,
				synth->mem.active, 0, 0) < 0)
*/
static int
pro10MemoryShim(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	if ((synth->flags & REQ_DEBUG_MASK) >= REQ_MIDI_DEBUG2)
		printf("Shim %i\n", location);

	/*
	 * Decide which manual is operational, load it
	 */
	if (synth->mem.param[PANEL_SWITCH] == 0)
		synth = ((guiSynth *) synth->second);

	loadMemory(synth, "prophet", 0, location, synth->mem.active, 0, 0);
	loadSequence(&synth->seq1, "prophet", location, 0);
	fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);

	return(0);
}

static void
pro10MemoryLower(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;

/* printf("pro10MemoryLower(%i, %i)\n", c, o); */

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->flags & SUPPRESS)
		return;

	if (synth->dispatch[RADIOSET_2].other2)
	{
		synth->dispatch[RADIOSET_2].other2 = 0;
		return;
	}

	if (synth->mem.param[PANEL_SWITCH] != 0)
		brightonParamChange(synth->win, 0, PANEL_SWITCH + 1, &event);

	synth = ((guiSynth *) synth->second);

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
			if (synth->dispatch[RADIOSET_2].other1 != -1)
			{
				synth->dispatch[RADIOSET_2].other2 = 1;

				if (synth->dispatch[RADIOSET_2].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_2].other1 + RADIOSET_2, &event);
			}
			synth->dispatch[RADIOSET_2].other1 = o;

			if (synth->flags & BANK_SELECT) {
				if ((synth->bank * 10 + o * 10) >= 1000)
				{
					synth->location = o;
					synth->flags &= ~BANK_SELECT;
					if (loadMemory(synth, "prophet", 0,
						synth->bank + synth->location, synth->mem.active,
							0, BRISTOL_STAT) < 0)
						displayText(synth, "FRE", synth->bank + synth->location,
							DISPLAY_DEV_2);
					else
						displayText(synth, "PRG", synth->bank + synth->location,
							DISPLAY_DEV_2);
				} else {
					synth->bank = synth->bank * 10 + o * 10;
					if (loadMemory(synth, "prophet", 0,
						synth->bank + synth->location, synth->mem.active,
							0, BRISTOL_STAT) < 0)
						displayText(synth, "FRE", synth->bank + synth->location,
							DISPLAY_DEV_2);
					else
						displayText(synth, "BANK",
							synth->bank + synth->location, DISPLAY_DEV_2);
				}
			} else {
				if (synth->bank < 10)
					synth->bank = 10;
				synth->location = o;
				if (loadMemory(synth, "prophet", 0,
					synth->bank + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FRE", synth->bank + synth->location,
						DISPLAY_DEV_2);
				else
					displayText(synth, "PRG", synth->bank + synth->location,
						DISPLAY_DEV_2);
			}
			break;
		case 1:
			if (synth->bank < 10)
				synth->bank = 10;
			if (synth->location == 0)
				synth->location = 1;
			if (loadMemory(synth, "prophet", 0, synth->bank + synth->location,
				synth->mem.active, 0, 0) < 0)
				displayText(synth, "FRE", synth->bank + synth->location,
					DISPLAY_DEV_2);
			else {
				displayText(synth, "PRG", synth->bank + synth->location,
					DISPLAY_DEV_2);
				loadSequence(&synth->seq1,
					"prophet", synth->bank + synth->location, 0);
				fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
			}
			synth->flags &= ~BANK_SELECT;
			break;
		case 2:
			if (synth->bank < 10)
				synth->bank = 10;
			if (synth->location == 0)
				synth->location = 1;
			saveMemory(synth, "prophet", 0, synth->bank + synth->location,
				0);
			saveSequence(synth, "prophet", synth->bank + synth->location, 0);
			displayText(synth, "PRG", synth->bank + synth->location,
				DISPLAY_DEV_2);
			synth->flags &= ~BANK_SELECT;
			break;
		case 3:
			if (synth->flags & BANK_SELECT) {
				synth->flags &= ~BANK_SELECT;
				if (loadMemory(synth, "prophet", 0,
					synth->bank + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FRE", synth->bank + synth->location,
						DISPLAY_DEV_2);
				else
					displayText(synth, "PRG", synth->bank + synth->location,
						DISPLAY_DEV_2);
			} else {
				synth->bank = 0;
				displayText(synth, "BANK", synth->bank, DISPLAY_DEV_2);
				synth->flags |= BANK_SELECT;
			}
			break;
	}
/*	printf("	pro10Memory(B: %i L %i: %i)\n", */
/*		synth->bank, synth->location, o); */
}

static void
pro10Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;

/* printf("pro10Memory(%i, %i)\n", c, o); */

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->flags & SUPPRESS)
		return;

	if (synth->dispatch[RADIOSET_1].other2)
	{
		synth->dispatch[RADIOSET_1].other2 = 0;
		return;
	}

	if (synth->mem.param[PANEL_SWITCH] == 0)
		brightonParamChange(synth->win, 0, PANEL_SWITCH, &event);

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
			if (synth->dispatch[RADIOSET_1].other1 != -1)
			{
				synth->dispatch[RADIOSET_1].other2 = 1;

				if (synth->dispatch[RADIOSET_1].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_1].other1 + RADIOSET_1, &event);
			}
			synth->dispatch[RADIOSET_1].other1 = o;

			if (synth->flags & BANK_SELECT) {
				if ((synth->bank * 10 + o * 10) >= 1000)
				{
					synth->location = o;
					synth->flags &= ~BANK_SELECT;
					if (loadMemory(synth, "prophet", 0,
						synth->bank + synth->location, synth->mem.active,
							0, BRISTOL_STAT) < 0)
						displayText(synth, "FRE", synth->bank + synth->location,
							DISPLAY_DEV);
					else
						displayText(synth, "PRG", synth->bank + synth->location,
							DISPLAY_DEV);
				} else {
					synth->bank = synth->bank * 10 + o * 10;
					if (loadMemory(synth, "prophet", 0,
						synth->bank + synth->location, synth->mem.active,
							0, BRISTOL_STAT) < 0)
						displayText(synth, "FRE", synth->bank + synth->location,
							DISPLAY_DEV);
					else
						displayText(synth, "BANK",
							synth->bank + synth->location, DISPLAY_DEV);
				}
			} else {
				if (synth->bank < 10)
					synth->bank = 10;
				synth->location = o;
				if (loadMemory(synth, "prophet", 0,
					synth->bank + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FRE", synth->bank + synth->location,
						DISPLAY_DEV);
				else
					displayText(synth, "PRG", synth->bank + synth->location,
						DISPLAY_DEV);
			}
			break;
		case 1:
			if (synth->bank < 10)
				synth->bank = 10;
			if (synth->location == 0)
				synth->location = 1;
			if (loadMemory(synth, "prophet", 0, synth->bank + synth->location,
				synth->mem.active, 0, 0) < 0)
				displayText(synth, "FRE", synth->bank + synth->location,
					DISPLAY_DEV);
			else {
				displayText(synth, "PRG", synth->bank + synth->location,
					DISPLAY_DEV);
				loadSequence(&synth->seq1, "prophet",
					synth->bank + synth->location, 0);
				fillSequencer(synth,
					(arpeggiatorMemory *) synth->seq1.param, 0);
			}
			synth->flags &= ~BANK_SELECT;
			break;
		case 2:
			if (synth->bank < 10)
				synth->bank = 10;
			if (synth->location == 0)
				synth->location = 1;
			saveMemory(synth, "prophet", 0, synth->bank + synth->location,
				0);
			saveSequence(synth, "prophet", synth->bank + synth->location, 0);
			displayText(synth, "PRG", synth->bank + synth->location,
				DISPLAY_DEV);
			synth->flags &= ~BANK_SELECT;
			break;
		case 3:
			if (synth->flags & BANK_SELECT) {
				synth->flags &= ~BANK_SELECT;
				if (loadMemory(synth, "prophet", 0,
					synth->bank + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FRE", synth->bank + synth->location,
						DISPLAY_DEV);
				else
					displayText(synth, "PRG", synth->bank + synth->location,
						DISPLAY_DEV);
			} else {
				synth->bank = 0;
				displayText(synth, "BANK", synth->bank, DISPLAY_DEV);
				synth->flags |= BANK_SELECT;
			}
			break;
	}
/*	printf("	pro10Memory(B: %i L %i: %i)\n", */
/*		synth->bank, synth->location, o); */
}

static void
pro10Midi(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
		if ((newchan = synth->midichannel + 1) > 14)
		{
			synth->midichannel = 14;
			return;
		}
	}

	if (global.libtest == 0)
	{
		/*
		 * Do second manual, then upper.
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid2,
			127, 0, (BRISTOL_MIDICHANNEL|newchan) + 1);
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

/*printf("P: going to display: %x, %x\n", synth, synth->win); */
	displayText(synth, "MIDI", synth->midichannel + 1, DISPLAY_DEV);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
pro10Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue, sid;

/*
printf("pro10Callback(%i, %f): %1.0f\n", index, value, 
global.synths->mem.param[PANEL_SWITCH]);
*/

	if (synth->flags & SUPPRESS)
		return(0);

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (pro10App.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * (CONTROLLER_RANGE - 1);
	else
		sendvalue = value;

	if (synth->mem.param[PANEL_SWITCH] != 0)
	{
		synth->mem.param[index] = value;
		sid = synth->sid;
	} else {
		((guiSynth *) synth->second)->mem.param[index] = value;
		if (index >= ACTIVE_DEVS)
			synth->mem.param[index] = value;
		sid = synth->sid2;
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
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#endif

	return(0);
}

static void
pro10Balance(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float vol, bal;

	vol = synth->mem.param[54];
	bal = synth->mem.param[55];

	bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 36,
		(int) (vol * (1.0 - bal) * C_RANGE_MIN_1));

	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 36,
		(int) (vol * bal * C_RANGE_MIN_1));
}

static void
pro10Chord(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int sid = synth->sid;

	if ((synth->flags & OPERATIONAL) == 0)
		return;
	if (synth->flags & MEM_LOADING)
		return;


	if (exclude)
		return;

	event.type = BRIGHTON_FLOAT;

//	if (jupiterDebug(synth, 1))
		printf("Chord request: %i %i %i (p=%f)\n", c, o, v, synth->mem.param[PANEL_SWITCH]);

	if (synth->mem.param[PANEL_SWITCH] == 0)
		sid = synth->sid2;

	/*
	 * We can be called from Record or Play with values of on or off. The first
	 * step is always to stop any peer processing such as stopping recording if
	 * Play is selected, etc.
	 *
	 * After that we decide what to do, it may be start/stop recording or 
	 * start/stop playing a chord.
	 */
	if (c == 0)
	{
		/* Record */
		if  (v != 0)
		{
			if (synth->seq1.param == NULL)
				loadSequence(&synth->seq1, "prophet", 0, 0);
			if (synth->seq2.param == NULL)
				loadSequence(&synth->seq2, "prophet", 0, BRISTOL_SID2);

			chordLearn = 1;

//			if (jupiterDebug(synth, 0))
				printf("Chord learn requested %x\n",  synth->flags);

			if (synth->mem.param[PANEL_SWITCH] == 0)
				synth->seq2.param[1] = 0;
			else
				synth->seq1.param[1] = 0;

			/*
			 * This is to start a re-seq of the chord. Disable play if it was
			 * selected then request resequence.
			 */
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 0);

			event.value = 0;
			exclude = 1;
			brightonParamChange(synth->win, 0, 58, &event);
			exclude = 0;

			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 1);
		} else {
			chordLearn = 0;
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);
		}

		return;
	}

	if (chordLearn == 1) {
		/*
		 * This is a button press during the learning sequence, in which case
		 * it needs to be terminated.
		 */
		chordLearn = 0;
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);

		event.value = 0;
		exclude = 1;
		brightonParamChange(synth->win, 0, 57, &event);
		exclude = 0;
	}

	bristolMidiSendMsg(global.controlfd, sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, v);

	return;
}

static void
keyTracking(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (synth->mem.param[PANEL_SWITCH] -= 0)
		bristolMidiSendMsg(fd, synth->sid, 4, 3, v / 2);
	else
		bristolMidiSendMsg(fd, synth->sid2, 4, 3, v / 2);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
pro10Init(brightonWindow *win)
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

	printf("Initialise the pro10 link to bristol: %p %p\n",
		synth, synth->win);

	synth->mem.param = (float *) brightonmalloc(
		(DEVICE_COUNT + P10_MOD_COUNT + PMOD_COUNT) * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
	synth->dispatch = (dispatcher *)
		brightonmalloc(
			(DEVICE_COUNT + P10_MOD_COUNT + PMOD_COUNT) * sizeof(dispatcher));
	dispatch = synth->dispatch;

	synth->second = brightonmalloc(sizeof(guiSynth));
	bcopy(synth, ((guiSynth *) synth->second), sizeof(guiSynth));
	((guiSynth *) synth->second)->mem.param =
		(float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	((guiSynth *) synth->second)->mem.count = DEVICE_COUNT;
	((guiSynth *) synth->second)->mem.active = ACTIVE_DEVS;
	((guiSynth *) synth->second)->dispatch = synth->dispatch;
	/* If we have a default voice count then limit it */
	if (synth->voices == BRISTOL_VOICECOUNT) {
		((guiSynth *) synth->second)->voices = 5;
		synth->voices = 10;
	} else
		((guiSynth *) synth->second)->voices = synth->voices >> 1;

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets.
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		int v = synth->voices;

		synth->synthtype = BRISTOL_PROPHET;

		bcopy(&global, &manual, sizeof(guimain));
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

		manual.flags |= BRISTOL_CONN_FORCE|BRIGHTON_NOENGINE;
		manual.host = global.host;
		manual.port = global.port;

		synth->voices = ((guiSynth *) synth->second)->voices;
		/*
		 * This looks a bit evil now but whatever happens it gets fixed later
		 * when the dual/split/layer is configured - that will 'retweak' the
		 * midi channels.
		 */
		if (++synth->midichannel > 15)
			synth->midichannel = 15;

		if ((synth->sid2 = initConnection(&manual, synth)) < 0)
			return(-1);

		global.manualfd = manual.controlfd;
		global.manual = &manual;
		manual.manual = &global;

		synth->midichannel--;
		synth->voices = v;
	} else {
		synth->sid = 5;
		synth->sid2 = 10;
	}

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = pro10MidiSendMsg;

	dispatch[0].controller = 126;
	dispatch[0].operator = 6;
	dispatch[1].controller = 126;
	dispatch[1].operator = 7;
	dispatch[2].controller = 126;
	dispatch[2].operator = 8;
	dispatch[3].controller = 126;
	dispatch[3].operator = 9;
	dispatch[4].controller = 126;
	dispatch[4].operator = 10;

	dispatch[5].controller = 2;
	dispatch[5].operator = 0;
	dispatch[6].controller = 126;
	dispatch[6].operator = 24;
	dispatch[7].controller = 126;
	dispatch[7].operator = 25;
	dispatch[8].controller = 126;
	dispatch[8].operator = 26;

	dispatch[9].controller = 126;
	dispatch[9].operator = 11;
	dispatch[10].controller = 126;
	dispatch[10].operator = 12;
	dispatch[11].controller = 126;
	dispatch[11].operator = 13;
	dispatch[12].controller = 126;
	dispatch[12].operator = 14;
	dispatch[13].controller = 126;
	dispatch[13].operator = 15;
	dispatch[14].controller = 126;
	dispatch[14].operator = 16;

	dispatch[15].controller = 0;
	dispatch[15].operator = 1;
	dispatch[16].controller = 0;
	dispatch[16].operator = 4;
	dispatch[17].controller = 0;
	dispatch[17].operator = 6;
	dispatch[18].controller = 0;
	dispatch[18].operator = 0;
	dispatch[19].controller = 0;
	dispatch[19].operator = 7;

	dispatch[20].controller = 1;
	dispatch[20].operator = 1;
	dispatch[21].controller = 1;
	dispatch[21].operator = 2;
	dispatch[22].controller = 1;
	dispatch[22].operator = 4;
	dispatch[23].controller = 1;
	dispatch[23].operator = 5;
	dispatch[24].controller = 1;
	dispatch[24].operator = 6;
	dispatch[25].controller = 1;
	dispatch[25].operator = 0;
	dispatch[26].controller = 126;
	dispatch[26].operator = 18;
	dispatch[27].controller = 126;
	dispatch[27].operator = 19;

	dispatch[28].controller = 126;
	dispatch[28].operator = 0;
	dispatch[29].controller = 126;
	dispatch[29].operator = 1;

	dispatch[30].controller = 126;
	dispatch[30].operator = 20;
	dispatch[31].controller = 126;
	dispatch[31].operator = 21;
	dispatch[32].controller = 126;
	dispatch[32].operator = 22;

	dispatch[33].controller = 4;
	dispatch[33].operator = 0;
	dispatch[34].controller = 4;
	dispatch[34].operator = 1;
	dispatch[35].controller = 126;
	dispatch[35].operator = 23;
	dispatch[36].controller = 4;
	dispatch[36].routine = (synthRoutine) keyTracking;
	dispatch[36].operator = 3;
	dispatch[37].controller = 3;
	dispatch[37].operator = 0;
	dispatch[38].controller = 3;
	dispatch[38].operator = 1;
	dispatch[39].controller = 3;
	dispatch[39].operator = 2;
	dispatch[40].controller = 3;
	dispatch[40].operator = 3;

	dispatch[41].controller = 5;
	dispatch[41].operator = 0;
	dispatch[42].controller = 5;
	dispatch[42].operator = 1;
	dispatch[43].controller = 5;
	dispatch[43].operator = 2;
	dispatch[44].controller = 5;
	dispatch[44].operator = 3;

	dispatch[45].controller = 126;
	dispatch[45].operator = 2;
	dispatch[45].routine = (synthRoutine) pro10Tune;
	/* Volume */
	dispatch[46].controller = 5;
	dispatch[46].operator = 4;

	dispatch[47].controller = 126;
	dispatch[47].operator = 34;

	dispatch[48].controller = 1;
	dispatch[48].operator = 1;
	dispatch[48].routine = (synthRoutine) midiRelease;
	dispatch[49].controller = 126;
	dispatch[49].operator = 3;
	dispatch[49].routine = (synthRoutine) multiA440;
	dispatch[50].controller = 1;
	dispatch[50].operator = 1;
	dispatch[50].routine = (synthRoutine) multiTune;

	/* D/L/P */
	dispatch[51].controller = 0;
	dispatch[51].routine = (synthRoutine) pro10DLP;
	dispatch[52].controller = 1;
	dispatch[52].routine = (synthRoutine) pro10DLP;
	dispatch[53].controller = 2;
	dispatch[53].routine = (synthRoutine) pro10DLP;

	/* Master gain */
	dispatch[54].controller = 126;
	dispatch[54].operator = 36;
	dispatch[54].routine = (synthRoutine) pro10Balance;
	/* Master balance */
	dispatch[55].controller = 126;
	dispatch[55].operator = 35;
	dispatch[55].routine = (synthRoutine) pro10Balance;
	dispatch[56].routine = (synthRoutine) pro10Tune;

	/* Hold/Chord */
	dispatch[57].controller = 0;
	dispatch[57].operator = BRISTOL_CHORD_ENABLE;
	dispatch[57].routine = (synthRoutine) pro10Chord;
	dispatch[58].controller = 1;
	dispatch[58].operator = BRISTOL_CHORD_ENABLE;
	dispatch[58].routine = (synthRoutine) pro10Chord;

	dispatch[71].operator = 1;
	dispatch[72].operator = 2;
	dispatch[73].operator = 3;
	dispatch[74].operator = 4;
	dispatch[75].operator = 5;
	dispatch[76].operator = 6;
	dispatch[77].operator = 7;
	dispatch[78].operator = 8;

	dispatch[79].controller = 1;
	dispatch[80].controller = 2;
	dispatch[81].controller = 3;

	dispatch[71].routine =
		dispatch[72].routine =
		dispatch[73].routine =
		dispatch[74].routine =
		dispatch[75].routine =
		dispatch[76].routine =
		dispatch[77].routine =
		dispatch[78].routine =
		dispatch[79].routine =
		dispatch[80].routine =
		dispatch[81].routine
			= (synthRoutine) pro10Memory;

	dispatch[82].operator = 1;
	dispatch[83].operator = 2;
	dispatch[84].operator = 3;
	dispatch[85].operator = 4;
	dispatch[86].operator = 5;
	dispatch[87].operator = 6;
	dispatch[88].operator = 7;
	dispatch[89].operator = 8;

	dispatch[90].controller = 1;
	dispatch[91].controller = 2;
	dispatch[92].controller = 3;

	dispatch[82].routine =
		dispatch[83].routine =
		dispatch[84].routine =
		dispatch[85].routine =
		dispatch[86].routine =
		dispatch[87].routine =
		dispatch[88].routine =
		dispatch[89].routine =
		dispatch[90].routine =
		dispatch[91].routine =
		dispatch[92].routine
			= (synthRoutine) pro10MemoryLower;

	dispatch[93].routine = dispatch[94].routine =
		(synthRoutine) pro10Midi;
	dispatch[93].controller = 1;
	dispatch[94].controller = 2;

	dispatch[95].routine = (synthRoutine) pro10PanelSwitch;
	dispatch[96].routine = (synthRoutine) pro10PanelSwitch2;

	dispatch[RADIOSET_1].other1 = -1;
	dispatch[RADIOSET_2].other1 = -1;
	dispatch[RADIOSET_3].other1 = -1;
	dispatch[RADIOSET_4].other1 = -1;
	dispatch[RADIOSET_5].other1 = -1;
	dispatch[DEVICE_COUNT].other1 = -1;

	/* Tune osc-1 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 2, 8192);
	/* Gain on filter contour */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4,
		CONTROLLER_RANGE - 1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 4,
		CONTROLLER_RANGE - 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 4);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 4, 4, 4);

	return(0);
}

static void
memForce(guiSynth *synth)
{
	brightonEvent event;
	int i;

	synth->flags |= SUPPRESS;

	event.type = BRIGHTON_FLOAT;
	event.value = 1;

	for (i = 0; i < ACTIVE_DEVS; i++)
	{
		brightonParamChange(synth->win, 0, i, &event);
	}
	event.value = 0;
	for (i = 0; i < ACTIVE_DEVS; i++)
	{
		brightonParamChange(synth->win, 0, i, &event);
	}

	synth->flags &= ~SUPPRESS;
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
pro10Configure(brightonWindow *win)
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
	synth->keypanel2 = 3;
	synth->transpose = 12;

	synth->mem.param[PANEL_SWITCH] = 1;

	synth->bank = 10;
	synth->location = 1;
	memForce(synth);
	loadMemory(synth, "prophet", 0, initmem, synth->mem.active, 0, 0);
	loadSequence(&synth->seq1, "prophet", initmem, 0);
	fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);

	synth->mem.param[PANEL_SWITCH] = 0;

	((guiSynth *) synth->second)->resources = synth->resources;
	((guiSynth *) synth->second)->win = synth->win;
	((guiSynth *) synth->second)->bank = 10;
	((guiSynth *) synth->second)->location = 2;
	((guiSynth *) synth->second)->panel = 0;
	((guiSynth *) synth->second)->mem.active = ACTIVE_DEVS;
	((guiSynth *) synth->second)->transpose = 12;

	memForce(((guiSynth *) synth->second));
	loadMemory(((guiSynth *) synth->second), "prophet", 0, initmem  + 1,
		synth->mem.active, 0, 0);

	synth->mem.param[PANEL_SWITCH] = 0;

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	brightonPut(win,
		"bitmaps/blueprints/prophet10shade.xpm", 0, 0, win->width, win->height);
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL + 2, -1, &event);

	event.type = BRIGHTON_FLOAT;
	event.value = 1.0f;
	brightonParamChange(synth->win, 2, 2, &event);
	brightonParamChange(synth->win, 2, 3, &event);
	brightonParamChange(synth->win, 2, 4, &event);
	brightonParamChange(synth->win, 2, 5, &event);
	brightonParamChange(synth->win, 4, 2, &event);
	brightonParamChange(synth->win, 4, 6, &event);
	brightonParamChange(synth->win, 4, 8, &event);
	brightonParamChange(synth->win, 4, 9, &event);
	brightonParamChange(synth->win, 0, RADIOSET_1 + 1, &event);
	brightonParamChange(synth->win, 0, RADIOSET_2 + 2, &event);
	configureGlobals(synth);

	brightonParamChange(synth->win, 0, 51, &event);

	/* Master controls */
	event.value = 0.8f;
	brightonParamChange(synth->win, 0, 54, &event);
	event.value = 0.5f;
	brightonParamChange(synth->win, 0, 55, &event);
	event.value = 0.5f;
	brightonParamChange(synth->win, 0, 56, &event);
	event.value = 1.0;
	brightonParamChange(synth->win, 0, PANEL_SWITCH, &event);

	synth->loadMemory = (loadRoutine) pro10MemoryShim;

	return(0);
}

