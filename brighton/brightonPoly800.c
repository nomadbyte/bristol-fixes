
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
 * We have 64 params available of which 50 are from the original and 12 others
 * are defined to be:
 *
 *	chorus params		47 58 68 78
 *	LFO single multi	85
 *	Osc PW - 1&2		34 36
 *	Osc PWM - 1&2		35 37
 *	Osc Sync - 2		28		
 *	Env velocities		57
 *	Detune globally		38
 *
 * DE 67/77 are currently unused. Add Glide?
 *
 * We have one more from seq clock 88 := config OMNI
 *
 * Poly/Chord/Hold options OK
 *
 * Check Sync OK
 *
 * Bend Implemented but check dynamics OK
 * 
 * Clear up diags output REQ
 *
 * Check all the parameters audibly.
 *
 * Check depth on current detune
 *
 * Check out click on note off events.
 */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int poly800Init();
static int poly800Configure();
static int poly800Callback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);
static int poly800KeyCallback(brightonWindow *, int, int, float);

extern guimain global;
static guimain manual;

#include "brightonKeys.h"

#define SYNTH_NAME synth->resources->name

#define ACTIVE_DEVS 89
#define MEM_MGT ACTIVE_DEVS
#define MIDI_MGT (MEM_MGT + 12)

#define ENTRY_POT 18
#define MODS_COUNT 32

#define DEVICE_COUNT (ACTIVE_DEVS + MODS_COUNT)

#define KEY_PANEL 1
#define MODS_PANEL 2
#define DISPLAY_DEV (MODS_COUNT - 6)

#define MODE_SINGLE	0
#define MODE_SPLIT	1
#define MODE_LAYER	2

static int dc, mbh = 0, crl = 0;

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
 * This example is for a poly800Bristol type synth interface.
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
#define POLY800_CONTINUOUS 0
#define POLY800_STEPPED 1

#define P800_INACTIVE		0x001 /* Skip this parameter */
#define P800_INHERIT_PRI	0x002 /* Copy from the primary layer when loading */
#define P800_INACTIVE_PEER	0x004 /* Not active in upper (second load) layer */
#define P800_USE_PRI		0x008 /* Take value from primary only */
#define P800_ALWAYS			0x010 /* All editing in double/split mode */
#define P800_INHERIT_SEC	0x020 /* Copy to the primary layer when changing */
#define P800_NO_LOAD		0x040 /* All editing in double/split mode */
#define P800_REZERO			0x080 /* Normalise signal to engine back to zero */

typedef struct P800range {
	int type;
	float min;
	float max;
	int op;
	int cont;  /* link to the engine code */
	int flags;
} p800range;

/*
 * STEPPED controllers are sent as that integer value. CONTINUOUS are sent
 * as values from 0 to 1.0 but the range values specify how they are displayed
 * on the LEDs.
 *
 * We only use 64 parameters, numbered 11 through 88 using just 8 digits. Of
 * these 50 are selectable from the 'membrane', the rest from the prog digits.
 *
 * The Mods:
 */
p800range poly800Range[DEVICE_COUNT] = {
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */

	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry 10 */
	{POLY800_STEPPED,	 1, 3, 0, 101, P800_REZERO}, /* Octave shim */
	{POLY800_CONTINUOUS, 1, 2, 0, 102, P800_REZERO}, /* Waveform shim */
	{POLY800_CONTINUOUS, 0, 1, 0, 4, 0}, /* 16 */
	{POLY800_CONTINUOUS, 0, 1, 0, 5, 0}, /* 8 */
	{POLY800_CONTINUOUS, 0, 1, 0, 6, 0}, /* 4 */
	{POLY800_CONTINUOUS, 0, 1, 0, 7, 0}, /* 2 */
	{POLY800_CONTINUOUS, 0, 31, 4, 9, 0}, /* Level = gain of env */
	{POLY800_STEPPED,	 1, 2, 126, 10, P800_REZERO}, /* Double */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */

	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry 20 */
	{POLY800_STEPPED,	 1, 3, 1, 101, P800_REZERO}, /* Octave */
	{POLY800_CONTINUOUS, 1, 2, 1, 102, P800_REZERO}, /* Waveform */
	{POLY800_CONTINUOUS, 0, 1, 1, 4, 0}, /* 16 */
	{POLY800_CONTINUOUS, 0, 1, 1, 5, 0}, /* 8 */
	{POLY800_CONTINUOUS, 0, 1, 1, 6, 0}, /* 4 */
	{POLY800_CONTINUOUS, 0, 1, 1, 7, 0}, /* 2 */
	{POLY800_CONTINUOUS, 0, 31, 5, 9, 0}, /* Level = gain of env */
	{POLY800_STEPPED,	 0, 1, 126, 13, 0}, /* DE Sync */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */

	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry 30 */
	{POLY800_STEPPED,	 0, 12, 2, 102, P800_REZERO}, /* Interval */
	{POLY800_CONTINUOUS, 0, 3, 1, 9, 0}, /* Detune - shim to half a semi */
	{POLY800_CONTINUOUS, 0, 15, 3, 0, 0}, /* Noise level - shim it down? */
	{POLY800_CONTINUOUS, 0, 99, 0, 13, 0}, /* DCO1 PW entry */
	{POLY800_CONTINUOUS, 0, 99, 126, 4, 0}, /* DCO1 PWM entry */
	{POLY800_CONTINUOUS, 0, 99, 1, 13, 0}, /* DCO2 PW entry */
	{POLY800_CONTINUOUS, 0, 99, 126, 5, 0}, /* DCO2 PWM entry */
	{POLY800_CONTINUOUS, 0, 99, 16, 101, 0}, /* Detune entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */

	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry 40 */
	{POLY800_CONTINUOUS, 0, 99, 2, 0, 0}, /* Filter Cutoff */
	{POLY800_CONTINUOUS, 0, 15, 2, 1, 0}, /* Filter res */
	{POLY800_CONTINUOUS, 0, 2, 2, 3, 0}, /* KBD tracking */
	{POLY800_STEPPED,	 1, 2, 126, 20, P800_REZERO}, /* Env polarity */
	{POLY800_CONTINUOUS, 0, 15, 126, 21, 0}, /* Env Amount */
	{POLY800_STEPPED,	 1, 2, 6, 12, P800_REZERO}, /* Env Trigger */
	{POLY800_CONTINUOUS, 0, 99, 100, 3, 0}, /* Chorus P-3 Multi */
	{POLY800_STEPPED, 0, 1, 100, 3, 0}, /* Chorus On/Off shim */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */

	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry 50 */
	{POLY800_CONTINUOUS, 0, 31, 4, 1, 0}, /* Env-1 */
	{POLY800_CONTINUOUS, 0, 31, 4, 3, 0}, /* Env-1 */
	{POLY800_CONTINUOUS, 0, 31, 4, 4, 0}, /* Env-1 */
	{POLY800_CONTINUOUS, 0, 31, 4, 5, 0}, /* Env-1 */
	{POLY800_CONTINUOUS, 0, 31, 4, 6, 0}, /* Env-1 */
	{POLY800_CONTINUOUS, 0, 31, 4, 7, 0}, /* Env-1 */
	{POLY800_STEPPED,	 0, 7, 126, 101, 0}, /* Env-1 touch */
	{POLY800_CONTINUOUS, 0, 99, 100, 0, 0}, /* DE 58 Chorus-0 entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */

	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry 60 */
	{POLY800_CONTINUOUS, 0, 31, 5, 1, 0}, /* Env-2 */
	{POLY800_CONTINUOUS, 0, 31, 5, 3, 0}, /* Env-2 */
	{POLY800_CONTINUOUS, 0, 31, 5, 4, 0}, /* Env-2 */
	{POLY800_CONTINUOUS, 0, 31, 5, 5, 0}, /* Env-2 */
	{POLY800_CONTINUOUS, 0, 31, 5, 6, 0}, /* Env-2 */
	{POLY800_CONTINUOUS, 0, 31, 5, 7, 0}, /* Env-2 */
	{POLY800_CONTINUOUS, 0, 1, 126, 0, 0}, /* Glide */
	{POLY800_CONTINUOUS, 0, 99, 100, 1, 0}, /* DE 58 Chorus-1 entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */

	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry 70 */
	{POLY800_CONTINUOUS, 0, 31, 6, 1, 0}, /* Env-3 */
	{POLY800_CONTINUOUS, 0, 31, 6, 3, 0}, /* Env-3 */
	{POLY800_CONTINUOUS, 0, 31, 6, 4, 0}, /* Env-3 */
	{POLY800_CONTINUOUS, 0, 31, 6, 5, 0}, /* Env-3 */
	{POLY800_CONTINUOUS, 0, 31, 6, 6, 0}, /* Env-3 */
	{POLY800_CONTINUOUS, 0, 31, 6, 7, 0}, /* Env-3 */
	{POLY800_STEPPED,	 0, 1, 6, 10, 0}, /* FREE */
	{POLY800_CONTINUOUS, 0, 99, 100, 2, 0}, /* DE 58 Chorus-2 entry */
	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry */

	{POLY800_STEPPED,	 0, 1, 126, 126, P800_NO_LOAD}, /* Dummy entry 80 */
	{POLY800_CONTINUOUS, 0, 15, 8, 0, 0}, /* LFO Freq */
	{POLY800_CONTINUOUS, 0, 15, 126, 24, 0}, /* LFO Delay */
	{POLY800_CONTINUOUS, 0, 15, 126, 22, 0}, /* DCO */
	{POLY800_CONTINUOUS, 0, 15, 126, 23, 0}, /* VCF */
	{POLY800_STEPPED,	 0, 1, 126, 11, 0}, /* LFO Multi entry */
	{POLY800_STEPPED,	 1, 16, 126, 126, P800_REZERO}, /* Midi Chan Shim */
	{POLY800_STEPPED,	 0, 1, 126, 126, 0}, /* Midi Prog Change */
	{POLY800_STEPPED,	 1, 2, 126, 126, P800_REZERO}, /* OMNI */

};

#define P8H 410
#define P8W 39
#define PR1 13
#define PR2 513

#define C1 3
#define C2 (C1 + P8W + 1)
#define C3 (C2 + P8W + 0)
#define C4 (C3 + P8W + 1)
#define C5 (C4 + P8W + 1)
#define C6 (C5 + P8W + 0)
#define C7 (C6 + P8W + 1)
#define C8 (C7 + P8W + 1)
#define C9 (C8 + P8W + 2)
#define C10 (C9 + P8W + 1)
#define C11 (C10 + P8W + 1)
#define C12 (C11 + P8W + 0)
#define C13 (C12 + P8W + 0)
#define C14 (C13 + P8W + 1)
#define C15 (C14 + P8W + 0)
#define C16 (C15 + P8W - 1)
#define C17 (C16 + P8W + 3)
#define C18 (C17 + P8W + 2)
#define C19 (C18 + P8W + 4)
#define C20 (C19 + P8W + 1)
#define C21 (C20 + P8W - 1)
#define C22 (C21 + P8W + 0)
#define C23 (C22 + P8W + 3)
#define C24 (C23 + P8W + 0)
#define C25 (C24 + P8W + 1)

#define LC1 3
#define LC2 (LC1 + P8W + 1)
#define LC3 (LC2 + P8W + 1)
#define LC4 (LC3 + P8W + 0)
#define LC5 (LC4 + P8W + 1)
#define LC6 (LC5 + P8W + 0)
#define LC7 (LC6 + P8W + 3)
#define LC8 (LC7 + P8W + 1)
#define LC9 (LC8 + P8W + 1)
#define LC10 (LC9 + P8W + 0)
#define LC11 (LC10 + P8W + 1)
#define LC12 (LC11 + P8W + 0)
#define LC13 (LC12 + P8W + 3)
#define LC14 (LC13 + P8W + 1)
#define LC15 (LC14 + P8W + 0)
#define LC16 (LC15 + P8W + 1)
#define LC17 (LC16 + P8W + 0)
#define LC18 (LC17 + P8W + 1)
#define LC19 (LC18 + P8W + 3)
#define LC20 (LC19 + P8W + 1)
#define LC21 (LC20 + P8W + 0)
#define LC22 (LC21 + P8W + 1)
#define LC23 (LC22 + P8W + 2)
#define LC24 (LC23 + P8W + 0)
#define LC25 (LC24 + P8W + 0)

static
brightonLocations locations[DEVICE_COUNT] = {
	/*
	 * The first are dummies, count starts at 11
	 */
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

	{"DCO1 Octave", 9, C1, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO1 Wave", 9, C2, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO1 16'", 9, C3, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO1 8'", 9, C4, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO1 4'", 9, C5, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO1 2'", 9, C6, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO1 Level", 9, C7, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	/* Noise 18 */
	{"DCO Mode", 9, C8, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},

	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/* DCO 21 */
	{"DCO2 Octave", 9, C9, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO2 Wave", 9, C10, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO2 16'", 9, C11, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO2 8'", 9, C12, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO2 4'", 9, C13, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO2 2'", 9, C14, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO2 Level", 9, C15, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* DCO 2 31 */
	{"DCO2 Interval", 9, C16, PR1, P8W + 2, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DCO2 Detune", 9, C17, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	/* Noise 33 */	
	{"Noise", 9, C18, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* VCF 41 */
	{"VCF Cutoff", 9, C19, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"VCF Resonance", 9, C20, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"VCF KBD", 9, C21, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"VCF EnvPolarity", 9, C22, PR1, P8W + 2, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"VCF EngTrack", 9, C23, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"VCF Env Trigger", 9, C24, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"On/Off", 9, C25, PR1, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* DEG 1 51 */
	{"DEG1 Attack", 9, LC1, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG1 Decay", 9, LC2, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG1 Break", 9, LC3, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG1 Slope", 9, LC4, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG1 Sustain", 9, LC5, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG1 Release", 9, LC6, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* DEG 2 61 */
	{"DEG2 Attack", 9, LC7, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG2 Decay", 9, LC8, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG2 Break", 9, LC9, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG2 Slope", 9, LC10, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG2 Sustain", 9, LC11, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG2 Release", 9, LC12, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* DEG 3 71 */
	{"DEG3 Attack", 9, LC13, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG3 Decay", 9, LC14, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG3 Break", 9, LC15, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG3 Slope", 9, LC16, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG3 Sustain", 9, LC17, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"DEG3 Release", 9, LC18, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	/* Mod 81 */
	{"LFO Freq", 9, LC19, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"LFO Delay", 9, LC20, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"LFO DCO", 9, LC21, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"LFO VCF", 9, LC22, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"", 2, 0, 0, 10, 10, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"MidiChannel", 9, LC23, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"ProgEnable", 9, LC24, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	{"OmniOnOff", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_NOSHADOW|BRIGHTON_CHECKBUTTON|BRIGHTON_TRACKING},
	/* Done */

	{"", 9, 5, 5, 5, 5, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},
	{"", 9, LC25, PR2, P8W, P8H, 0, 1, 0, 
		"bitmaps/buttons/blue.xpm", "bitmaps/buttons/green.xpm",
		BRIGHTON_WITHDRAWN},

};

#define PMW 40
#define PMH 130

static
brightonLocations p800mods[MODS_COUNT] = {
	/* Bank */
	{"", 2, 647, 820, 100, 33, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	/* Digits 1 */
	{"", 2, 600, 700, 50, 40, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 670, 700, 50, 40, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 740, 700, 50, 40, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 600, 590, 50, 40, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 670, 590, 50, 40, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 740, 590, 50, 40, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 600, 460, 50, 40, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 670, 460, 50, 40, 0, 1, 0, "bitmaps/buttons/polyblue.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	/* Prog button 9 */
	{"", 2, 740, 460, 50, 40, 0, 1, 0, "bitmaps/buttons/polywhiteH.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},

	{"MasterVolume", 0, 210, 645, 130, 180, 0, 1, 0, "bitmaps/knobs/line.xpm", 0,
		BRIGHTON_REDRAW|BRIGHTON_NOSHADOW},
	{"", 1, 190, 500, 150, 35, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm", 0, 
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOTCH|BRIGHTON_NOSHADOW},
	{"", 1, 362, 540, 15, 300, 0, 11, 0, "bitmaps/buttons/polywhiteV.xpm", 0, 
		BRIGHTON_HALFSHADOW},

	{"", 2, 430, 530, 25, 200, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 490, 530, 25, 200, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 550, 530, 25, 200, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},

// 16
	{"", 2, 835, 530, 25, 200, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 885, 530, 25, 200, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		"bitmaps/buttons/polyredV.xpm", BRIGHTON_CHECKBUTTON},

	/* Entry pot 18 */
	{"DataEntry", 0, 835, 750, 100, 140, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 0},

	/* Midi is now in the memory */
	{"", 2, 0, 0, 25, 140, 0, 1, 0, "bitmaps/buttons/polywhiteV.xpm",
		"bitmaps/buttons/polyredV.xpm",BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 600, 340, 18, 35, 0, 1, 0, "bitmaps/textures/p8b.xpm",
		"bitmaps/images/led.xpm", BRIGHTON_NOSHADOW},

	{"", 2, 940, 530, 25, 140, 0, 1, 0, "bitmaps/buttons/polyredV.xpm",
		"bitmaps/buttons/polyblue.xpm", BRIGHTON_CHECKBUTTON},

	/* Joystick 22 - we need to keep the dummy since it dispatches X/Y events */
	{"", 5, 3, 620, 150, 240, 0, 1, 0, "bitmaps/images/sphere.xpm", 
		0, BRIGHTON_WIDE},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW}, /* DUMMY */

	/* Two LED for denoting prog or param - 24*/
	{"", 2, 693, 340, 18, 35, 0, 1, 0, "bitmaps/textures/p8b.xpm",
		"bitmaps/images/led.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 843, 340, 18, 35, 0, 1, 0, "bitmaps/textures/p8b.xpm",
		"bitmaps/images/led.xpm", BRIGHTON_NOSHADOW},

	{"", 8, 610, 200, PMW, PMH, 0, 9, 0, 0, 0, 0},
	{"", 8, 610 + PMW + 5, 200, PMW, PMH, 0, 9, 0, 0, 0, 0},

	{"", 8, 750, 200, PMW, PMH, 0, 9, 0, 0, 0, 0},
	{"", 8, 750 + PMW + 5, 200, PMW, PMH, 0, 9, 0, 0, 0, 0},

	{"", 8, 890, 200, PMW, PMH, 0, 9, 0, 0, 0, 0},
	{"", 8, 890 + PMW + 5, 200, PMW, PMH, 0, 9, 0, 0, 0, 0},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 * Hm, the bit-1 was black, and the bit 99 was in various builds including a 
 * white one, but also a black one and a black one with wood panels. I would
 * like the black one with wood panels, so that will have to be the bit-1, the
 * bit-99 will be white with thin metal panels.
 */
brightonApp poly800App = {
	"poly800",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/p8b.xpm",
	BRIGHTON_STRETCH,
	poly800Init,
	poly800Configure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{8, 100, 2, 2, 5, 520, 0, 0},
	1016, 370, 0, 0,
	3, /* one panel only */
	{
		{
			"Poly800",
			"bitmaps/blueprints/poly800.xpm",
			"bitmaps/textures/p8b.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			poly800Callback,
			317, 40, 674, 444,
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
			poly800KeyCallback,
			32, 550, 952, 430,
			KEY_COUNT_4OCTAVE,
			keys4octave2
//			KEY_COUNT,
//			keysprofile2
		},
		{
			"Poly800Mods",
			"bitmaps/blueprints/poly800mods.xpm",
			"bitmaps/textures/p8b.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			poly800Callback,
			7, 20, 300, 444,
			MODS_COUNT,
			p800mods
		},
	}
};

/*
 * The next two should come out of here and go into a separate arpeggiator file
 * or failing that into brightonRoutines.c
 *
 * This has been commented out until it is integrated into the p800 emulator
 *
static int
poly800seqInsert(arpeggiatorMemory *seq, int note, int transpose)
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

static void
poly800SeqRelearn(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (v != 0) {
		bristolMidiSendMsg(fd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
		bristolMidiSendMsg(fd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 1);
		synth->seq1.param[BRISTOL_AM_SMAX] = 0;

		return;
	}

	bristolMidiSendMsg(fd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
}
 */

static void
poly800ChordRelearn(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (synth->seq1.param == NULL)
		loadSequence(&synth->seq1, SYNTH_NAME, 0, 0);
	if (synth->seq2.param == NULL)
		loadSequence(&synth->seq2, SYNTH_NAME, 0, BRISTOL_SID2);

// We need to turn off chording to do this and reset one index
	if (v != 0) {
		bristolMidiSendMsg(fd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 0);
		bristolMidiSendMsg(fd, synth->sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 1);
		synth->seq1.param[BRISTOL_AM_CCOUNT] = 0;

		return;
	}

	bristolMidiSendMsg(fd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_RESEQ, 0);
}

static int
poly800ChordInsert(arpeggiatorMemory *seq, int note, int transpose)
{
	//printf("chord %i + %i at %i\n", note, transpose, (int) seq->c_count);
	if (seq->c_count == 0)
		seq->c_dif = note + transpose;

	seq->chord[(int) (seq->c_count)] = (float) (note + transpose - seq->c_dif);

	if ((seq->c_count += 1) >= BRISTOL_CHORD_MAX) {
		seq->c_count = BRISTOL_CHORD_MAX;
		crl = 0;
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
poly800KeyCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	/*
	if ((synth->mem.param[155] != 0) && (value != 0))
	{
		seqInsert((arpeggiatorMemory *) synth->seq1.param,
			(int) index, synth->transpose); 
	}
	*/

	if (crl)
	{
		poly800ChordInsert((arpeggiatorMemory *) synth->seq1.param,
			(int) index, synth->transpose); 
	}

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
loadMemoryShim(guiSynth *synth, int from)
{
	int i, current;
	brightonEvent event;

	current = synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 2] * 10
		+ synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3];

	loadMemory(synth, SYNTH_NAME, 0, from,
		synth->mem.active, 0, BRISTOL_FORCE|BRISTOL_NOCALLS);

	synth->flags |= MEM_LOADING;

	event.type = BRIGHTON_FLOAT;

	for (i = 0; i < synth->mem.active; i++)
	{
		if (poly800Range[i].flags & P800_NO_LOAD)
			continue;

		synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 2] = i / 10;
		synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3] = i % 10;

		event.value = synth->mem.param[i];

		brightonParamChange(synth->win, MODS_PANEL, ENTRY_POT, &event);
	}
	synth->flags &= ~MEM_LOADING;

	synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 2] = current / 10;
	synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3] = current % 10;

	event.value = synth->mem.param[current];
	brightonParamChange(synth->win, MODS_PANEL, ENTRY_POT, &event);
}

int
p800loadMemory(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	loadMemoryShim(synth, location);
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
			loadMemoryShim(synth, synth->location);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
poly800UpdateDisplayDec(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int ho, lo, dev = DISPLAY_DEV + o;
	brightonEvent event;

	/* printf("Dec %i %i %i\n", c, o, v); */

	if (v < 0)
	{
		ho = lo = -10;
	} else {
		lo = v % 10;
		ho = v / 10;
	}

	event.type = BRIGHTON_FLOAT;
	event.value = (float) ho;

	brightonParamChange(synth->win, MODS_PANEL, dev, &event);

	event.value = (float) lo;
	brightonParamChange(synth->win, MODS_PANEL, dev + 1, &event);

	return(0);
}

static int
poly800UpdateDisplay(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int ho, lo, dev = DISPLAY_DEV + o;
	brightonEvent event;

	/* printf("Disp %i %i %i\n", c, o, v); */

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

	brightonParamChange(synth->win, MODS_PANEL, dev, &event);

	event.value = (float) lo;
	brightonParamChange(synth->win, MODS_PANEL, dev + 1, &event);

	synth->mem.param[ACTIVE_DEVS + dev] = ho;
	synth->mem.param[ACTIVE_DEVS + dev + 1] = lo;

	return(0);
}

static int
poly800MidiNull(void *synth, int fd, int chan, int c, int o, int v)
{
	if (global.libtest)
		printf("This is a null callback on panel 0 id 0\n");

	return(0);
}

static void
poly800MidiShim(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid, c, o, v);
}

static int
poly800MidiSendMsg(guiSynth *synth, int fd, int chan, int o, int c, int v)
{
	c = poly800Range[o].cont;
	o = poly800Range[o].op;

	if (global.libtest)
		return(0);

	bristolMidiSendMsg(fd, synth->sid, o, c, v);

	return(0);
}

static int
poly800EntryPot(guiSynth *synth, int fd, int chan, int o, int c, float v)
{
	int index;
	float decimal;
	int value;

//printf("EP %f %f: %f\n", synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 2], synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3], v);

	/*
	 * Get the param index from the second display
	 */
	if ((index = synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 2] * 10
		+ synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3]) < 11)
		return(0);

	if (poly800Range[index].flags & P800_INACTIVE) {
		v = 0;
		decimal = 0;
	}

	synth->mem.param[index] = v;

	/*
	 * We have a value for the pot position and that has to be worked into 
	 * the entry range.
	 *
	 * This was originally wrong:

	decimal = poly800Range[index].min +
		(v + 0.00001) * (poly800Range[index].max - poly800Range[index].min);

	 * This only changes at the limits, rather than halfway through a range
	 * which appears better - it truncates whilst it should round...
	 */
	decimal = (float) ((int) (0.5 + poly800Range[index].min +
		(v + 0.00001) * (poly800Range[index].max - poly800Range[index].min)));

	/*
	if ((synth->flags & MEM_LOADING) != 0)
	{
		synth->mem.param[index] = v;
		poly800UpdateDisplayDec(synth, fd, chan, c, 4, decimal);
		return(0);
	}
	*/

	if (poly800Range[index].type == POLY800_STEPPED) {
		value = (int) decimal;
		if (poly800Range[index].flags & P800_REZERO)
			value -= poly800Range[index].min;
	} else
		value = (int) (v * C_RANGE_MIN_1);

//printf("EP %i %f = send %i\n", index, decimal, value);

	if ((synth->flags & MEM_LOADING) == 0)
		poly800UpdateDisplayDec(synth, fd, chan, c, 4, decimal);

	if (synth->dispatch[index].routine != NULL)
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			poly800Range[index].op,
			poly800Range[index].cont,
			value);
	else
		poly800MidiSendMsg(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			value);

	return(0);
}

static int
poly800Select(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	poly800UpdateDisplay(synth, fd, chan, c, 2,
		(c + 1) * CONTROLLER_RANGE / 100);

	event.type = BRIGHTON_FLOAT;
	event.value = synth->mem.param[c];
	brightonParamChange(synth->win, MODS_PANEL, ENTRY_POT, &event);

	/* Select Param entry */
	event.value = 1;
	brightonParamChange(synth->win, MODS_PANEL, 25, &event);
	event.value = 0;
	brightonParamChange(synth->win, MODS_PANEL, 24, &event);

	return(0);
}

static int memSave = -1;
static int bankHold = 0;

static void
poly800Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

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
		/*
		 * This is not a doubeClick: Select write, then select two digits for
		 * the location
		if (brightonDoubleClick(dc) != 0)
			poly800SaveMemory(synth);
		 */

		if (memSave < 0)
			return;

		if ((memSave = 1 - memSave) != 0)
		{
			poly800UpdateDisplayDec(synth, fd, chan, c, 0, -1);
		} else {
			/*
			 * Put back original memory
			 */
			if (synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV] <= 0)
				poly800UpdateDisplayDec(synth, fd, chan, c, 0,
					synth->bank * 10 + synth->location);
		}

		return;
	}

	if (c == 1) {
		/*
		 * Enter digit:
		 *	If memSave selected, display digits, when two save mem.
		 *
		 * 	If Bank hold is selected then single digit will load the memory.
		 *
		 * 	Otherwise two digits are required
		 */
		if (memSave) {
			if (synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV] <= 0)
			{
				event.type = BRIGHTON_FLOAT;
				event.value = o;
				brightonParamChange(synth->win, MODS_PANEL,
					DISPLAY_DEV, &event);
			} else {
				event.type = BRIGHTON_FLOAT;
				event.value = o;
				brightonParamChange(synth->win, MODS_PANEL,
					DISPLAY_DEV + 1, &event);

				synth->bank = synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV];
				synth->location = o;

				saveMemory(synth, SYNTH_NAME, 0, mbh + synth->bank * 10 + o, 0);

				memSave = 0;
			}
			return;
		}

		if (bankHold)
		{
			/* Load memory */
			synth->location = o;

			event.type = BRIGHTON_FLOAT;
			event.value = o;
			brightonParamChange(synth->win, MODS_PANEL,
				DISPLAY_DEV + 1, &event);

			loadMemoryShim(synth, mbh + synth->bank * 10 + synth->location);

			/* Even though we have now set all the parameters we need to return
			 * our Prog status */
			event.type = BRIGHTON_FLOAT;
			event.value = 1;
			brightonParamChange(synth->win, MODS_PANEL, 24, &event);
			event.value = 0;
			brightonParamChange(synth->win, MODS_PANEL, 25, &event);

			memSave = 0;

			return;
		}

		if (synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 1] > 0)
		{
			event.type = BRIGHTON_FLOAT;
			event.value = o;
			brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV, &event);

			event.type = BRIGHTON_FLOAT;
			event.value = -1;
			brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV+1, &event);
			synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 1] = -1;
		} else {
			event.type = BRIGHTON_FLOAT;
			event.value = o;
			brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV+1, &event);

			synth->bank = synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV];
			synth->location = o;

			loadMemoryShim(synth, mbh + synth->bank * 10 + synth->location);

			/* Even though we have now set all the parameters we need to return
			 * our Prog status */
			event.type = BRIGHTON_FLOAT;
			event.value = 1;
			brightonParamChange(synth->win, MODS_PANEL, 24, &event);
			event.value = 0;
			brightonParamChange(synth->win, MODS_PANEL, 25, &event);

			memSave = 0;
		}
	}
}

static void
poly800ParamSelect(guiSynth *synth, int v)
{
	brightonEvent event;

	if (synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3] > 0)
	{
		/* First digit */
		event.type = BRIGHTON_FLOAT;
		event.value = v;
		brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV + 2, &event);

		event.type = BRIGHTON_FLOAT;
		event.value = -1;
		brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV + 3, &event);
		synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3] = -1;
	} else {
		/* Second digit */
		event.type = BRIGHTON_FLOAT;
		event.value = v;
		brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV + 3, &event);
		poly800Select(synth, synth->sid, synth->midichannel, 
			synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 2] * 10
				+ synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3], 0, 0);
	}
}

static int
poly800ModCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int off = 24, on = 25;
	brightonEvent event;

	if (global.libtest)
		printf("poly800ModCallback(%i, %f)\n", index, value);

	synth->mem.param[ACTIVE_DEVS + index] = value;

	switch (index) {
		case 0:
			memSave = 1;
			poly800Memory(synth, synth->sid, synth->midichannel, 0, 0, 0);
			/* Bank hold button */
			bankHold = 1 - bankHold;

			event.type = BRIGHTON_FLOAT;
			event.value = bankHold;
			brightonParamChange(win, MODS_PANEL, 20, &event);

			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			/* Digits, dispatch according to prog button */
			if (synth->mem.param[ACTIVE_DEVS + 24] != 0)
			{
				/* Program selectors */
				poly800Memory(synth, synth->sid, synth->midichannel,
					1, index, 0);
			} else {
				/* Param selectors */
				poly800ParamSelect(synth, index);
			}
			break;
		case 21:
			/* Write */
			if (synth->mem.param[ACTIVE_DEVS + 24] == 0)
			{
				event.type = BRIGHTON_FLOAT;
				event.value = 1;
				brightonParamChange(win, MODS_PANEL, 24, &event);
				event.value = 0;
				brightonParamChange(win, MODS_PANEL, 25, &event);
			}

			poly800Memory(synth, synth->sid, synth->midichannel, 0, index, 0);
			break;
		case 9:
			/* Prog button */
			memSave = 1;
			poly800Memory(synth, synth->sid, synth->midichannel, 0, 0, 0);

			if (synth->mem.param[ACTIVE_DEVS + 24] == 0) {
				on = 24; off = 25;
			}

			event.type = BRIGHTON_FLOAT;
			event.value = 1;
			brightonParamChange(win, MODS_PANEL, on, &event);
			event.value = 0;
			brightonParamChange(win, MODS_PANEL, off, &event);

			break;
		case 10:
			/* Volume */
			bristolMidiSendMsg(global.controlfd, synth->midichannel,
				126, 2, (int) (value * C_RANGE_MIN_1));
			break;
		case 11:
			/* Tune */
			bristolMidiSendMsg(global.controlfd, synth->midichannel,
				126, 1, (int) (value * C_RANGE_MIN_1));
			break;
		case 12:
			/* Bend depth - make this glide */
			bristolMidiSendRP(global.controlfd, synth->sid,
				MIDI_RP_PW, (int) (value) + 1);
			bristolMidiSendMsg(global.controlfd, synth->midichannel,
				126, 6, (int) (value * C_RANGE_MIN_1 / 12));
			break;
		case 13:
			/* Poly mode - cancel chord/hold */
			bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
				BRISTOL_HOLD|0);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 0);
			crl = 0;
			break;
		case 14:
			/*
			 * Clear the sustain setting and start chord learning
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
				BRISTOL_HOLD|0);

			/*
			 * Chord. To configure it you pressed hold, then the notes in the
			 * chord, then pressed chord. We can't really use this so will try
			 * to get the logic to go Hold->Chord->notes->Chord/Hold
			 */
			if (brightonDoubleClick(dc))
			{
				printf("chord hold\n");
				poly800ChordRelearn(synth, synth->sid, synth->midichannel,
					0, 0, 1);
				crl = 1;
				break;
			}

			crl = 0;
			/*
			 * Otherwise it is to enable chording.
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 1);
			break;
		case 15:
			/* Hold mode - sustain */
			brightonDoubleClick(dc);
			bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
				BRISTOL_HOLD|1);
			bristolMidiSendMsg(global.controlfd, synth->sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 0);
			break;
		case ENTRY_POT:
			memSave = 1;
			poly800Memory(synth, synth->sid, synth->midichannel, 0, 0, 0);

			if (synth->mem.param[ACTIVE_DEVS + 24] != 0)
			{
				event.type = BRIGHTON_FLOAT;
				event.value = 0;
				brightonParamChange(win, MODS_PANEL, 24, &event);
				event.value = 1;
				brightonParamChange(win, MODS_PANEL, 25, &event);
			}

			poly800EntryPot(synth, synth->sid, synth->midichannel, 0, 0, value);
			break;
		case 17:
			/*
			 * Inc
			 * Get the current display value and move it up towards the limit.
			 * Then convert this into an int and send it to the entry pot.
			 */ 
			event.type = BRIGHTON_FLOAT;
			index = synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 2] * 10
				+ synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3];
			value = synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 4] * 10
				+ synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 5] + 1;

			if (value > poly800Range[index].max)
				value = poly800Range[index].max;

			event.value = (value - poly800Range[index].min)
				/ (poly800Range[index].max - poly800Range[index].min); 

//printf("%i: min %f max %f now %f/%f\n", index,
//poly800Range[index].min, poly800Range[index].max, value, event.value);
			brightonParamChange(win, MODS_PANEL, ENTRY_POT, &event);
			break;
		case 16:
			/* Dec */
			event.type = BRIGHTON_FLOAT;
			index = synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 2] * 10
				+ synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 3];
			value = synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 4] * 10
				+ synth->mem.param[ACTIVE_DEVS + DISPLAY_DEV + 5] - 1;

			if (value < poly800Range[index].min)
				value = poly800Range[index].min;

			event.value = (value - poly800Range[index].min)
				/ ((float) poly800Range[index].max - poly800Range[index].min); 

//printf("%i: min %f max %f now %f/%f\n", index,
//poly800Range[index].min, poly800Range[index].max, value, event.value);
			brightonParamChange(win, MODS_PANEL, ENTRY_POT, &event);
			break;
		case 22:
			/* Mod */
			if (!global.libtest)
				bristolMidiSendControlMsg(global.controlfd,
					synth->midichannel,
					1,
					((int) (value * C_RANGE_MIN_1)) >> 7);
			break;
		case 23:
			/* Joystick X motion - pitchbend */
			if (!global.libtest)
				bristolMidiSendMsg(global.controlfd, synth->midichannel,
					BRISTOL_EVENT_PITCH, 0, (int) (value * C_RANGE_MIN_1));
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
poly800Callback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (global.libtest)
		printf("poly800Callback(%i, %f)\n", index, value);

	if (synth == 0)
		return(0);

	if (panel == MODS_PANEL)
		return(poly800ModCallback(win, panel, index, value));

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (poly800App.resources[0].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	if (index < 100)
		poly800Select(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

	return(0);
}

static void
p800Detune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int value = v;

	if (!global.libtest) {
		bristolMidiSendNRP(global.controlfd, synth->sid,
			BRISTOL_NRP_DETUNE, value / 50);
	}

	bristolMidiSendMsg(fd, synth->sid, 0, 3, value);
	bristolMidiSendMsg(fd, synth->sid, 1, 3, value);
}

static int pmc = -1;

static void
p800midichannel(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (global.libtest)
		return;

	if (synth->mem.param[88] != 0)
		v = BRISTOL_CHAN_OMNI;
	else
		v = synth->mem.param[86] * 15;

	if (v == pmc)
		return;

	bristolMidiSendMsg(global.controlfd, synth->sid,
		127, 0, BRISTOL_MIDICHANNEL|v);

	pmc = v;
}

static void
p800lfo(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * We have to configure the engine to generate one or more LFO but more
	 * subtly we also have to tell the LFO not to synchronise to key_on.
	 */
	bristolMidiSendMsg(fd, synth->sid, 8, 1, v);
	bristolMidiSendMsg(fd, synth->sid, 126, 11, v);
}

static void
p800filterEnv(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid, 126, 12, v);
	/* May have to change envelope triggering as well? */
	bristolMidiSendMsg(fd, synth->sid, 6, 12, v);
}

static void
p800chorus(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (synth->mem.param[48] <= 0.5)
		bristolMidiSendMsg(fd, synth->sid, 100, 3, 0);
	else
		bristolMidiSendMsg(fd, synth->sid, 100, 3, 
			(int) (synth->mem.param[47] * C_RANGE_MIN_1));
}

static void
p800velocity(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int x = 0, y = 0, z = 0;

	if (v & 0x01) x = 1;
	if (v & 0x02) y = 1;
	if (v & 0x04) z = 1;

	bristolMidiSendMsg(fd, synth->sid, 4, 10, x);
	bristolMidiSendMsg(fd, synth->sid, 5, 10, y);
	bristolMidiSendMsg(fd, synth->sid, 6, 10, z);
}

static void
p800detune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
//printf("detune %i %i %i\n", c, o, v);
	bristolMidiSendMsg(fd, synth->sid, 1, 0, 8192 + v / 2);
}

static void
p800waveform(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
//printf("waveform %i %i %i\n", c, o, v);
	bristolMidiSendMsg(fd, synth->sid, c, 9, (int) (C_RANGE_MIN_1 - v));
	bristolMidiSendMsg(fd, synth->sid, c, 10, v);
}

static void
p800octave(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * We are delivered 0, 1 or 2, need to multiply by 12 notes
	 */
	if (c == 0)
		bristolMidiSendMsg(fd, synth->sid, 0, 2, v * 12);
	else
		bristolMidiSendMsg(fd, synth->sid, 1, 2,
			((int) (synth->mem.param[21] * 2)) * 12 +
			(int) (synth->mem.param[31] * 12));
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
poly800Init(brightonWindow *win)
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

	printf("Initialise the poly800 link to bristol: %p\n", synth->win);

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
		bcopy(&global, &manual, sizeof(guimain));
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

		manual.flags |= BRISTOL_CONN_FORCE|BRIGHTON_NOENGINE;
		manual.port = global.port;
	}

	for (i = 0; i < ACTIVE_DEVS; i++)
	{
		synth->dispatch[i].routine = NULL;
		synth->dispatch[i].controller = i;
		synth->dispatch[i].operator = i;
	}

	synth->dispatch[11].routine = (synthRoutine) p800octave;
	synth->dispatch[12].routine = (synthRoutine) p800waveform;

	synth->dispatch[21].routine = (synthRoutine) p800octave;
	synth->dispatch[22].routine = (synthRoutine) p800waveform;
	synth->dispatch[31].routine = (synthRoutine) p800octave;

	synth->dispatch[32].routine = (synthRoutine) p800detune; /* DCO-2 */
	synth->dispatch[38].routine = (synthRoutine) p800Detune; /* Global */

	synth->dispatch[46].routine = (synthRoutine) p800filterEnv; /* Global */

	synth->dispatch[47].routine = (synthRoutine) p800chorus; /* Global */
	synth->dispatch[48].routine = (synthRoutine) p800chorus; /* Global */

	synth->dispatch[57].routine = (synthRoutine) p800velocity;

	synth->dispatch[85].routine = (synthRoutine) p800lfo;
	synth->dispatch[86].routine = (synthRoutine) p800midichannel;
	synth->dispatch[88].routine = (synthRoutine) p800midichannel;

	for (; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = (synthRoutine) poly800MidiShim;

	synth->dispatch[0].routine = (synthRoutine) poly800MidiNull;

	/* Osc-1 settings */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 1, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 8, 0); /* No 32' */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 11, 0); /* No tri */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 12, 0); /* No sin */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 15, 0); /* P800 type */

	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 8, 0); /* No 32' */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 11, 0); /* No tri */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 12, 0); /* No sin */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 15, 0); /* P800 type */

	/* Env settings */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 0, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 8, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 11, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 12, 1);

	bristolMidiSendMsg(global.controlfd, synth->sid, 5, 0, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 5, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 5, 8, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 5, 11, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 5, 12, 1);

	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 8, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 9, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 11, 1);
	/* Filter */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 4, 4);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 5, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 6, 0);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
poly800Configure(brightonWindow *win)
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
	} else {
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
		"bitmaps/blueprints/poly800shade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	/* Setup the displays */
	event.value = 1;
	brightonParamChange(synth->win, MODS_PANEL, 24, &event);
	event.value = synth->bank;
	brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV, &event);
	event.value = synth->location;
	brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV + 1, &event);
	event.value = 1;
	brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV + 2, &event);
	brightonParamChange(synth->win, MODS_PANEL, DISPLAY_DEV + 3, &event);

	/* Volume, tuning, bend */
	event.value = 0.822646;
	brightonParamChange(synth->win, MODS_PANEL, 10, &event);
	event.value = 0.5;
	brightonParamChange(synth->win, MODS_PANEL, 11, &event);
	event.value = 0.05;
	brightonParamChange(synth->win, MODS_PANEL, 12, &event);

	loadMemoryShim(synth, mbh + synth->bank * 10 + synth->location);

	/*
	 * Go into Prog rather than Param mode
	 */
	event.value = 1;
	brightonParamChange(synth->win, MODS_PANEL, 24, &event);
	event.value = 0;
	brightonParamChange(synth->win, MODS_PANEL, 25, &event);

	bristolMidiSendRP(global.controlfd, synth->sid, MIDI_RP_PW, 8192);
	bristolMidiSendMsg(global.controlfd, synth->midichannel,
		BRISTOL_EVENT_PITCH, 0, 8192);

	event.value = 2.0;
	brightonParamChange(synth->win, MODS_PANEL, 12, &event);

	dc = brightonGetDCTimer(2000000);

	memSave = 0;

	synth->loadMemory = (loadRoutine) p800loadMemory;

	return(0);
}

