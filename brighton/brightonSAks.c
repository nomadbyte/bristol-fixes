
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

/* Synthi Sound alike */

#include <fcntl.h>
#include <stdlib.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int sAksInit();
static int sAksConfigure();
static int sAksCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define DEVICE_COUNT (81 + 256)
#define ACTIVE_DEVS (59 + 256)
#define DISPLAY (DEVICE_COUNT - 1)

#define MEM_START (ACTIVE_DEVS + 5)
#define MIDI_START (MEM_START + 14)

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
 * This example is for a sAksBristol type synth interface.
 */

#define CD1 88

#define CC1 83
#define CC2 (CC1 + CD1)
#define CC3 (CC2 + CD1)
#define CC4 (CC3 + CD1)
#define CC5 (CC4 + CD1)
#define CC6 (CC5 + CD1)
#define CC7 (CC6 + CD1)
#define CC8 (CC7 + CD1)
#define CC9 (CC8 + CD1)
#define CC10 (CC9 + CD1)

#define CC4b (CC3 + CD1 + 45)
#define CC5b (CC4 + CD1 + 45)
#define CC6b (CC5 + CD1 + 45)
#define CC7b (CC6 + CD1 + 45)

#define RD1 140
#define RD2 190
#define R0 129
#define R1 (R0 + RD1)
#define R2 (R1 + RD1)
#define R3 (R2 + RD1)
#define R4 (R3 + RD1)
#define R5 (R4 + RD1)

#define S1 75
#define S2 75
#define S3 95
#define S4 25

#define PSPX 508
#define PSPY 500

#define PS1 ((float) 250 / 16.1f)
#define PS2 ((float) 349 / 16.1f)

#define PATCHBUS(y) \
	{"", 2, PSPX + PS1 * 0 - 1, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", 0 , BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 1, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 2, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 3, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 4, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 5, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 6, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 7, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 8, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 9, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 10, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 11, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 12, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 13, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 14, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\
	{"", 2, PSPX + PS1 * 15, PSPY + PS2 * y, PS1, PS2, 0, 2, 0, \
		"patchonSA", "patchonSA", BRIGHTON_NOSHADOW|BRIGHTON_FIVEWAY},\

static brightonLocations locations[DEVICE_COUNT] = {
	/* 0 Filter Check controller 50 that adjusts filter/fine. */
	{"", 0, CC4b, R0, S1 - 3, S2 - 3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC5b, R0, S1, S2, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC6b, R0, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* Ring Mod */
	{"", 0, CC7b, R0, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 4 osc 1 Inner and outer tuners first. */
	{"", 0, CC1 + 1, R1 + 0, S1 - 3, S2 - 3, 0, 1, 0,
		"bitmaps/knobs/knobbluenew.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOSHADOW},
	{"", 0, CC1 - 7, R1 - 10, S3, S3, 0, 1, 0,
		"bitmaps/knobs/knobhollownew.xpm", 0, 0},
	{"", 0, CC2, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC3, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC4, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 9 Env */
	{"", 0, CC5, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC6, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC7, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC8, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC9, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC10, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 15 Osc 2 */
	{"", 0, CC1 + 1, R2 + 0, S1 - 3, S2 - 3, 0, 1, 0,
		"bitmaps/knobs/knobbluenew.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOSHADOW},
	{"", 0, CC1 - 7, R2 - 10, S3, S3, 0, 1, 0,
		"bitmaps/knobs/knobhollownew.xpm", 0, 0},
	{"", 0, CC2, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC3, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC4, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 0, CC9, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC10, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 22 Osc 3 */
	{"", 0, CC1 + 1, R3 + 0, S1 - 3, S2 - 3, 0, 1, 0,
		"bitmaps/knobs/knobbluenew.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOSHADOW},
	{"", 0, CC1 - 7, R3 - 10, S3, S3, 0, 1, 0,
		"bitmaps/knobs/knobhollownew.xpm", 0, 0},
	{"", 0, CC2, R3, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC3, R3, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC4, R3, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 0, CC9, R3, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC10, R3, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* Noise 29 */
	{"", 0, CC1, R4, S1, S2, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC2, R4, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 0, CC3, R4, S1, S2, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC4, R4, S1, S2, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 0, CC9 - 15, R4, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC10, R4, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 35 */
	{"", 0, CC1, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, CC2, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, CC3, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 39 - Filter Fine. */
	/*{"", 0, CC4b, R0, S1, S2, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",  */
	{"", 0, CC4b - 7, R0 - 10, S3, S3, 0, 1, 0,
		"bitmaps/knobs/knobhollownew.xpm", 0, 0},

	/* 40 - 19 Dummies for later use. */
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 923, 890, 40, 40, 0, 1.0, 0, "bitmaps/knobs/knob2.xpm", 
		0, BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 923, 890, 40, 40, 0, 1.0, 0, "bitmaps/knobs/knob2.xpm", 
		0, BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, CC4, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},

	/* From 59 - the patch matrix */
	PATCHBUS(0.0f)
	PATCHBUS(1.0f)
	PATCHBUS(2.0f)
	PATCHBUS(3.0f)
	PATCHBUS(4.0f)
	PATCHBUS(5.0f)
	PATCHBUS(6.0f)
	PATCHBUS(7.0f)
	PATCHBUS(8.0f)
	PATCHBUS(9.0f)
	PATCHBUS(10.0f)
	PATCHBUS(11.0f)
	PATCHBUS(12.0f)
	PATCHBUS(13.0f)
	PATCHBUS(14.0f)
	PATCHBUS(15.0f)

	/* 315 (59 + 256) Touch panel */
	{"", 5, 810, 814, 79, 128, 0, 1, 0, "bitmaps/images/sphere.xpm", 
		0, BRIGHTON_WIDE},
	{"", 5, 810, 814, 79, 128, 0, 1, 0, "bitmaps/images/sphere.xpm", 
		0, BRIGHTON_WITHDRAWN},

	/* 317 Monitor switches */
	{"", 2, 130, RD2, 37, 18, 0, 2, 0, "switch", 0,
		BRIGHTON_VERTICAL|BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"", 2, 850, RD2, 37, 18, 0, 2, 0, "switch", 0,
		BRIGHTON_VERTICAL|BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},

	/* 319 Trigger switch */
	{"", 2, 928, 850, 20, 30, 0, 1.01, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlo.xpm", BRIGHTON_NOSHADOW},

#define C18 230
#define C19 260

#define C20 306
#define C21 334
#define C22 362

#define R1_1 87
#define R21_0 120
#define R21_1 154
#define R22_1 186

#define S5 17
#define S6 20

	/* Memory/Midi */
	/* Load/Save */
	{"", 2, C20, R22_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnl.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C22, R22_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlo.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},

	/* 1 - 3 */
/*	{"", 2, C20, R1_1, S5, S6, 0, 1, 0, "bitmaps/digits/1.xpm", */
	{"", 2, C20, R1_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C21, R1_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C22, R1_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* 4 - 6 */
	{"", 2, C20, R21_0, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C21, R21_0, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C22, R21_0, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* 7 - 9 */
	{"", 2, C20, R21_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C21, R21_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C22, R21_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* 0 */
	{"", 2, C21, R22_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},

	/* mem/midi Up/down */
	{"", 2, C19, R21_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C19, R22_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},

	{"", 2, C18, R22_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C18, R21_1, S5, S6, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},

	{"", 3, 199, R1_1 + 23, 106, 25, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp sAksApp = {
	"aks",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	0,
	sAksInit,
	sAksConfigure, /* 3 callbacks, unused? */
	0,
	destroySynth,
	{1, 0, 2, 2, 5, 520, 0, 0},
	700, 500, 0, 0,
	1, /* Panels */
	{
		{
			"synthi",
			"bitmaps/blueprints/sAks.xpm",
			"bitmaps/textures/metal1.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			sAksCallback,
			0, 0, 1000, 1000,
			DEVICE_COUNT,
			locations
		},
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

static void
sAksReverb(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	v = v >> 1;

	bristolMidiSendMsg(fd, chan, 8, 0, v);
	bristolMidiSendMsg(fd, chan, 8, 1, v);
	bristolMidiSendMsg(fd, chan, 8, 2, v >> 1);
}

static void
aksManualStart(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
printf("trigger %i\n", v);
	if (v == 0)
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, 6);
	else
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, 6);
}

static int
sAksMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
/*printf("%i, %i, %i\n", c, o, v); */
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
sAksMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int cmem = synth->location;

/*printf("mem(%i, %i, %i)\n", c, o, v); */

	if (synth->flags & MEM_LOADING)
		return;

	switch (c) {
		default:
		case 0:
			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;
			if (loadMemory(synth, "aks", 0, synth->location,
				synth->mem.active, 0, BRISTOL_STAT) < 0)
				displayPanelText(synth, "FRE", synth->location, 0, DISPLAY);
			else
				displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			break;
		case 1:
			loadMemory(synth, "aks", 0, synth->location,
				synth->mem.active, 0, 0);
			displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			synth->location = 0;
			break;
		case 2:
			saveMemory(synth, "aks", 0, synth->location, 0);
			displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			synth->location = 0;
			break;
		case 3:
			while (loadMemory(synth, "aks", 0, --synth->location,
				synth->mem.active, 0, 0) < 0)
			{
				if (synth->location < 0)
					synth->location = 1000;
				if (synth->location == cmem)
					break;
			}
			displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			break;
		case 4:
			while (loadMemory(synth, "aks", 0, ++synth->location,
				synth->mem.active, 0, 0) < 0)
			{
				if (synth->location > 999)
					synth->location = -1;
				if (synth->location == cmem)
					break;
			}
			displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			break;
	}
}

static void
sAksPatchPanel(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int value = C_RANGE_MIN_1;

/*	if (v != 0) */
/*		printf("requested %i, %i, %i\n", c, o, v); */

	/*
	 * values:
	 *
	 *	4 = 5%
	 *	3 = -6dB
	 *	2 = 1%
	 *	1 = invert.
	 *	0 = 0
	 *
	 * These are sent such that the sent value of 0.5 is off. below that is
	 * negative, and above that tends to one.
	 */

	switch (v) {
		case 0:
			/* Value zero, send zero to turn pin off */
			bristolMidiSendMsg(fd, chan, c + 100, o, 0);
			break;
		case 1:
			/* when the value is 1 send 1, which will invert the signal. */
			/* Trust me, see the engine code for this synth. */
			bristolMidiSendMsg(fd, chan, c + 100, o, C_RANGE_MIN_1);
			break;
		case 2:
	 		/* Accurate mapping */
			value = C_RANGE_MIN_1 * 98 / 100
				+ ((C_RANGE_MIN_1 / 50) * (rand() >> 17)) / C_RANGE_MIN_1;
			bristolMidiSendMsg(fd, chan, c + 100, o, value * 19 / 20);
			break;
		case 3:
			/* damped by about -6dB */
			bristolMidiSendMsg(fd, chan, c + 100, o, (C_RANGE_MIN_1 >> 2));
			break;
		case 4:
			/*
			 * Some half random value, perhaps with a 10% margin to emulate the
			 * resistor accuracy of the original synth.
			 */
			value = C_RANGE_MIN_1 * 9 / 10
				+ ((C_RANGE_MIN_1 / 10) * (rand() >> 17)) / C_RANGE_MIN_1;
printf("requested %i, %i, %i\n", c, o, value);
			bristolMidiSendMsg(fd, chan, c + 100, o, value);
			break;
	}
}

static int
sAksMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			displayPanelText(synth, "MIDI", synth->midichannel + 1, 0, DISPLAY);
			return(0);
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 15)
		{
			synth->midichannel = 15;
			displayPanelText(synth, "MIDI", synth->midichannel + 1, 0, DISPLAY);
			return(0);
		}
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

	displayPanelText(synth, "MIDI", synth->midichannel + 1, 0, DISPLAY);

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
sAksCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*printf("sAksCallback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (sAksApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
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
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
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
sAksInit(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	dispatcher *dispatch;
	int i, in, out;

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

	printf("Initialise the sAks link to bristol: %p\n", synth->win);

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
	synth->voices = 1;
	if (!global.libtest)
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = sAksMidiSendMsg;

	/* Filter */
	dispatch[0].controller = 3;
	dispatch[0].operator = 0;
	dispatch[1].controller = 3;
	dispatch[1].operator = 1;
	dispatch[2].controller = 3;
	dispatch[2].operator = 5;

	/* Ring mod gain */
	dispatch[3].controller = 7;
	dispatch[3].operator = 2;

	/* Osc1 tuning */
	dispatch[4].controller = 0;
	dispatch[4].operator = 0;
	dispatch[5].controller = 0;
	dispatch[5].operator = 1;
	dispatch[6].controller = 0;
	dispatch[6].operator = 2;
	dispatch[7].controller = 0;
	dispatch[7].operator = 3;
	dispatch[8].controller = 0;
	dispatch[8].operator = 4;

	/* ENV controllers */
	dispatch[9].controller = 4;
	dispatch[9].operator = 0;
	dispatch[10].controller = 4;
	dispatch[10].operator = 1;
	dispatch[11].controller = 4;
	dispatch[11].operator = 2;
	dispatch[12].controller = 4;
	dispatch[12].operator = 3;
	dispatch[13].controller = 126;
	dispatch[13].operator = 16;
	dispatch[14].controller = 126;
	dispatch[14].operator = 17;

	/* Osc2 tuning */
	dispatch[15].controller = 1;
	dispatch[15].operator = 0;
	dispatch[16].controller = 1;
	dispatch[16].operator = 1;
	dispatch[17].controller = 1;
	dispatch[17].operator = 2;
	dispatch[18].controller = 1;
	dispatch[18].operator = 3;
	dispatch[19].controller = 1;
	dispatch[19].operator = 4;

	/* Reverb */
	dispatch[20].controller = 8;
	dispatch[20].operator = 0;
	/* This should be a compound controller for delay and feedback */
	dispatch[20].routine = (synthRoutine) sAksReverb;
	dispatch[21].controller = 8;
	dispatch[21].operator = 4;

	/* Osc3 tuning */
	dispatch[22].controller = 2;
	dispatch[22].operator = 0;
	dispatch[23].controller = 2;
	dispatch[23].operator = 1;
	dispatch[24].controller = 2;
	dispatch[24].operator = 2;
	dispatch[25].controller = 2;
	dispatch[25].operator = 3;
	dispatch[26].controller = 2;
	dispatch[26].operator = 4;

	/* Input gain */
	dispatch[27].controller = 126;
	dispatch[27].operator = 2;
	dispatch[28].controller = 126;
	dispatch[28].operator = 3;

	/* Noise */
	dispatch[29].controller = 6;
	dispatch[29].operator = 2;
	dispatch[30].controller = 6;
	dispatch[30].operator = 0;

	/* Output */
	dispatch[31].controller = 126;
	dispatch[31].operator = 4;
	dispatch[32].controller = 126;
	dispatch[32].operator = 5;

	/* Range */
	dispatch[33].controller = 126;
	dispatch[33].operator = 6;
	dispatch[34].controller = 126;
	dispatch[34].operator = 7;

	/* Mix */
	dispatch[35].controller = 126;
	dispatch[35].operator = 8;
	dispatch[36].controller = 126;
	dispatch[36].operator = 9;
	dispatch[37].controller = 126;
	dispatch[37].operator = 10;
	dispatch[38].controller = 126;
	dispatch[38].operator = 11;
	/* filter fine */
	dispatch[39].controller = 3;
	dispatch[39].operator = 2;

	/* Patch panel */
	i = 59;
	for (out = 0; out < 16; out++)
	{
		for (in = 0; in < 16; in++)
		{
			dispatch[i].controller = in;
			dispatch[i].operator = out;
			dispatch[i].routine = (synthRoutine) sAksPatchPanel;
			i++;
		}
	}

	dispatch[ACTIVE_DEVS + 0].controller = 126;
	dispatch[ACTIVE_DEVS + 0].operator = 12;
/*	dispatch[ACTIVE_DEVS + 0].routine = (synthRoutine) sAksHShift; */
	dispatch[ACTIVE_DEVS + 1].controller = 126;
	dispatch[ACTIVE_DEVS + 1].operator = 13;
/*	dispatch[ACTIVE_DEVS + 1].routine = (synthRoutine) sAksHShift; */

	/* Switches */
	dispatch[ACTIVE_DEVS + 2].controller = 126;
	dispatch[ACTIVE_DEVS + 2].operator = 14;
	dispatch[ACTIVE_DEVS + 3].controller = 126;
	dispatch[ACTIVE_DEVS + 3].operator = 15;

	dispatch[ACTIVE_DEVS + 4].routine = (synthRoutine) aksManualStart;

	dispatch[MEM_START +0].routine = dispatch[MEM_START +1].routine =
		dispatch[MEM_START +2].routine = dispatch[MEM_START +3].routine =
		dispatch[MEM_START +4].routine = dispatch[MEM_START +5].routine =
		dispatch[MEM_START +6].routine = dispatch[MEM_START +7].routine =
		dispatch[MEM_START +8].routine = dispatch[MEM_START +9].routine =
		dispatch[MEM_START +10].routine = dispatch[MEM_START +11].routine =
		dispatch[MEM_START +12].routine = dispatch[MEM_START +13].routine
			= (synthRoutine) sAksMemory;

	dispatch[MEM_START + 0].controller = 1;
	dispatch[MEM_START + 1].controller = 2;
	dispatch[MEM_START + 2].operator = 1;
	dispatch[MEM_START + 3].operator = 2;
	dispatch[MEM_START + 4].operator = 3;
	dispatch[MEM_START + 5].operator = 4;
	dispatch[MEM_START + 6].operator = 5;
	dispatch[MEM_START + 7].operator = 6;
	dispatch[MEM_START + 8].operator = 7;
	dispatch[MEM_START + 9].operator = 8;
	dispatch[MEM_START + 10].operator = 9;
	dispatch[MEM_START + 11].operator = 0;

	dispatch[MEM_START + 12].controller = 4;
	dispatch[MEM_START + 13].controller = 3;

	dispatch[MIDI_START].controller = 1;
	dispatch[MIDI_START + 1].controller = 2;
	dispatch[MIDI_START].routine = dispatch[MIDI_START + 1].routine =
		(synthRoutine) sAksMidi;

	/* Set the oscillator indeces */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 5, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 5, 2);
	/* Pink noise source (is variable with filter) */
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1, 1);
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 6, C_RANGE_MIN_1); */
	/* LFO Env parameters. */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, 12000); */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 2, 16383); */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 3, 0); */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 4, 16383); */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 5, 0); */

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
sAksConfigure(brightonWindow *win)
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
	synth->transpose = 48;

	synth->bank = 0;
	event.value = 1;
	/* Poly */
	loadMemory(synth, "aks", 0, synth->location, synth->mem.active, 0, 0);

/*	event.type = BRIGHTON_PARAMCHANGE; */
/*	event.value = 1.0; */
/*	brightonParamChange(synth->win, 0, ACTIVE_DEVS, &event); */
	configureGlobals(synth);

	return(0);
}

