
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

#include "bristolarpeggiation.h"

static int jupiterInit();
static int jupiterConfigure();
static int jupiterCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);
static void jupiterSetSplit(guiSynth *);
static void jupiterSetVoices(guiSynth *);

extern guimain global;
static guimain manual;

static float *pcache;
static float *scache;

#define MODE_SINGLE	0
#define MODE_SPLIT	1
#define MODE_LAYER	2

static int debuglevel = 0;
static int setsplit = 0;
static int mode = MODE_SINGLE;
static int seqLearn = 0, chordLearn = 0;

static void jupiterSetSplit(guiSynth *);
static int jupiterKeyCallback(brightonWindow *, int, int, float);

#include "brightonKeys.h"

/*
 * These are a pair of double click timers and mw is a memory weighting to
 * scale the 64 available memories into banks of banks
 */
static int dc1, mw = 0;

#define DEVICE_COUNT 134
#define LAYER_DEVS 65 /* 65 voice devices */
#define REAL_DEVS 92 /* 65 layer + 14 mod + 9 dummy + 4 reserved */
#define ACTIVE_DEVS (REAL_DEVS + 9) /* 19 arpeg/mode buttons */
#define MEM_MGT (ACTIVE_DEVS + 5)
#define MIDI_MGT (MEM_MGT + 18)
#define FUNCTION (MIDI_MGT + 3)
#define KEY_HOLD 7
#define CHORUS 32

#define PATCH_BUTTONS MEM_MGT
#define BANK_BUTTONS (MEM_MGT + 9)
#define LOAD_BUTTON (MEM_MGT + 8)

#define KEY_PANEL 1
#define MODS_PANEL 2

/* For dual load options */
#define MY_MEM (REAL_DEVS - 1)
#define MEM_LINK (REAL_DEVS - 2)
#define SPLIT_POINT (REAL_DEVS - 3)
#define FUNCTION_SAVE (REAL_DEVS - 4)

#define MOD_ENABLE 94

#define ARPEG_RANGE 53
#define ARPEG_MODE 57
#define POLY_MODE 61
#define SEQ_MODE (DEVICE_COUNT - 6)

#define HOLD_LOWER (REAL_DEVS)
#define MOD_LOWER (REAL_DEVS + 2)
#define MOD_UPPER (REAL_DEVS + 3)
#define MODE_DUAL (REAL_DEVS + 4)
#define KEY_MODE MODE_DUAL

#define PANEL_SELECT (ACTIVE_DEVS + 4)

#define DISPLAY_DEV (DEVICE_COUNT - 5)

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
 * This example is for a jupiterBristol type synth interface.
 */
#define D3 80
#define BR1 300
#define BR2 BR1 + D3
#define BR3 BR2 + D3
#define BR4 BR3 + D3

#define MBR2 BR4
#define MBR1 MBR2 - 100
#define MBR3 MBR2 + 100
#define MBR4 MBR3 + 100

#define R2 390
#define R1 (R2 + 130)
#define R3 (R2 + 100)
#define R4 650

#define JR1 90
#define JR2 JR1 - 20
#define JR3 JR1 - 30

#define RB1 430
#define RB2 (RB1 + JR3 + 2)
#define RB3 (RB2 + JR3 + 2)
#define RB4 (RB3 + JR3 + 2)
#define RB5 (RB4 + JR3 + 2)

#define D1 (980 / 41)
#define D2 (D1 + 11)

#define C1 (13)
#define C2 (C1 + D2)
#define C3 (C2 + D2)
#define C4 (C3 + D1)
#define C5 (C4 + D1)
#define C6 (C5 + D1 - 6)
#define C7 (C6 + D2 - 3)
#define C8 (C7 + D2 - 1)
#define C9 (C8 + D1 - 6)
#define C10 (C9 + D1 - 7)
#define C10a (C9 + D1 + 22)
#define C11 (C10 + D1 + 22)
#define C12 (C11 + D1)
#define C13 (C12 + D1 - 5)
#define C13a (C12 + D1 + 14)
#define C14 (C13 + D1 + 9)
#define C15 (C14 + D2 - 9)
#define C16 (C15 + D2 + 2)
#define C17 (C16 + D1 + 0)
#define C18 (C17 + D1 - 6)
#define C19 (C18 + D2 - 3)
#define C20 (C19 + D2 - 2)
#define C21 (C20 + D2)
#define C22 (C21 + D2)
#define C23 (C22 + D1)
#define C24 (C23 + D1 - 6)
#define C25 (C24 + D1 - 8)
#define C26 (C25 + D1 + 2)
#define C27 (C26 + D1 - 2)
#define C28 (C27 + D1 - 4)
#define C29 (C28 + D1 - 6)
#define C30 (C29 + D1 - 1)
#define C31 (C30 + D1 - 6)
#define C32 (C31 + D1)
#define C33 (C32 + D1 - 6)
#define C34 (C33 + D1 - 6)
#define C35 (C34 + D1 - 7)
#define C36 (C35 + D1 - 2)
#define C37 (C36 + D1 - 4)
#define C38 (C37 + D1)
#define C39 (C38 + D1 - 6)
#define C40 (C39 + D1 - 6)
#define C41 (C40 + D1 - 7)
#define C42 (C41 + D1 - 2)

#define W1 12
#define L1 310

#define S2 120

#define BS1 17
#define BS2 100
#define BS3 125
#define BS4 15
#define BS5 160

/* Selector buttons */
#define JBS 14
#define JBH 80
#define JBD 18
#define JBD2 8
#define JBD3 17
#define JBD4 5

#define JBC1 60
#define JBC2 (JBC1 + JBD + JBD2)
#define JBC3 (JBC2 + JBD3)
#define JBC4 (JBC3 + JBD3)
#define JBC5 (JBC4 + JBD3)
#define JBC6 (JBC5 + JBD3 + JBD4)
#define JBC7 (JBC6 + JBD3)
#define JBC8 (JBC7 + JBD3)
#define JBC9 (JBC8 + JBD3)
#define JBC9a (JBC9 + JBD3 + JBD4)
#define JBC10 (JBC9a + JBD + JBD4 - 3)
#define JBC11 (JBC10 + JBD)
#define JBC12 (JBC11 + JBD)
#define JBC13 (JBC12 + JBD)

/* #define JBC14 (JBC13 + JBD + JBD2) */
#define JBC14 (326)
#define JBC15 (JBC14 + JBD)
#define JBC16 (JBC15 + JBD + JBD2)
#define JBC17 (JBC16 + JBD)
#define JBC18 (JBC17 + JBD)
#define JBC19 (JBC18 + JBD)
#define JBC20 (JBC19 + JBD)
#define JBC20_2 (JBC20 + JBD + JBD2)
/* Second half of buttons */
#define JBC21 (JBC1 + 512)
#define JBC22 (JBC21 + JBD)
#define JBC23 (JBC22 + JBD)
#define JBC24 (JBC23 + JBD)
#define JBC25 (JBC24 + JBD)
#define JBC26 (JBC25 + JBD)
#define JBC27 (JBC26 + JBD)
#define JBC28 (JBC27 + JBD)
#define JBC29 (JBC28 + JBD + JBD2)
#define JBC30 (JBC29 + JBD)
#define JBC31 (JBC30 + JBD)
#define JBC32 (JBC31 + JBD)
#define JBC33 (JBC32 + JBD)
#define JBC34 (JBC33 + JBD)
#define JBC35 (JBC34 + JBD)
#define JBC36 (JBC35 + JBD)
#define JBC37 (JBC36 + JBD + JBD2 - 2)
#define JBC38 (JBC37 + JBD + 4)
#define JBC39 (JBC38 + JBD + JBD2)
#define JBC40 (JBC39 + JBD)
#define JBC41 (JBC40 + JBD)
#define JBC42 (JBC41 + JBD)

#define JBCD (JBC1 + 440)

/* 42 lines controls, 43rd in button line, then 41 buttons plus display */
static brightonLocations locations[DEVICE_COUNT] = {
	/* 0 - LFO */
	{"LFO-Rate", 1, C5, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"LFO-Delay", 1, C6, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"LFO-Waveform", 0, C7 - 4, R1, JR1, JR1, 0, 3, 0, "bitmaps/knobs/knob1.xpm", 0, 
		BRIGHTON_STEPPED},
	/* 3 - mods */
	{"Mod-LFO-lvl", 1, C8, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Mod-Env-lvl", 1, C9, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Mod-Osc1/2", 2, C10, R1 - 20, 10, JR2 + 35, 0, 2, 0, 0,
		0, BRIGHTON_NOSHADOW|BRIGHTON_THREEWAY},

	{"Mod-PWM", 1, C11, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},

	/* PWM */
	{"ModPWM-EnvLFO", 2, C12, R1, 10, JR2, 0, 1.01, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"Mod-XMod", 1, C13, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	/*
	 * Transpose - replace with 4 buttons as multiselectors, this one and
	 * three more in the 'free' section - 9
	 */
	{"VCO1-2'", 2, C14 + 2, RB1, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCA1-4'", 2, C14 + 2, RB2, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCA1-8'", 2, C14 + 2, RB3, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCA1-16'", 2, C14 + 2, RB4, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},

	/*
	 * Waveform buttons - 13
	 */
	{"VCO1-Tri", 2, C15 + 4, RB1, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO1-Ramp", 2, C15 + 4, RB2, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO1-Pulse", 2, C15 + 4, RB3, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO1-Square", 2, C15 + 4, RB4, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},

	/* Sync, sub - 17 */
	{"VCO-Sync", 2, C16, R1 - 20, 10, JR2 + 35, 0, 2, 0, 0,
		0, BRIGHTON_NOSHADOW|BRIGHTON_THREEWAY},
	{"VCO2-Sub", 2, C17, R1, 10, JR2, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},

	/*
	 * 4 transpose buttons for OSC-2. They were 5 however we already have one
	 * button for sub wave - 19.
	 */
	{"VCO2-2'", 2, C18 + 1, RB1, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO2-4'", 2, C18 + 1, RB2, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO2-8'", 2, C18 + 1, RB3, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO2-16'", 2, C18 + 1, RB4, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},

	{"VCO2-Tune", 0, C19, R1, JR1, JR1, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 
		BRIGHTON_NOTCH},
	
	/*
	 * Waveform buttons - 24
	 */
	{"VCO2-Tri", 2, C20 + 0, RB1, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO2-Ramp", 2, C20 + 0, RB2, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO2-Pulse", 2, C20 + 0, RB3, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"VCO2-Noise", 2, C20 + 0, RB4, 11, JR3, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},

	/* Mix, HPF - 28 */
	{"Mix Osc1/2", 0, C21, R1, JR1, JR1, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 
		BRIGHTON_NOTCH},
	{"HPF", 1, C22, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	
	/* Filter - 30 */
	{"VCF-Cutoff", 1, C23, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"VCF-Resonance", 1, C24, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"VCF-Mode", 2, C25, R1 - 20, 10, JR2 + 35, 0, 2, 0, 0,
		0, BRIGHTON_NOSHADOW|BRIGHTON_THREEWAY},
	{"VCF-EnvLvl", 1, C26, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"VCF-EnvSel", 2, C27, R1, 10, JR2, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"VCF-LFO", 1, C28, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"VCF-KBD", 1, C29, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},

	/* AMP - 37 */
	{"VCA-Env", 1, C30, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"VCA-LFO", 1, C31 - 1, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
/*
	{"", 1, C31 + 1, R1 - 38, 5, JR2 + 75, 0, 3, 0,
		"bitmaps/buttons/klunk3.xpm", 0, 0},
*/

	/* ADSR1 - 39 */
	{"ADSR1-Attack", 1, C32 + 1, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"ADSR1-Decay", 1, C33, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"ADSR1-Sustain", 1, C34, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"ADSR1-Release", 1, C35, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"ADSR1-Kbd", 1, C36 - 6, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
/*
	{"", 2, C36, R1, 10, JR2, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
*/
	{"ADSR1-+/-", 2, C37 + 1, R1, 10, JR2, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},

	/* ADSR1 - 45 */
	{"ADSR2-Attack", 1, C38 + 1, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"ADSR2-Decay", 1, C39, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"ADSR2-Sustain", 1, C40, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"ADSR2-Release", 1, C41, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"ADSR2-Kdb", 1, C42 - 6, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
/*
	{"", 2, C42, R1, 10, JR2, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
*/

	/* PWM, XMOD env - 50 */
	{"Mod-Osc-PW", 1, C10a, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Mod-XMod-Lvl", 2, C13a, R1, 10, JR2, 0, 1.01, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	/* Layer Pan */
	{"Layer Pan", 0, C2, R2 + 50, JR1, JR1, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 
		BRIGHTON_NOTCH},

	/* Arpeg range - 53 */
	{"Arpeg 1 Oct", 2, JBC2, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbo.xpm",
		"bitmaps/buttons/jboo.xpm", 0},
	{"Arpeg 2 Oct", 2, JBC3, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbo.xpm",
		"bitmaps/buttons/jboo.xpm", 0},
	{"Arpeg 3 Oct", 2, JBC4, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbo.xpm",
		"bitmaps/buttons/jboo.xpm", 0},
	{"Arpeg 4 Oct", 2, JBC5, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbo.xpm",
		"bitmaps/buttons/jboo.xpm", 0},

	/* Arpeg mode - 57 */
	{"Arpeg Mode Up", 2, JBC6, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},
	{"Arpeg Mode Down", 2, JBC7, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},
	{"Arpeg Mode UpDn", 2, JBC8, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},
	{"Arpeg Mode Rand", 2, JBC9, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},

	/* Key assign mode - 61 */
	{"Mode Solo", 2, JBC10, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"Mode Uni", 2, JBC11, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"Mode Poly1", 2, JBC12, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"Mode Poly2", 2, JBC13, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},

	/* Holders - 15 for mods panel - 65 */
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/*
	 * Dummies - 8 from 80
	 */
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/*
	 * These are reserved already for two memory links, split point and function
	 * settings. From 88
	 */
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, JBH, JBH, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},

	/*
	 * Total of 65 layer controls - 0 to 49 plus 30 dummies, 14 of which are
	 * used by the mod panel.
	 *
	 * The above will be replicated per layer, the rest will be global to the
	 * whole synth.
	 */

	/*
	 * 92 - Second row Hold up/down
	 */
	{"Hold Lower", 2, JBC14, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbg.xpm",
		"bitmaps/buttons/jbgo.xpm", 0},
	{"Hold Upper", 2, JBC15, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbg.xpm",
		"bitmaps/buttons/jbgo.xpm", 0},

	/* layer mod enable - 94 */
	{"Mod Layer 1", 2, JBC19, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbb2.xpm",
		"bitmaps/buttons/jbb2o.xpm", 0},
	{"Mod Layer 2", 2, JBC20, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbb2.xpm",
		"bitmaps/buttons/jbb2o.xpm", 0},

	/* Dual/Split/Layer - 96 */
	{"Dual", 2, JBC16, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbb1.xpm",
		"bitmaps/buttons/jbb1o.xpm", 0},
	{"Split", 2, JBC17, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbb1.xpm",
		"bitmaps/buttons/jbb1o.xpm", 0},
	{"All", 2, JBC18, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbb1.xpm",
		"bitmaps/buttons/jbb1o.xpm", 0},

	/*
	 * The preceeding parameters are unique per synth and need to be loaded
	 * from different memory files? Alternatively just load the mem and let
	 * the last loaded parameters be active for the following ones:
	 *
	 * From 99:
	 *
	 * These are global to both layers - Vol/Balance/Arpeg rate and clock
	 *
	 * It could be worthwhile having the arpeggiator clock within the memory?
	 */
	{"Arpeg Rate", 1, C3, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Arpeg Int/Ext", 2, C4, R1, 10, JR2, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	/* Volume and Balance 101 and 102 */
	{"MasterVolume", 0, C1, R2 + 50, JR1, JR1, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 0},
	{"LayerBalance", 0, C1 +18, R1 +30, JR1, JR1, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0,
		BRIGHTON_NOTCH},

	/* Second row, tuning - 103 */
	{"MasterTuning", 0, 15, 800, JR1, JR1, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 
		BRIGHTON_NOTCH},
	{"Tune", 2, JBC1, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jbr.xpm",
		"bitmaps/buttons/jbro.xpm", BRIGHTON_CHECKBUTTON},

	/* Panel switch - 105 */
	{"LayerSelect", 2, JBC20_2, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbb2.xpm",
		"bitmaps/buttons/jbb2o.xpm", 0},

	/* Second half - memories and MIDI, not saved - 106 */
	{"", 2, JBC21, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"", 2, JBC22, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"", 2, JBC23, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"", 2, JBC24, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"", 2, JBC25, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"", 2, JBC26, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"", 2, JBC27, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},
	{"", 2, JBC28, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbw.xpm",
		"bitmaps/buttons/jbwo.xpm", 0},

	/* Load */
	{"", 2, JBC37, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbg.xpm",
		"bitmaps/buttons/jbgo.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, JBC29, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbo.xpm",
		"bitmaps/buttons/jboo.xpm", 0},
	{"", 2, JBC30, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbo.xpm",
		"bitmaps/buttons/jboo.xpm", 0},
	{"", 2, JBC31, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbo.xpm",
		"bitmaps/buttons/jboo.xpm", 0},
	{"", 2, JBC32, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbo.xpm",
		"bitmaps/buttons/jboo.xpm", 0},
	{"", 2, JBC33, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},
	{"", 2, JBC34, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},
	{"", 2, JBC35, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},
	{"", 2, JBC36, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},

	/* Save */
	{"", 2, JBC38, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbr.xpm",
		"bitmaps/buttons/jbro.xpm", BRIGHTON_CHECKBUTTON},

	/* MIDI */
	{"", 2, JBC39, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbb1.xpm",
		"bitmaps/buttons/jbb1o.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, JBC40, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbb1.xpm",
		"bitmaps/buttons/jbb1o.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, JBC41, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbb2.xpm",
		"bitmaps/buttons/jbb2o.xpm", BRIGHTON_CHECKBUTTON},

	/* Fn */
	{"", 2, JBC42, 800, JBS, JBH, 0, 1, 0,
		"bitmaps/buttons/jbb2.xpm",
		"bitmaps/buttons/jbb2o.xpm", 0},

	{"", 2, JBC9a, 800, JBS, JBH, 0, 4, 0,
		"bitmaps/buttons/jby.xpm",
		"bitmaps/buttons/jbyo.xpm", 0},

	/* RED LED display */
	{"", 8, JBCD,				813, JBS - 2, JBH - 8, 0, 9, 0, 0, 0, 0},
	{"", 8, JBS + JBCD - 2,		813, JBS - 2, JBH - 8, 0, 9, 0, 0, 0, 0},
	{"", 8, JBS * 2 + JBCD + 2, 813, JBS - 2, JBH - 8, 0, 9, 0, 0, 0, 0},
	{"", 8, JBS * 3 + JBCD + 1 ,813, JBS - 2, JBH - 8, 0, 9, 0, 0, 0, 0},

	{"", 3, JBCD, 818, 75, 55, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", BRIGHTON_WITHDRAWN},
};

/*
 * Mod panel
 */
static brightonLocations jmods[15] = {
	{"", 1, 75, 395, 70, 505, 0, 1, 0, "bitmaps/knobs/sliderblack5.xpm", 0, 0},
	{"", 1, 175, 395, 70, 505, 0, 1, 0, "bitmaps/knobs/sliderblack5.xpm", 0, 0},
	{"", 1, 275, 395, 70, 505, 0, 1, 0, "bitmaps/knobs/sliderblack5.xpm", 0, 0},
	{"", 1, 375, 395, 70, 505, 0, 1, 0, "bitmaps/knobs/sliderblack5.xpm", 0, 0},
	{"", 2, 50, 150, 60, 160, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 115, 150, 60, 160, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 185, 150, 60, 160, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 285, 150, 60, 160, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 385, 150, 60, 160, 0, 1, 0, "bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"", 0, 580, 130, 195, 195, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 0},
	{"", 0, 780, 130, 195, 195, 0, 1, 0, "bitmaps/knobs/knob1.xpm", 0, 0},
	{"", 2, 820, 400, 60, 200, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"", 2, 550, 400, 60, 200, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"", 2, 690, 400, 60, 200, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
/*
	{"", 2, 550, 460, 190, 130, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
*/
	{"", 1, 520, 715, 500, 100, 0, 1, 0, "bitmaps/knobs/knob8.xpm", 0,
		BRIGHTON_VERTICAL|BRIGHTON_CENTER|BRIGHTON_NOSHADOW|BRIGHTON_REVERSE},
};

static int
jupiterDestroySynth(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);

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
brightonApp jupiterApp = {
	"jupiter8",
	0,
	"bitmaps/textures/metal4.xpm",
	0,
	jupiterInit,
	jupiterConfigure, /* 3 callbacks, unused? */
	midiCallback,
	jupiterDestroySynth,
	{8, 50, 32, 2, 5, 520, 0, 0},
	900, 350, 0, 0,
	5,
	{
		{
			"Jupiter",
			"bitmaps/blueprints/jupiter.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			jupiterCallback,
			5, 30, 990, 670,
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
			jupiterKeyCallback,
			180, 695, 810, 300,
			KEY_COUNT,
			keysprofile
		},
		{
			"Mods",
			"bitmaps/blueprints/jupitermods.xpm",
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			jupiterCallback,
			5, 695, 175, 300,
			15,
			jmods
		},
		{
			"Metal Edge",
			0,
			"bitmaps/textures/metal5.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 5, 1000,
			0,
			0
		},
		{
			"Metal Edge",
			0,
			"bitmaps/textures/metal5.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			995, 0, 5, 1000,
			0,
			0
		},
	}
};

static int
jupiterDebug(guiSynth *synth, int level)
{
	if (debuglevel >= level)
		return(1);
	return(0);
}

static void
jupiterSeqInsert(guiSynth *synth, int note)
{
	arpeggiatorMemory *seq = (arpeggiatorMemory *) synth->seq1.param;

	if (synth->mem.param[PANEL_SELECT] != 0) {
printf("	Learning on upper layer\n");
		seq = (arpeggiatorMemory *) synth->seq2.param;
	}

	if (seq->s_max == 0)
		seq->s_dif = note + synth->transpose;

	seq->sequence[(int) (seq->s_max)] =
		(float) (note + synth->transpose - seq->s_dif);

	if (jupiterDebug(synth, 0))
		printf("Seq put %i into %i\n", (int) seq->sequence[(int) (seq->s_max)],
			(int) (seq->s_max));

	if ((seq->s_max += 1) >= BRISTOL_SEQ_MAX)
		seq->s_max = BRISTOL_SEQ_MAX;
}

static void
jupiterChordInsert(guiSynth *synth, int note)
{
	arpeggiatorMemory *seq = (arpeggiatorMemory *) synth->seq1.param;

	if (synth->mem.param[HOLD_LOWER] == 0)
		seq = (arpeggiatorMemory *) synth->seq2.param;

	if (seq->c_count == 0)
		seq->c_dif = note + synth->transpose;

	seq->chord[(int) (seq->c_count)]
		= (float) (note + synth->transpose - seq->c_dif);

	if (jupiterDebug(synth, 0))
		printf("Chord put %i into %i\n", (int) seq->chord[(int) (seq->c_count)],
			(int) (seq->c_count));

	if ((seq->c_count += 1) >= BRISTOL_CHORD_MAX)
		seq->c_count = BRISTOL_CHORD_MAX;
}

static int
jupiterKeyCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (setsplit != 0) {
		if (jupiterDebug(synth, 1))
			printf("setSplit at %i\n", index);
		synth->mem.param[SPLIT_POINT] = index;
		jupiterSetSplit(synth);
	}

	if ((chordLearn) && (value != 0))
		jupiterChordInsert(synth, index);

	if ((seqLearn) && (value != 0))
		jupiterSeqInsert(synth, index);

	if (global.libtest)
		return(0);

	if (jupiterDebug(synth, 2))
		printf("keyCallback(%i)\n", index);

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 */
	if (value)
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
	else
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);

	return(0);
}

static void
jupiterDualSend(guiSynth *synth, int fd, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, c, o, v);
	bristolMidiSendMsg(global.controlfd, synth->sid2, c, o, v);
}

static void
midiRelease(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (jupiterDebug(synth, 1))
		printf("midiRelease()\n");

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

static void
modPanelFix(brightonWindow *win, guiSynth *synth)
{
	int i;
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;

	for (i = 0; i < 14; i++)
	{
		event.value = synth->mem.param[LAYER_DEVS + i];
		brightonParamChange(win, MODS_PANEL, i, &event);
	}
}

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (jupiterDebug(synth, 1))
		printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			if (jupiterDebug(synth, 2))
				printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, "jupiter8", 0, synth->bank + synth->location,
				synth->mem.active, 0, 0);
			modPanelFix(win, synth);
			break;
		case MIDI_BANK_SELECT:
			if (jupiterDebug(synth, 2))
				printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value + 10;
			loadMemory(synth, "jupiter8", 0, synth->bank + synth->location,
				synth->mem.active, 0, 0);
			modPanelFix(win, synth);
			break;
	}
	return(0);
}

static int
jupiterMidiSendMsg(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (jupiterDebug(synth, 5))
		printf("%i, %i, %i\n", c, o, v);

	if (synth->mem.param[PANEL_SELECT] != 0)
		bristolMidiSendMsg(fd, synth->sid2, c, o, v);
	else
		bristolMidiSendMsg(fd, synth->sid, c, o, v);
	return(0);
}

static void
jupiterSetDisplay(guiSynth *synth)
{
	brightonEvent event;

	if (jupiterDebug(synth, 3))
		printf("jupiterSetDisplay(%f, %f)\n",
			synth->mem.param[MY_MEM], synth->mem.param[MEM_LINK]);

	event.type = BRIGHTON_FLOAT;

	/*
	 * Load will set the displays and nothing else, they will always point to
	 * the active patches.
	 *
	 * The buttons selections should come from the layer select or from being
	 * pressed themselves.
	 */
	if (synth->mem.param[PANEL_SELECT] != 0) {
		event.value = ((int) synth->mem.param[MY_MEM]) / 10;
		brightonParamChange(synth->win, 0, DISPLAY_DEV + 2, &event);
		event.value = ((int) synth->mem.param[MY_MEM]) % 10;
		brightonParamChange(synth->win, 0, DISPLAY_DEV + 3, &event);

		event.value = ((int) synth->mem.param[MEM_LINK]) / 10;
		brightonParamChange(synth->win, 0, DISPLAY_DEV + 0, &event);
		event.value = ((int) synth->mem.param[MEM_LINK]) % 10;
		brightonParamChange(synth->win, 0, DISPLAY_DEV + 1, &event);
	} else {
		event.value = ((int) synth->mem.param[MY_MEM]) / 10;
		brightonParamChange(synth->win, 0, DISPLAY_DEV + 0, &event);
		event.value = ((int) synth->mem.param[MY_MEM]) % 10;
		brightonParamChange(synth->win, 0, DISPLAY_DEV + 1, &event);

		event.value = ((int) synth->mem.param[MEM_LINK]) / 10;
		brightonParamChange(synth->win, 0, DISPLAY_DEV + 2, &event);
		event.value = ((int) synth->mem.param[MEM_LINK]) % 10;
		brightonParamChange(synth->win, 0, DISPLAY_DEV + 3, &event);
	}
}

static int exclude = 0;
static int fPmask = 0x00;
static int fBmask = 0x00;

typedef struct FArray {
	int c;
	int o;
} farray;

static struct {
	farray pFuncs[8];
	farray bFuncs[8];
} functions = {
	{
		{7, 6}, /* Env-1 rezero, condition, attack tracking. */
		{7, 7},
		{7, 8},
		{9, 6}, /* Env-2 rezero, condition, attack tracking. */
		{9, 7},
		{9, 8},
		{126, 31}, /* Noise per layer */
		{10, 1} /* White/Pink */
	},
	{
		{126, 29}, /* LFO-1 uni */
		{0, 1}, /* LFO-1 uni */
		{2, 5}, /* LFO-1 gain velocity tracking */
		{BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_TRIGGER}, /* Arpeggiator retrig */
		{127, 33}, /* LFO Touch Sense */
		{127, 115}, /* NRP enable */
		{127, 116}, /* MIDI Debug */
		{127, 117} /* Internal debug */
	}
};

static int width;

static void
jupiterFunctionKey(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int i;
	brightonEvent event;

	if (jupiterDebug(synth, 1))
		printf("jupiterFunctionKey(%i, %i, %i)\n", c, o, v);

	if (synth->win->width != width)
	{
		brightonPut(synth->win, "bitmaps/blueprints/jupitershade.xpm", 0, 0,
			synth->win->width, synth->win->height);
		width = synth->win->width;
	}

	if (exclude == 1)
		return;

	if ((c == 2) && (v != 0))
	{
		/*
		 * Turn on any active function keys. TBD.
		 */
		exclude = 1;
		event.type = BRIGHTON_FLOAT;
		for (i = 0; i < 8; i++) {
			event.value = fPmask & (1 << i);
			brightonParamChange(synth->win, 0, MEM_MGT + i, &event);
			event.value = fBmask & (1 << i);
			brightonParamChange(synth->win, 0, MEM_MGT + 9 + i, &event);
		}
		exclude = 0;
	}

	if ((c == 2) && (v == 0))
	{
		/*
		 * Turning off - clear all the buttons and stuff the memory value
		 */
		exclude = 1;

		event.type = BRIGHTON_FLOAT;
		event.value = 0;

		for (i = 0; i < 8; i++) {
			brightonParamChange(synth->win, 0, MEM_MGT + i, &event);
			brightonParamChange(synth->win, 0, MEM_MGT + 9 + i, &event);
		}

		/*
		 * set the buttons according to the display digits
		 */
		if (synth->mem.param[PANEL_SELECT] == 0) {
			event.value = 1.0;
			i = synth->mem.param[DISPLAY_DEV + 0] - 1;
			brightonParamChange(synth->win, 0, MEM_MGT + 9 + i, &event);
			i = synth->mem.param[DISPLAY_DEV + 1] - 1;
			brightonParamChange(synth->win, 0, MEM_MGT + i, &event);
		} else {
			event.value = 1.0;
			i = synth->mem.param[DISPLAY_DEV + 3] - 1;
			brightonParamChange(synth->win, 0, MEM_MGT + i, &event);
			i = synth->mem.param[DISPLAY_DEV + 2] - 1;
			brightonParamChange(synth->win, 0, MEM_MGT + 9 + i, &event);
		}

		exclude = 0;
	}

	/*
	 * The functions will be on, or off. We could consider how to make them
	 * continuous however we don't really have a data entry pot we could
	 * reasonably consider using. We should also consider that using just a
	 * set of bits as a mask makes them easy to save.
	 */
	if (c == 0) {
		if (v != 0)
			/*
			 * Prog functions on, keep mask to track them.
			 */
			fPmask |= 0x01 << o;
		if (v == 0)
			/*
			 * Prog functions off.
			 */
			fPmask &= ~(0x01 << o);

		if (jupiterDebug(synth, 1)) {
			if (v != 0)
				printf("call f(p): %i, %i ON\n",
					functions.pFuncs[o].c,
					functions.pFuncs[o].o);
			else
				printf("call f(p): %i, %i OFF\n",
					functions.pFuncs[o].c,
					functions.pFuncs[o].o);
		}

		if (exclude == 0)
			jupiterDualSend(synth, synth->sid,
				functions.pFuncs[o].c, functions.pFuncs[o].o, v);

		return;
	}

	if (c == 1) {
		if (v != 0)
			/*
			 * Prog functions on, keep mask to track them.
			 */
			fBmask |= 0x01 << o;
		if (v == 0)
			/*
			 * Prog functions off.
			 */
			fBmask &= ~(0x01 << o);

		if (jupiterDebug(synth, 1)) {
			if (v != 0)
				printf("call f(p): %i, %i ON\n",
					functions.bFuncs[o].c,
					functions.bFuncs[o].o);
			else
				printf("call f(b): %i, %i OFF\n",
					functions.bFuncs[o].c,
					functions.bFuncs[o].o);
		}

		/* LFO-2 (mods) keyboard control */
		if (o == 4) {
			if (v != 0)
				v = 1;

			/* LFO sync, LFO frequency tracking KBD, Env tracking */
			jupiterDualSend(synth, synth->sid, 1, 1, v);
			jupiterDualSend(synth, synth->sid, 1, 2, v);
			jupiterDualSend(synth, synth->sid, 3, 5, v);

			return;
		}

		/* NRP Enable */
		if (o == 5) {
			if (v != 0)
				v = 1;
			if (!global.libtest) {
				bristolMidiSendNRP(global.controlfd, synth->sid,
					BRISTOL_NRP_ENABLE_NRP, v);
				bristolMidiSendNRP(global.controlfd, synth->sid2,
					BRISTOL_NRP_ENABLE_NRP, v);
			}
			return;
		}

		/* Midi debug */
		if (o == 6) {
			if (v != 0)
				v = 1;
			if (!global.libtest) {
				bristolMidiSendNRP(global.controlfd, synth->sid,
					BRISTOL_NRP_DEBUG, v);
				bristolMidiSendNRP(global.controlfd, synth->sid2,
					BRISTOL_NRP_DEBUG, v);
			}
			return;
		}

		/* Internal debug */
		if (o == 7) {
			debuglevel = v;
			return;
		}

		if (exclude == 0) {
			jupiterDualSend(synth, synth->sid,
				functions.bFuncs[o].c, functions.bFuncs[o].o, v);
		}

		return;
	}
}

static void
jupiterSetFuncs(guiSynth *synth)
{
	int i;

	for (i = 0; i < 8; i++)
		jupiterFunctionKey(synth, global.controlfd, synth->sid,
			0, i, fPmask & (1 << i));
	for (i = 0; i < 8; i++)
		jupiterFunctionKey(synth, global.controlfd, synth->sid,
			1, i, fBmask & (1 << i));
}

static void
jupiterButtonPanel(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;

	if (jupiterDebug(synth, 1))
		printf("jupiterButtonPanel(%i, %i, %i)\n", c, o, v);

	if (synth->mem.param[FUNCTION] != 0)
		return(jupiterFunctionKey(synth, fd, chan, c, o, v));

	switch (c) {
		case 0:
			/* patch selector */
			if (jupiterDebug(synth, 2))
				printf("memory\n");
			/*
			 * Force exclusion
			 */
			if (synth->dispatch[MEM_MGT + c].other2)
			{
				synth->dispatch[MEM_MGT + c].other2 = 0;
				return;
			}
			if (synth->dispatch[MEM_MGT + c].other1 != -1)
			{
				synth->dispatch[MEM_MGT + c].other2 = 1;

				if (synth->dispatch[MEM_MGT + c].other1 != MEM_MGT + o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[MEM_MGT + c].other1, &event);
			}
			synth->dispatch[MEM_MGT + c].other1 = MEM_MGT + o;

			synth->location = o + 1;

			break;
		case 1:
			/* Bank selector */
			if (jupiterDebug(synth, 2))
				printf("bank\n");
			/*
			 * Force exclusion
			 */
			if (synth->dispatch[MEM_MGT + c].other2)
			{
				synth->dispatch[MEM_MGT + c].other2 = 0;
				return;
			}
			if (synth->dispatch[MEM_MGT + c].other1 != -1)
			{
				synth->dispatch[MEM_MGT + c].other2 = 1;

				if (synth->dispatch[MEM_MGT + c].other1 != MEM_MGT + o + 9)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[MEM_MGT + c].other1, &event);
			}
			synth->dispatch[MEM_MGT + c].other1 = MEM_MGT + o + 9;

			synth->bank = (o + 1) * 10;

			break;
	}
}

/*
 * All the synth->second stuff is seriously painful. We are going to remove
 * it all and go for a single layer with backing stores. Panel switch will be
 * responsible for those two stores.
 */
static void
jupiterPanelSwitch(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int i, mymem, mylink;

	mymem = synth->mem.param[MY_MEM];
	mylink = synth->mem.param[MEM_LINK];

	if (jupiterDebug(synth, 1))
		printf("jupiterPanelSwitch(%i, %i, %i) %f\n", c, o, v,
			synth->mem.param[PANEL_SELECT]);

	if (jupiterDebug(synth, 5)) {
		int i;

		for (i = 0; i < LAYER_DEVS; i+= 8)
			printf("%f %f %f %f %f %f %f %f\n",
				pcache[i], pcache[i + 1], pcache[i + 2], pcache[i + 3],
				pcache[i + 4], pcache[i + 5], pcache[i + 6], pcache[i + 7]);
		printf("\n");
		for (i = 0; i < LAYER_DEVS; i+= 8)
			printf("%f %f %f %f %f %f %f %f\n",
				scache[i], scache[i + 1], scache[i + 2], scache[i + 3],
				scache[i + 4], scache[i + 5], scache[i + 6], scache[i + 7]);
	}

	/*
	 * Copy from either the p or s cache into the synth then request updating
	 * the whole shebang.
	 */
	if (v != 0)
	{
		if (jupiterDebug(synth, 1))
			printf("converge secondary layer\n");

		/*
		 * Save pcache, copy in the scache
		 */
		bcopy(synth->mem.param, pcache, LAYER_DEVS * sizeof(float));

		if (jupiterDebug(synth, 3))
			printf("PS: my mem is %f, will be %f\n",
				synth->mem.param[MY_MEM], synth->mem.param[MEM_LINK]);

		event.type = BRIGHTON_FLOAT;
		for (i = 0; i < LAYER_DEVS; i++)
		{
			event.value = scache[i];
			brightonParamChange(synth->win, 0, i, &event);
		}
	} else {
		if (jupiterDebug(synth, 1))
			printf("converge primary layer\n");

		/*
		 * Save scache, copy in the pcache
		 */
		bcopy(synth->mem.param, scache, LAYER_DEVS * sizeof(float));

		if (jupiterDebug(synth, 3))
			printf("PS: my mem is %f, will be %f\n",
				synth->mem.param[MY_MEM], synth->mem.param[MEM_LINK]);

		event.type = BRIGHTON_FLOAT;
		for (i = 0; i < LAYER_DEVS; i++)
		{
			event.value = pcache[i];
			brightonParamChange(synth->win, 0, i, &event);
		}
	}

	synth->mem.param[MY_MEM] = mylink;
	synth->mem.param[MEM_LINK] = mymem;

	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;
	if (((i = (mylink / 10) - 1) >= 0) && (i < 8))
		brightonParamChange(synth->win, 0, BANK_BUTTONS + i, &event);
	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;
	if (((i = (mylink % 10) - 1) >= 0) && (i < 8))
		brightonParamChange(synth->win, 0, PATCH_BUTTONS + i, &event);

	if (jupiterDebug(synth, 5)) {
		int i;

		for (i = 0; i < LAYER_DEVS; i+= 8)
			printf("%f %f %f %f %f %f %f %f\n",
				pcache[i], pcache[i + 1], pcache[i + 2], pcache[i + 3],
				pcache[i + 4], pcache[i + 5], pcache[i + 6], pcache[i + 7]);
		printf("\n");
		for (i = 0; i < LAYER_DEVS; i+= 8)
			printf("%f %f %f %f %f %f %f %f\n",
				scache[i], scache[i + 1], scache[i + 2], scache[i + 3],
				scache[i + 4], scache[i + 5], scache[i + 6], scache[i + 7]);
	}

	jupiterSetDisplay(synth);
}

static void
jupiterMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (synth->flags & MEM_LOADING)
		return;

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (jupiterDebug(synth, 1))
		printf("jupiterMemory(%i, %i, %i)\n", c, o, v);

	switch (c) {
		case 2:
			/* Memory load */
			if (jupiterDebug(synth, 2))
				printf("load\n");

			/*
			 * Changed some of the original logic here. Rather than load all
			 * parameters on a first load, then just the layer devs on a double
			 * click we are only ever going to load the layer devs unless we
			 * double click, then we load everything including peer layer if
			 * configured. If there is no peer layer then we should put the
			 * emulation into mode All and potentially put it into Poly-1 if
			 * it was in Solo.
			 *
			 * This code should also invistigate how to load some arpeggiator
			 * information, automated if possible into the library.
			 */
			if (brightonDoubleClick(dc1)) {
				brightonEvent event;
				int mymem, mylink;
				int flags = BRISTOL_SEQLOAD|BRISTOL_SEQFORCE;

				if (synth->mem.param[PANEL_SELECT] != 0)
					flags |= BRISTOL_SID2;

				/*
				 * Do a full load of my memory, fix any mod panel controls,
				 * set up the function masks and activate them.
				 */
				if (loadMemory(synth, "jupiter8", 0,
					synth->bank + synth->location + mw,
					synth->mem.active, 0, flags) >= 0)
				{
					if (jupiterDebug(synth, 2))
						printf("am %2.0f (%i), peer layer loading %2.0f\n",
							synth->mem.param[MY_MEM],
							synth->bank + synth->location,
							synth->mem.param[MEM_LINK]);

					modPanelFix(synth->win, synth);

					fBmask = ((int) synth->mem.param[FUNCTION_SAVE]) >> 8;
					fPmask = ((int) synth->mem.param[FUNCTION_SAVE]) & 0x00ff;

					if (jupiterDebug(synth, 4))
						printf("loading functions %x %x from %x\n",
							fBmask, fPmask,
							(int) synth->mem.param[FUNCTION_SAVE]);

					jupiterSetFuncs(synth);
				} else
					return;

				/*
				 * Build our links and also set our display values.
				 */
				mymem = synth->mem.param[MY_MEM]
					= synth->bank + synth->location;

				flags = BRISTOL_SEQLOAD|BRISTOL_SEQFORCE;

				/*
				 * Take a peek at the peer memory to see if one is configured.
				 * If there is one and we have split/double selected then load
				 * it and activate the splits, etc.
				 */
				if ((mylink = synth->mem.param[MEM_LINK]) >= 0)
				{
					event.type = BRIGHTON_FLOAT;
					event.value = 1.0 - synth->mem.param[PANEL_SELECT];
					brightonParamChange(synth->win, 0, PANEL_SELECT, &event);

					/*
					 * This should actually have happened due to the fully
					 * loaded memory.
					jupiterSetSplit(synth);
					jupiterSetVoices(synth);
					 */

					if (jupiterDebug(synth, 2))
						printf("dual load %i (%i), peer layer is %2.0f\n",
							mylink,
							synth->bank + synth->location,
							synth->mem.param[MEM_LINK]);

					if (synth->mem.param[PANEL_SELECT] != 0)
						flags |= BRISTOL_SID2;

					loadMemory(synth, "jupiter8", 0,
						mylink + mw, LAYER_DEVS, 0, flags);

					synth->mem.param[MY_MEM] = mylink;
					synth->mem.param[MEM_LINK] = mymem;
				} else {
					/*
					 * Clear any dual/split state
					 */
					event.type = BRIGHTON_FLOAT;
					event.value = 1.0;

					/* Set 'All' state */
					brightonParamChange(synth->win, 0, KEY_MODE + 2, &event);
					/* Set 'Poly-1' state */
					brightonParamChange(synth->win, 0, POLY_MODE + 2, &event);
				}
			} else {
				int link = synth->mem.param[MEM_LINK];

				if (jupiterDebug(synth, 1))
					printf("Single load %i of %i params\n",
						synth->bank + synth->location + mw, LAYER_DEVS);

				loadMemory(synth, "jupiter8", 0,
					synth->bank + synth->location + mw,
					LAYER_DEVS, 0, 0);

				if (jupiterDebug(synth, 1))
					printf("am %2.0f (%i), no peer layer loading (%2.0f)\n",
						synth->mem.param[MY_MEM],
						synth->bank + synth->location,
						synth->mem.param[MEM_LINK]);

				/*
				 * Build our links and also set our display values.
				 */
				synth->mem.param[MY_MEM] = synth->bank + synth->location;
				synth->mem.param[MEM_LINK] = link;

			}
			break;
		case 3:
			/* Memory save */
			if (brightonDoubleClick(dc1)) {
				if (jupiterDebug(synth, 2))
					printf("save\n");

				if (synth->flags & MEM_LOADING)
					return;

				/*
				 * I have my memory setup, I need to make sure that my linked
				 * memory is correct. The linked in memory changes by layer but
				 * the panel switch should have taken care of that.
				 */
				synth->mem.param[MY_MEM] = synth->bank + synth->location;

				if (synth->mem.param[PANEL_SELECT] != 0)
					synth->mem.param[MEM_LINK]
						= synth->mem.param[DISPLAY_DEV + 0] * 10
						+ synth->mem.param[DISPLAY_DEV + 1];
				else
					synth->mem.param[MEM_LINK]
						= synth->mem.param[DISPLAY_DEV + 2] * 10
						+ synth->mem.param[DISPLAY_DEV + 3];

				if (jupiterDebug(synth, 4))
					printf("saving functions %x %x\n", fBmask, fPmask);

				synth->mem.param[FUNCTION_SAVE] = (fBmask << 8) + fPmask;

				saveMemory(synth, "jupiter8", 0,
					synth->bank + synth->location + mw, 0);

				if (synth->mem.param[PANEL_SELECT] != 0)
					saveSequence(synth, "jupiter8",
						synth->bank + synth->location + mw, BRISTOL_SID2);
				else
					saveSequence(synth, "jupiter8",
						synth->bank + synth->location + mw, 0);

				if (jupiterDebug(synth, 1))
					printf("	am %2.0f (%i), peer layer saved %2.0f\n",
						synth->mem.param[MY_MEM],
						synth->bank + synth->location,
						synth->mem.param[MEM_LINK]);
			}
			break;
	}

	if (synth->mem.param[PANEL_SELECT] == 0) {
		synth->mem.param[DISPLAY_DEV + 0]
			= ((int) synth->mem.param[MY_MEM]) / 10;
		synth->mem.param[DISPLAY_DEV + 1]
			= ((int) synth->mem.param[MY_MEM]) % 10;
		synth->mem.param[DISPLAY_DEV + 2]
			= ((int) synth->mem.param[MEM_LINK]) / 10;
		synth->mem.param[DISPLAY_DEV + 3]
			= ((int) synth->mem.param[MEM_LINK]) % 10;
	} else {
		synth->mem.param[DISPLAY_DEV + 0]
			= ((int) synth->mem.param[MEM_LINK]) / 10;
		synth->mem.param[DISPLAY_DEV + 1]
			= ((int) synth->mem.param[MEM_LINK]) % 10;
		synth->mem.param[DISPLAY_DEV + 2]
			= ((int) synth->mem.param[MY_MEM]) / 10;
		synth->mem.param[DISPLAY_DEV + 3]
			= ((int) synth->mem.param[MY_MEM]) % 10;
	}

/*
printf("am showing %1.0f %1.0f, %1.0f %1.0f from %1.0f %1.0f (%1.0f)\n",
synth->mem.param[DISPLAY_DEV + 0],
synth->mem.param[DISPLAY_DEV + 1],
synth->mem.param[DISPLAY_DEV + 2],
synth->mem.param[DISPLAY_DEV + 3],
synth->mem.param[MY_MEM],
synth->mem.param[MEM_LINK],
synth->mem.param[PANEL_SELECT]);
*/

	/*
	 * Load will set the displays and nothing else, they will alsways point to
	 * the active patches.
	 *
	 * The buttons selections should come from the layer select or from being
	 * pressed themselves.
	 */
	jupiterSetDisplay(synth);
}

static void
jupiterMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
			newchan = synth->midichannel = 0;
	} else {
		/*
		 * This is a little incorrect - if we are layered then we can go to
		 * midi channel 15.
		 */
		if ((newchan = synth->midichannel + 1) >= 14)
			newchan = synth->midichannel = 14;
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid2,
			127, 0, (BRISTOL_MIDICHANNEL|newchan) + 1);
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
jupiterCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if ((synth == 0) || (synth->flags & SUPPRESS))
		return(0);

	if (jupiterDebug(synth, 2))
		printf("jupiterCallback(%i, %i, %f)\n", panel, index, value);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (panel == 2) {
		index += LAYER_DEVS;
		sendvalue = value * C_RANGE_MIN_1;
	} else {
		if (jupiterApp.resources[0].devlocn[index].to == 1)
			sendvalue = value * C_RANGE_MIN_1;
		else
			sendvalue = value;
	}

	/*
	 * If the index is less than LAYER_DEVS then write the value to the desired
	 * layer only, otherwise write the value to both/top layers?
	 */
	synth->mem.param[index] = value;

	synth->dispatch[index].routine(synth,
		global.controlfd, synth->sid,
		synth->dispatch[index].controller,
		synth->dispatch[index].operator,
		sendvalue);

	return(0);
}

/*
 * The Jupiter does not actually have a sequencer feature, just the arpeggiator
 * and chording.
 */
static void
jupiterSeqMode(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int sid = synth->sid;

	if (jupiterDebug(synth, 1))
		printf("jupiterSeqMode(%i, %i)\n", c, v);

	event.type = BRIGHTON_FLOAT;

	if (synth->mem.param[PANEL_SELECT] == 0)
		sid = synth->sid;
	else
		sid = synth->sid2;

	if (synth->mem.param[FUNCTION] != 0)
	{
		if  (v != 0) {
			if ((synth->flags & OPERATIONAL) == 0)
				return;
			if (synth->flags & MEM_LOADING)
				return;

			if (jupiterDebug(synth, 0))
				printf("Arpeg learn mode requested %x\n",  synth->flags);

			if (synth->seq1.param == NULL)
				loadSequence(&synth->seq1, "jupiter8", 0, 0);
			if (synth->seq2.param == NULL)
				loadSequence(&synth->seq2, "jupiter8", 0, BRISTOL_SID2);

			seqLearn = 1;

			/* Reset the counter */
			if (synth->mem.param[PANEL_SELECT] == 0)
				synth->seq1.param[0] = 0;
			else
				synth->seq2.param[0] = 0;

			/*
			 * Stop the arpeggiator and send a learn request
			 */
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 1);
		} else {
			/*
			 * Stop the learning process
			 */
			seqLearn = 0;
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
		}

		return;
	}

	if (seqLearn == 1) {
		/*
		 * This is a button press during the learning sequence, in which case
		 * it needs to be terminated.
		 */
		seqLearn = 0;
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RESEQ, 0);
	}

	if (v != 0) 
	{
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_DIR, o);
		/* And enable it */
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 1);

		return;
	} else {
		/*
		 * OK, the value is zero. If this is the same controller going off
		 * then disable the arpeggiator, otherwise do nothing.
		 */
		if (synth->dispatch[ARPEG_MODE].other1 == c)
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_ENABLE, 0);
		return;
	}
}

static void
jupiterArpegRate(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * We need a shim since this needs to go to both SEQ and ARPEG and both
	 * layers (as we only have a single controller).
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RATE, v);
	bristolMidiSendMsg(global.controlfd, synth->sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RATE, v);

	bristolMidiSendMsg(global.controlfd, synth->sid2,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RATE, v);
	bristolMidiSendMsg(global.controlfd, synth->sid2,
		BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RATE, v);
}

static void
jupiterArpegMode(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int sid = synth->sid;

	/*
	 * Force exclusion
	 */
	if (synth->dispatch[ARPEG_MODE].other2)
	{
		synth->dispatch[ARPEG_MODE].other2 = 0;
		return;
	}

	if (jupiterDebug(synth, 1))
		printf("jupiterArpegMode(%i, %i)\n", c, v);

	event.type = BRIGHTON_FLOAT;

	if (synth->mem.param[PANEL_SELECT] == 0)
		sid = synth->sid;
	else
		sid = synth->sid2;

	if (v != 0) 
	{
		if (synth->dispatch[ARPEG_MODE].other1 != -1)
		{
			synth->dispatch[ARPEG_MODE].other2 = 1;

			if (synth->dispatch[ARPEG_MODE].other1 != c)
				event.value = 0;
			else
				event.value = 1;

			brightonParamChange(synth->win, synth->panel,
				synth->dispatch[ARPEG_MODE].other1, &event);
		}
		synth->dispatch[ARPEG_MODE].other1 = c;

		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_DIR, o);
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_DIR, o);
		/* And enable it */
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, 1);

		return;
	}

	/*
	 * If we are going off then disable the arpeggiator, otherwise set the
	 * mode and start it.
	 */
	if ((v == 0) && (synth->dispatch[ARPEG_MODE].other1 == c))
		/*
		 * OK, the value is zero. If this is the same controller going off
		 * then disable the arpeggiator, otherwise do nothing.
		 */
		bristolMidiSendMsg(global.controlfd, sid,
			BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_ENABLE, 0);
}

static void
jupiterArpegRange(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int sid;

	if (jupiterDebug(synth, 1))
		printf("jupiterArpegRange(%i, %i) %i\n", c, o, v);

	event.type = BRIGHTON_FLOAT;

	/*
	 * Force exclusion
	 */
	if (synth->dispatch[ARPEG_RANGE].other2)
	{
		synth->dispatch[ARPEG_RANGE].other2 = 0;
		return;
	}

	if (v == 0)
	{
		if (synth->dispatch[ARPEG_RANGE].other1 == c)
		{
			event.value = 1;
			brightonParamChange(synth->win, synth->panel, c, &event);
		}
		return;
	}

	if (synth->dispatch[ARPEG_RANGE].other1 != -1)
	{
		synth->dispatch[ARPEG_RANGE].other2 = 1;

		if (synth->dispatch[ARPEG_RANGE].other1 != c)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, 0,
			synth->dispatch[ARPEG_RANGE].other1, &event);
	}
	synth->dispatch[ARPEG_RANGE].other1 = c;

	/*
	 * Then do whatever 
	 */
	if (synth->mem.param[PANEL_SELECT] == 0)
		sid = synth->sid;
	else
		sid = synth->sid2;

	bristolMidiSendMsg(global.controlfd, sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_SEQ_RANGE, o);
	bristolMidiSendMsg(global.controlfd, sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_ARPEG_RANGE, o);

	return;
}

static void
jupiterSetVoices(guiSynth *synth)
{
	int sid;

	/*
	 * If we are in 'All' mode then sid is for lower layer only, otherwise
	 * we need to look at the layer selection.
	 */
	if (synth->mem.param[KEY_MODE + 2] != 0)
		sid = synth->sid;
	else if (synth->mem.param[PANEL_SELECT] == 0)
		sid = synth->sid;
	else
		sid = synth->sid2;

	/*
	 * Take a peek at the Poly and Key modes to set either:
	 *
	 *	Solo:	1 voice
	 *	Uni:	Unify:
	 *		All:	all voices else half
	 *	Poly:	clear unison:
	 *		All:	all voices else half
	 */
	if (synth->mem.param[POLY_MODE] != 0) {
		/*
		 * Solo. One voice no Unison.
		 */
		bristolMidiSendMsg(global.controlfd, sid, BRISTOL_ARPEGGIATOR,
			BRISTOL_ARPEG_POLY_2, 0);
		bristolMidiSendMsg(global.controlfd, sid, 126, 2, 2);
		bristolMidiSendMsg(global.controlfd, sid, 126, 7, 0);
	} else if (synth->mem.param[POLY_MODE + 1] != 0) {
		/*
		 * Unison all voices
		 */
		bristolMidiSendMsg(global.controlfd, sid, BRISTOL_ARPEGGIATOR,
			BRISTOL_ARPEG_POLY_2, 0);
		if (synth->mem.param[KEY_MODE + 2] != 0)
			bristolMidiSendMsg(global.controlfd, sid, 126, 2, 1);
		else
			bristolMidiSendMsg(global.controlfd, sid, 126, 2, 0);
		bristolMidiSendMsg(global.controlfd, sid, 126, 7, 1);
	} else if (synth->mem.param[POLY_MODE + 2] != 0) {
		/*
		 * Poly-1
		 */
		bristolMidiSendMsg(global.controlfd, sid, BRISTOL_ARPEGGIATOR,
			BRISTOL_ARPEG_POLY_2, 0);
		if (synth->mem.param[KEY_MODE + 2] != 0)
			bristolMidiSendMsg(global.controlfd, sid, 126, 2, 1);
		else
			bristolMidiSendMsg(global.controlfd, sid, 126, 2, 0);
		bristolMidiSendMsg(global.controlfd, sid, 126, 7, 0);
	} else if (synth->mem.param[POLY_MODE + 3] != 0) {
		/*
		 * Poly-2 is not operational at the moment. Configure as per Poly-1
		 */
		if (synth->mem.param[KEY_MODE + 2] != 0) {
			bristolMidiSendMsg(global.controlfd, sid, 126, 2, 1);
		} else {
			bristolMidiSendMsg(global.controlfd, sid, 126, 2, 0);
		}
		bristolMidiSendMsg(global.controlfd, sid, BRISTOL_ARPEGGIATOR,
			BRISTOL_ARPEG_POLY_2, 1);
	}
}

/*
 * This may have to move to within the Layer parameters since assuming the JP-8
 * was the same as the JP6 and MKS-80 these were accorded to each layer.
 *
 * We could leave that for later study since the JP-8 arpeggiator was officially
 * only on the lower layer - the JP-6 and MKS-80 could have them on either
 * layer so we may move all these arpeggiator and key modes into the layer
 * parameters.
 */
static void
jupiterUniPoly(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (jupiterDebug(synth, 2))
		printf("jupiterUniPoly(%i, %i)\n", c, v);

	event.type = BRIGHTON_FLOAT;
	/*
	 * Force exclusion
	 */
	if (synth->dispatch[POLY_MODE].other2)
	{
		synth->dispatch[POLY_MODE].other2 = 0;
		return;
	}

	if (v == 0)
	{
		if (synth->dispatch[POLY_MODE].other1 == c)
		{
			event.value = 1;
			brightonParamChange(synth->win, synth->panel, c, &event);
		} else
			return;
	}
	if (synth->dispatch[POLY_MODE].other1 != -1)
	{
		synth->dispatch[POLY_MODE].other2 = 1;

		if (synth->dispatch[POLY_MODE].other1 != c)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[POLY_MODE].other1, &event);
	}

	synth->dispatch[POLY_MODE].other1 = c;

	if (v == 0)
		return;

	/*
	 * Do what needs to be done
	 */
	switch (o) {
		case 0:
			if (jupiterDebug(synth, 1))
				printf("Solo Mode\n");
			/*
			 * Set single voice, no uni
			 */
			jupiterSetVoices(synth);
			break;
		case 1:
			if (jupiterDebug(synth, 1))
				printf("Uni Mode\n");
			/*
			 * Set X voices depending on mode, unison all of them
			 */
			jupiterSetVoices(synth);
			break;
		case 2:
			if (jupiterDebug(synth, 1))
				printf("Poly-1 Mode\n");
			/*
			 * Set X voices depending on mode, no Unison
			 */
			jupiterSetVoices(synth);
			break;
		case 3:
			if (jupiterDebug(synth, 1))
				printf("Poly-2 Mode\n");
			/*
			 * Not implemented
			 */
			jupiterSetVoices(synth);
			break;
	}

	return;
}

static void
jupiterSetSplit(guiSynth *synth)
{
	int lsp, usp;

	if (jupiterDebug(synth, 1))
		printf("setSplit: %i\n", (int) synth->mem.param[SPLIT_POINT]);

	if (mode == MODE_SINGLE)
		/* We could consider clearing any splitpoints here as well */
		return;

	/*
	 * Find and apply split points
	 * We need to consider DE-121 for multiple splits.
	 */
	if (mode == MODE_SPLIT) {
		lsp = synth->mem.param[SPLIT_POINT] + synth->transpose;
		usp = lsp + 1;
	} else {
		lsp = 127;
		usp = 0;
	}

	if (global.libtest == 0) {
		bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
			BRISTOL_HIGHKEY|lsp);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
			BRISTOL_LOWKEY|usp);
	}

	setsplit = 0;
}

/*
 * Dual, split and layer. One button needs to be on constantly
 */
static void
jupiterKeyMode(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (jupiterDebug(synth, 1))
		printf("jupiterKeyMode(%i, %i)\n", c, v);

	event.type = BRIGHTON_FLOAT;
	/*
	 * Force exclusion
	 */
	if (synth->dispatch[KEY_MODE].other2)
	{
		synth->dispatch[KEY_MODE].other2 = 0;
		return;
	}

	if (v == 0)
	{
		if (synth->dispatch[KEY_MODE].other1 == c)
		{
			event.value = 1;
			brightonParamChange(synth->win, synth->panel, c, &event);
		} else
			return;
	}

	if (synth->dispatch[KEY_MODE].other1 != -1)
	{
		synth->dispatch[KEY_MODE].other2 = 1;

		if (synth->dispatch[KEY_MODE].other1 != c)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[KEY_MODE].other1, &event);
	}

	synth->dispatch[KEY_MODE].other1 = c;

	/*
	 * Do what needs to be done
	 */
	switch (o) {
		case 0:
			if (jupiterDebug(synth, 1))
				printf("Dual mode\n");
			mode = MODE_DUAL;

			/*
			 * Configure split points to cover whole keyboard, halve notes on
			 * lower layer or 1 if Solo.
			 */
			bristolMidiSendMsg(fd, synth->sid, 126, 2, 0);

			if (global.libtest == 0) {
				bristolMidiSendMsg(global.controlfd, synth->sid,
					127, 0, BRISTOL_HIGHKEY|127);
				bristolMidiSendMsg(global.controlfd, synth->sid2,
					127, 0, BRISTOL_LOWKEY|0);
			}
			jupiterSetVoices(synth);
			break;
		case 1:
			if (jupiterDebug(synth, 1))
				printf("Split mode\n");
			mode = MODE_SPLIT;

			/*
			 * Configure split points to cover whole keyboard, halve notes on
			 * lower layer or 1 if Solo.. If double click then clear splitpoint
			 * until next key.
			 */
			bristolMidiSendMsg(fd, synth->sid, 126, 2, 0);
			
			if (brightonDoubleClick(dc1)) {
				if (synth->flags & MEM_LOADING)
					return;

				if (jupiterDebug(synth, 1))
					printf("Split mode\n");

				synth->mem.param[SPLIT_POINT] = 0;
				setsplit = 1;
				return;
			}
			jupiterSetSplit(synth);
			jupiterSetVoices(synth);
			break;
		case 2:
			if (jupiterDebug(synth, 1))
				printf("All mode\n");
			mode = MODE_SINGLE;

			/*
			 * Move upper split out of range, increase notes on lower layer
			 * or 1 if Solo.
			 */
			bristolMidiSendMsg(fd, synth->sid, 126, 2, 1);

			if (global.libtest == 0) {
				bristolMidiSendMsg(global.controlfd, synth->sid,
					127, 0, BRISTOL_HIGHKEY|127);
				bristolMidiSendMsg(global.controlfd, synth->sid2,
					127, 0, BRISTOL_LOWKEY|127);
			}
			jupiterSetVoices(synth);
			break;
	}

	return;
}

static void
jupiterBend(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (jupiterDebug(synth, 4))
		printf("bend %i\n", v);
	if (global.libtest == 0)
		bristolMidiControl(global.controlfd, synth->midichannel, 0, 1, v >> 7);
}

static void
jupiterGlide(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (jupiterDebug(synth, 4))
		printf("glide %i %f to %i/%i\n", v, synth->mem.param[LAYER_DEVS + 11],
			synth->sid, synth->sid2);

	/*
	 * Glide can be up/off/on, which I don't like (since it is in the memory)
	 * and may go for up/both/lower instead.
	 */
	if (synth->mem.param[LAYER_DEVS + 11] == 2) {
		/*
		 * Upper only
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 0, 0);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 0,
			(int) (synth->mem.param[LAYER_DEVS + 10] * C_RANGE_MIN_1));
	} else if (synth->mem.param[LAYER_DEVS + 11] == 1) {
		/*
		 * Both
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 0,
			(int) (synth->mem.param[LAYER_DEVS + 10] * C_RANGE_MIN_1));
		bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 0,
			(int) (synth->mem.param[LAYER_DEVS + 10] * C_RANGE_MIN_1));
	} else {
		/*
		 * Lower
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 0,
			(int) (synth->mem.param[LAYER_DEVS + 10] * C_RANGE_MIN_1));
		bristolMidiSendMsg(global.controlfd, synth->sid2, 126, 0, 0);
	}
}

static void
jupiterLFODelay(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (jupiterDebug(synth, 4))
		printf("LFODelay %i\n", v);

	jupiterDualSend(synth, synth->sid, 3, 0, v);
}

static void
jupiterUME(guiSynth *synth, int sid, int chan, int c, int o, int v)
{
	if (o == 40)
		bristolMidiSendMsg(global.controlfd, synth->sid, c, 40, v);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid2, c, 40, v);
}

/*
 * This is a conditional dual send, it first checks for the Mod enable flags
 * before sending the codes.
 */
static void
jupiterDSM(guiSynth *synth, int sid, int c, int o, int v)
{
	if (--v < 0)
		v = 0;

	bristolMidiSendMsg(global.controlfd, synth->sid, c, o, v);
	bristolMidiSendMsg(global.controlfd, synth->sid2, c, o, v);
}

static void
jupiterLFOVCF(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float depth;

	if (jupiterDebug(synth, 4))
		printf("LFOVCF %i\n", v);

	depth = synth->mem.param[LAYER_DEVS + 3];

	if (synth->mem.param[LAYER_DEVS + 8] != 0)
		jupiterDSM(synth, synth->sid, 126, 27, depth * C_RANGE_MIN_1);
	else
		jupiterDSM(synth, synth->sid, 126, 27, 0); // VCF
}

static void
jupiterLFOVCO(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float depth;

	if (jupiterDebug(synth, 4))
		printf("LFOVCO %i\n", v);

	depth = synth->mem.param[LAYER_DEVS + 2];

	if (synth->mem.param[LAYER_DEVS + 7] != 0)
		jupiterDSM(synth, synth->sid, 126, 26, depth * C_RANGE_MIN_1);
	else
		jupiterDSM(synth, synth->sid, 126, 26, 0); // VCF
}

static void
jupiterBendVCF(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float depth;

	if (jupiterDebug(synth, 4))
		printf("BendVCF %i\n", v);

	depth = synth->mem.param[LAYER_DEVS + 1];

	if (synth->mem.param[LAYER_DEVS + 6] != 0)
		jupiterDSM(synth, synth->sid, 126, 25, depth * C_RANGE_MIN_1);
	else
		jupiterDSM(synth, synth->sid, 126, 25, 0); // VCF
}

static void
jupiterBendVCO(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float depth;

	if (jupiterDebug(synth, 4))
		printf("BendVCO %i\n", v);

	/*
	 * We expect 3 operators here, they are depth of bend plus direction.
	 * 65 = VCO, 70 = VCO1, 71 = VCO2.
	 */
	depth = synth->mem.param[LAYER_DEVS];

	if (synth->mem.param[LAYER_DEVS + 4] != 0)
		jupiterDSM(synth, synth->sid, 126, 23, depth * C_RANGE_MIN_1);
	else
		jupiterDSM(synth, synth->sid, 126, 23, 0); // VCO 1 level;

	if (synth->mem.param[LAYER_DEVS + 5] != 0)
		jupiterDSM(synth, synth->sid, 126, 24, depth * C_RANGE_MIN_1);
	else
		jupiterDSM(synth, synth->sid, 126, 24, 0); // VCO 2 level;
}

static void
jupiterVolume(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float volume = synth->mem.param[101];
	float balance = synth->mem.param[102];

	if (jupiterDebug(synth, 4))
		printf("Volume %i\n", v);

	/*
	 * We have to consider layer balance and volume here, they multiply out.
	 * These are linear controls. They may become constand power curves but
	 * that reponse is not too great.
	 */
	bristolMidiSendMsg(fd, synth->sid, 126, 3,
		(int) (volume * (1.0 - balance) * C_RANGE_MIN_1));
	bristolMidiSendMsg(fd, synth->sid2, 126, 3,
		(int) (volume * balance * C_RANGE_MIN_1));
}

static void
jupiterPW(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (jupiterDebug(synth, 4))
		printf("PW %i\n", v);

	jupiterMidiSendMsg(synth, fd, chan, 4, 7, v);
	jupiterMidiSendMsg(synth, fd, chan, 5, 7, v);
}

static void
jupiterLFOwave(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	jupiterMidiSendMsg(synth, fd, chan, 126, 12 + v, 1);
}

static void
jupiterTranspose(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (v > 2)
		v = 2;

	if (c == 0)
	{
		if (jupiterDebug(synth, 2))
			printf("lower transpose %i\n", (v - 1) * 12);
		bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
			BRISTOL_TRANSPOSE + ((v - 1) * 12));
	} else {
		if (jupiterDebug(synth, 2))
			printf("upper transpose %i\n", (v - 1) * 12);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 127, 0,
			BRISTOL_TRANSPOSE + ((v - 1) * 12));
	}
}

static void
jupiterChord(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int sid = synth->sid;

	if (jupiterDebug(synth, 1))
		printf("Chord request: %i\n", v);

	if (c == 81)
		sid = synth->sid2;

	if (synth->mem.param[FUNCTION] != 0)
	{
		if  (v != 0)
		{
			if ((synth->flags & OPERATIONAL) == 0)
				return;
			if (synth->flags & MEM_LOADING)
				return;

			if (synth->seq1.param == NULL)
				loadSequence(&synth->seq1, "jupiter8", 0, 0);
			if (synth->seq2.param == NULL)
				loadSequence(&synth->seq2, "jupiter8", 0, BRISTOL_SID2);

			chordLearn = 1;

			if (jupiterDebug(synth, 0))
				printf("Chord learn requested %x\n",  synth->flags);

			if (c == 81)
				synth->seq2.param[1] = 0;
			else
				synth->seq1.param[1] = 0;

			/*
			 * This is to start a re-seq of the chord.
			 */
			bristolMidiSendMsg(global.controlfd, sid,
				BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, 0);
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
	}

	bristolMidiSendMsg(global.controlfd, sid,
		BRISTOL_ARPEGGIATOR, BRISTOL_CHORD_ENABLE, v);
}

static void
jupiterFilter(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int sid;

	if ((synth->mem.param[PANEL_SELECT] == 0) || (mode == MODE_SINGLE))
		sid = synth->sid;
	else
		sid = synth->sid2;

	/*
	 * The preferred filter is the Houvilainen however as of 0.20.8 this only
	 * does LPF. For the HPF and BPF we have to revert back to the previous
	 * chaimberlains
	 */
	switch (v) {
		case 0: /* LP - 24 dB */
			bristolMidiSendMsg(global.controlfd, sid, 6, 4, 4);
			bristolMidiSendMsg(global.controlfd, sid, 6, 6, 0);
			break;
		case 1:
			bristolMidiSendMsg(global.controlfd, sid, 6, 4, 0);
			bristolMidiSendMsg(global.controlfd, sid, 6, 6, 1);
			break;
		case 2:
			bristolMidiSendMsg(global.controlfd, sid, 6, 4, 0);
			bristolMidiSendMsg(global.controlfd, sid, 6, 6, 2);
			break;
	}
}

static void
jupiterGlobalTune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * Configures all the tune settings to zero (ie, 0.5).
	 *
	 * 14 is the fine tune of osc-2. 12 and 13 are also potentials here but
	 * they are more associated with sound than tuning. 74 is global tuning.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, c, o, v);
	bristolMidiSendMsg(global.controlfd, synth->sid2, c, o, v);
}

static void
jupiterTune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	/*
	 * Configures all the tune settings to zero (ie, 0.5).
	 *
	 * 14 is the fine tune of osc-2. 12 and 13 are also potentials here but
	 * they are more associated with sound than tuning. 74 is global tuning.
	 */
	event.type = BRIGHTON_FLOAT;
	event.value = 0.5;
	brightonParamChange(synth->win, synth->panel, 23, &event);
	brightonParamChange(synth->win, synth->panel, 103, &event);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
jupiterInit(brightonWindow *win)
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

	printf("Initialise the jupiter link to bristol: %p\n", synth->win);

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

	/* If we have a default voice count then limit it */
	if (synth->voices == BRISTOL_VOICECOUNT) {
		synth->voices = 8;
		((guiSynth *) synth->second)->voices = 4;
	} else
		((guiSynth *) synth->second)->voices = synth->voices >> 1;

	pcache = (float *) brightonmalloc(LAYER_DEVS * sizeof(float));
	scache = (float *) brightonmalloc(LAYER_DEVS * sizeof(float));

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets.
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		int v = synth->voices;

		bcopy(&global, &manual, sizeof(guimain));

		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

		/*
		 * We have to tweak the voicecount as we use the same synth definition
		 * to request the second layer
		 */
		synth->voices = synth->voices >> 1;
		manual.flags |= BRISTOL_CONN_FORCE|BRIGHTON_NOENGINE;
		manual.port = global.port;
		manual.host = global.host;

		if ((synth->sid2 = initConnection(&manual, synth)) < 0)
			return(-1);

		global.manualfd = manual.controlfd;
		global.manual = &manual;
		manual.manual = &global;

		synth->voices = v;
	}

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		/* Default all controllers to send a message that is ignored */
		synth->dispatch[i].controller = 126;
		synth->dispatch[i].operator = 101;
		synth->dispatch[i].routine = (synthRoutine) jupiterMidiSendMsg;
	}

	/* LFO-1 rate and delay */
	dispatch[0].controller = 0;
	dispatch[0].operator = 0;
	dispatch[1].controller = 2;
	dispatch[1].operator = 0;
	dispatch[2].routine = (synthRoutine) jupiterLFOwave;

	/* Mods */
	dispatch[3].controller = 126;
	dispatch[3].operator = 9;
	dispatch[4].controller = 126;
	dispatch[4].operator = 10;
	dispatch[5].controller = 126;
	dispatch[5].operator = 11;

	dispatch[6].controller = 126;
	dispatch[6].operator = 16;
	dispatch[7].controller = 126;
	dispatch[7].operator = 17;

	/* VCO-1 */
	dispatch[8].controller = 126;
	dispatch[8].operator = 8;
	dispatch[9].controller = 4;
	dispatch[9].operator = 3;
	dispatch[10].controller = 4;
	dispatch[10].operator = 2;
	dispatch[11].controller = 4;
	dispatch[11].operator = 1;
	dispatch[12].controller = 4;
	dispatch[12].operator = 0;
	dispatch[13].controller = 4;
	dispatch[13].operator = 4;
	dispatch[14].controller = 4;
	dispatch[14].operator = 5;
	dispatch[15].controller = 4;
	dispatch[15].operator = 6;
	dispatch[16].controller = 4;
	dispatch[16].operator = 11;
	dispatch[17].controller = 126;
	dispatch[17].operator = 38; /* Sync */

	/* VCO-2 */
	dispatch[18].controller = 5;
	dispatch[18].operator = 12;
	dispatch[19].controller = 5;
	dispatch[19].operator = 3;
	dispatch[20].controller = 5;
	dispatch[20].operator = 2;
	dispatch[21].controller = 5;
	dispatch[21].operator = 1;
	dispatch[22].controller = 5;
	dispatch[22].operator = 0;

	dispatch[23].controller = 5;
	dispatch[23].operator = 9;

	dispatch[24].controller = 5;
	dispatch[24].operator = 4;
	dispatch[25].controller = 5;
	dispatch[25].operator = 5;
	dispatch[26].controller = 5;
	dispatch[26].operator = 6;

	/* Noise */
	dispatch[27].controller = 126;
	dispatch[27].operator = 5;

	/* MIX */
	dispatch[28].controller = 126;
	dispatch[28].operator = 6;

	/* HPF */
	dispatch[29].controller = 11;
	dispatch[29].operator = 0;

	/* VCF */
	dispatch[30].controller = 6;
	dispatch[30].operator = 0;
	dispatch[31].controller = 6;
	dispatch[31].operator = 1;
	dispatch[32].controller = 6; /* Filter type - make this LP/HP */
	dispatch[32].operator = 6;
	dispatch[32].routine = (synthRoutine) jupiterFilter;
	dispatch[33].controller = 126;
	dispatch[33].operator = 18; /* Env amount */
	dispatch[34].controller = 126;
	dispatch[34].operator = 19; /* Env 1/2 */
	dispatch[35].controller = 126;
	dispatch[35].operator = 20; /* LFO amount */
	dispatch[36].controller = 6;
	dispatch[36].operator = 3; /* KBD */

	/* Amp */
	dispatch[37].controller = 126;
	dispatch[37].operator = 21; /* Env to amp */
	dispatch[38].controller = 126;
	dispatch[38].operator = 22; /* LFO to AMP */

	/* ADSR-1 */
	dispatch[39].controller = 7;
	dispatch[39].operator = 0;
	dispatch[40].controller = 7;
	dispatch[40].operator = 1;
	dispatch[41].controller = 7;
	dispatch[41].operator = 2;
	dispatch[42].controller = 7;
	dispatch[42].operator = 3;
	dispatch[43].controller = 7;
	dispatch[43].operator = 5; /* Velocity tracking */
	dispatch[44].controller = 126;
	dispatch[44].operator = 39; /* Env Invert */

	/* ADSR-2 */
	dispatch[45].controller = 9;
	dispatch[45].operator = 0;
	dispatch[46].controller = 9;
	dispatch[46].operator = 1;
	dispatch[47].controller = 9;
	dispatch[47].operator = 2;
	dispatch[48].controller = 9;
	dispatch[48].operator = 3;
	dispatch[49].controller = 9;
	dispatch[49].operator = 5; /* Velocity tracking */
	/* PW Both Osc */
	dispatch[50].controller = 9;
	dispatch[50].operator = 5; /* PW */
	dispatch[50].routine = (synthRoutine) jupiterPW;
	dispatch[51].controller = 126;
	dispatch[51].operator = 30; /* XMOD env */
	/* Pan */
	dispatch[52].controller = 126;
	dispatch[52].operator = 4; /* Layer panning */

	dispatch[53].controller = 53;
	dispatch[53].operator = 0;
	dispatch[53].routine = (synthRoutine) jupiterArpegRange;
	dispatch[54].controller = 54;
	dispatch[54].operator = 1;
	dispatch[54].routine = (synthRoutine) jupiterArpegRange;
	dispatch[55].controller = 55;
	dispatch[55].operator = 2;
	dispatch[55].routine = (synthRoutine) jupiterArpegRange;
	dispatch[56].controller = 56;
	dispatch[56].operator = 3;
	dispatch[56].routine = (synthRoutine) jupiterArpegRange;

	dispatch[57].controller = 57;
	dispatch[57].operator = 2; // 0 is down will be 
	dispatch[57].routine = (synthRoutine) jupiterArpegMode;
	dispatch[58].controller = 58;
	dispatch[58].operator = 0; // 1 is up/down
	dispatch[58].routine = (synthRoutine) jupiterArpegMode;
	dispatch[59].controller = 59;
	dispatch[59].operator = 1; // 2 is up 
	dispatch[59].routine = (synthRoutine) jupiterArpegMode;
	dispatch[60].controller = 60;
	dispatch[60].operator = 3;
	dispatch[60].routine = (synthRoutine) jupiterArpegMode;

	/* Uni, Solo, Poly 1&2 */
	dispatch[61].controller = 61;
	dispatch[61].operator = 0;
	dispatch[61].routine = (synthRoutine) jupiterUniPoly;
	dispatch[62].controller = 62;
	dispatch[62].operator = 1;
	dispatch[62].routine = (synthRoutine) jupiterUniPoly;
	dispatch[63].controller = 63;
	dispatch[63].operator = 2;
	dispatch[63].routine = (synthRoutine) jupiterUniPoly;
	dispatch[64].controller = 64;
	dispatch[64].operator = 3;
	dispatch[64].routine = (synthRoutine) jupiterUniPoly;

	/* Layer Devs were 65 - start with Bend to VCO */
	dispatch[LAYER_DEVS + 0].controller = 0;
	dispatch[LAYER_DEVS + 0].operator = 0;
	dispatch[LAYER_DEVS + 0].routine = (synthRoutine) jupiterBendVCO;
	dispatch[LAYER_DEVS + 4].controller = 0;
	dispatch[LAYER_DEVS + 4].operator = 1;
	dispatch[LAYER_DEVS + 4].routine = (synthRoutine) jupiterBendVCO;
	dispatch[LAYER_DEVS + 5].controller = 0;
	dispatch[LAYER_DEVS + 5].operator = 2;
	dispatch[LAYER_DEVS + 5].routine = (synthRoutine) jupiterBendVCO;

	/* Bend to VCF */
	dispatch[LAYER_DEVS + 1].controller = 0;
	dispatch[LAYER_DEVS + 1].operator = 0;
	dispatch[LAYER_DEVS + 1].routine = (synthRoutine) jupiterBendVCF;
	dispatch[LAYER_DEVS + 6].controller = 0;
	dispatch[LAYER_DEVS + 6].operator = 0;
	dispatch[LAYER_DEVS + 6].routine = (synthRoutine) jupiterBendVCF;

	/* LFO to VC) */
	dispatch[LAYER_DEVS + 2].controller = 0;
	dispatch[LAYER_DEVS + 2].operator = 0;
	dispatch[LAYER_DEVS + 2].routine = (synthRoutine) jupiterLFOVCO;
	dispatch[LAYER_DEVS + 7].controller = 0;
	dispatch[LAYER_DEVS + 7].operator = 0;
	dispatch[LAYER_DEVS + 7].routine = (synthRoutine) jupiterLFOVCO;

	/* LFO to VCF */
	dispatch[LAYER_DEVS + 3].controller = 0;
	dispatch[LAYER_DEVS + 3].operator = 0;
	dispatch[LAYER_DEVS + 3].routine = (synthRoutine) jupiterLFOVCF;
	dispatch[LAYER_DEVS + 8].controller = 0;
	dispatch[LAYER_DEVS + 8].operator = 0;
	dispatch[LAYER_DEVS + 8].routine = (synthRoutine) jupiterLFOVCF;

	dispatch[LAYER_DEVS + 9].routine = (synthRoutine) jupiterLFODelay;
	dispatch[LAYER_DEVS + 10].routine = (synthRoutine) jupiterGlide;
	dispatch[LAYER_DEVS + 11].routine = (synthRoutine) jupiterGlide;

	dispatch[LAYER_DEVS + 12].controller = 0;
	dispatch[LAYER_DEVS + 12].routine = (synthRoutine) jupiterTranspose;
	dispatch[LAYER_DEVS + 13].controller = 1;
	dispatch[LAYER_DEVS + 13].routine = (synthRoutine) jupiterTranspose;

	dispatch[LAYER_DEVS + 14].controller = 0;
	dispatch[LAYER_DEVS + 14].operator = 2;
	dispatch[LAYER_DEVS + 14].routine = (synthRoutine) jupiterBend;

	/* Hold/Chord */
	dispatch[92].controller = BRISTOL_ARPEGGIATOR;
	dispatch[92].operator = BRISTOL_CHORD_ENABLE;
	dispatch[92].routine = (synthRoutine) jupiterChord;
	dispatch[93].controller = BRISTOL_ARPEGGIATOR;
	dispatch[93].operator = BRISTOL_CHORD_ENABLE;
	dispatch[93].routine = (synthRoutine) jupiterChord;

	dispatch[MOD_ENABLE].controller = 126;
	dispatch[MOD_ENABLE].operator = 40;
	dispatch[MOD_ENABLE].routine = (synthRoutine) jupiterUME;
	dispatch[MOD_ENABLE + 1].controller = 126;
	dispatch[MOD_ENABLE + 1].operator = 41;
	dispatch[MOD_ENABLE + 1].routine = (synthRoutine) jupiterUME;

	/* Dual, split, layer */
	dispatch[96].controller = 96;
	dispatch[96].operator = 0;
	dispatch[96].routine = (synthRoutine) jupiterKeyMode;
	dispatch[97].controller = 97;
	dispatch[97].operator = 1;
	dispatch[97].routine = (synthRoutine) jupiterKeyMode;
	dispatch[98].controller = 98;
	dispatch[98].operator = 2;
	dispatch[98].routine = (synthRoutine) jupiterKeyMode;

	/* Volume/Balance */
	dispatch[101].controller = 126;
	dispatch[101].operator = 3;
	dispatch[101].routine = (synthRoutine) jupiterVolume;
	dispatch[102].controller = 126; /* This is not pan rather layer balance */
	dispatch[102].operator = 4;
	dispatch[102].routine = (synthRoutine) jupiterVolume;

	/* Arpeggio rate and clock source (latter not supported) */
	dispatch[99].controller = BRISTOL_ARPEGGIATOR;
	dispatch[99].operator = BRISTOL_ARPEG_RATE;
	dispatch[99].routine = (synthRoutine) jupiterArpegRate;

	/* Tuning and Tune */
	dispatch[103].controller = 126;
	dispatch[103].operator = 1;
	dispatch[103].routine = (synthRoutine) jupiterGlobalTune;
	dispatch[104].routine = (synthRoutine) jupiterTune;

	dispatch[SEQ_MODE].routine = (synthRoutine) jupiterSeqMode;

	/* 105 */
	dispatch[PANEL_SELECT].routine = (synthRoutine) jupiterPanelSwitch;

	/*
	 * Memory management
	 *
	 * Two sets of locations - 8 bank and 8 memory, then a load and a save
	 * button. Save will be doubleclick requirement. the selectors will be
	 * radio buttons.
	 * There are also issues with the dual layering that needs to be included.
	 * This file was copied from the Juno, but the interface is more like the
	 * prophet10 implementation so will have to borrow code from there.
	 */
	dispatch[MEM_MGT + 0].operator = 0;
	dispatch[MEM_MGT + 1].operator = 1;
	dispatch[MEM_MGT + 2].operator = 2;
	dispatch[MEM_MGT + 3].operator = 3;
	dispatch[MEM_MGT + 4].operator = 4;
	dispatch[MEM_MGT + 5].operator = 5;
	dispatch[MEM_MGT + 6].operator = 6;
	dispatch[MEM_MGT + 7].operator = 7;
	dispatch[MEM_MGT + 0].controller =
		dispatch[MEM_MGT + 1].controller =
		dispatch[MEM_MGT + 2].controller =
		dispatch[MEM_MGT + 3].controller =
		dispatch[MEM_MGT + 4].controller =
		dispatch[MEM_MGT + 5].controller =
		dispatch[MEM_MGT + 6].controller =
		dispatch[MEM_MGT + 7].controller = 0;

	/* Load button */
	dispatch[MEM_MGT + 8].operator = 0;
	dispatch[MEM_MGT + 8].controller = 2;
	dispatch[MEM_MGT + 8].routine = (synthRoutine) jupiterMemory;

	/* Bank selectors */
	dispatch[MEM_MGT + 9].operator = 0;
	dispatch[MEM_MGT + 10].operator = 1;
	dispatch[MEM_MGT + 11].operator = 2;
	dispatch[MEM_MGT + 12].operator = 3;
	dispatch[MEM_MGT + 13].operator = 4;
	dispatch[MEM_MGT + 14].operator = 5;
	dispatch[MEM_MGT + 15].operator = 6;
	dispatch[MEM_MGT + 16].operator = 7;
	dispatch[MEM_MGT + 9].controller =
		dispatch[MEM_MGT + 10].controller =
		dispatch[MEM_MGT + 11].controller =
		dispatch[MEM_MGT + 12].controller =
		dispatch[MEM_MGT + 13].controller =
		dispatch[MEM_MGT + 14].controller =
		dispatch[MEM_MGT + 15].controller =
		dispatch[MEM_MGT + 16].controller = 1;

	/* Save button */
	dispatch[MEM_MGT + 17].operator = 0;
	dispatch[MEM_MGT + 17].controller = 3;
	dispatch[MEM_MGT + 17].routine = (synthRoutine) jupiterMemory;

	dispatch[MEM_MGT].routine = dispatch[MEM_MGT + 1].routine
		= dispatch[MEM_MGT + 2].routine = dispatch[MEM_MGT + 3].routine
		= dispatch[MEM_MGT + 4].routine = dispatch[MEM_MGT + 5].routine
		= dispatch[MEM_MGT + 6].routine = dispatch[MEM_MGT + 7].routine
		= dispatch[MEM_MGT + 9].routine = dispatch[MEM_MGT + 10].routine
		= dispatch[MEM_MGT + 11].routine = dispatch[MEM_MGT + 12].routine
		= dispatch[MEM_MGT + 13].routine = dispatch[MEM_MGT + 14].routine
		= dispatch[MEM_MGT + 15].routine = dispatch[MEM_MGT + 16].routine
			= (synthRoutine) jupiterButtonPanel;

	/* Midi management */
	dispatch[MIDI_MGT].controller = 1;
	dispatch[MIDI_MGT + 1].controller = 2;
	dispatch[MIDI_MGT].routine = dispatch[MIDI_MGT + 1].routine
			= (synthRoutine) jupiterMidi;
	dispatch[MIDI_MGT + 2].routine = (synthRoutine) midiRelease;

	dispatch[MIDI_MGT + 3].controller = 2;
	dispatch[MIDI_MGT + 3].routine = (synthRoutine) jupiterFunctionKey;

	/* Noise gain */
	jupiterDualSend(synth, synth->sid, 11, 0, 1024);
	/* LFO-1 ADSR */
	jupiterDualSend(synth, synth->sid, 2, 1, 12000);
	jupiterDualSend(synth, synth->sid, 2, 2, 16000);
	jupiterDualSend(synth, synth->sid, 2, 3, 1000);
	jupiterDualSend(synth, synth->sid, 2, 4, 16000);
	jupiterDualSend(synth, synth->sid, 2, 5, 0);
	jupiterDualSend(synth, synth->sid, 2, 9, 0);
	/* LFO-2 ADSR */
	jupiterDualSend(synth, synth->sid, 3, 1, 12000);
	jupiterDualSend(synth, synth->sid, 3, 2, 16000);
	jupiterDualSend(synth, synth->sid, 3, 3, 1000);
	jupiterDualSend(synth, synth->sid, 3, 4, 16000);
	jupiterDualSend(synth, synth->sid, 3, 5, 0);
	jupiterDualSend(synth, synth->sid, 3, 8, 0);
	/* LFO PW */
	jupiterDualSend(synth, synth->sid, 4, 7, 8192);
	jupiterDualSend(synth, synth->sid, 5, 7, 8192);
	/* ADSR gains */
	jupiterDualSend(synth, synth->sid, 7, 4, 16383);
	jupiterDualSend(synth, synth->sid, 9, 4, 16383);
	/* Filter Mod */
	jupiterDualSend(synth, synth->sid, 6, 2, 16383);
	jupiterDualSend(synth, synth->sid, 6, 4, 4);

	/* Fix some default pan and gain */
	jupiterDualSend(synth, synth->sid, 126, 3, 16383);
	jupiterDualSend(synth, synth->sid, 126, 4, 16383);
	jupiterDualSend(synth, synth->sid, 126, 5, 16383);
	jupiterDualSend(synth, synth->sid, 126, 6, 16383);
	/* Configure pink noise filter level */
	jupiterDualSend(synth, synth->sid, 10, 2, 4096);

	/* Configure PWM for both DCO
	jupiterDualSend(synth, synth->sid, 126, 33, 1);
	jupiterDualSend(synth, synth->sid, 126, 34, 1);
	*/

	dispatch[ARPEG_RANGE].other1 = -1;
	dispatch[POLY_MODE].other1 = -1;
	dispatch[KEY_MODE].other1 = -1;

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
jupiterConfigure(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;
	int offset;

	if (synth == 0)
	{
		printf("problems going operational\n");
		return(-1);
	}

	if (synth->flags & OPERATIONAL)
		return(0);

	printf("going operational\n");

	if (synth->location == 0)
	{
		synth->bank = 10;
		synth->location = 1;
	} else {
		if ((mw = synth->location / 10) == 0)
			synth->bank = 10;
		else {
			synth->bank = mw % 10;
			mw -= synth->bank;
			mw *= 10;
			synth->bank *= 10;
		}
		if ((synth->location = (synth->location % 10)) == 0)
			synth->location = 1;
	}

	if (jupiterDebug(synth, 2))
		printf("weight %i, bank %i, mem %i\n", mw, synth->bank,synth->location);

	configureGlobals(synth);

	synth->keypanel = KEY_PANEL;
	synth->keypanel2 = -1;
	synth->transpose = 12;

	((guiSynth *) synth->second)->resources = synth->resources;
	((guiSynth *) synth->second)->win = synth->win;
	((guiSynth *) synth->second)->bank = 10;
	((guiSynth *) synth->second)->location = 2;
	((guiSynth *) synth->second)->panel = 0;
	((guiSynth *) synth->second)->mem.active = ACTIVE_DEVS;
	((guiSynth *) synth->second)->transpose = 12;

	synth->flags |= OPERATIONAL;

	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;

	/* Push some radio buttons to set their starting point */
	event.value = 1.0;
	brightonParamChange(synth->win, 0, ARPEG_RANGE, &event);
	brightonParamChange(synth->win, 0, ARPEG_MODE, &event);
	brightonParamChange(synth->win, 0, POLY_MODE, &event);
	brightonParamChange(synth->win, 0, KEY_MODE + 2, &event);

	dc1 = brightonGetDCTimer(win->dcTimeout);

	/* Set up the load buttons for an initial memory load */
	if ((offset = synth->location - 1) > 7) offset = 0;
	if (offset < 0) offset = 0;
	brightonParamChange(synth->win, 0, PATCH_BUTTONS + offset, &event);
	if ((offset = synth->bank - 1) > 7) offset = 0;
	if (offset < 0) offset = 0;
	brightonParamChange(synth->win, 0, BANK_BUTTONS + offset, &event);
	/* And this will appear as a double click */
	jupiterMemory(synth, global.controlfd, synth->sid, 2, 0, 1);
	usleep(100000);
	jupiterMemory(synth, global.controlfd, synth->sid, 2, 0, 1);

	if (jupiterDebug(synth, 5)) {
		int i;

		for (i = 0; i < LAYER_DEVS; i+= 8)
			printf("%f %f %f %f %f %f %f %f\n",
				pcache[i], pcache[i + 1], pcache[i + 2], pcache[i + 3],
				pcache[i + 4], pcache[i + 5], pcache[i + 6], pcache[i + 7]);
		printf("\n");
		for (i = 0; i < LAYER_DEVS; i+= 8)
			printf("%f %f %f %f %f %f %f %f\n",
				scache[i], scache[i + 1], scache[i + 2], scache[i + 3],
				scache[i + 4], scache[i + 5], scache[i + 6], scache[i + 7]);
	}

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);

	brightonPut(win,
		"bitmaps/blueprints/jupitershade.xpm", 0, 0, win->width, win->height);
	width = win->width;

	/* Then set some defaults for volume, balance and tuning */
	event.type = BRIGHTON_FLOAT;
	event.value = 0.9;
	brightonParamChange(synth->win, 0, 101, &event);
	event.value = 0.5;
	brightonParamChange(synth->win, 0, 102, &event);
	brightonParamChange(synth->win, 0, 103, &event);

	if (jupiterDebug(synth, 3))
		printf("dct = %i\n", win->dcTimeout);

	return(0);
}

