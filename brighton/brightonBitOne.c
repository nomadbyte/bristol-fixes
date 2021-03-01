
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

static int bitoneInit();
static int bitoneConfigure();
static int bitoneCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);
static int bitoneExtendedEntry(guiSynth *, int, int, int, int, int);
static int bitoneKeyCallback(brightonWindow *, int, int, float);

extern guimain global;
static guimain manual;

#include "brightonKeys.h"

/*
 * This will be changed for an eventual release however this allows me to test
 * all the emulations with the same memories.
#define SYNTH_NAME "bitone"
 */
#define SYNTH_NAME synth->resources->name

/*
 * We have 75 controls, will generate 150 for voice parameters. After that:
 *	2 + 2 + 1 + 10 + 3 + 8 + 8 = 184 and then a few for inc/dec buttons, etc.
 */
#define ACTIVE_DEVS 200
#define DEVICE_COUNT (ACTIVE_DEVS + 39) 
#define DISPLAY_DEV (ACTIVE_DEVS + 30) 
#define MEM_MGT ACTIVE_DEVS
#define MIDI_MGT (MEM_MGT + 12)
#define KEY_HOLD (7)

#define KEY_PANEL 1

#define SAVE_MODE (ACTIVE_DEVS - 4) 
#define SAVE_STEREO (ACTIVE_DEVS - 3) 
#define SAVE_PEER (ACTIVE_DEVS - 2) 

#define STEREO_BUTTON (ACTIVE_DEVS + 2) 
#define PARK_BUTTON (ACTIVE_DEVS + 22) 
#define COMPARE_BUTTON (ACTIVE_DEVS + 23) 

#define DCO_1_EXCLUSIVE 95
#define DCO_2_EXCLUSIVE 96
#define EXTENDED_BUTTON 00
#define NON_EXTENDED_BUTTON 100

#define RADIOSET_1 (ACTIVE_DEVS + 9)
#define RADIOSET_2 (ACTIVE_DEVS + 19)
#define RADIOSET_3 ACTIVE_DEVS

#define MODE_SINGLE	0
#define MODE_SPLIT	1
#define MODE_LAYER	2

#define ENTER_ADDRESS 0
#define ENTER_LOWER 1
#define ENTER_UPPER 2

static int mode = MODE_SINGLE, setsplit = 0.0, selectExtended = 0;
#define DEF_SPLIT 72

/*
 * Max data entry identifier for bit-1 parameters. Extended data entry needs
 * to be handled separately.
 */
#define ENTRY_MAX 100
#define ENTRY_POT (ACTIVE_DEVS + 8) 

static int entryPoint = ENTER_ADDRESS;
static int B1display[5];

/*
 * This is the scratchpad memory - we have on disk memories, on synth memories
 * with potentially one loaded memory per layer, then we have the prePark mem
 * where changes are made temporarily until parkted into the loaded memory.
 *
 * All parameter changes go into these layers and can be used as a quick compare
 * to the last loaded memories. If the settings are parked they become semi
 * permanant (as they have not actually been written to disk yet). We should
 * have a double click on Park to save both memories to disk as well.
 *
 * We need 5 scratchpads, one for each layer, a backup for each layer, and
 * a final one for memory copies.
 */
static float scratchpad0[ACTIVE_DEVS];
static float scratchpad1[ACTIVE_DEVS];
static float scratchpad2[ACTIVE_DEVS];
static float scratchpad3[ACTIVE_DEVS];
static float scratchpad4[ACTIVE_DEVS];
static float scratchpad5[DEVICE_COUNT];

/*
 * We are going to require 4 to cover upper and lower shadows, a fifth to
 * cover park/compare shadow of active memory, and we will take a sixth for
 * saving other memory types
 */
static guiSynth prePark[6];

static int memLower = 0, memUpper = -1;

static int dc;

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
 * This example is for a bitoneBristol type synth interface.
 */
#define S1 418
#define S2 312
#define S3 228
#define S4 644
#define S5 262
#define S6 558
#define S7 644
#define S8 600
#define D1 42
#define W1 33

/*
 * We really need to define parameter types and ranges here to make the
 * interface complete. For example, the midi channels only go from 1 to 16 and
 * when selected as the input then the pot should configure this range.
 *
 * Similarly, if this is a button it should be 0 for the first half and 1 for
 * the second half.
 *
 * This only applies to the displays, not to the parameters.
 *
 * Continuous controllers actually deliver a value between 0 and 1 to the 
 * engine - the Inc/Dec buttons actually do the steps emulating the original
 * by going from 0.0 to 1.0 in the given number of stages. The pot, however,
 * is continuous. Memories save the floating point value between 0 and 1.0, 
 * the stepped values are firstly put on the display and sent to the engine
 * as the last stage of interpretation.
 *
 * The stepped controllers deliver integer values along the given range.
 */
#define BITONE_CONTINUOUS 0
#define BITONE_STEPPED 1

#define B1_INACTIVE			0x001 /* Skip this parameter */
#define B1_INHERIT_PRI		0x002 /* Copy from the primary layer when loading */
#define B1_INACTIVE_PEER	0x004 /* Not active in upper (second load) layer */
#define B1_USE_PRI			0x008 /* Take value from primary only */
#define B1_ALWAYS			0x010 /* All editing in double/split mode */
#define B1_INHERIT_SEC		0x020 /* Copy to the primary layer when changing */
#define B1_NO_LOAD			0x040 /* All editing in double/split mode */
#define B1_INHERIT_120		0x080 /* Copy to the primary layer when changing */
#define B1_INHERIT_121		0x100 /* Copy to the primary layer when changing */
#define B1_INHERIT_122		0x200 /* Copy to the primary layer when changing */

typedef struct B1range {
	int type;
	float min;
	float max;
	int op, cont;  /* link to the engine code */
	int flags;
} b1range;

/*
 * STEPPED controllers are sent as that integer value. CONTINUOUS are sent
 * as values from 0 to 1.0 but the range values specify how they are displayed
 * on the LEDs.
 */
b1range bitoneRange[ACTIVE_DEVS] = {
	{BITONE_STEPPED, 0, 1, 126, 126, B1_NO_LOAD}, /* Dummy entry */
	/* LFO-1 */
	{BITONE_STEPPED,    0, 1, 126, 13, 0},
	{BITONE_STEPPED,    0, 1, 126, 14, 0},
	{BITONE_STEPPED,    0, 1, 126, 15, 0},
	{BITONE_CONTINUOUS, 0, 1, 126, 17, 0},
	{BITONE_CONTINUOUS, 0, 1, 126, 18, 0},
	{BITONE_CONTINUOUS, 0, 1, 126, 19, 0},
	{BITONE_CONTINUOUS, 0, 1, 126, 20, 0},
	{BITONE_CONTINUOUS, 0, 31, 2, 0, 0},
	{BITONE_CONTINUOUS, 0, 63, 0, 0, 0},
	{BITONE_CONTINUOUS, 0, 31, 0, 2, 0},
	{BITONE_CONTINUOUS, 1, 32, 2, 4, 0},
	{BITONE_CONTINUOUS, 0, 31, 126, 12, 0},
	/* VCF - 13 */
	{BITONE_CONTINUOUS, 0, 63, 7, 0, 0},
	{BITONE_CONTINUOUS, 0, 63, 7, 1, 0},
	{BITONE_CONTINUOUS, 0, 63, 7, 2, 0},
	{BITONE_CONTINUOUS, 0, 63, 7, 3, 0},
	{BITONE_CONTINUOUS, 0, 63, 7, 5, 0},
	{BITONE_CONTINUOUS, 0,  1, 6, 3, 0},
	{BITONE_CONTINUOUS, 0, 63, 6, 0, 0},
	{BITONE_CONTINUOUS, 0, 63, 6, 1, 0},
	{BITONE_CONTINUOUS, 0, 63, 6, 2, 0},
	{BITONE_CONTINUOUS, 0, 63, 7, 8, 0},
	{BITONE_STEPPED, 0, 1, 126, 39, 0}, /* invert */
	/* DCO 1 - 24 */
	{BITONE_CONTINUOUS, 0,  1, 4, 0, 0},
	{BITONE_CONTINUOUS, 0,  1, 4, 1, 0},
	{BITONE_CONTINUOUS, 0,  1, 4, 2, 0},
	{BITONE_CONTINUOUS, 0,  1, 4, 3, 0},
	{BITONE_CONTINUOUS, 0,  1, 4, 4, 0},
	{BITONE_CONTINUOUS, 0,  1, 4, 5, 0},
	{BITONE_CONTINUOUS, 0,  1, 4, 6, 0},
	{BITONE_STEPPED,    0, 24, 4, 8, 0},
	{BITONE_CONTINUOUS, 0, 31, 4, 7, 0},
	{BITONE_CONTINUOUS, 0, 63, 4, 10, 0},
	{BITONE_CONTINUOUS, 0, 63, 126, 8, 0},
	/* DCO 2 - 35 */
	{BITONE_CONTINUOUS, 0,  1, 5, 0, 0},
	{BITONE_CONTINUOUS, 0,  1, 5, 1, 0},
	{BITONE_CONTINUOUS, 0,  1, 5, 2, 0},
	{BITONE_CONTINUOUS, 0,  1, 5, 3, 0},
	{BITONE_CONTINUOUS, 0,  1, 5, 4, 0},
	{BITONE_CONTINUOUS, 0,  1, 5, 5, 0},
	{BITONE_CONTINUOUS, 0,  1, 5, 6, 0},
	{BITONE_STEPPED,    0, 24, 5, 8, 0},
	{BITONE_CONTINUOUS, 0, 31, 5, 7, 0},
	{BITONE_CONTINUOUS, 0, 63, 5, 10, 0},
	{BITONE_CONTINUOUS, 0, 63, 5, 9, 0},
	/* VCA - 46 */
	{BITONE_CONTINUOUS, 0, 63, 9, 5, 0},
	{BITONE_CONTINUOUS, 0, 63, 9, 8, 0},
	{BITONE_CONTINUOUS, 0, 63, 9, 4, 0},
	{BITONE_CONTINUOUS, 0, 63, 9, 0, 0},
	{BITONE_CONTINUOUS, 0, 63, 9, 1, 0},
	{BITONE_CONTINUOUS, 0, 63, 9, 2, 0},
	{BITONE_CONTINUOUS, 0, 63, 9, 3, 0},
	/* LFO-2 - 53 */
	{BITONE_STEPPED,    0, 1, 126, 21, 0},
	{BITONE_STEPPED,    0, 1, 126, 22, 0},
	{BITONE_STEPPED,    0, 1, 126, 23, 0},
	{BITONE_CONTINUOUS, 0, 1, 126, 25, 0},
	{BITONE_CONTINUOUS, 0, 1, 126, 26, 0},
	{BITONE_CONTINUOUS, 0, 1, 126, 27, 0},
	{BITONE_CONTINUOUS, 0, 1, 126, 28, 0},
	{BITONE_CONTINUOUS, 0, 31, 3, 0, 0},
	{BITONE_CONTINUOUS, 0, 63, 1, 0, 0},
	{BITONE_CONTINUOUS, 0, 31, 1, 2, 0},
	{BITONE_CONTINUOUS, 1, 32, 3, 4, 0},
	/* Split Point, transpose and layer volumes - 64 */
	{BITONE_STEPPED, 1, 60, 126, 101, B1_ALWAYS|B1_INHERIT_121},
	{BITONE_STEPPED, 1, 60, 126, 101, B1_ALWAYS|B1_INHERIT_122},
	{BITONE_CONTINUOUS, 0, 63, 126, 101, B1_USE_PRI|B1_ALWAYS},
	{BITONE_CONTINUOUS, 0, 63, 126, 101, B1_USE_PRI|B1_ALWAYS},
	/* MIDI REC - 68 */
	{BITONE_CONTINUOUS, 0, 63, 126, 43, 0},
	{BITONE_STEPPED, 0, 25, 126, 42, 0}, /* sensitivity */
	{BITONE_STEPPED, 0, 2, 126, 101, 0}, /* debug */
	{BITONE_STEPPED, 0, 1, 126, 101, 0}, /* Accept Program Change */
	{BITONE_STEPPED, 0, 1, 126, 101, 0}, /* OMNI */
	{BITONE_STEPPED, 1, 16, 126, 101, B1_ALWAYS|B1_INHERIT_120},
	/* MIDI TRS - 74 */
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD}, /* MEM SEARCH UP */
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD}, /* MEM SEARCH DOWN */
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD}, /* release */
	/* BIT-99 FM DCO-1->2, SYNC 2-<1, GLIDE, Unison - 77 */
	{BITONE_CONTINUOUS, 0, 31, 126, 40, 0},
	{BITONE_STEPPED, 0, 1, 126, 38, 0},
	{BITONE_CONTINUOUS, 0, 63, 126, 0,
		B1_INHERIT_SEC|B1_USE_PRI|B1_INACTIVE_PEER|B1_ALWAYS},
	{BITONE_STEPPED, 0, 1, 126, 7, B1_ALWAYS},
	/* 81 upwards are bit-100 options */
	{BITONE_STEPPED, 0, 1, 126, 101, 0},
	{BITONE_CONTINUOUS, 0, 31, 126, 34, 0},
	{BITONE_CONTINUOUS, 0, 31, 126, 35, 0},
	{BITONE_CONTINUOUS, 0, 64, 0, 3, 0},
	{BITONE_CONTINUOUS, 0, 63, 2, 5, 0},
	{BITONE_STEPPED, 0, 1, 126, 29, 0},
	{BITONE_STEPPED, 0, 1, 126, 101, 0},
	{BITONE_CONTINUOUS, 0, 31, 126, 36, 0},
	{BITONE_CONTINUOUS, 0, 31, 126, 37, 0},
	{BITONE_CONTINUOUS, 0, 64, 1, 3, 0},
	{BITONE_CONTINUOUS, 0, 63, 3, 5, 0},
	{BITONE_STEPPED, 0, 1, 126, 30, 0},
	{BITONE_CONTINUOUS, 0, 63, 126, 32, 0},
	{BITONE_CONTINUOUS, 0, 63, 126, 33, 0},
	{BITONE_STEPPED, 0, 1, 126, 101, 0},
	{BITONE_STEPPED, 0, 1, 126, 101, 0},
	{BITONE_STEPPED, 0, 2, 6, 4, 0},
	{BITONE_CONTINUOUS, 0, 63, 126, 41, 0},
	{BITONE_STEPPED, 0, 1, 126, 31, 0},
	/* 100 is the extended entry deselector */
	{BITONE_STEPPED, 0, 1, 0, 0, B1_NO_LOAD},
	/* 101 is the writethru scratchpad */
	{BITONE_STEPPED, 0, 1, 0, 0, 0},
	{BITONE_STEPPED, 0, 1, 0, 0, 0},
	/* LFO Sync */
	{BITONE_STEPPED, 0, 1, 0, 1, 0},
	{BITONE_STEPPED, 0, 1, 1, 1, 0},
	/* Env rezero - 105 */
	{BITONE_STEPPED, 0, 1, 7, 6, 0},
	{BITONE_STEPPED, 0, 1, 9, 6, 0},
	/* LFO rezero */
	{BITONE_STEPPED, 0, 1, 2, 6, 0},
	{BITONE_STEPPED, 0, 1, 3, 6, 0},
	/* DEBUG debug Debug dbg 3 levels - 109 */
	{BITONE_STEPPED, 0, 5, 126, 101, 0},
	/* Channel gains - LR Mono LR Stereo, first lower then upper - 110*/
	{BITONE_CONTINUOUS, 0, 99, 0, 0, 0},
	{BITONE_CONTINUOUS, 0, 99, 0, 0, 0},
	{BITONE_CONTINUOUS, 0, 99, 0, 0, 0},
	{BITONE_CONTINUOUS, 0, 99, 0, 0, 0},
	{BITONE_CONTINUOUS, 0, 99, 0, 0, 0},
	{BITONE_CONTINUOUS, 0, 99, 0, 0, 0},
	{BITONE_CONTINUOUS, 0, 99, 0, 0, 0},
	{BITONE_CONTINUOUS, 0, 99, 0, 0, 0},
	{BITONE_STEPPED, 0, 1, 126, 101, 0},
	{BITONE_STEPPED, 0, 99, 126, 101, 0},
	/* Channel/split/transpose coherency - holders only - 120 */
	{BITONE_STEPPED, 0, 1, 126, 101, 0},
	{BITONE_STEPPED, 0, 1, 126, 101, 0},
	{BITONE_STEPPED, 0, 1, 126, 101, 0},
	{BITONE_STEPPED, 0, 1, 126, 101, 0}, /* NRP Enable */
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	/* Search Up/Down - 130 */
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD},
	/* Env conditional -  132 */
	{BITONE_STEPPED, 0, 1, 7, 7, 0},
	{BITONE_STEPPED, 0, 1, 9, 7, 0},
	{BITONE_STEPPED, 0, 1, 2, 7, 0},
	{BITONE_STEPPED, 0, 1, 3, 7, 0},
	{BITONE_STEPPED, 0, 1, 10, 1, 0},
	{BITONE_CONTINUOUS, 0, 63, 10, 2, 0},
	{BITONE_CONTINUOUS, 0, 63, 126, 101, 0}, /* GLIDE */
	{BITONE_CONTINUOUS, 0, 63, 126, 101, 0}, /* Gain */
	/* Square wave - 140 */
	{BITONE_CONTINUOUS, 0, 63, 4, 11, 0},
	{BITONE_CONTINUOUS, 0, 63, 4, 12, 0},
	{BITONE_CONTINUOUS, 0, 63, 5, 11, 0},
	{BITONE_CONTINUOUS, 0, 63, 5, 12, 0},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	/* Arpeggiator - 150 */
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_ARPEG_ENABLE, B1_INACTIVE},
	{BITONE_STEPPED, 0, 3, 125, BRISTOL_ARPEG_DIR, B1_INACTIVE},
	{BITONE_STEPPED, 0, 3, 125, BRISTOL_ARPEG_RANGE, B1_INACTIVE},
	{BITONE_CONTINUOUS, 0, 99, 125, BRISTOL_ARPEG_RATE, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_ARPEG_CLOCK, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_ARPEG_TRIGGER, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_ARPEG_POLY_2, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_CHORD_ENABLE, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_CHORD_RESEQ, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_SEQ_ENABLE, B1_INACTIVE},
	{BITONE_STEPPED, 0, 3, 125, BRISTOL_SEQ_DIR, B1_INACTIVE},
	{BITONE_STEPPED, 0, 3, 125, BRISTOL_SEQ_RANGE, B1_INACTIVE},
	{BITONE_CONTINUOUS, 0, 99, 125, BRISTOL_SEQ_RATE, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_SEQ_CLOCK, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_SEQ_TRIGGER, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 125, BRISTOL_SEQ_RESEQ, B1_INACTIVE},
	/* Free 166 */
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_INACTIVE},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD},
	{BITONE_STEPPED, 0, 1, 126, 101, B1_NO_LOAD},
	/* This should be 199, the wheel controller for the second LFO */
	{BITONE_CONTINUOUS, 0, 31, 126, 12, B1_NO_LOAD},

};

static
brightonLocations locations[DEVICE_COUNT] = {
	/*
	 * 100 withdrawn buttons for later use
	 *
	 * The first one was just to look at some stuff with use of the transparency
	 * layer to highlight entry, exit and button click so that this panel of
	 * parameters can be used to select data entry address selections.
	 *
	 * The first is a dummy here since the parameters start from '1'.
	 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* LFO-1 - 1 */
	{"", 9, 291, S1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 291, S1 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 291, S1 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 291, S1 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 291, S1 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 291, S1 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 362, S1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 362, S1 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 362, S1 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 362, S1 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 362, S1 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 362, S1 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},

	/* VCF - 13 */
	{"", 9, 409, S2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 409, S2 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 409, S2 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 409, S2 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 409, S2 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 478, S2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 478, S2 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 478, S2 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 478, S2 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 478, S2 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 478, S2 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* DCO 1 - 24 */
	{"", 9, 524, S3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 524, S3 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 524, S3 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 524, S3 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 576, S3 + D1, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 576, S3 + D1 * 2, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 576, S3 + D1 * 3, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 636, S3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 636, S3 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 636, S3 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 636, S3 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* DCO 2 - 35 */
	{"", 9, 524, S4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 524, S4 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 524, S4 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 524, S4 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 576, S4, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 576, S4 + D1 * 1, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 576, S4 + D1 * 2, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 636, S4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 636, S4 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 636, S4 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 636, S4 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* VCA - 46 */
	{"", 9, 681, S5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 681, S5 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 681, S5 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 751, S5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 751, S5 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 751, S5 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 751, S5 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* LFO-2 - 53 */
	{"", 9, 681, S6, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 681, S6 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 681, S6 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 681, S6 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 681, S6 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 681, S6 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 751, S6, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 751, S6 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 751, S6 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 751, S6 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 751, S6 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* Split Double - 64 */
	{"", 9, 409, S7, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 409, S7 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	/* Layer volumes */
	{"", 9, 478, S7, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 478, S7 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* MIDI - 68 */
	{"", 9, 811, S5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 811, S5 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 811, S5 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 811, S5 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 811, S5 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 811, S5 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 811, S8, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 811, S8 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 811, S8 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/*
	 * Bit-99: FM 1->2,SYNC,GLIDE L/U - 77/78/79/80
	 */
	{"", 9, 0, 0, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},
	{"", 9, 576, S4 + D1 * 3, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},
	{"", 9, 409, S7 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},
	{"", 9, 478, S7 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},
	/* These are unison - 81 and 82 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* PWM DCO-2 - 83 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* DCO harmonics - 84 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* Dummies - 86 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 9, 576, S3, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* 196 is the split/double placeholder */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* This is 197 and may be the stereo placeholder */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* This is 198 and will be the peer layer placeholder */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/*
	 * This needed to move as it is parameter 12, not 64 - now at 199. The 
	 * withdrawn flag is only removed if the bit-99 is requested.
	 */
	{"", 9, 751, S6 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},

	/* split double - 200 */
	{"", 2, 217, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 217, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	/* Stereo - 202 */
	{"", 2, 257, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	/* Pot for tune */
	{"", 0, 250, 150, 80, 80, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0,
		BRIGHTON_NOTCH},
	/*
	 * The following two are in the memory as non-visible placeholder for
	 * combined patches upper/lower/split/layer, etc. It should use the same
	 * peer memory logic employed by the Jupiter GUI. Moved to within first 100
	 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* Two sliders for volume controls - 206 */
	{"", 1, 51, 570, 10, 260, 0, 1, 0, "bitmaps/knobs/sliderblack5.xpm", 0, 0},
	{"", 1, 72, 570, 10, 260, 0, 1, 0, "bitmaps/knobs/sliderblack5.xpm", 0, 0},

	/* Pot for entry - 208 */
	{"", 0, 54, 330, 80, 80, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 0},

	/* Ten data entry buttons - 209 */
	{"", 2, 100, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 120, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 140, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 160, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 180, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"", 2, 100, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 120, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 140, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 160, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 180, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Addr, lower, upper - 219 */
	{"", 2, 100, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 140, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 180, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Park, Compare - 222 */
	{"", 2, 217, 150, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 217, 320, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Prog load - 224 */
	{"", 2, 257, 320, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	/* save - 225 */
	{"", 2, 257, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	/* Two sliders for bend and mod - 226 */
	{"", 1, 17, 150, 10, 280, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0,
		BRIGHTON_HALFSHADOW|BRIGHTON_REVERSE},
	{"", 1, 17, 500, 10, 280, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0,
		BRIGHTON_HALFSHADOW|BRIGHTON_CENTER|BRIGHTON_REVERSE},

	/* Inc/Dec for data entry - 228 */
	{"", 2, 50, 430, 13, 60, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 70, 430, 13, 60, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	/* 8 LED displays - 230 */
	{"", 8, 52, 150, 15, 80, 0, 9, 0, 0, 0, 0},
	{"", 8, 67, 150, 15, 80, 0, 9, 0, 0, 0, 0},

	{"", 8, 97, 150, 15, 80, 0, 9, 0, 0, 0, 0},
	{"", 8, 111, 150, 15, 80, 0, 9, 0, 0, 0, 0},

	{"", 8, 133, 150, 15, 80, 0, 9, 0, 0, 0, 0},
	{"", 8, 148, 150, 15, 80, 0, 9, 0, 0, 0, 0},

	{"", 8, 172, 150, 15, 80, 0, 9, 0, 0, 0, 0},
	{"", 8, 187, 150, 15, 80, 0, 9, 0, 0, 0, 0},

	/* The Bit-1 had a Unison button, not stereo - 238 */
	{"", 2, 257, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_WITHDRAWN},
};

#define SS1 155
#define SS2 580
#define SS3 240
#define SS4 253
#define S5 262
#define SS6 580
#define S7 644
#define S8 600

static
brightonLocations locations100[DEVICE_COUNT] = {
	/*
	 * 100 withdrawn buttons for later use
	 *
	 * The first one was just to look at some stuff with use of the transparency
	 * layer to highlight entry, exit and button click so that this panel of
	 * parameters can be used to select data entry address selections.
	 *
	 * The first is a dummy here since the parameters start from '1'.
	 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* LFO-1 - 1 */
	{"", 9, 293, SS1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS1 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS1 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS1 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS1 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS1 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 345, SS1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS1 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS1 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS1 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS1 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS1 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},

	/* VCF - 13 */
	{"", 9, 763, SS4 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 763, SS4 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 763, SS4 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 763, SS4 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 763, SS4 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 833, SS4 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 833, SS4 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 833, SS4 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 833, SS4 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 833, SS4 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 833, SS4 + D1 * 6, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* DCO 1 - 24 */
	{"", 9, 456, SS4 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 456, SS4 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 456, SS4 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 456, SS4 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 508, SS4 + D1 * 2, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 508, SS4 + D1 * 3, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 508, SS4 + D1 * 4, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 568, SS4 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 568, SS4 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 568, SS4 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 568, SS4 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* DCO 2 - 35 */
	{"", 9, 610, SS3 + D1 * 0, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 610, SS3 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 610, SS3 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 610, SS3 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 662, SS3 + D1 * 1, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 662, SS3 + D1 * 2, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 662, SS3 + D1 * 3, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 721, SS3 + D1 * 0, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 721, SS3 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 721, SS3 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 721, SS3 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* VCA - 46 */
	{"", 9, 876, SS3 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 876, SS3 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 876, SS3 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 946, SS3 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 946, SS3 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 946, SS3 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 946, SS3 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* LFO-2 - 53 */
	{"", 9, 293, SS2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS2 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS2 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS2 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS2 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 293, SS2 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 345, SS2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS2 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS2 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS2 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 345, SS2 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* Split/Transpose - 64 */
	{"", 9, 763, SS6 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 833, SS6 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	/* Layer volumes */
	{"", 9, 833, SS6 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 50, 50 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN|BRIGHTON_TRACKING},

	/* MIDI - 68 */
	{"", 9, 876, SS6 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 876, SS6 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 876, SS6 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 946, SS6 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 946, SS6 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 946, SS6 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 607, SS6 + D1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 607, SS6 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 763, SS6 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/*
	 * Bit-99: FM 1->2,SYNC,GLIDE L/U - 77/78/79/80
	 */
	{"", 9, 508, SS3 + D1, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 662, SS3 + D1 * 4, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 763, SS6 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 833, SS6 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* LFO 1/2 options 81 - 92 */
	{"", 9, 397, SS1 + D1 * 0, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS1 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS1 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS1 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS1 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS1 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 9, 397, SS2 + D1 * 0, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS2 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS2 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS2 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS2 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 397, SS2 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* 93 ENV->PWM */
	{"", 9, 456, SS4 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 610, SS3 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 568, SS4 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 721, SS3 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	/* Filter type */
	{"", 9, 763, SS4 + D1 * 6, W1, 39, 0, 2, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	/* DCO-1 vol - 98 */
	{"", 9, 508, SS4 + D1 * 5, 23, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	/* Noise per layer */
	{"", 9, 650, SS2 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* LFO/ENV options - 103 */
	{"", 9, 650, SS2 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 650, SS2 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 720, SS2 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 720, SS2 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 720, SS2 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 720, SS2 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* Volumes - 110  */
	{"", 9, 525, SS6 + D1 * 3, 17, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 580, SS6 + D1 * 3, 17, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 525, SS6 + D1 * 4, 17, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 580, SS6 + D1 * 4, 17, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 525, SS6 + D1 * 1, 17, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 580, SS6 + D1 * 1, 17, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 525, SS6 + D1 * 2, 17, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 580, SS6 + D1 * 2, 17, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* Temperature - 118 */
	{"", 9, 650, SS2 + D1 * 2, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	/* 120 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* MEM Free Search - 130 */
	{"", 9, 607, SS6 + D1 * 3, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 607, SS6 + D1 * 4, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* Save -1 and -99 */
	{"", 9, 876, SS6 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 9, 946, SS6 + D1 * 1, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* 196 is the split/double placeholder */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* This is 197 and may be the stereo placeholder */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* This is 148 and will be the peer layer placeholder */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/*
	 * This needed to move as it is parameter 12, not 64 - now at 199. The 
	 * withdrawn flag is only removed if the bit-99 is requested.
	 */
	{"", 9, 345, SS6 + D1 * 5, W1, 39, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING|BRIGHTON_WITHDRAWN},

	/* split double - 200 */
	{"", 2, 217, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 217, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	/* Stereo - 202 */
	{"", 2, 257, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	/* Pot for tune */
	{"", 0, 250, 150, 80, 80, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0,
		BRIGHTON_NOTCH},
	/*
	 * The following two are in the memory as non-visible placeholder for
	 * combined patches upper/lower/split/layer, etc. It should use the same
	 * peer memory logic employed by the Jupiter GUI. Moved to within first 100
	 */
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* Two sliders for volume controls - 206 */
	{"", 1, 51, 570, 10, 260, 0, 1, 0, "bitmaps/knobs/sliderblack5.xpm", 0, 0},
	{"", 1, 72, 570, 10, 260, 0, 1, 0, "bitmaps/knobs/sliderblack5.xpm", 0, 0},

	/* Pot for entry - 208 */
	{"", 0, 54, 330, 80, 80, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 0},

	/* Ten data entry buttons - 209 */
	{"", 2, 100, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 120, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 140, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 160, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 180, 350, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"", 2, 100, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 120, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 140, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 160, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 180, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Addr, lower, upper - 219 */
	{"", 2, 100, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 140, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 180, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Park, Compare - 222 */
	{"", 2, 217, 150, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 217, 320, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Prog load - 224 */
	{"", 2, 257, 320, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	/* save - 225 */
	{"", 2, 257, 700, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	/* Two sliders for bend and mod - 226 */
	{"", 1, 17, 150, 10, 280, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0,
		BRIGHTON_HALFSHADOW|BRIGHTON_REVERSE},
	{"", 1, 17, 500, 10, 280, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0,
		BRIGHTON_HALFSHADOW|BRIGHTON_CENTER|BRIGHTON_REVERSE},

	/* Inc/Dec for data entry - 228 */
	{"", 2, 50, 430, 13, 60, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 70, 430, 13, 60, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	/* 8 LED displays - 230 */
	{"", 8, 52, 150, 15, 80, 0, 9, 0, 0, 0, 0},
	{"", 8, 67, 150, 15, 80, 0, 9, 0, 0, 0, 0},

	{"", 8, 97, 150, 15, 80, 0, 9, 0, 0, 0, 0},
	{"", 8, 111, 150, 15, 80, 0, 9, 0, 0, 0, 0},

	{"", 8, 133, 150, 15, 80, 0, 9, 0, 0, 0, 0},
	{"", 8, 148, 150, 15, 80, 0, 9, 0, 0, 0, 0},

	{"", 8, 172, 150, 15, 80, 0, 9, 0, 0, 0, 0},
	{"", 8, 187, 150, 15, 80, 0, 9, 0, 0, 0, 0},

	/* The Bit-1 had a Unison button, not stereo - 238 */
	{"", 2, 257, 490, 15, 80, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_WITHDRAWN},
};

#define PSIZE 680

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 * Hm, the bit-1 was black, and the bit 99 was in various builds including a 
 * white one, but also a black one and a black one with wood panels. I would
 * like the black one with wood panels, so that will have to be the bit-1, the
 * bit-99 will be white with thin metal panels.
 */
brightonApp bitoneApp = {
	"bitone",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH,
	bitoneInit,
	bitoneConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{6, 100, 2, 2, 5, 520, 0, 0},
	788, 350, 0, 0,
	4, /* one panel only */
	{
		{
			"BitOne",
			"bitmaps/blueprints/bitone.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			bitoneCallback,
			12, 4, 978, PSIZE - 3,
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
			bitoneKeyCallback,
			14, PSIZE, 1000, 1000 - PSIZE - 3,
			KEY_COUNT,
			keysprofile
		},
		{
			"Wood Panel",
			0,
			"bitmaps/textures/wood6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			0,
			2, 4, 10, 996,
			0,
			0
		},
		{
			"Wood Panel",
			0,
			"bitmaps/textures/wood6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			0,
			991, 4, 10, 996,
			0,
			0
		},
	}
};

/*
 * This may need some reworks to make sure the bit-1 screenprint displays 
 * correctly.
 */
brightonApp bit99App = {
	"bit99",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH,
	bitoneInit,
	bitoneConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{6, 100, 2, 2, 5, 520, 0, 0},
	788, 350, 0, 0,
	4, /* one panel only */
	{
		{
			"Bit99",
			"bitmaps/blueprints/bit99.xpm",
			"bitmaps/textures/white.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			bitoneCallback,
			12, 0, 978, PSIZE - 3,
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
			bitoneKeyCallback,
			12, PSIZE, 1006, 997 - PSIZE,
			KEY_COUNT,
			keysprofile
		},
		{
			"Metal Panel",
			0,
			"bitmaps/textures/metal7.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			0,
			1, 0, 8, 1000,
			0,
			0
		},
		{
			"Metal Panel",
			0,
			"bitmaps/textures/metal7.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			991, 0, 7, 1000,
			0,
			0
		},
	}
};

brightonApp bit100App = {
	"bit100",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH,
	bitoneInit,
	bitoneConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{6, 100, 2, 2, 5, 520, 0, 0},
	788, 350, 0, 0,
	4, /* one panel only */
	{
		{
			"BitOne",
			"bitmaps/blueprints/bit100.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			bitoneCallback,
			12, 4, 978, PSIZE - 3,
			DEVICE_COUNT,
			locations100
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			bitoneKeyCallback,
			14, PSIZE, 1000, 1000 - PSIZE - 3,
			KEY_COUNT,
			keysprofile
		},
		{
			"Wood Panel",
			0,
			"bitmaps/textures/wood6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			0,
			2, 4, 10, 996,
			0,
			0
		},
		{
			"Wood Panel",
			0,
			"bitmaps/textures/wood6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			0,
			991, 4, 10, 996,
			0,
			0
		},
	}
};

static int
b1decimal(int index, float value)
{
	return(bitoneRange[index].min +
		(int) (value *
			((float) (bitoneRange[index].max - bitoneRange[index].min + 1))));
}

static int
b1debug(guiSynth *synth, int level)
{
	if (b1decimal(109, synth->mem.param[109]) >= level)
		return(1);
	return(0);
}

/*
 * These will return just the prePark memory, not the actual synth.
 */
static guiSynth *
b1getScratch(guiSynth *synth)
{
	if (synth->mem.param[101] != 0) {
		if (entryPoint == ENTER_UPPER)
			return ((guiSynth *) synth->second);
		else
			return (synth);
	} else {
		if (entryPoint == ENTER_UPPER)
			return (&prePark[1]);
		else
			return (&prePark[0]);
	}
}

/*
 * Param is a float from 0 to 1.0, turn it into a value in the range of the
 * parameter
 */
static float
b1GetRangeValue(guiSynth *synth, int index)
{
	switch (bitoneRange[index].type) {
		default:
		case BITONE_CONTINUOUS:
			return(synth->mem.param[index]);
		case BITONE_STEPPED:
			return(bitoneRange[index].min + synth->mem.param[index]
				* (bitoneRange[index].max - bitoneRange[index].min + 1));
	}
}

static void
b1SetSplit(guiSynth *synth)
{
	int lsp, usp;

	if (b1debug(synth, 1))
		printf("setSplit: %i/%i\n",
			b1decimal(64, prePark[0].mem.param[64]),
			b1decimal(64, prePark[1].mem.param[64]));

	if (mode == MODE_SINGLE)
		/* We could consider clearing any splitpoints here as well */
		return;

	/*
	 * Find and apply split points
	 * We need to consider DE-121 for multiple splits.
	 */
	if (mode == MODE_SPLIT) {
		if (synth->mem.param[121] != 0) {
			lsp = b1GetRangeValue(&prePark[0], 64) + synth->transpose;
			usp = b1GetRangeValue(&prePark[1], 64) + 1 + synth->transpose;
		} else {
			lsp = b1GetRangeValue(&prePark[0], 64) + synth->transpose;
			usp = b1GetRangeValue(&prePark[0], 64) + 1 + synth->transpose;
		}
	} else {
		if (synth->mem.param[121] != 0) {
			lsp = b1GetRangeValue(&prePark[0], 64) + synth->transpose;
			usp = b1GetRangeValue(&prePark[1], 64) + 1 + synth->transpose;
		} else {
			lsp = 127;
			usp = 0;
		}
	}

	if (global.libtest == 0) {
		bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
			BRISTOL_HIGHKEY|lsp);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
			BRISTOL_LOWKEY|usp);
	}

	setsplit = 0.0;
}

/*
 * The next two should come out of here and go into a separate arpeggiator file
 * or failing that into brightonRoutines.c
 */
int
seqInsert(arpeggiatorMemory *seq, int note, int transpose)
{
printf("arpeg %i + %i at %i\n", note, transpose, (int) seq->s_max);
	if (seq->s_max == 0)
		seq->s_dif = note + transpose;

	seq->sequence[(int) (seq->s_max)] = (float) (note + transpose - seq->s_dif);

	if ((seq->s_max += 1) >= BRISTOL_SEQ_MAX) {
		seq->s_max = BRISTOL_SEQ_MAX;
		return(-1);
	}

	return(0);
}

int
chordInsert(arpeggiatorMemory *seq, int note, int transpose)
{
printf("chord %i + %i at %i\n", note, transpose, (int) seq->c_count);
	if (seq->c_count == 0)
		seq->c_dif = note + transpose;

	seq->chord[(int) (seq->c_count)] = (float) (note + transpose - seq->c_dif);

	if ((seq->c_count += 1) >= BRISTOL_CHORD_MAX) {
		seq->c_count = BRISTOL_CHORD_MAX;
		return(-1);
	}

	return(0);
}

/*
 * We really want to just use one midi channel and let the midi library decide
 * that we have multiple synths on the channel with their own split points.
 * The lower layer should define the midi channel, split point and transpose 
 * of upper layer.
 */
static int
bitoneKeyCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	guiSynth *layer = b1getScratch(synth);

	if ((synth->mem.param[155] != 0) && (value != 0))
	{
		if (entryPoint == ENTER_UPPER)
			seqInsert((arpeggiatorMemory *) synth->seq2.param,
				(int) index, synth->transpose); 
		else
			seqInsert((arpeggiatorMemory *) synth->seq1.param,
				(int) index, synth->transpose); 
	}

	if ((synth->mem.param[157] != 0) && (value != 0))
	{
		if (entryPoint == ENTER_UPPER)
			chordInsert((arpeggiatorMemory *) synth->seq2.param,
				(int) index, synth->transpose); 
		else
			chordInsert((arpeggiatorMemory *) synth->seq1.param,
				(int) index, synth->transpose); 
	}

	if (global.libtest)
		return(0);

	if (b1debug(synth, 4))
		printf("b1keycallback(%i, %i, %f): %i %i\n",
			panel, index, value, synth->transpose, global.controlfd);

	if (setsplit != 0.0)
	{
		if (b1debug(synth, 1))
			printf("setting (1) splitpoint to %i\n", index);
		layer->mem.param[64] = index + synth->transpose;
		setsplit = 0;

		b1SetSplit(synth);
	}

	if ((mode == MODE_SINGLE) || (synth->mem.param[120] == 0)
		|| (synth->midichannel == ((guiSynth *) synth->second)->midichannel)) {
		/*
		 * Want to send a note event, on or off, for this index + transpose only
		 * on the lower layer midichannel. This actually suffices for all cases
		 * where we use a single midi channel hence the logic.
		 */
		if (value)
			bristolMidiSendMsg(global.controlfd, synth->midichannel,
				BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
		else
			bristolMidiSendMsg(global.controlfd, synth->midichannel,
				BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);

		return(0);
	}

	if (b1debug(synth, 2))
		printf("key event to chan %i/%i (%0.0f)\n", synth->midichannel,
			((guiSynth *) synth->second)->midichannel, synth->mem.param[120]);

	/*
	 * So we have a single key event and two MIDI channels. Just send the
	 * event on both channels, no need to be difficult about it since if this
	 * was a split configuration the library filters out the events.
	 */
	if (value) {
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
		bristolMidiSendMsg(global.controlfd,
			((guiSynth *) synth->second)->midichannel, BRISTOL_EVENT_KEYON, 0,
			index + synth->transpose);
	} else {
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);
		bristolMidiSendMsg(global.controlfd,
			((guiSynth *) synth->second)->midichannel, BRISTOL_EVENT_KEYOFF, 0,
			index + synth->transpose);
	}

	return(0);
}

static void
bitoneCompareOff(guiSynth *synth)
{
	brightonEvent event;

	/*
	 * If we are changing entryPoint we should first disactivate any compare
	 * button to ensure we have the correct scratchpad, etc.
	 */
	if (synth->mem.param[COMPARE_BUTTON] != 0.0)
	{
		event.type = BRIGHTON_FLOAT;
		event.value = 0.0;
		brightonParamChange(synth->win, 0, COMPARE_BUTTON, &event);
	}
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
bitoneMemoryShim(guiSynth *synth, int from)
{
	int i, uh, lh;
	brightonEvent event;
	guiSynth *layer = synth, *scratch = b1getScratch(synth), *scratch2;

	/*
	 * Clear any mem search flags.
	 */
	synth->mem.param[74] = prePark[0].mem.param[74]
		= prePark[1].mem.param[74] = 0;
	synth->mem.param[75] = prePark[0].mem.param[75]
		= prePark[1].mem.param[75] = 0;
	synth->mem.param[130] = prePark[0].mem.param[130]
		= prePark[1].mem.param[130] = 0;
	synth->mem.param[131] = prePark[0].mem.param[131]
		= prePark[1].mem.param[131] = 0;

	if (b1debug(synth, 2))
		printf("bitoneMemoryShim(%i)\n", from);

	/*
	 * Save the current display settings
	 */
	uh = synth->mem.param[DISPLAY_DEV + 2];
	lh = synth->mem.param[DISPLAY_DEV + 3];

	/*
	 * Select layer
	 */
	event.type = BRIGHTON_FLOAT;
	if (entryPoint == ENTER_UPPER)
	{
		layer = (guiSynth *) synth->second;
		scratch2 = &prePark[0];
	} else
		scratch2 = &prePark[1];

	synth->flags |= MEM_LOADING;
	layer->flags |= MEM_LOADING;

	/*
	 * scan through all the parameters backwards. We go backwards since the
	 * higher numbers parameter are hidden but may actually affect the operation
	 * of the lower ordered ones - the higher parameters may override the 
	 * default Bit features.
	 */
	for (i = from - 1; i >= 0 ; i--)
	{
		if ((bitoneRange[i].flags & B1_INACTIVE) != 0)
			continue;
		if ((bitoneRange[i].flags & B1_NO_LOAD) != 0)
			continue;

		synth->mem.param[DISPLAY_DEV + 2] = i / 10;
		synth->mem.param[DISPLAY_DEV + 3] = i % 10;

		if (i >= 100) {
			synth->mem.param[DISPLAY_DEV + 2] -= 10;
			synth->mem.param[EXTENDED_BUTTON] = 1.0;
		} else
			synth->mem.param[EXTENDED_BUTTON] = 0;

		/*
		 * We really ought to consider rounding here? No, the entry pot will
		 * do that for us, that is the whole point of dispatching these events.
		 */
		event.value = scratch->mem.param[i] = layer->mem.param[i];

		/*
		 * Check for the DC flag, then check the bitoneRange flags for whether
		 * this parameter should be put into effect, should be inherrited from
		 * the lower layer, etc.
		 *
		 * The inherit flags need to be reviewed since they depend on the
		 * DE 120/121/122 options for dual settings.....
		 */
		if ((dc != 0) || (entryPoint == ENTER_UPPER))
		{
			if (bitoneRange[i].flags & B1_INHERIT_PRI)
			{
				if (b1debug(synth, 2))
					printf("inherrit %i\n", i);
				event.value = scratch->mem.param[i] = prePark[0].mem.param[i];
			}

			if ((bitoneRange[i].flags & B1_INACTIVE_PEER) != 0)
				continue;
			if ((bitoneRange[i].flags & B1_USE_PRI) != 0)
				event.value = prePark[0].mem.param[i];
		}

		brightonParamChange(synth->win, 0, ENTRY_POT, &event);
	}

	synth->flags &= ~MEM_LOADING;
	layer->flags &= ~MEM_LOADING;

	/*
	 * Return the previous values to display.
	 */
	synth->mem.param[DISPLAY_DEV + 2] = uh;
	synth->mem.param[DISPLAY_DEV + 3] = lh;

	synth->mem.param[EXTENDED_BUTTON] = 0;

	/*
	 * Since the memory was loaded with NOCALLS we have to request our
	 * arpeggiation stuffing manually too.
	 */
	if (synth->seq1.param == NULL) {
		loadSequence(&synth->seq1, SYNTH_NAME, 0, 0);
		if (synth->seq2.param == NULL)
			loadSequence(&synth->seq2, SYNTH_NAME, 0, BRISTOL_SID2);
	} else {
		if (entryPoint == ENTER_UPPER)
			fillSequencer(synth,
				(arpeggiatorMemory *) synth->seq2.param, BRISTOL_SID2);
		else
			fillSequencer(synth, (arpeggiatorMemory *) synth->seq1.param, 0);
	}

	if (b1debug(synth, 2))
		printf("bitoneMemoryShim done\n");
}

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (b1debug(synth, 3))
		printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			if (synth->mem.param[71] != 0)
			{
				if (b1debug(synth, 0))
					printf("midi program change not active\n");
				return(0);
			}
			/*
			 * We should accept 0..74 as lower layer and above that as dual
			 * loading requests.
			 */
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, SYNTH_NAME, 0,
				synth->bank * 100 + synth->location,
				synth->mem.active, 0, BRISTOL_NOCALLS|BRISTOL_FORCE);
			bitoneMemoryShim(synth, ENTRY_MAX);
			break;
		case MIDI_BANK_SELECT:
			if (synth->mem.param[71] != 0)
			{
				if (b1debug(synth, 0))
					printf("midi program change not active\n");
				return(0);
			}
			printf("midi banksel: %x, %i\n", controller, value);
			synth->location %= 100;
			synth->bank = value;
			loadMemory(synth, SYNTH_NAME, 0,
				synth->bank * 100 + synth->location,
				synth->mem.active, 0, BRISTOL_NOCALLS|BRISTOL_FORCE);
			break;
	}
	return(0);
}

static int
bitoneUpdateDisplayDec(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int ho, lo, dev = DISPLAY_DEV + o;
	brightonEvent event;

	if (v < 0)
	{
		ho = lo = -10;
	} else {
		lo = v % 10;
		ho = v / 10;
	}

	event.type = BRIGHTON_FLOAT;
	event.value = (float) ho;

	brightonParamChange(synth->win, 0, dev, &event);

	event.value = (float) lo;
	brightonParamChange(synth->win, 0, dev + 1, &event);

	return(0);
}

static int
bitoneUpdateDisplay(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int ho, lo, dev = DISPLAY_DEV + o;
	brightonEvent event;

	if (v < 0)
	{
		ho = lo = -10;
	} else {
		ho = v * 100 / (CONTROLLER_RANGE + 1);
		lo = ho % 10;
		ho = ho / 10;
	}

	event.type = BRIGHTON_FLOAT;
	event.value = (float) ho;

	brightonParamChange(synth->win, 0, dev, &event);

	event.value = (float) lo;
	brightonParamChange(synth->win, 0, dev + 1, &event);

	return(0);
}

/*
 * Double/Split has certain logical requirements. Two two buttons are mutally
 * exclusive, so when one goes on we have to disable the other.
 */
static int
bitoneSplitDouble(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	guiSynth *layer = &prePark[1];

	if (synth->dispatch[RADIOSET_3].other2 != 0)
	{
		synth->dispatch[RADIOSET_3].other2 = 0;
		return(0);
	}

	if (b1debug(synth, 4))
		printf("SD %i %i\n",c,o);

	event.type = BRIGHTON_FLOAT;
	event.value = 0.0;

	/*
	 * If we are going to zero and peer is zero then the operating mode is
	 * now SINGLE.
	 */
	if ((v == 0) && (synth->mem.param[RADIOSET_3 + (1 - c)] == 0))
	{
		mode = MODE_SINGLE;

		if (b1debug(synth, 1))
			printf("mode single\n");

		bitoneUpdateDisplay(synth, fd, chan, c, 6, -10);

		if (entryPoint == ENTER_UPPER)
		{
			event.value = 1.0;
			brightonParamChange(synth->win, 0, RADIOSET_2 + 1, &event);
		}

		/*
		 * Actions are to mute the upper layer and revoice the lower layer to
		 * full count, remove splits.
		 */
		bristolMidiSendMsg(fd, synth->sid2, 126, 5, 0);
		bristolMidiSendMsg(fd, synth->sid2, 126, 6, 0);
		/* Raise voices on lower layer, lower them on upper to reduce CPU */
		bristolMidiSendMsg(fd, synth->sid, 126, 2, 1);
		bristolMidiSendMsg(fd, synth->sid2, 126, 2, 0);
		/* Split */
		if (global.libtest == 0) {
			bristolMidiSendMsg(global.controlfd, synth->sid,
				127, 0, BRISTOL_HIGHKEY|127);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, BRISTOL_LOWKEY|126);
		}

		return(0);
	}

	if (c == 0)
	{
		float lvol, rvol;

		if ((synth->mem.param[RADIOSET_3 + 1] != 0)
			&& (b1debug(synth, 3)))
			printf("reset peer 1 and exclude\n");

		synth->dispatch[RADIOSET_3].other2 = 1;
		brightonParamChange(synth->win, 0, RADIOSET_3 + 1, &event);

		mode = MODE_SPLIT;

		if (b1debug(synth, 1))
			printf("mode split\n");

		bitoneUpdateDisplay(synth, fd, chan, c, 6,
			(B1display[2] + 1) * CONTROLLER_RANGE / 100);

		/*
		 * Actions are to unmute the upper layer, revoice the lower layer to
		 * half count, apply splits. The layer volumes have to take into
		 * account the stereo button status and configured volume:
		 *
		 * We have issues here regarding the bit100 and its single volume
		 * control.
		 *
		 * We we are a bit-100 then take the layer volumes. Other models will
		 * take the synth volume unless otherwise denoted.
		 */
		if (synth->mem.param[118] == 0) {
			if (synth->mem.param[202] != 0) {
				/* Stereo */
				lvol = prePark[0].mem.param[67] * synth->mem.param[116];
				rvol = prePark[0].mem.param[67] * synth->mem.param[117];
			} else {
				/* Mono, ie, take a stereo panned position */
				lvol = prePark[0].mem.param[67] * synth->mem.param[114];
				rvol = prePark[0].mem.param[67] * synth->mem.param[115];
			}
			if (b1debug(synth, 4))
				printf("split: %f %f, %f %f %f %f\n", lvol, rvol,
					prePark[0].mem.param[67], synth->mem.param[114],
					prePark[0].mem.param[67], synth->mem.param[115]);
		} else {
			if (synth->mem.param[202] != 0) {
				/* Stereo */
				lvol = layer->mem.param[67] * synth->mem.param[116];
				rvol = layer->mem.param[67] * synth->mem.param[117];
			} else {
				/* Mono, ie, take a stereo panned position */
				lvol = layer->mem.param[67] * synth->mem.param[114];
				rvol = layer->mem.param[67] * synth->mem.param[115];
			}
			if (b1debug(synth, 4))
				printf("split: %f %f, %f %f %f %f\n", lvol, rvol,
					layer->mem.param[67], synth->mem.param[114],
					layer->mem.param[67], synth->mem.param[115]);
		}

		bristolMidiSendMsg(fd, synth->sid2, 126, 5,
			(int) (C_RANGE_MIN_1 * lvol));
		bristolMidiSendMsg(fd, synth->sid2, 126, 6,
			(int) (C_RANGE_MIN_1 * rvol));
		/* Lower voices on lower layer, raise them on upper to reduce CPU */
		bristolMidiSendMsg(fd, synth->sid, 126, 2, 0);
		bristolMidiSendMsg(fd, synth->sid2, 126, 2, 1);

		if (synth->mem.param[64] == 0)
			setsplit = 1.0;
		else
			b1SetSplit(synth);

		return(0);
	}

	if (c == 1)
	{
		float lvol, rvol;

		if (synth->mem.param[RADIOSET_3] != 0)
			printf("reset peer 2 and exclude\n");

		synth->dispatch[RADIOSET_3].other2 = 1;
		brightonParamChange(synth->win, 0, RADIOSET_3, &event);

		mode = MODE_LAYER;

		if (b1debug(synth, 1))
			printf("mode layer\n");

		bitoneUpdateDisplay(synth, fd, chan, c, 6,
			(B1display[2] + 1) * CONTROLLER_RANGE / 100);

		/*
		 * Actions are to unmute the upper layer, revoice the lower layer to
		 * half count, remove splits.
		 */
		if (synth->mem.param[118] == 0) {
			if (synth->mem.param[202] != 0) {
				/* Stereo */
				lvol = prePark[0].mem.param[67] * synth->mem.param[116];
				rvol = prePark[0].mem.param[67] * synth->mem.param[117];
			} else {
				/* Mono, ie, take a stereo panned position */
				lvol = prePark[0].mem.param[67] * synth->mem.param[114];
				rvol = prePark[0].mem.param[67] * synth->mem.param[115];
			}
			if (b1debug(synth, 4))
				printf("split: %f %f, %f %f %f %f\n", lvol, rvol,
					prePark[0].mem.param[67], synth->mem.param[114],
					prePark[0].mem.param[67], synth->mem.param[115]);
		} else {
			if (synth->mem.param[202] != 0) {
				/* Stereo */
				lvol = layer->mem.param[67] * synth->mem.param[116];
				rvol = layer->mem.param[67] * synth->mem.param[117];
			} else {
				/* Mono, ie, take a stereo panned position */
				lvol = layer->mem.param[67] * synth->mem.param[114];
				rvol = layer->mem.param[67] * synth->mem.param[115];
			}
			if (b1debug(synth, 4))
				printf("split: %f %f, %f %f %f %f\n", lvol, rvol,
					layer->mem.param[67], synth->mem.param[114],
					layer->mem.param[67], synth->mem.param[115]);
		}

		bristolMidiSendMsg(fd, synth->sid2, 126, 5,
			(int) (C_RANGE_MIN_1 * lvol));
		bristolMidiSendMsg(fd, synth->sid2, 126, 6,
			(int) (C_RANGE_MIN_1 * rvol));
		/* Lower voices on lower layer, raise them on upper to reduce CPU */
		bristolMidiSendMsg(fd, synth->sid, 126, 2, 0);
		bristolMidiSendMsg(fd, synth->sid2, 126, 2, 1);
		/* Split */
		if (global.libtest == 0) {
			bristolMidiSendMsg(global.controlfd, synth->sid,
				127, 0, BRISTOL_HIGHKEY|127);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, BRISTOL_LOWKEY|0);
		}
	}

	return(0);
}

/*
 * This will take a value out of the scratchpad unless it is negative.
 */
static void
bitoneDAUpdate(guiSynth *synth)
{
	brightonEvent event;
	guiSynth *layer = b1getScratch(synth);
	int address = 0;

	/*
	 * Update the display for the given data entry point
	 */
	address = synth->mem.param[DISPLAY_DEV + 2] * 10 + 
		synth->mem.param[DISPLAY_DEV + 3];

	if (synth->mem.param[EXTENDED_BUTTON] != 0)
	{
		address += 100;
		event.value = synth->mem.param[address];
	} else
		event.value = layer->mem.param[address];

	event.type = BRIGHTON_FLOAT;

	brightonParamChange(synth->win, 0, ENTRY_POT, &event);
}

static int
bitoneALU(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	/*
	 * So, these are radio buttons, first force exclusion
	 */
	if (synth->dispatch[RADIOSET_2].other2)
	{
		synth->dispatch[RADIOSET_2].other2 = 0;
		return(0);
	}

	/*
	 * See if we are trying to select Data Entry whilst in Split/Double without
	 * DE 102 enabled.
	 *
	 * Pan that, we do need to be able to select DE but only actually edit a
	 * few of the under control of the range flags.
	if ((o == 0) && (mode != MODE_SINGLE) && (synth->mem.param[102] == 0))
	{
		synth->dispatch[RADIOSET_2].other2 = 1;

		event.type = BRIGHTON_FLOAT;
		event.value = 0.0;
		brightonParamChange(synth->win, 0, 219, &event);
		bitoneUpdateDisplay(synth, fd, chan, c, 2, -10);

		return(0);
	}
	 */

	if ((mode == MODE_SINGLE) && (o == 2))
	{
		/*
		 * This is 6 voice, do not allow selection of the upper layer
		 */
		event.type = BRIGHTON_FLOAT;
		event.value = 0.0;
		synth->dispatch[RADIOSET_2].other2 = 1;
		brightonParamChange(synth->win, 0, 221, &event);

		bitoneUpdateDisplay(synth, fd, chan, c, 6, -10);

		return(0);
	}

	bitoneCompareOff(synth);

	if (synth->dispatch[RADIOSET_2].other1 != -1)
	{
		synth->dispatch[RADIOSET_2].other2 = 1;

		event.type = BRIGHTON_FLOAT;
		if (synth->dispatch[RADIOSET_2].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[RADIOSET_2].other1 + RADIOSET_2, &event);
	}
	synth->dispatch[RADIOSET_2].other1 = o;

	entryPoint = o;

	/*
	 * Update the data entry for the currently selected parameter plus show
	 * the active memories
	 */
	bitoneDAUpdate(synth);
	bitoneUpdateDisplay(synth, fd, chan, c, 4, memLower * C_RANGE_MIN_1 / 99);
	if (mode == MODE_SINGLE)
		bitoneUpdateDisplay(synth, fd, chan, c, 6, -10);
	else
		bitoneUpdateDisplay(synth, fd, chan, c, 6,
			memUpper * C_RANGE_MIN_1 / 99);

	return(0);
}

/*
 * These are the 10 digit buttons for selecting addresses and memories
 */
static int
bitoneEntry(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int ho, lo;

	/*
	 * So, these are radio buttons, first force exclusion
	 */
	if (synth->dispatch[RADIOSET_1].other2)
	{
		synth->dispatch[RADIOSET_1].other2 = 0;
		return(0);
	}

	if (synth->dispatch[RADIOSET_1].other1 != -1)
	{
		synth->dispatch[RADIOSET_1].other2 = 1;

		event.type = BRIGHTON_FLOAT;
		if (synth->dispatch[RADIOSET_1].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[RADIOSET_1].other1 + RADIOSET_1, &event);
	}
	synth->dispatch[RADIOSET_1].other1 = o;

	/*
	 * What I want to happen here is that the numbers just rotate through those
	 * being pressed, no effective start or endpoint.
	 */

	if (B1display[4] < 0)
		B1display[4] = c;
	else {
		ho = synth->mem.param[DISPLAY_DEV + entryPoint * 2 + 2]
			= synth->mem.param[DISPLAY_DEV + entryPoint * 2 + 3];
		lo = synth->mem.param[DISPLAY_DEV + entryPoint * 2 + 3] = c;
		B1display[4] = ho * 10 + lo;
		if (entryPoint == ENTER_ADDRESS)
		{
			if (B1display[4] > ENTRY_MAX)
				B1display[4] = c;
		} else {
			while (B1display[4] >= 100)
				B1display[4] = c;
		}

		/* 
		 * Unset the compare button?
		 */
		bitoneCompareOff(synth);
	}

	B1display[entryPoint] = B1display[4];

	bitoneUpdateDisplay(synth, fd, chan, c, entryPoint * 2 + 2,
		(B1display[4] + 1) * CONTROLLER_RANGE / 100);
	bitoneDAUpdate(synth);

	/*
	 * This should happen early and the extended interface should take care
	 * of all of this.
	if (synth->mem.param[EXTENDED_BUTTON] != 0)
		return(bitoneExtendedEntry(synth, fd, chan, 2, o, v));
	 */

	return(0);
}

static int
bitoneMidiNull(void *synth, int fd, int chan, int c, int o, int v)
{
	if (global.libtest)
		printf("This is a null callback on panel 0 id 0\n");

	return(0);
}

static void
bitoneMidiShim(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (b1debug(synth, 3))
		printf("bitoneMidiShim(%i, %i, %i)\n", c, o, v);
	bristolMidiSendMsg(fd, synth->sid, c, o, v);
}

static int
bitoneMidiSendMsg(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if ((c == 0) && (o == 0))
		return(0);

	o = bitoneRange[c].cont;
	c = bitoneRange[c].op;

	if (global.libtest)
	{
		if (b1debug(synth, 3))
			printf("bristolMidiSendMsg(%i, %i, %i)\n", c, o, v);
		return(0);
	}

	if (b1debug(synth, 4))
		printf("bitoneMidiSendMsg(%i, %i, %i) %i\n", c, o, v, entryPoint);

	if (entryPoint == ENTER_UPPER)
		bristolMidiSendMsg(fd, synth->sid2, c, o, v);
	else
		bristolMidiSendMsg(fd, synth->sid, c, o, v);

	return(0);
}

static int
bitoneExtendedEntry(guiSynth *synth, int fd, int chan, int c, int o, int v) {
	int index;
	float decimal;
	guiSynth *scratch = b1getScratch(synth); 

	/*
	 * If c = 0 we have come from entryPot
	 * if c = 1 we have come from IncDec
	 * if c = 2 we have come from entry buttons
	if (synth->flags & MEM_LOADING)
		return(0);
	 */
	index = synth->mem.param[DISPLAY_DEV + 2] * 10
		+ synth->mem.param[DISPLAY_DEV + 3];

	if (b1debug(synth, 5))
		printf("Extended 1 index %i %f (%i)\n", index,
			synth->mem.param[EXTENDED_BUTTON], v);

	if ((index == EXTENDED_BUTTON) && (v != 0)
		&& (synth->mem.param[EXTENDED_BUTTON] != 0))
	{
		if (b1debug(synth, 1))
			printf("Exit extended data entry\n");

		synth->mem.param[EXTENDED_BUTTON] = 0;
		synth->mem.param[NON_EXTENDED_BUTTON] = 0;
		scratch->mem.param[EXTENDED_BUTTON] = 0;
		scratch->mem.param[NON_EXTENDED_BUTTON] = 0;

		/*
		 * Return second display to blank
		 */
		bitoneUpdateDisplay(synth, fd, chan, 6, 6, -10);

		return(0);
	}

	index += 100;

	/*
	 * We have a value for the pot position and that has to be worked into 
	 * the entry range.
	 */
	decimal = bitoneRange[index].min +
		v * (bitoneRange[index].max - bitoneRange[index].min + 1)
		/ CONTROLLER_RANGE;

	if (b1debug(synth, 5))
		printf("Extended 2 index/decimal %i: %f\n", index, decimal);

	if (index == NON_EXTENDED_BUTTON)
	{
		if (b1debug(synth, 1))
			printf("ExtendedEntry(%i, %i = %1.0f)\n", index, v, decimal);

	if (b1debug(synth, 5))
		printf("Extended 3 index/decimal %i: %f (%i)\n", index, decimal, mode);
		if (mode == MODE_SINGLE)
		{
			if ((synth->mem.param[EXTENDED_BUTTON] = v) != 0.0)
			{
				brightonEvent event;

				if (b1debug(synth, 1))
					printf("Enter extended data entry\n");

				event.type = BRIGHTON_FLOAT;
				event.value = 10.0;
				/* Set last display to some value.
				bitoneUpdateDisplay(synth, fd, chan, 6, 6,
					100 * C_RANGE_MIN_1 / 99);
				*/
				brightonParamChange(synth->win, 0, DISPLAY_DEV + 6, &event);
				brightonParamChange(synth->win, 0, DISPLAY_DEV + 7, &event);
			}
		} else {
			if ((v != 0) && (b1debug(synth, 1)))
				printf("Extended DE (addr=00) not in split or double\n");
			synth->mem.param[EXTENDED_BUTTON] = 0;
			synth->mem.param[NON_EXTENDED_BUTTON] = 0;
			scratch->mem.param[EXTENDED_BUTTON] = 0;
			scratch->mem.param[NON_EXTENDED_BUTTON] = 0;
			v = 0;
		}
	}

	/* No scratchpad for compare/park with extended data sets */
	synth->mem.param[index] = scratch->mem.param[index] =
		((float) v) / CONTROLLER_RANGE;

/*
	if (bitoneRange[index].type == BITONE_STEPPED)
		v = decimal;
*/

	/*
	 * We might not be able to send directly. The first 100 buttons are not
	 * handled directly to the dispatcher but passed through to the select code.
	 * If they had dispatchers configured they are now called here on data
	 * entry. If none is available then I just send the parameter index/value
	 */
	if (synth->dispatch[index].routine != NULL)
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			v);
	else
		bitoneMidiSendMsg(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			v);

	if ((synth->flags & MEM_LOADING) == 0)
		bitoneUpdateDisplayDec(synth, fd, chan, 0, 0, decimal);

	return(0);
}

static int
bitoneEntryPot(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int index;
	float decimal;
	guiSynth *layer = synth, *scratch = b1getScratch(synth); 

	/*
	 * Get the index from the second display
	 */
	index = synth->mem.param[DISPLAY_DEV + 2] * 10
		+ synth->mem.param[DISPLAY_DEV + 3];

	if (b1debug(synth, 5))
		printf("EntryPot %i, %f\n", index, synth->mem.param[EXTENDED_BUTTON]);

	/*
	 * This should happen early and the extended interface should take care
	 * of all of this.
	 */
	if ((synth->mem.param[EXTENDED_BUTTON] != 0) || (index == EXTENDED_BUTTON))
		return(bitoneExtendedEntry(synth, fd, chan, 0, o, v));

	if (entryPoint == ENTER_UPPER)
		layer = (guiSynth *) synth->second;

	if ((synth->mem.param[102] == 0) && (mode != MODE_SINGLE)
		&& ((bitoneRange[index].flags & B1_ALWAYS) == 0)
		&& ((synth->flags & MEM_LOADING) == 0))
		v = scratch->mem.param[index] * CONTROLLER_RANGE;

	/*
	 * We have a value for the pot position and that has to be worked into 
	 * the entry range.
	 */
	decimal = bitoneRange[index].min +
		v * (bitoneRange[index].max - bitoneRange[index].min + 1)
		/ CONTROLLER_RANGE;

	if (bitoneRange[index].flags & B1_INACTIVE) {
		v = 0;
		decimal = 0;
	}

	if ((synth->mem.param[102] == 0) && (mode != MODE_SINGLE)
		&& ((bitoneRange[index].flags & B1_ALWAYS) == 0)
		&& ((synth->flags & MEM_LOADING) == 0))
	{
		bitoneUpdateDisplayDec(synth, fd, chan, c, 0, decimal);
		return(0);
	}

	if (b1debug(synth, 2))
		printf("entryPot(%i, %i, %i): %i %2.0f, %x\n",
			c, o, v, index, decimal, synth->flags);

	/*
	 * What we now have to do is decide into which register to put this
	 * parameter - upper or lower manual, and also to send the value.
	 *
	 * We have to be careful with Park here since this is also used to load 
	 * memories and if we don't park stuff now then we lose the load. What we
	 * need to do is check for the MEM_LOADING flag and if set then we need to
	 * always park the values.
	 */
	if ((synth->flags & MEM_LOADING) == 0) {
		bitoneUpdateDisplayDec(synth, fd, chan, c, 0, decimal);
		/*
		 * We should make considerations here for some B1_INHERIT options. We
		 * are not loading but then we have have to pass it to the primary layer
		 * event though the layer is secondary. We just override 'scratch'.
		 */
		scratch->mem.param[index] = ((float) v) / CONTROLLER_RANGE;
		if (bitoneRange[index].flags & B1_INHERIT_120) {
			if (synth->mem.param[120] == 0)
				prePark[0].mem.param[index] = ((float) v) / CONTROLLER_RANGE;
		} else if (bitoneRange[index].flags & B1_INHERIT_121) {
			if (synth->mem.param[121] == 0)
				prePark[0].mem.param[index] = ((float) v) / CONTROLLER_RANGE;
		} else if (bitoneRange[index].flags & B1_INHERIT_122) {
			if (synth->mem.param[122] == 0)
				prePark[0].mem.param[index] = ((float) v) / CONTROLLER_RANGE;
		} else if (bitoneRange[index].flags & B1_INHERIT_SEC)
			prePark[0].mem.param[index] = ((float) v) / CONTROLLER_RANGE;
		else if (bitoneRange[index].flags & B1_INHERIT_PRI)
			layer->mem.param[index] = scratch->mem.param[index] =
				prePark[0].mem.param[index];
	} else {
		/*
		 * Else we are loading a memory. See if we have 'INHERIT_PRI' set
		 * which would override this value. We cheat a little here, put the
		 * value into the layer then see about inherit primary. If this is the
		 * primary layer then there is little effect, if not though we will
		 * inherit its value....
		 *
		 * We also need to consider the diverse INHERIT flags here too. If we
		 * have separate split/transpose requested then do not inherit those
		 * flags.
		 */
		layer->mem.param[index] = scratch->mem.param[index] =
			((float) v) / CONTROLLER_RANGE;

		if (bitoneRange[index].flags & B1_INHERIT_120) {
			if (synth->mem.param[120] == 0)
				layer->mem.param[index] = scratch->mem.param[index] =
					prePark[0].mem.param[index];
		} else if (bitoneRange[index].flags & B1_INHERIT_121) {
			if (synth->mem.param[121] == 0)
				layer->mem.param[index] = scratch->mem.param[index] =
					prePark[0].mem.param[index];
		} else if (bitoneRange[index].flags & B1_INHERIT_122) {
			if (synth->mem.param[122] == 0)
				layer->mem.param[index] = scratch->mem.param[index] =
					prePark[0].mem.param[index];
		} else if (bitoneRange[index].flags & B1_INHERIT_PRI)
			layer->mem.param[index] = scratch->mem.param[index] =
				prePark[0].mem.param[index];
	}

	if (bitoneRange[index].type == BITONE_STEPPED)
		v = (int) decimal;

	/*
	 * We might not be able to send directly. The first 100 buttons are not
	 * handled directly to the dispatcher but passed through to the select code.
	 * If they had dispatchers configured they are now called here on data
	 * entry. If none is available then I just send the parameter index/value
	 */
	if (synth->dispatch[index].routine != NULL)
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			v);
	else
		bitoneMidiSendMsg(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			v);

	return(0);
}

static int
bitoneIncDec(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	float value = v;
	int index;

	/*
	 * Get the index from the second display
	 */
	index = synth->mem.param[DISPLAY_DEV + 2] * 10
		+ synth->mem.param[DISPLAY_DEV + 3];

	event.type = BRIGHTON_FLOAT;

	if (synth->mem.param[EXTENDED_BUTTON] != 0)
		index += 100;

	if ((v = synth->mem.param[DISPLAY_DEV + 0] * 10) < 0)
		v = 0;
	else if (v > 100)
		v = 0;
	v += synth->mem.param[DISPLAY_DEV + 1];

	if (c == 0) {
		/*
		 * Decrement.
		 */
		if (--v < bitoneRange[index].min)
			v = bitoneRange[index].min;
	} else {
		/*
		 * Increment.
		 */
		if (++v > bitoneRange[index].max)
			v = bitoneRange[index].max;
	}

	if ((value = (v - bitoneRange[index].min)
		/ (bitoneRange[index].max - bitoneRange[index].min)) > 1.0)
		value = 1.0;
	if (value < 0.0)
		value = 0.0;

	/*
	 * This should happen early and the extended interface should take care
	 * of all of this.
	if ((synth->mem.param[EXTENDED_BUTTON] != 0) || (index == EXTENDED_BUTTON))
		return(bitoneExtendedEntry(synth, fd, chan, 1, c, v));
	 */

	event.value = value;
	brightonParamChange(synth->win, 0, ENTRY_POT, &event);

	return(0);
}

static int
bitoneSelectExtended(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	guiSynth *layer = b1getScratch(synth);

	if (b1debug(synth, 1))
		printf("SelectExtended data entry %i %i\n", c, v);

	selectExtended = 100;
	synth->mem.param[EXTENDED_BUTTON] = 1.0;

	bitoneUpdateDisplay(synth, fd, chan, c - 100, 2,
		(c - 99) * C_RANGE_MIN_1 / 100);

	event.type = BRIGHTON_FLOAT;
	event.value = layer->mem.param[c];
	brightonParamChange(synth->win, 0, ENTRY_POT, &event);

	return(0);
}

static int
bitoneSelect(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	guiSynth *layer = b1getScratch(synth);

	if (synth->mem.param[EXTENDED_BUTTON] != 0)
	{
		if (b1debug(synth, 1))
			printf("Extended data entry on address only\n");

		if (selectExtended != 0) {
			selectExtended = 0;
			synth->mem.param[EXTENDED_BUTTON] = 0;
		}
		return(0);
	}

	selectExtended = 0;
	synth->mem.param[EXTENDED_BUTTON] = 0.0;

	bitoneUpdateDisplay(synth, fd, chan, c, 2,
		(c + 1) * CONTROLLER_RANGE / 100);

	event.type = BRIGHTON_FLOAT;
	event.value = layer->mem.param[c];
	brightonParamChange(synth->win, 0, ENTRY_POT, &event);

	return(0);
}

static void
bitoneSaveMemory(guiSynth *synth)
{
	guiSynth *layer = synth;
	int loc, oloc = 0; /* This does not need to be initted but GCC complains */
	float dbg;

	if (entryPoint == ENTER_UPPER)
	{
		if (synth->mem.param[102] == 0)
		{
			if (b1debug(synth, 1))
				printf("No save upper layer (DE 102)\n");
			return;
		}
		layer = (guiSynth *) synth->second;
		if ((loc = synth->mem.param[DISPLAY_DEV + 6] * 10 +
			synth->mem.param[DISPLAY_DEV + 7]) >= 100)
			oloc %= 100;
		if ((oloc = synth->mem.param[DISPLAY_DEV + 4] * 10 +
			synth->mem.param[DISPLAY_DEV + 5]) >= 100)
			oloc = -1;
	} else {
		if ((loc = synth->mem.param[DISPLAY_DEV + 4] * 10 +
			synth->mem.param[DISPLAY_DEV + 5]) >= 100)
			loc %= 100;
		if ((oloc = synth->mem.param[DISPLAY_DEV + 6] * 10 +
			synth->mem.param[DISPLAY_DEV + 7]) >= 100)
			oloc = -1;
	}

	layer->mem.param[SAVE_MODE] = mode;
	if (mode != MODE_SINGLE)
		layer->mem.param[SAVE_PEER] = oloc;
	else
		layer->mem.param[SAVE_PEER] = -1;
	layer->mem.param[SAVE_STEREO] = synth->mem.param[STEREO_BUTTON];

	/*
	 * Clear extended and non-extended flags, we don't want either at
	 * startup.
	 */
	layer->mem.param[EXTENDED_BUTTON] = 0.0;
	layer->mem.param[74] = 0.0;
	layer->mem.param[75] = 0.0;
	layer->mem.param[130] = 0.0;
	layer->mem.param[131] = 0.0;
	layer->mem.param[NON_EXTENDED_BUTTON] = 0.0;

	if ((dbg = synth->mem.param[109]) != 0)
		printf("saving %i, peer %i\n", loc, oloc);
	if (b1debug(synth, 5) == 0)
		synth->mem.param[109] = 0;

	saveMemory(layer, SYNTH_NAME, 0, loc, 0);

	if (entryPoint == ENTER_UPPER)
		saveSequence(synth, SYNTH_NAME, loc, BRISTOL_SID2);
	else
		saveSequence(synth, SYNTH_NAME, loc, 0);

	synth->mem.param[109] = dbg;

	return;
}

int
bitoneMemoryKey(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	float dbgH = synth->mem.param[109];
	guiSynth *layer = synth;
	brightonEvent event;

	if (entryPoint == ENTER_UPPER)
	{
		layer = (guiSynth *) synth->second;
		memUpper = location;
	} else {
		memLower = location;
		if (mode == MODE_SINGLE)
			memUpper = -1;
	}

	/*
	 * Load memory - need to look at layers and selections.
	 */
	loadMemory(layer, SYNTH_NAME, 0, location, layer->mem.active, 0,
		BRISTOL_FORCE|BRISTOL_NOCALLS|BRISTOL_SEQLOAD|BRISTOL_SEQFORCE);

	if (b1debug(synth, 1))
		printf("First load: %i: mode %2.0f, peer %2.0f, %i\n", location,
			layer->mem.param[SAVE_MODE],
			layer->mem.param[SAVE_PEER], entryPoint);

	synth->mem.param[109] = dbgH;

	/* call a second shim first for extended data parameters? */
	if (entryPoint == ENTER_UPPER)
		bitoneMemoryShim(synth, ENTRY_MAX);
	else
		bitoneMemoryShim(synth, ACTIVE_DEVS - 10);

	bitoneDAUpdate(synth);

	/*
	 * Set the stereo from the placeholder.
	 */
	event.type = BRIGHTON_FLOAT;
	event.value = synth->mem.param[SAVE_STEREO];
	brightonParamChange(synth->win, 0, STEREO_BUTTON, &event);

	return(0);
}

static void
bitoneMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	guiSynth *layer = synth, *other = (guiSynth *) synth->second;
	brightonEvent event;
	int loc;

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	/*
	 * Saving memories requires that we take a peek at the split/double settings
	 * and then select the memory location accordingly. These are not going to
	 * be performance parameters so we only save a single memory at a time.
	 */
	if (c == 0) {
		if (brightonDoubleClick(dc) != 0)
			bitoneSaveMemory(synth);

		return;
	}

	if (c == 1) {
		/*
		 * What we want to do here is have a single click load the current
		 * layer, and a double click load the other layer IF we are in split
		 * or double mode
		 */
		if (brightonDoubleClick(dc) == 0)
		{
			float dbgH = synth->mem.param[109];

			if (entryPoint == ENTER_UPPER)
			{
				layer = (guiSynth *) synth->second;
				memUpper = loc = synth->mem.param[DISPLAY_DEV + 6] * 10 +
					synth->mem.param[DISPLAY_DEV + 7];
			} else {
				memLower = loc = synth->mem.param[DISPLAY_DEV + 4] * 10 +
					synth->mem.param[DISPLAY_DEV + 5];
				if (mode == MODE_SINGLE)
					memUpper = -1;
			}

			/*
			 * Load memory - need to look at layers and selections.
			 */
			loadMemory(layer, SYNTH_NAME, 0, synth->bank * 100 + loc,
				layer->mem.active, 0,
				BRISTOL_FORCE|BRISTOL_NOCALLS|BRISTOL_SEQLOAD|BRISTOL_SEQFORCE);

			if (b1debug(synth, 1))
				printf("First load: %i: mode %2.0f, peer %2.0f, %i\n", loc,
					layer->mem.param[SAVE_MODE],
					layer->mem.param[SAVE_PEER], entryPoint);

			synth->mem.param[109] = dbgH;

			/* call a second shim first for extended data parameters? */
			if (entryPoint == ENTER_UPPER)
				bitoneMemoryShim(synth, ENTRY_MAX);
			else
				bitoneMemoryShim(synth, ACTIVE_DEVS - 10);

			bitoneDAUpdate(synth);

			/*
			 * Set the stereo from the placeholder.
			 */
			event.type = BRIGHTON_FLOAT;
			event.value = synth->mem.param[SAVE_STEREO];
			brightonParamChange(synth->win, 0, STEREO_BUTTON, &event);
		} else {
			int peer;

			if (entryPoint == ENTER_UPPER)
			{
				layer = synth;
				other = ((guiSynth *) synth->second);
				memLower = loc =
					((guiSynth *) synth->second)->mem.param[SAVE_PEER];
				peer = 1;
			} else {
				layer = (guiSynth *) synth->second;
				other = synth;
				memUpper = loc = synth->mem.param[SAVE_PEER];
				peer = 2;
			}

			/*
			 * See if we have a dual loadable memory:
			 */
			if ((other->mem.param[SAVE_MODE] <= 0) ||
				(other->mem.param[SAVE_MODE] > MODE_LAYER) ||
				(other->mem.param[SAVE_PEER] < 0))
			{
				if (b1debug(synth, 1))
					printf("not a dual loadable memory\n");
				return;
			}

			if (b1debug(synth, 1))
				printf("Dual load %1.0f, %1.0f: %f\n",
					other->mem.param[SAVE_MODE],
					other->mem.param[SAVE_PEER],
					synth->mem.param[202]);
			/*
			 * Set the operative mode
			 */
			event.type = BRIGHTON_FLOAT;
			event.value = 1.0;
			brightonParamChange(synth->win, 0,
				RADIOSET_3 + (int) other->mem.param[SAVE_MODE] - 1, &event);
			/* And select a layer */
			brightonParamChange(synth->win, 0, RADIOSET_2 + peer, &event);

			if (b1debug(synth, 1))
				printf("Dual load now %i\n", entryPoint);
			loadMemory(layer, SYNTH_NAME, 0, synth->bank * 100 + loc,
				ENTRY_MAX, 0,
				BRISTOL_NOCALLS|BRISTOL_FORCE|BRISTOL_SEQLOAD|BRISTOL_SEQFORCE);

			bitoneMemoryShim(synth, ENTRY_MAX);

			bitoneUpdateDisplay(synth, fd, chan, c, entryPoint * 2 + 2,
				(loc + 1) * CONTROLLER_RANGE / 100);

			bitoneDAUpdate(synth);
		}

		/*
		 * And update parameters on the display to reflect both memories and
		 * the selected entry parameter
		 */
	}
}

/*
 * This currently (12/02/08) implements a single channel for both layers. It
 * needs to be extended based on DE-120 to support different channels.
 */
static void
bitoneMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	guiSynth *layer = synth, *scratch = &prePark[0];
	int newchan;

	if (b1debug(synth, 1))
		printf("bitoneMidi(%i, %i) %i\n",
			b1decimal(73, prePark[0].mem.param[73]),
			b1decimal(73, prePark[1].mem.param[73]),
			entryPoint);

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (entryPoint == ENTER_UPPER) {
		if (b1debug(synth, 5))
			printf("BitOne MIDI Upper: %i, %i, %i (%i)\n", c, o, v, entryPoint);
		layer = (guiSynth *) synth->second;
		scratch = &prePark[1];
	} else
		if (b1debug(synth, 5))
			printf("BitOne MIDI Lower: %i, %i, %i (%i)\n", c, o, v, entryPoint);

/*
	newchan = synth->mem.param[DISPLAY_DEV] * 10 +
		synth->mem.param[DISPLAY_DEV + 1];
*/
	newchan = v;

	/*
	 * Logically we want to take the lower layer midi channel in all cases
	 * except when on upper layer with DE 120 set
	 */
	if ((entryPoint == ENTER_UPPER) && (synth->mem.param[120] == 0))
		newchan = b1decimal(73, prePark[0].mem.param[73]);

	if (newchan < 1)
		newchan = 1;
	newchan--;

	if (newchan < 0)
		newchan = 0;
	if (newchan > 15)
		newchan = 15;

	if (layer->mem.param[72] != 0)
	{
		layer->midichannel = newchan;

		if (b1debug(synth, 2))
			printf("Midi channel request not active in OMNI: %i\n", newchan);
		return;
	}

	/*
	 * See if we have separate MIDI channel by layer (not the default).
	 */
	if  (synth->mem.param[120] == 0)
	{
		if (b1debug(synth, 2))
			printf("dual send %i\n", newchan);

		/*
		 * Send this to both channels. This is not always intuitive, we may
		 * only be using one layer however we have to keep both on same channel.
		 */
		if (global.libtest == 0)
		{
			bristolMidiSendMsg(global.controlfd, synth->sid,
				127, 0, BRISTOL_MIDICHANNEL|newchan);

			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, BRISTOL_MIDICHANNEL|newchan);
		}
	} else {
		if (b1debug(synth, 2))
			printf("single send %i on %i\n", newchan, entryPoint);
		/*
		 * If we get here then we are only going to configure the channel for
		 * the one layer and they can then be different.
		 */
		if (global.libtest == 0)
		{
			if (entryPoint == ENTER_UPPER)
				bristolMidiSendMsg(global.controlfd, layer->sid2,
					127, 0, BRISTOL_MIDICHANNEL|newchan);
			else
				bristolMidiSendMsg(global.controlfd, layer->sid,
					127, 0, BRISTOL_MIDICHANNEL|newchan);
		}
	}

	layer->midichannel = newchan;

	return;
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
bitoneCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*	printf("bitoneCallback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (bitoneApp.resources[0].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	/*
	 * See how the Split/Double is organised. If we are under the first 100
	 * memories and we are not split or double then we should set both synth
	 * layers?
	 */
	if (index >= ACTIVE_DEVS)
	{
		synth->mem.param[index] = value;

		if (synth->dispatch[index].routine == NULL)
			return(0);

		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
	} else {
		if (index < 100)
			bitoneSelect(synth,
				global.controlfd, synth->sid,
				synth->dispatch[index].controller,
				synth->dispatch[index].operator,
				sendvalue);
		else
			bitoneSelectExtended(synth,
				global.controlfd, synth->sid,
				synth->dispatch[index].controller,
				synth->dispatch[index].operator,
				sendvalue);
	}

	return(0);
}

/*
 * Park will first copy the scratchpads (both of them) to the synth memory
 * structures. A double click will save them (both?) to memories.
 *
 * I don't think we should do both at once, just the current layer.....
 */
static void
bitonePark(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int i;
	guiSynth *scratch = &prePark[0], *layer = synth;

	if (brightonDoubleClick(dc) != 0)
	{
		/*
		 * We should save and exit here. We do allow saving of the current
		 * memory only if it is the lower layer in some circumstances.
		 */
		if ((entryPoint != ENTER_UPPER) || (synth->mem.param[102] != 0))
			bitoneSaveMemory(synth);
		else if (b1debug(synth, 1))
			printf("No save upper layers (DE 102)\n");
		return;
	}

	if ((synth->mem.param[102] == 0) && (mode != MODE_SINGLE))
	{
		if (b1debug(synth, 1))
			printf("no parking layer edits (DE 102)\n");
		return;
	}

	if (synth->mem.param[101] != 0)
	{
		if (b1debug(synth, 1))
			printf("no park/compare scratchpad\n");
		return;
	}

	if (b1debug(synth, 2))
		printf("bitonePark(%i)\n", v);

	if (entryPoint == ENTER_UPPER) {
		scratch = &prePark[1];
		layer = (guiSynth *) synth->second;
	}

	/*
	 * Turn off the Compare button if lit. Oops, no, if we park when compared
	 * then there we should revert back to the previous memory and discard the
	 * changes - we are currently listening to the original sound, that is the
	 * one we want to park.
	 */
	if (synth->mem.param[223] != 0) 
	{
		guiSynth *p1 = &prePark[0], *p2 = &prePark[2];

		if (b1debug(synth, 1))
			printf("Parking with Compare, discard edits, keep original\n");

		/* 
		 * Parking when compared means dump the edits we made. Copy 0->2 or
		 * 1->3 then turn off compare.
		 */
		if (entryPoint == ENTER_UPPER) {
			p1 = &prePark[1];
			p2 = &prePark[3];
		}

		for (i = 0; i < ENTRY_MAX; i++)
			p2->mem.param[i] = p1->mem.param[i];

		bitoneCompareOff(synth);
	}

	/*
	 * Copy over respective memories. We only copy the first 100 since they are
	 * the only ones that use the prePark cache - to compare the audio. The
	 * Extended Entry are put directly into the synth memory bypassing the 
	 * cache.
	 */
	for (i = 0; i < ENTRY_MAX; i++)
		layer->mem.param[i] = scratch->mem.param[i];
}

static void
bitoneCompare(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int i;
	guiSynth *scratch, *scratch2, *layer;

	if ((synth->flags & MEM_LOADING) != 0)
		return;

	if (synth->mem.param[101] != 0)
	{
		if (b1debug(synth, 1))
			printf("no park/compare scratchpad\n");
		return;
	}

	if (b1debug(synth, 2))
		printf("bitoneCompare(%i)\n", v);

	if (v != 0) {
		/*
		 * We want to reactivate the loaded memory, not the scratchpad.
		 */
		if (entryPoint == ENTER_UPPER)
		{
			scratch2 = &prePark[3];
			layer = (guiSynth *) synth->second;
		} else {
			scratch2 = &prePark[2];
			layer = synth;
		}
		scratch = b1getScratch(synth);

		/* Save current scratchpad */
		for (i = 0; i < ENTRY_MAX; i++)
			scratch2->mem.param[i] = scratch->mem.param[i];

		/* Call the memory shim to activate the memory values */
		bitoneMemoryShim(synth, ENTRY_MAX);

		bitoneDAUpdate(synth);

		return;
	}

	/*
	 * Compare is going off so we want to active the layer scratchpad.
	 *
	 * Find the layer.
	 * Copy layer memory to the 5th scratchpad.
	 * Copy the layer scratch to the memory.
	 * Shim it.
	 * Copy 5th scratch back to memory.
	 * Copy saved scratch back to scratch
	 */
	if (entryPoint == ENTER_UPPER) {
		layer = (guiSynth *) synth->second;
		scratch = &prePark[3];
	} else {
		layer = synth;
		scratch = &prePark[2];
	}
	scratch2 = &prePark[4];

	/* Save a copy of the real memory */
	for (i = 0; i < ENTRY_MAX; i++)
		scratch2->mem.param[i] = layer->mem.param[i];

	/* Copy the layer scratch to the memory */
	for (i = 0; i < ENTRY_MAX; i++)
		layer->mem.param[i] = scratch->mem.param[i];

	/* Call the memory shim to activate this stuff */
	bitoneMemoryShim(synth, ENTRY_MAX);

	/* Restore a copy of the real memory */
	for (i = 0; i < ENTRY_MAX; i++)
		layer->mem.param[i] = scratch2->mem.param[i];

	/* Restore a copy of original scratch */
	if (entryPoint == ENTER_UPPER)
		scratch2 = &prePark[3];
	else
		scratch2 = &prePark[2];
	scratch = b1getScratch(synth);
	scratch = &prePark[0];
	for (i = 0; i < ENTRY_MAX; i++)
		scratch->mem.param[i] = scratch2->mem.param[i];

	bitoneDAUpdate(synth);
}

/*
 * The LFO waveform is exclusive and that needs to be managed here. There are
 * some issues related to the parking of parameters that gets kind of side-
 * stepped here.
 *
 * The LFO has three active waveforms, tri, ramp/saw and square. If all are
 * unselected I will have the LFO generate sine.
 */
static void
bitoneLFO(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	guiSynth *layer = b1getScratch(synth);
	int c0, c1, c2, c3;

	if (v == 0)
		return;

	/*
	 * Now we have to form exclusions
	 */
	switch (o) {
		case 0:
		default:
			c0 = c < 4? 13:21;
			c1 = c < 4? 2:54;
			c2 = c < 4? 3:55;
			c3 = c < 4? 81:87;
			break;
		case 1:
			c0 = c < 4? 14:22;
			c1 = c < 4? 1:53;
			c2 = c < 4? 3:55;
			c3 = c < 4? 81:87;
			break;
		case 2:
			c0 = c < 4? 15:23;
			c1 = c < 4? 1:53;
			c2 = c < 4? 2:54;
			c3 = c < 4? 81:87;
			break;
		case 3:
			c0 = c == 81? 16:24;
			c1 = c == 81? 1:53;
			c2 = c == 81? 2:54;
			c3 = c == 81? 3:55;
			break;
	}

	bristolMidiSendMsg(fd, synth->sid, 126, c0, C_RANGE_MIN_1);

	layer->mem.param[c1] = 0;
	layer->mem.param[c2] = 0;
	layer->mem.param[c3] = 0;
}

static void
bitoneStereo(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	guiSynth *llayer = &prePark[0];
	guiSynth *ulayer = &prePark[0];
	int lli = 66, uli = 67;

	float ll, lr, ul, ur;

	if (b1debug(synth, 1))
		printf("bitoneStereo(%i, %i, %i)\n", c, o, v);

	/* Intercept the requests to see if we want to rework the settings? */
	if (o == 1) {
		/*
		 * Stereo/Mono button
		 */
	} else {
		/*
		 * Upper/Lower layer gains
		 */
	}

	if (synth->mem.param[118] != 0)
	{
		/*
		 * The bit-100 flag, separate layer volumes but from the single param.
		 */
		ulayer = &prePark[1];
		uli = 66;
	}

	/*
	 * Check layer volumes and set them. The defaults will be 0.7 even panning
	 * for Mono and hard 1.0 panning for Stereo.
	 */
	if (synth->mem.param[202] == 0) {
		/*
		 * Mono panning.
		 */
		ll = llayer->mem.param[lli] * synth->mem.param[110];
		lr = llayer->mem.param[lli] * synth->mem.param[111];
		ul = ulayer->mem.param[uli] * synth->mem.param[114];
		ur = ulayer->mem.param[uli] * synth->mem.param[115];
	} else {
		/*
		 * Stereo panning.
		 */
		ll = llayer->mem.param[lli] * synth->mem.param[112];
		lr = llayer->mem.param[lli] * synth->mem.param[113];
		ul = ulayer->mem.param[uli] * synth->mem.param[116];
		ur = ulayer->mem.param[uli] * synth->mem.param[117];
	}

	if (b1debug(synth, 3))
	{
		printf("M: %f %f %f %f\n",
			synth->mem.param[110],synth->mem.param[111],
			synth->mem.param[114],synth->mem.param[115]);
		printf("S: %f %f %f %f\n", 
			synth->mem.param[112],synth->mem.param[113],
			synth->mem.param[116],synth->mem.param[117]);
		printf("C: %f %f %f %f\n", ll,  lr, ul, ur);
	}

	/*
	 * Always send the lower layer volumes.
	 * Send the upper layer volumes if MODE_SINGLE is not in operation
	 */
	bristolMidiSendMsg(fd, synth->sid, 126, 5, (int) (C_RANGE_MIN_1 * ll));
	bristolMidiSendMsg(fd, synth->sid, 126, 6, (int) (C_RANGE_MIN_1 * lr));

	if (mode != MODE_SINGLE) {
		if (b1debug(synth, 1))
			printf("mode dual\n");
		bristolMidiSendMsg(fd, synth->sid2, 126, 5, (int) (C_RANGE_MIN_1 * ul));
		bristolMidiSendMsg(fd, synth->sid2, 126, 6, (int) (C_RANGE_MIN_1 * ur));
	} else {
		if (b1debug(synth, 1))
			printf("mode single\n");
		bristolMidiSendMsg(fd, synth->sid2, 126, 5, 0);
		bristolMidiSendMsg(fd, synth->sid2, 126, 6, 0);
	}
}

/*
 * This is joint unison from the bit-01 controls. There are separate selectors
 * for the emulations per layer, control ID 80 which is consequently set here.
 * The value would have to be parked to be saved in memory.
 */
static void
bitoneUnison(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (v == 0)
	{
		prePark[0].mem.param[80]= 0.0;
		prePark[1].mem.param[80]= 0.0;
	} else {
		prePark[0].mem.param[80]= 1.0;
		prePark[1].mem.param[80]= 1.0;
	}
	bristolMidiSendMsg(fd, synth->sid, 126, 7, v);
	bristolMidiSendMsg(fd, synth->sid2, 126, 7, v);
}

static void
bitoneDualSend(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid, c, o, v);
	bristolMidiSendMsg(fd, synth->sid2, c, o, v);
}

static void
bitoneHarmonics(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	guiSynth *layer = b1getScratch(synth);
	float value;
	int oscIndex = 24, i;
	int sid = synth->sid;

	if (c == DCO_1_EXCLUSIVE)
	{
		c = 2;
		o = 0;
	} else if (c == DCO_2_EXCLUSIVE) {
		c = 2;
		o = 1;
	}

	if (entryPoint == 2)
		sid = synth->sid2;

	if (b1debug(synth, 2))
		printf("bitoneHarmonics(%i, %i, %i)\n", c, o, v);

	/*
	 * If we have the value 0 then we need to force exclusion on the different
	 * harmonics of the DCO.
	 */
	if (c == 0)
	{
		if (b1debug(synth, 3))
			printf("control\n");

		/*
		 * Select which osc to manipulate
		 */
		if (o != 0)
			oscIndex = 35;

		/*
		 * This is the controlling boolean. If this goes to one we need to 
		 * start enforcing exclusion - take the first harmonic and clear the
		 * rest
		 */
		if (v != 0) {
			value =  1.0;

			/* Remove all but the first harmonic */
			for (i = 0;i < 4; i++)
			{
				if (layer->mem.param[oscIndex + i] != 0)
				{
					layer->mem.param[oscIndex + i] = value;
					/* Send event to engine */
					/* Decrement the value - only the first hit will remain 1 */
					if ((value -=  1.0) < 0)
						value = 0.0;
				}
			}
		}
	} else {
		int osc = 4, parm;

		if (b1debug(synth, 3))
			printf("DCO\n");
		if (c >= 35)
		{
			oscIndex = 35;
			osc = 5;
		}
		parm = c - oscIndex;

		/*
		 * These are the harmonic selectors from the DCO. If the value is zero
		 * just clear the harmonic, otherwise when going on we need to check
		 * the boolean to ensure exclusion if configured.
		 */
		if (v == 0) {
			/* Send value to engine unless its the last one */
			if ((layer->mem.param[oscIndex] == 0)
				&& (layer->mem.param[oscIndex + 1] == 0) 
				&& (layer->mem.param[oscIndex + 2] == 0)
				&& (layer->mem.param[oscIndex + 3] == 0))
				return;

			bristolMidiSendMsg(fd, sid, osc, parm, v);

			return;
		}

		/*
		 * Now we can turn the harmonic on
		 */
		bristolMidiSendMsg(fd, sid, osc, parm, v);

		/*
		 * then we start exclusion.
		 */
		if (c >= 35) {
			if (layer->mem.param[DCO_2_EXCLUSIVE] == 0.0)
				return;
			oscIndex =  35;
		} else {
			if (layer->mem.param[DCO_1_EXCLUSIVE] == 0.0)
				return;
			oscIndex =  24;
		}

		value = 0.0;

		for (i = 0;i < 4; i++)
		{
			if ((oscIndex + i) == c)
				continue;

			if (layer->mem.param[oscIndex + i] != 0)
			{
				layer->mem.param[oscIndex + i] = 0.0;
				/* send value off to osc */
				bristolMidiSendMsg(fd, sid, osc, i, 0);
			}
		}
	}
}

static void
bitoneMod(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (global.libtest)
		return;

	if (v == 0)
		v = 1;

	/*
	 * If this is controller 0 it is the frequency control, otherwise a 
	 * generic controller 1.
	 */
	if (c == 1) {
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_PITCH, 0, (int) (v * C_RANGE_MIN_1));
		if ((synth->mem.param[120] != 0) &&
			(synth->midichannel != ((guiSynth *) synth->second)->midichannel))
			bristolMidiSendMsg(global.controlfd,
				((guiSynth *) synth->second)->midichannel,
				BRISTOL_EVENT_PITCH, 0, (int) (v * C_RANGE_MIN_1));
	} else {
		bristolMidiControl(global.controlfd, synth->midichannel,
			0, 1, ((int) (v * C_RANGE_MIN_1)) >> 7);
		if ((synth->mem.param[120] != 0) &&
			(synth->midichannel != ((guiSynth *) synth->second)->midichannel))
			bristolMidiControl(global.controlfd,
				((guiSynth *) synth->second)->midichannel,
				0, 1, ((int) (v * C_RANGE_MIN_1)) >> 7);
	}
	return;
}

static void
bitoneGlide(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (!global.libtest) {
		bristolMidiSendNRP(global.controlfd, synth->sid, BRISTOL_NRP_GLIDE, v);
		bristolMidiSendNRP(global.controlfd, synth->sid2, BRISTOL_NRP_GLIDE, v);
	}
}

static void
bitoneMidiDebug(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (!global.libtest) {
		bristolMidiSendNRP(global.controlfd, synth->sid, BRISTOL_NRP_DEBUG, v);
		bristolMidiSendNRP(global.controlfd, synth->sid2, BRISTOL_NRP_DEBUG, v);
	}
}

static void
bitoneVelocity(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	synth->velocity = v + 500;

	if (global.libtest)
		return;

	if (mode == MODE_SINGLE) {
		bristolMidiSendNRP(global.controlfd, synth->sid,
			BRISTOL_NRP_VELOCITY, synth->velocity);
		bristolMidiSendNRP(global.controlfd, synth->sid2,
			BRISTOL_NRP_VELOCITY, synth->velocity);
	} else {
		if (entryPoint == ENTER_UPPER)
			bristolMidiSendNRP(global.controlfd, synth->sid2,
				BRISTOL_NRP_VELOCITY, synth->velocity);
		else
			bristolMidiSendNRP(global.controlfd, synth->sid,
				BRISTOL_NRP_VELOCITY, synth->velocity);
	}
}

static void
bitoneTranspose(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int lt = -1, ut = -1;

	if (b1debug(synth, 1))
		printf("Transpose (%i, %f)\n", v, prePark[0].mem.param[65]);

	if (v > 61)
		/*
		 * We have probably been given an integer from the pot range, convert
		 * it to decimal.
		 */
		v = b1decimal(65, prePark[0].mem.param[65]);

	/*
	 * We are going to request transpose to the engine. At the moment this
	 * would cause issues with memory loading or transpose changes. The former
	 * is avoided due to allNotesOff() when the midi channel is changed when
	 * memories are loaded however alterations to transpose when holding notes
	 * will need to be addressed. Resolved.
	 *
	 * We need to look at separate split points, this pends on the DE 121 flag.
	 *
	 * We either transpose just the upper layer unless we have DE 121 and we
	 * are setting the lower layer transpose.
	 */
	if (mode == MODE_SINGLE) {
		lt = v;
	} else {
		if (synth->mem.param[122] != 0) {
			if (entryPoint == ENTER_UPPER)
				ut = v;
			else
				lt = v;
		} else
			ut = b1decimal(65, prePark[0].mem.param[65]);
	}

	if (b1debug(synth, 2))
		printf("lt %i, ut %i\n", lt, ut);

	if (!global.libtest) {
		if (lt != -1)
			bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
				BRISTOL_TRANSPOSE + lt - 12);
		else
			bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
				BRISTOL_TRANSPOSE + ut - 12);
	}
}

static void
bitoneOmni(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	guiSynth *layer = b1getScratch(synth);
	int channel = synth->midichannel;

	if (v != 0) {
		if (b1debug(synth, 1))
			printf("OMNI On Requested\n");
		channel = BRISTOL_CHAN_OMNI;
	} else {
		if (b1debug(synth, 1))
			printf("OMNI Off Requested\n");
	}

	/*
	 * See if we have separate MIDI channel by layer (not the default).
	 */
	if  (layer->mem.param[120] == 0)
	{
		/*
		 * Send this to both channels.
		 */
		if (global.libtest == 0)
		{
			bristolMidiSendMsg(global.controlfd, synth->sid,
				127, 0, BRISTOL_MIDICHANNEL|channel);
			bristolMidiSendMsg(global.controlfd, synth->sid2,
				127, 0, BRISTOL_MIDICHANNEL|channel);
		}
	} else {
		/*
		 * If we get here then we are only going to configure the channel for
		 * the one layer and they can then be different.
		 */
		if (global.libtest == 0)
		{
			bristolMidiSendMsg(global.controlfd, layer->sid,
				127, 0, BRISTOL_MIDICHANNEL|channel);
		}
	}
}

static void
bitoneEmulationGain(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	synth->gain = v / 16;
	((guiSynth *) synth->second)->gain = v / 16;

	if (!global.libtest) {
		bristolMidiSendNRP(global.controlfd, synth->sid,
			BRISTOL_NRP_GAIN, v / 16);
		bristolMidiSendNRP(global.controlfd, synth->sid2,
			BRISTOL_NRP_GAIN, v / 16);
	}
}

static void
bitoneNRPEnable(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (!global.libtest) {
		bristolMidiSendNRP(global.controlfd, synth->sid,
			BRISTOL_NRP_ENABLE_NRP, v);
		bristolMidiSendNRP(global.controlfd, synth->sid2,
			BRISTOL_NRP_ENABLE_NRP, v);
	}
}

static void
bitoneDetune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int value = v / 50;

	if (!global.libtest) {
		bristolMidiSendNRP(global.controlfd, synth->sid,
			BRISTOL_NRP_DETUNE, value);
		bristolMidiSendNRP(global.controlfd, synth->sid2,
			BRISTOL_NRP_DETUNE, value);
	}
}

static int
bitoneGetMem(guiSynth *synth, int dir)
{
	int i = synth->location;

	if ((dir > 0) && (i >= 99))
		i = -1;
	if ((dir < 0) && (i <= 0))
		i = 100;

	while ((i += dir) != synth->location)
	{
		if (loadMemory(synth, SYNTH_NAME, 0, synth->bank * 100 + i,
			synth->mem.active, 0, BRISTOL_STAT) >= 0)
			return(i);

		if ((dir > 0) && (i >= 99))
			i = -1;
		if ((dir < 0) && (i <= 0))
			i = 100;
	}

	return(-1);
}

static int
bitoneGetFreeMem(guiSynth *synth, int dir)
{
	int i, start;

	start = i = synth->mem.param[DISPLAY_DEV + 4] * 10 +
		synth->mem.param[DISPLAY_DEV + 5];

	if ((dir > 0) && (i >= 99))
		i = -1;
	if ((dir < 0) && (i <= 0))
		i = 100;

	while ((i += dir) != start)
	{
		if (loadMemory(synth, SYNTH_NAME, 0, synth->bank * 100 + i,
			synth->mem.active, 0, BRISTOL_STAT) < 0)
			return(i);

		if ((dir > 0) && (i >= 99))
			i = -1;
		if ((dir < 0) && (i <= 0))
			i = 100;
	}

	return(-1);
}

static int exclude = 0;

static void
bitoneMemSearch(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int i;

	if ((synth->flags & MEM_LOADING) != 0)
		return;

//	if ((v == 0) || (exclude != 0))
	if (exclude != 0)
		return;

	/* This does not go in a memory */
	synth->mem.param[c] = prePark[0].mem.param[c] = prePark[1].mem.param[c] = 0;

	/*
	 * Find the next mem
	 */
	if (c == 74) {
		if ((i = bitoneGetMem(synth, 1)) < 0)
			return;
	} else {
		if ((i = bitoneGetMem(synth, -1)) < 0)
			return;
	}

	/*
	 * Then stuff this into a display, exit extended and then load the memory.
	 */
	if (entryPoint == ENTER_UPPER)
		bitoneUpdateDisplayDec(synth, synth->sid, synth->midichannel, 0, 6, i);
	else
		bitoneUpdateDisplayDec(synth, synth->sid, synth->midichannel, 0, 4, i);

	synth->location = i;

	exclude = 1;
	bitoneMemory(synth, fd, chan, 1, o, 1);
	exclude = 0;
}

static void
bitoneMemFreeSearch(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int i;

	if (synth->flags & MEM_LOADING)
		return;

//	if ((v == 0) || (exclude != 0))
	if (exclude != 0)
		return;

	/* This does not go in a memory */
	synth->mem.param[c] = prePark[0].mem.param[c] = prePark[1].mem.param[c] = 0;

	/*
	 * Find a free mem
	 */
	if (c == 130) {
		if ((i = bitoneGetFreeMem(synth, 1)) < 0)
			return;
	} else {
		if ((i = bitoneGetFreeMem(synth, -1)) < 0)
			return;
	}

	/*
	 * Then stuff this into a display
	 */
	if (entryPoint == ENTER_UPPER)
		bitoneUpdateDisplayDec(synth, synth->sid, synth->midichannel, 0, 6, i);
	else
		bitoneUpdateDisplayDec(synth, synth->sid, synth->midichannel, 0, 4, i);
}

static void
bitoneSaveOther(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int i;
	guiSynth *scratch = b1getScratch(synth);

	/*
	 * If we have extended entry and the value is zero then return.
	 */
	if ((synth->mem.param[DISPLAY_DEV + 5] * 10
		+ synth->mem.param[DISPLAY_DEV + 6] == 110) && (v == 0))
		return;

	synth->mem.param[c] = 0;

	if (c == 193) {
		printf("bristol save bit-1\n");

		prePark[5].location = synth->location;
		prePark[5].bank = synth->bank;

		prePark[5].win = synth->win;
		prePark[5].mem.active = ACTIVE_DEVS;
		prePark[5].resources = &bitoneApp;

		/*
		 * Copy over all the parameters.
		 */
		for (i = 0; i < ACTIVE_DEVS; i++)
			prePark[5].mem.param[i] = scratch->mem.param[i];
		for (; i < DEVICE_COUNT; i++)
			prePark[5].mem.param[i] = synth->mem.param[i];

		/*
		 * Clean up the ones that do not apply to this synth
		 *
		 * FM/Glide, etc.
		 */
		prePark[5].mem.param[72] = 1.0; /* OMNI On */
		prePark[5].mem.param[77] = 0;
		prePark[5].mem.param[78] = 0;
		prePark[5].mem.param[79] = 0;

		prePark[5].mem.param[81] = 0;
		prePark[5].mem.param[82] = 0;
		prePark[5].mem.param[83] = 0;
		prePark[5].mem.param[84] = 0;
		prePark[5].mem.param[85] = 0;
		prePark[5].mem.param[86] = 1.0; /* LFO UNI */

		prePark[5].mem.param[87] = 0;
		prePark[5].mem.param[88] = 0;
		prePark[5].mem.param[89] = 0;
		prePark[5].mem.param[90] = 0;
		prePark[5].mem.param[91] = 0;
		prePark[5].mem.param[92] = 1.0; /* LFO UNI */

		prePark[5].mem.param[93] = 0.0; /* ENV PWM */
		prePark[5].mem.param[94] = 0.0; /* ENV PWM */

		prePark[5].mem.param[95] = 1.0; /* Restrict harmonics */
		prePark[5].mem.param[96] = 1.0; /* Restrict harmonics */

		prePark[5].mem.param[97] = 2.0; /* Filter */
		prePark[5].mem.param[98] = 1.0; /* MIX DCO1 */
		prePark[5].mem.param[99] = 1.0; /* Noise UNI */

		prePark[5].mem.param[101] = 0.0;
		prePark[5].mem.param[102] = 0.0;
		prePark[5].mem.param[103] = 0.0;
		prePark[5].mem.param[104] = 0.0;
		prePark[5].mem.param[105] = 0.0;
		prePark[5].mem.param[106] = 0.0;
		prePark[5].mem.param[107] = 0.0;
		prePark[5].mem.param[108] = 0.0;
		prePark[5].mem.param[109] = 0.0;

		prePark[5].mem.param[110] = 0.78;
		prePark[5].mem.param[111] = 0.78;
		prePark[5].mem.param[112] = 1.0;
		prePark[5].mem.param[113] = 0.0;
		prePark[5].mem.param[114] = 0.78;
		prePark[5].mem.param[115] = 0.78;
		prePark[5].mem.param[116] = 0.0;
		prePark[5].mem.param[117] = 1.0;

		prePark[5].mem.param[118] = 0.0;

		for (i = 120; i < ACTIVE_DEVS; i++)
			prePark[5].mem.param[i] = 0;

		/* Carry over glide and noise settings */
		prePark[5].mem.param[136] = 0.0;
		prePark[5].mem.param[137] = 1.0;
		prePark[5].mem.param[138] = synth->mem.param[138];
		prePark[5].mem.param[139] = synth->mem.param[139];
	} else if (c == 194) {
		printf("bristol save bit-99\n");

		prePark[5].location = synth->location;
		prePark[5].bank = synth->bank;

		prePark[5].win = synth->win;
		prePark[5].resources = &bit99App;
		prePark[5].mem.active = ACTIVE_DEVS;

		/*
		 * Copy over all the parameters.
		 */
		for (i = 0; i < ACTIVE_DEVS; i++)
			prePark[5].mem.param[i] = scratch->mem.param[i];
		for (; i < DEVICE_COUNT; i++)
			prePark[5].mem.param[i] = synth->mem.param[i];

		/*
		 * Clean up the ones that do not apply to this synth
		 */
		prePark[5].mem.param[86] = 1.0; /* LFO UNI */
		prePark[5].mem.param[92] = 1.0; /* LFO UNI */

		prePark[5].mem.param[97] = 2.0; /* Filter */
		prePark[5].mem.param[98] = 1.0; /* MIX DCO1 */
		prePark[5].mem.param[99] = 1.0; /* Noise UNI */

		prePark[5].mem.param[101] = 0.0;
		prePark[5].mem.param[102] = 0.0;
		prePark[5].mem.param[103] = 0.0;
		prePark[5].mem.param[104] = 0.0;
		prePark[5].mem.param[105] = 0.0;
		prePark[5].mem.param[106] = 0.0;
		prePark[5].mem.param[107] = 0.0;
		prePark[5].mem.param[108] = 0.0;
		prePark[5].mem.param[109] = 0.0;

		prePark[5].mem.param[118] = 0.0;

		for (i = 120; i < ACTIVE_DEVS; i++)
			prePark[5].mem.param[i] = 0;

		/* Carry over glide */
		prePark[5].mem.param[136] = 0.0;
		prePark[5].mem.param[137] = 1.0;
		prePark[5].mem.param[138] = synth->mem.param[138];
		prePark[5].mem.param[139] = synth->mem.param[139];
	} else {
		printf("bristol save bit-100\n");

		prePark[5].location = synth->location;
		prePark[5].bank = synth->bank;

		for (i = 0; i < ACTIVE_DEVS; i++)
			prePark[5].mem.param[i] = scratch->mem.param[i];
		for (; i < DEVICE_COUNT; i++)
			prePark[5].mem.param[i] = synth->mem.param[i];

		prePark[5].resources = &bit100App;
		prePark[5].win = synth->win;
		prePark[5].mem.active = ACTIVE_DEVS;
	}

	/* Arpeggio relearn */
	prePark[5].mem.param[155] = 0;
	prePark[5].mem.param[157] = 0;

	/*
	 * Save our 200 parameters to scratchpad, restate some of them to reflect
	 * the configurations of the -1 or -99, save the memory then return the
	 * scratched data. We actually just need to copy over the 100 lowest params,
	 * the rest are defaulted.
	 */
	bitoneSaveMemory(&prePark[5]);

	synth->mem.param[c] = 0;
}

static void
bitoneSeqRelearn(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (b1debug(synth, 1))
		printf("bitoneSeqRelearn\n");

	if (synth->seq1.param == NULL)
		loadSequence(&synth->seq1, SYNTH_NAME, 0, 0);
	if (synth->seq2.param == NULL)
		loadSequence(&synth->seq2, SYNTH_NAME, 0, BRISTOL_SID2);

	/*
	 * We need to turn off the arpeggiator to do this and reset one index
	 */
	if (v != 0) {
		if (entryPoint == ENTER_UPPER) {
			bristolMidiSendMsg(fd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			bristolMidiSendMsg(fd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 1);
			synth->seq1.param[0] = 0;
		} else {
			bristolMidiSendMsg(fd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			bristolMidiSendMsg(fd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 1);
			synth->seq1.param[BRISTOL_AM_SMAX] = 0;
		}
		return;
	}

	if (entryPoint == ENTER_UPPER)
		bristolMidiSendMsg(fd, synth->sid2,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
	else
		bristolMidiSendMsg(fd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
}

static void
bitoneChordRelearn(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (b1debug(synth, 1))
		printf("bitoneChordRelearn\n");

	if (synth->seq1.param == NULL)
		loadSequence(&synth->seq1, SYNTH_NAME, 0, 0);
	if (synth->seq2.param == NULL)
		loadSequence(&synth->seq2, SYNTH_NAME, 0, BRISTOL_SID2);

	/*
	 * We need to turn off chording to do this and reset one index
	 */
	if (v != 0) {
		if (entryPoint == ENTER_UPPER) {
			bristolMidiSendMsg(fd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 0);
			bristolMidiSendMsg(fd, synth->sid2,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 1);
			synth->seq1.param[1] = 0;
		} else {
			bristolMidiSendMsg(fd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 0);
			bristolMidiSendMsg(fd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 1);
			synth->seq1.param[BRISTOL_AM_CCOUNT] = 0;
		}
		return;
	}

	if (entryPoint == ENTER_UPPER)
		bristolMidiSendMsg(fd, synth->sid2,
			BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);
	else
		bristolMidiSendMsg(fd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);
}

static void
bitoneFilter(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (v == 2)
		v = 4;

	if (entryPoint == ENTER_UPPER)
		bristolMidiSendMsg(fd, synth->sid2, 6, 4, v);
	else
		bristolMidiSendMsg(fd, synth->sid, 6, 4, v);
}

static void
bitoneRelease(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (b1debug(synth, 1))
		printf("Midi Panic (%i)\n", v);

	if (v == 0)
		return;

	prePark[0].mem.param[76] = 0;

	if (!global.libtest)
		bristolMidiSendMsg(fd, synth->sid, 127, 0,
			BRISTOL_ALL_NOTES_OFF);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
bitoneInit(brightonWindow *win)
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

	printf("Initialise the bitone link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
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
	synth->synthtype = BRISTOL_BIT_ONE;
	((guiSynth *) synth->second)->synthtype = BRISTOL_BIT_ONE;

	prePark[0].mem.param = scratchpad0;
	prePark[1].mem.param = scratchpad1;
	prePark[2].mem.param = scratchpad2;
	prePark[3].mem.param = scratchpad3;
	prePark[4].mem.param = scratchpad4;
	prePark[5].mem.param = scratchpad5;

	/* If we have a default voice count then limit it */
	if (synth->voices == BRISTOL_VOICECOUNT) {
		synth->voices = 6; /* Uses twice the number for single mode only */
		((guiSynth *) synth->second)->voices = 3;
	} else
		((guiSynth *) synth->second)->voices = synth->voices >> 1;

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets (actually implements TCP unfortunately).
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		int vhold = synth->voices;

		bcopy(&global, &manual, sizeof(guimain));

		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

		manual.flags |= BRISTOL_CONN_FORCE|BRIGHTON_NOENGINE;
		manual.host = global.host;
		manual.port = global.port;

		synth->voices = synth->voices >> 1;
		if ((synth->sid2 = initConnection(&manual, synth)) < 0)
			return(-1);

		global.manualfd = manual.controlfd;
		global.manual = &manual;
		manual.manual = &global;

		((guiSynth *) synth->second)->sid = synth->sid2;

		synth->voices = vhold;
	}

	for (i = 0; i < ACTIVE_DEVS; i++)
	{
		synth->dispatch[i].controller = i;
		synth->dispatch[i].operator = i;
	}

	synth->dispatch[ACTIVE_DEVS - 1].controller = 12;

	for (; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = (synthRoutine) bitoneMidiShim;

	synth->dispatch[0].routine = (synthRoutine) bitoneMidiNull;
	synth->dispatch[73].routine = (synthRoutine) bitoneMidi;

	/* LFO Waveform */
	synth->dispatch[1].operator = 0;
	synth->dispatch[1].routine = (synthRoutine) bitoneLFO;
	synth->dispatch[2].operator = 1;
	synth->dispatch[2].routine = (synthRoutine) bitoneLFO;
	synth->dispatch[3].operator = 2;
	synth->dispatch[3].routine = (synthRoutine) bitoneLFO;

	/* LFO Waveform */
	synth->dispatch[53].operator = 0;
	synth->dispatch[53].routine = (synthRoutine) bitoneLFO;
	synth->dispatch[54].operator = 1;
	synth->dispatch[54].routine = (synthRoutine) bitoneLFO;
	synth->dispatch[55].operator = 2;
	synth->dispatch[55].routine = (synthRoutine) bitoneLFO;

	/* Harmonics */
	synth->dispatch[24].controller = 24;
	synth->dispatch[24].operator = 0;
	synth->dispatch[24].routine = (synthRoutine) bitoneHarmonics;
	synth->dispatch[25].controller = 25;
	synth->dispatch[25].operator = 1;
	synth->dispatch[25].routine = (synthRoutine) bitoneHarmonics;
	synth->dispatch[26].controller = 26;
	synth->dispatch[26].operator = 2;
	synth->dispatch[26].routine = (synthRoutine) bitoneHarmonics;
	synth->dispatch[27].controller = 27;
	synth->dispatch[27].operator = 3;
	synth->dispatch[27].routine = (synthRoutine) bitoneHarmonics;

	/* Harmonics */
	synth->dispatch[35].controller = 35;
	synth->dispatch[35].operator = 0;
	synth->dispatch[35].routine = (synthRoutine) bitoneHarmonics;
	synth->dispatch[36].controller = 36;
	synth->dispatch[36].operator = 1;
	synth->dispatch[36].routine = (synthRoutine) bitoneHarmonics;
	synth->dispatch[37].controller = 37;
	synth->dispatch[37].operator = 2;
	synth->dispatch[37].routine = (synthRoutine) bitoneHarmonics;
	synth->dispatch[38].controller = 38;
	synth->dispatch[38].operator = 3;
	synth->dispatch[38].routine = (synthRoutine) bitoneHarmonics;

	synth->dispatch[64].routine = (synthRoutine) b1SetSplit;

	synth->dispatch[65].routine = (synthRoutine) bitoneTranspose;

	synth->dispatch[66].operator = 2;
	synth->dispatch[66].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[67].operator = 2;
	synth->dispatch[67].routine = (synthRoutine) bitoneStereo;

	synth->dispatch[69].routine = (synthRoutine) bitoneVelocity;
	synth->dispatch[70].routine = (synthRoutine) bitoneMidiDebug;

	synth->dispatch[72].routine = (synthRoutine) bitoneOmni;

	synth->dispatch[74].routine = (synthRoutine) bitoneMemSearch;
	synth->dispatch[75].routine = (synthRoutine) bitoneMemSearch;

	synth->dispatch[76].routine = (synthRoutine) bitoneRelease;

	/* LFO S/H */
	synth->dispatch[81].operator = 3;
	synth->dispatch[81].routine = (synthRoutine) bitoneLFO;
	synth->dispatch[87].operator = 3;
	synth->dispatch[87].routine = (synthRoutine) bitoneLFO;

	/* Filter shim */
	synth->dispatch[97].routine = (synthRoutine) bitoneFilter;

	/* Harmonic control */
	synth->dispatch[DCO_1_EXCLUSIVE].routine = (synthRoutine) bitoneHarmonics;
	synth->dispatch[DCO_2_EXCLUSIVE].routine = (synthRoutine) bitoneHarmonics;

	synth->dispatch[123].routine = (synthRoutine) bitoneNRPEnable;

	/* Search */
	synth->dispatch[130].routine = (synthRoutine) bitoneMemFreeSearch;
	synth->dispatch[131].routine = (synthRoutine) bitoneMemFreeSearch;
	synth->dispatch[139].routine = (synthRoutine) bitoneEmulationGain;

	/* Split Double */
	synth->dispatch[200].controller = 0;
	synth->dispatch[200].routine = (synthRoutine) bitoneSplitDouble;
	synth->dispatch[201].controller = 1;
	synth->dispatch[201].routine = (synthRoutine) bitoneSplitDouble;

	/* Stereo */
	synth->dispatch[202].operator = 1;
	synth->dispatch[202].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[110].operator = 1;
	synth->dispatch[110].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[111].operator = 1;
	synth->dispatch[111].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[112].operator = 1;
	synth->dispatch[112].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[113].operator = 1;
	synth->dispatch[113].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[114].operator = 1;
	synth->dispatch[114].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[115].operator = 1;
	synth->dispatch[115].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[116].operator = 1;
	synth->dispatch[116].routine = (synthRoutine) bitoneStereo;
	synth->dispatch[117].operator = 1;
	synth->dispatch[117].routine = (synthRoutine) bitoneStereo;

	synth->dispatch[119].routine = (synthRoutine) bitoneDetune;

	synth->dispatch[138].routine = (synthRoutine) bitoneGlide;

	synth->dispatch[158].routine = (synthRoutine) bitoneChordRelearn;
	synth->dispatch[165].routine = (synthRoutine) bitoneSeqRelearn;

	synth->dispatch[193].routine = (synthRoutine) bitoneSaveOther;
	synth->dispatch[194].routine = (synthRoutine) bitoneSaveOther;
	synth->dispatch[195].routine = (synthRoutine) bitoneSaveOther;

	/* Tuning */
	synth->dispatch[203].controller = 126;
	synth->dispatch[203].operator = 1;

	synth->dispatch[206].controller = 126;
	synth->dispatch[206].operator = 3;
	synth->dispatch[206].routine = (synthRoutine) bitoneDualSend;
	synth->dispatch[207].controller = 126;
	synth->dispatch[207].operator = 4;
	synth->dispatch[207].routine = (synthRoutine) bitoneDualSend;

	/* Entry */
	synth->dispatch[ENTRY_POT].controller = 0;
	synth->dispatch[ENTRY_POT].routine = (synthRoutine) bitoneEntryPot;

	/* Data Entry buttons */
	synth->dispatch[209].controller = 1;
	synth->dispatch[209].operator = 0;
	synth->dispatch[209].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[210].controller = 2;
	synth->dispatch[210].operator = 1;
	synth->dispatch[210].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[211].controller = 3;
	synth->dispatch[211].operator = 2;
	synth->dispatch[211].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[212].controller = 4;
	synth->dispatch[212].operator = 3;
	synth->dispatch[212].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[213].controller = 5;
	synth->dispatch[213].operator = 4;
	synth->dispatch[213].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[214].controller = 6;
	synth->dispatch[214].operator = 5;
	synth->dispatch[214].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[215].controller = 7;
	synth->dispatch[215].operator = 6;
	synth->dispatch[215].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[216].controller = 8;
	synth->dispatch[216].operator = 7;
	synth->dispatch[216].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[217].controller = 9;
	synth->dispatch[217].operator = 8;
	synth->dispatch[217].routine = (synthRoutine) bitoneEntry;
	synth->dispatch[218].controller = 0;
	synth->dispatch[218].operator = 9;
	synth->dispatch[218].routine = (synthRoutine) bitoneEntry;

	/* Second radio set for address, upper and lower manual selection */
	synth->dispatch[219].operator = ENTER_ADDRESS;
	synth->dispatch[219].controller = 6;
	synth->dispatch[219].routine = (synthRoutine) bitoneALU;
	synth->dispatch[220].operator = ENTER_LOWER;
	synth->dispatch[220].controller = 4;
	synth->dispatch[220].routine = (synthRoutine) bitoneALU;
	synth->dispatch[221].operator = ENTER_UPPER;
	synth->dispatch[221].controller = 2;
	synth->dispatch[221].routine = (synthRoutine) bitoneALU;

	synth->dispatch[222].routine = (synthRoutine) bitonePark;
	synth->dispatch[223].routine = (synthRoutine) bitoneCompare;

	synth->dispatch[224].operator = 1;
	synth->dispatch[224].controller = 1;
	synth->dispatch[224].routine = (synthRoutine) bitoneMemory;
	synth->dispatch[225].controller = 0;
	synth->dispatch[225].routine = (synthRoutine) bitoneMemory;

	synth->dispatch[226].controller = 0;
	synth->dispatch[226].routine = (synthRoutine) bitoneMod;
	synth->dispatch[227].controller = 1;
	synth->dispatch[227].routine = (synthRoutine) bitoneMod;

	synth->dispatch[228].controller = 0;
	synth->dispatch[228].routine = (synthRoutine) bitoneIncDec;
	synth->dispatch[229].controller = 1;
	synth->dispatch[229].routine = (synthRoutine) bitoneIncDec;

	synth->dispatch[230].routine = NULL;
	synth->dispatch[231].routine = NULL;
	synth->dispatch[232].routine = NULL;
	synth->dispatch[233].routine = NULL;
	synth->dispatch[234].routine = NULL;
	synth->dispatch[235].routine = NULL;
	synth->dispatch[236].routine = NULL;
	synth->dispatch[237].routine = NULL;
	synth->dispatch[238].routine = NULL;
	synth->dispatch[239].routine = NULL;

	/* Unison */
	synth->dispatch[238].controller = 126;
	synth->dispatch[238].operator = 7;
	synth->dispatch[238].routine = (synthRoutine) bitoneUnison;

	/*
	 * We need to default the LFO Envelope parameters and some other stuff.
	 *
	 * There are lots of defaults we need to set for the bit-1: stereo on, MIDI
	 * channel, we need to build a method to enable disable OMNI and some of the
	 * midi controllers such as wheel, etc.
	 */
	bitoneDualSend(synth, global.controlfd, 0, 7, 4, 16000);
	/* Set noise gain - reasonably low as it is going to be used for S/H also */
	bitoneDualSend(synth, global.controlfd, 0, 10, 0, 1024);
	/* LFO Env parameters */
	bitoneDualSend(synth, global.controlfd, 0, 2, 1, 12000);
	bitoneDualSend(synth, global.controlfd, 0, 2, 2, 16000);
	bitoneDualSend(synth, global.controlfd, 0, 2, 3, 1000);
	bitoneDualSend(synth, global.controlfd, 0, 2, 5, 0);
	bitoneDualSend(synth, global.controlfd, 0, 2, 8, 0);

	bitoneDualSend(synth, global.controlfd, 0, 3, 1, 12000);
	bitoneDualSend(synth, global.controlfd, 0, 3, 2, 16000);
	bitoneDualSend(synth, global.controlfd, 0, 3, 3, 1000);
	bitoneDualSend(synth, global.controlfd, 0, 3, 5, 0);
	bitoneDualSend(synth, global.controlfd, 0, 3, 8, 0);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
bitoneConfigure(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	if (synth == 0)
	{
		printf("problems going operational\n");
		return(-1);
	}

	synth->bank = synth->location / 100;
	synth->location = synth->location % 100;

	if (synth->flags & OPERATIONAL)
		return(0);

	printf("going operational\n");

	synth->flags |= OPERATIONAL;
	synth->keypanel = 1;
	synth->keypanel2 = -1;
	synth->transpose = 36;

	B1display[0] = 76;
	B1display[1] = synth->location;
	B1display[2] = -10;
	synth->mem.param[DISPLAY_DEV + 4] = synth->location / 10;
	synth->mem.param[DISPLAY_DEV + 5] = synth->location % 10;
	prePark[0].mem.param[DISPLAY_DEV + 4] = synth->location / 10;
	prePark[0].mem.param[DISPLAY_DEV + 5] = synth->location % 10;

	bitoneUpdateDisplay(synth, synth->sid, synth->midichannel, 0, 2,
		(B1display[0]) * CONTROLLER_RANGE / 99);
	bitoneUpdateDisplay(synth, synth->sid, synth->midichannel, 0, 4,
		(B1display[1]) * CONTROLLER_RANGE / 99);
	bitoneUpdateDisplay(synth, synth->sid, synth->midichannel, 0, 6,
		(B1display[2]) * CONTROLLER_RANGE / 99);

	event.type = BRIGHTON_FLOAT;

	/*
	 * Ping the split button to reset the voice allocations
	 */
	event.value = 0.0;
	brightonParamChange(synth->win, 0, RADIOSET_3, &event);

	event.value = 1.0;
	brightonParamChange(synth->win, 0, RADIOSET_2 + 1, &event);

	bitoneUpdateDisplay(synth, synth->sid, synth->midichannel, 0, 2,
		(B1display[0]) * CONTROLLER_RANGE / 99);
	bitoneUpdateDisplay(synth, synth->sid, synth->midichannel, 0, 4,
		(B1display[1]) * CONTROLLER_RANGE / 99);
	bitoneUpdateDisplay(synth, synth->sid, synth->midichannel, 0, 6,
		(B1display[2]) * CONTROLLER_RANGE / 99);

	event.type = BRIGHTON_FLOAT;

	/*
	 * Ping the split button to reset the voice allocations
	 */
	event.value = 0.0;
	brightonParamChange(synth->win, 0, RADIOSET_3, &event);

	event.value = 1.0;
	brightonParamChange(synth->win, 0, RADIOSET_2 + 1, &event);

	/*
	 * Set the synth to a known state from which to start
	loadMemory(synth, SYNTH_NAME, 0, 0,
		synth->mem.active, 0, BRISTOL_FORCE|BRISTOL_NOCALLS);
	brightonParamChange(synth->win, 0, 224, &event);
	event.type = 0.0;
	brightonParamChange(synth->win, 0, 224, &event);
	 */
	loadMemory(synth, SYNTH_NAME, 0, synth->bank * 100 + synth->location,
		synth->mem.active, 0, BRISTOL_FORCE|BRISTOL_NOCALLS);
	bitoneMemoryShim(synth, ACTIVE_DEVS - 10);
	loadMemory(synth, SYNTH_NAME, 0, synth->bank * 100 + synth->location,
		synth->mem.active, 0, BRISTOL_FORCE|BRISTOL_NOCALLS);

	/*
	synth->mem.param[109] = 5;
	*/
	bitoneMemoryShim(synth, ACTIVE_DEVS - 10);
	bitoneDAUpdate(synth);

	/*
	 * OK, this is a bit of a hack but it works. If the stereo pot has a
	 * WITHDRAWN flag against it then we have a bit-1, otherwise we have been
	 * given a bit-99
	 */
	if (bit99App.resources[0].devlocn[STEREO_BUTTON].flags & BRIGHTON_WITHDRAWN)
		/* We have a bit-1 */
		brightonPut(win, "bitmaps/blueprints/bitoneshade.xpm",
			0, 0, win->width, win->height);
	else {
		if (strcmp("bit100", synth->resources->name) == 0)
			/* bit99m2/bit100 */
			brightonPut(win, "bitmaps/blueprints/bitoneshade.xpm",
				0, 0, win->width, win->height);
		else
			brightonPut(win, "bitmaps/blueprints/bit99shade.xpm",
				0, 0, win->width, win->height);
	}

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	event.type = BRIGHTON_FLOAT;
	event.value = 0.8;
	brightonParamChange(synth->win, 0, 206, &event);
	brightonParamChange(synth->win, 0, 207, &event);

	event.value = 1.0;
	brightonParamChange(synth->win, 0, 209, &event);
	brightonParamChange(synth->win, 0, 219, &event);

	event.value = 0.51;
	brightonParamChange(synth->win, 0, ENTRY_POT, &event);
	event.value = 0.5;
	brightonParamChange(synth->win, 0, 203, &event);

	dc = brightonGetDCTimer(win->dcTimeout);

	synth->loadMemory = (loadRoutine) bitoneMemoryKey;
	synth->saveMemory = (saveRoutine) bitoneSaveMemory;

	return(0);
}

