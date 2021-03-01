
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

static int dxInit();
static int dxConfigure();
static int dxCallback(brightonWindow * , int, int, float);
static int dxMidiCallback(brightonWindow * , int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define DX7

#ifdef DX7
#define BLUEPRINT "bitmaps/blueprints/dx7.xpm"
#else
#define BLUEPRINT "bitmaps/blueprints/dx.xpm"
#endif

#define OP_COUNT 6
#define FIRST_DEV 0
#define PARAM_COUNT 20
#define ALGOS_COUNT 29
#define MEM_COUNT 17
#define ACTIVE_DEVS (PARAM_COUNT * OP_COUNT + ALGOS_COUNT + FIRST_DEV)

#define DISPLAY_DEV (MEM_COUNT - 3)
#define DISPLAY_PANEL 7

#define OP_START FIRST_DEV
#define OP1_START FIRST_DEV
#define OP2_START (OP1_START + PARAM_COUNT)
#define OP3_START (OP2_START + PARAM_COUNT)
#define OP4_START (OP3_START + PARAM_COUNT)
#define OP5_START (OP4_START + PARAM_COUNT)
#define OP6_START (OP5_START + PARAM_COUNT)
#define ALGO_START (PARAM_COUNT * OP_COUNT + FIRST_DEV)
#define MEM_START ACTIVE_DEVS

#define OP_PANEL 0
#define ALGOS_PANEL OP_COUNT
#define MEM_PANEL (ALGOS_PANEL + 1)

#define KEY_PANEL 8

#define DEVICE_COUNT (ACTIVE_DEVS + MEM_COUNT)

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
 * This example is for a dxBristol type synth interface.
 */
#define OP1XPM "bitmaps/images/op1.xpm"
#define OP2XPM "bitmaps/images/op2.xpm"
#define OP3XPM "bitmaps/images/op3.xpm"
#define OP4XPM "bitmaps/images/op4.xpm"
#define OP5XPM "bitmaps/images/op5.xpm"
#define OP6XPM "bitmaps/images/op6.xpm"

#define R1 150
#define R2 665

#define D3 90

#ifdef DX7
#define D1 123

#define C0 35
#define C1 (C0 + D1)
#define C2 (C1 + D1)
#define C3 (C2 + D1)
#define C4 (C3 + D1)
#define C5 (C4 + D1)
#define C6 (C5 + D1)
#define C7 (C6 + D1)
#else
#define D1 139

#define C0 35
#define C1 (C0 + D1)
#define C2 (C1 + D1)
#define C3 (C2 + D1)
#define C4 (C3 + D1)
#define C5 (C4 + D1)
#define C6 (C5 + D1)
#endif

#define S1 300
#define S2 40
#define S3 250

/*
 * This needs to be changed for a 7 parameter envelope, A-L1-A-L2-D-S-R.
 * Rework is non trivial, need to rework the env, which is ok, the GUI, also
 * easy, but it requires extra operator parameters.
 */
#ifdef DX7
static brightonLocations locations[PARAM_COUNT] = {
	/* in gain */
	{"", 0, C1, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* tune */
	{"", 0, C3, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	/* transpose */
	{"", 0, C4, R2, S1, S1, 0, 5, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* out gain */
	{"", 0, C2, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* lfo */
	{"", 2, C5, R2, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},

	/* attack */
	{"", 0, C1, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C5, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C6, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C7, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* touch */
	{"", 0, C0, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* pan */
	{"", 0, C7, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 
		BRIGHTON_NOTCH},

	/* igc, ogc */
	{"", 2, C5 + D3, R2, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C5 + D3 + D3, R2, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* L1-A-L2 */
	{"", 0, C2, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C3, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C4, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* spares */
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},

	{"", 4, 0, 200, 140, 250, 0, 1, 0, OP1XPM, 0, 0}
};
#else
static brightonLocations locations[PARAM_COUNT] = {
	/* in gain */
	{"", 0, C0, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	/* tune */
	{"", 0, C2, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knoborange.xpm", 0, 0},
	/* transpose */
	{"", 0, C3, R1, S1, S1, 0, 5, 0, "bitmaps/knobs/knoborange.xpm", 0, 0},
	/* out gain */
	{"", 0, C6, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},

	/* lfo */
	{"", 2, C4, R1, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},

	/* attack */
	{"", 0, C2, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, C3, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, C4, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, C5, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},

	/* touch */
	{"", 0, C1, R2, S1, S1, 0, 1, 0, 0, 0, 0},
	/* pan */
	{"", 0, C6, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knobyellow.xpm", 0, 0},

	/* igc, ogc */
	{"", 2, C4 + D3, R1, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, C4 + D3 + D3, R1, S2, S3, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* spares */
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", -1, C6, R1, S1, S1, 0, 1, 0, 0, 0, 0},

	{"", 4, 60, 100, 150, 300, 0, 1, 0, OP1XPM, 0, 0}
};
#endif

#define D2 50

#define S4 45
#define S5 250

#define AR1 150
#define AR2 425
#define AR3 700

#define AC0 50
#define AC1 (AC0 + D2)
#define AC2 (AC1 + D2)
#define AC3 (AC2 + D2)
#define AC4 (AC3 + D2)
#define AC5 (AC4 + D2)
#define AC6 (AC5 + D2)
#define AC7 (AC6 + D2)
#define AC8 (AC7 + D2 + 253)
#define AC9 (AC8 + D2 + 142)
#define AC10 (AC9 + D2 + 10)

static brightonLocations algopanel[ALGOS_COUNT] = {
	{"", 2, AC0, AR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC1, AR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC2, AR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC3, AR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC4, AR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC5, AR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC6, AR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC7, AR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC0, AR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC1, AR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC2, AR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC3, AR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC4, AR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC5, AR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC6, AR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC7, AR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC0, AR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC1, AR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC2, AR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC3, AR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC4, AR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC5, AR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC6, AR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, AC7, AR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 0, AC8, R1, S1 + 10, S1 + 10, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, AC8, R2, S1 + 10, S1 + 10, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* Volume */
	{"", 0, AC9 - 20, R2, S1 + 10, S1 + 10, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 2, AC9, R1 + 40, S4, S5, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 4, AC7 + 90, AR1, 190, 800, 0, 1, 0, 0, 0, 0},
};

#define MS1 75
#define MS2 200

#define MD1 210
#define MD2 80

#define MR0 100
#define MR1 (MR0 + MD1)
#define MR2 (MR1 + MD1)
#define MR3 (MR2 + MD1)

#define MC0 500
#define MC1 (MC0 + MD2)
#define MC2 (MC1 + MD2)

static brightonLocations mempanel[MEM_COUNT] = {
	/* Memory buttons from 0 */
	{"", 2, MC1, MR3, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	/* from 1 */
	{"", 2, MC0, MR0, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC1, MR0, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC2, MR0, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	/* from 4 */
	{"", 2, MC0, MR1, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC1, MR1, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC2, MR1, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	/* from 7 */
	{"", 2, MC0, MR2, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC1, MR2, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC2, MR2, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", BRIGHTON_CHECKBUTTON},

	/* load save */
	{"", 2, 290, MR3, 45, MS2, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 410, MR3, 45, MS2, 0, 1, 0,
		"bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},

	/* midi up/down */
	{"", 2, 60, MR3, 45, MS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 175, MR3, 45, MS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	{"", 3, 15, 100, 485, 310, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0},

	/* up/down */
	{"", 2, MC0, MR3, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnl.xpm",
		"bitmaps/buttons/touchnl.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC2, MR3, MS1, MS2, 0, 1, 0,
		"bitmaps/buttons/touchnl.xpm",
		"bitmaps/buttons/touchnl.xpm", BRIGHTON_CHECKBUTTON},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp dxApp = {
	"dx",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH,
	dxInit,
	dxConfigure, /* 3 callbacks, unused? */
	dxMidiCallback,
	destroySynth,
	{8, 0, 2, 2, 5, 520, 0, 0},
	820, 300, 0, 0,
	15, /* one panel only */
	{
		{
			"DX",
			BLUEPRINT,
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			dxCallback,
			20, 15, 310, 230,
			PARAM_COUNT,
			locations
		},
		{
			"DX",
			BLUEPRINT,
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			dxCallback,
			345, 15, 310, 230,
			PARAM_COUNT,
			0
		},
		{
			"DX",
			BLUEPRINT,
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			dxCallback,
			666, 15, 310, 230,
			PARAM_COUNT,
			locations
		},
		{
			"DX",
			BLUEPRINT,
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			dxCallback,
			20, 254, 310, 230,
			PARAM_COUNT,
			locations
		},
		{
			"DX",
			BLUEPRINT,
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			dxCallback,
			345, 254, 310, 230,
			PARAM_COUNT,
			locations
		},
		{
			"DX",
			BLUEPRINT,
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			dxCallback,
			666, 254, 310, 230,
			PARAM_COUNT,
			locations
		},
		{
			"DX",
			"bitmaps/blueprints/dxalgo.xpm",
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			dxCallback,
			20, 508, 310, 220,
			ALGOS_COUNT,
			algopanel
		},
		{
			"DX",
			"bitmaps/blueprints/dxmem.xpm",
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			dxCallback,
			666, 508, 310, 220,
			MEM_COUNT,
			mempanel
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/dkbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			110, 735, 870, 260,
			KEY_COUNT_6_OCTAVE,
			keys6octave
		},
		{
			"Mods",
			"bitmaps/blueprints/mods.xpm",
			"bitmaps/textures/metal5.xpm",
			0,
			0,
			0,
			modCallback,
			19, 745, 90, 230,
			2,
			mods
		},
		/* Wood rim */
		{
			"Wood",
			0,
			"bitmaps/textures/wood2.xpm",
			0,
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
			"bitmaps/textures/wood2.xpm",
			0,
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood.xpm",
			0,
			0,
			0,
			0,
			2, 495, 13, 490,
			0,
			0
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood.xpm",
			0,
			0,
			0,
			0,
			986, 495, 13, 490,
			0,
			0
		},
		{
			"Envelope",
			0,
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			0,
			330, 508, 337, 223,
			0,
			0
		},
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

static int
dxMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
/*printf("%i, %i, %i\n", c, o, v); */
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

int
dxLoadMem(guiSynth *synth, char *algo, char *name, int location, int active,
int skip, int flags)
{
	brightonEvent event;
	int op;
	float mo, fmop[OP_COUNT][2];

	/*
	 * See if the memory actually exists. This is a bit of file system overhead
	 * but prevents attempting to load non-existant memories
	 */
	op = loadMemory(synth, algo, name, location, active, skip, BRISTOL_STAT);

	if (flags == 2)
		return(op);

	if (op < 0)
		return(op);

	event.type = BRIGHTON_FLOAT;
	event.value = 0.0;

	/*
	 * Zero out diverse gain functions to prevent a noisy transition.
	 * What happens is that as the parameters change and the algorithm alters
	 * we get a LOT of FM noise. We can dump this by zeroing the output
	 * parameters. We have to be careful, since if the memory does not load,
	 * for example it does not exist, then we have to reset the parameters
	 * afterwards.
	 *
	 * Start with main out.
	 */
	mo = synth->mem.param[OP_COUNT * PARAM_COUNT + 26];
	brightonParamChange(synth->win, ALGOS_PANEL, 26, &event);

	for (op = 0; op < 6; op++)
	{
		/*
		 * Output gain
		 */
		fmop[op][0] = synth->mem.param[op * PARAM_COUNT];
		brightonParamChange(synth->win, op, 0, &event);
		/*
		 * Input gain
		 */
		fmop[op][1] = synth->mem.param[op * PARAM_COUNT + 3];
		brightonParamChange(synth->win, op, 3, &event);
	}

	if (loadMemory(synth, algo, name, location, active, skip, flags) == 0)
		return(0);

	/*
	 * Load failed, return the gain parameters to their previous values.
	 */
	event.value = mo;
	brightonParamChange(synth->win, ALGOS_PANEL, 26, &event);

	for (op = 0; op < 6; op++)
	{
		/*
		 * Output gain
		 */
		event.value = fmop[op][0];
		brightonParamChange(synth->win, op, 0, &event);
		/*
		 * Input gain
		 */
		event.value = fmop[op][1];
		brightonParamChange(synth->win, op, 3, &event);
	}
	return(-1);
}

static int
dxMidiCallback(brightonWindow *win, int command, int value, float v)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", command, value);

	switch(command)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", command, value);
			synth->location = value;
			dxLoadMem(synth, "dx", 0, synth->location,
				synth->mem.active, FIRST_DEV, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", command, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static void
dxMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	switch (c) {
		default:
		case 0:
			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;
			if (dxLoadMem(synth, "dx", 0, synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
				displayPanelText(synth, "FRE", synth->location,
					DISPLAY_PANEL, DISPLAY_DEV);
			else
				displayPanelText(synth, "PRG", synth->location,
					DISPLAY_PANEL, DISPLAY_DEV);
			break;
		case 1:
			if (dxLoadMem(synth, "dx", 0, synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayPanelText(synth, "FRE", synth->location,
					DISPLAY_PANEL, DISPLAY_DEV);
			else
				displayPanelText(synth, "PRG", synth->location,
					DISPLAY_PANEL, DISPLAY_DEV);
			break;
		case 2:
			saveMemory(synth, "dx", 0, synth->location, FIRST_DEV);
			displayPanelText(synth, "PRG", synth->location,
				DISPLAY_PANEL, DISPLAY_DEV);
			break;
		case 3:
			while (dxLoadMem(synth, "dx", 0, --synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location < 0)
					synth->location = 999;
			}
			displayPanelText(synth, "PRG", synth->location,
				DISPLAY_PANEL, DISPLAY_DEV);
			break;
		case 4:
			while (dxLoadMem(synth, "dx", 0, ++synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location > 999)
					synth->location = -1;
			}
			displayPanelText(synth, "PRG", synth->location,
				DISPLAY_PANEL, DISPLAY_DEV);
			break;
	}
}

static int
dxMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			return(0);
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 16)
		{
			synth->midichannel = 15;
			return(0);
		}
	}

	if (global.libtest == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);

	synth->midichannel = newchan;

	displayPanelText(synth, "MIDI", synth->midichannel + 1,
		DISPLAY_PANEL, DISPLAY_DEV);

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
dxCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (synth == 0)
		return(0);

	if (dxApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	switch (panel) {
		case OP_PANEL:
		case OP_PANEL + 1:
		case OP_PANEL + 2:
		case OP_PANEL + 3:
		case OP_PANEL + 4:
		case OP_PANEL + 5:
			index += panel * PARAM_COUNT;
			break;
		case ALGOS_PANEL:
			index += ALGO_START;
			break;
		case MEM_PANEL:
			index += MEM_START;
			break;
		default:
			printf("unknown panel\n");
			break;
	}

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

/*	printf("dxCallback(%i, %f): %x\n", index, value, synth); */

	synth->mem.param[index] = value;

	synth->dispatch[index].routine(synth,
		global.controlfd, synth->sid,
		synth->dispatch[index].controller,
		synth->dispatch[index].operator,
		sendvalue);
#ifdef DEBUG
	else
		printf("dispatch[%x,%i](%i, %i, %i, %i, %i)\n", (size_t) synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#endif

	return(0);
}

static void
dxAlgo(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

/*	printf("dxAlgo(%x, %i, %i, %i, %i, %i): %i\n", */
/*		synth, fd, chan, c, o, v, synth->mem.param[v - 1]); */

	/*
	 * These will be radio buttons
	 */
	if (synth->dispatch[ALGO_START].other2)
	{
		synth->dispatch[ALGO_START].other2 = 0;
		return;
	}

	if (v == 0)
		return;

	if (synth->dispatch[ALGO_START].other1 >= 0)
	{
		synth->dispatch[ALGO_START].other2 = 1;

		if (synth->dispatch[ALGO_START].other1 != o)
			event.value = 0;
		else
			event.value = 1;

/*printf("other one is %i\n", synth->dispatch[ALGO_START].other1); */
		brightonParamChange(synth->win, ALGOS_PANEL,
			synth->dispatch[ALGO_START].other1, &event);
	}

	if (v != 0)
	{
		char bitmap[128];

		event.type = BRIGHTON_MEM;

		sprintf(bitmap, "bitmaps/images/algo%i.xpm", o);
		event.m = bitmap;

/*printf("%i %i: RECONFIGURING BITMAP\n", v, synth->mem.param[ALGO_START + o]); */
		brightonParamChange(synth->win,
			ALGOS_PANEL, ALGOS_COUNT - 1, &event);
	}

	bristolMidiSendMsg(global.controlfd, synth->sid,
		126, 101, o);

	synth->dispatch[ALGO_START].other1 = o;
}

static void
dxTune(guiSynth *synth)
{
	brightonEvent event;

	printf("dxTune(%p, %i, %i)\n", synth->win, OP_PANEL, OP1_START);

	event.value = 0.5;
	brightonParamChange(synth->win, 0, 1, &event);
	brightonParamChange(synth->win, 1, 1, &event);
	brightonParamChange(synth->win, 2, 1, &event);
	brightonParamChange(synth->win, 3, 1, &event);
	brightonParamChange(synth->win, 4, 1, &event);
	brightonParamChange(synth->win, 5, 1, &event);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
dxInit(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	dispatcher *dispatch;
	int i;
/*
	brightonEvent event;
	char bitmap[128];

	event.type = BRIGHTON_MEM;
	event.m = OP2XPM;
	brightonParamChange(synth->win,
		ALGOS_PANEL, ALGOS_COUNT - 1, &event);
*/

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

	printf("Initialise the dx link to bristol: %p\n", synth->win);

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
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = dxMidiSendMsg;

	/* algo panel */
	dispatch[ALGO_START].operator = 0;
	dispatch[ALGO_START + 1].operator = 1;
	dispatch[ALGO_START + 2].operator = 2;
	dispatch[ALGO_START + 3].operator = 3;
	dispatch[ALGO_START + 4].operator = 4;
	dispatch[ALGO_START + 5].operator = 5;
	dispatch[ALGO_START + 6].operator = 6;
	dispatch[ALGO_START + 7].operator = 7;
	dispatch[ALGO_START + 8].operator = 8;
	dispatch[ALGO_START + 9].operator = 9;
	dispatch[ALGO_START + 10].operator = 10;
	dispatch[ALGO_START + 11].operator = 11;
	dispatch[ALGO_START + 12].operator = 12;
	dispatch[ALGO_START + 13].operator = 13;
	dispatch[ALGO_START + 14].operator = 14;
	dispatch[ALGO_START + 15].operator = 15;
	dispatch[ALGO_START + 16].operator = 16;
	dispatch[ALGO_START + 17].operator = 17;
	dispatch[ALGO_START + 18].operator = 18;
	dispatch[ALGO_START + 19].operator = 19;
	dispatch[ALGO_START + 20].operator = 20;
	dispatch[ALGO_START + 21].operator = 21;
	dispatch[ALGO_START + 22].operator = 22;
	dispatch[ALGO_START + 23].operator = 23;
	dispatch[ALGO_START].routine = dispatch[ALGO_START + 1].routine =
		dispatch[ALGO_START + 2].routine = dispatch[ALGO_START + 3].routine =
		dispatch[ALGO_START + 4].routine = dispatch[ALGO_START + 5].routine =
		dispatch[ALGO_START + 6].routine = dispatch[ALGO_START + 7].routine =
		dispatch[ALGO_START + 8].routine = dispatch[ALGO_START + 9].routine =
		dispatch[ALGO_START + 10].routine = dispatch[ALGO_START + 11].routine =
		dispatch[ALGO_START + 12].routine = dispatch[ALGO_START + 13].routine =
		dispatch[ALGO_START + 14].routine = dispatch[ALGO_START + 15].routine =
		dispatch[ALGO_START + 16].routine = dispatch[ALGO_START + 17].routine =
		dispatch[ALGO_START + 18].routine = dispatch[ALGO_START + 19].routine =
		dispatch[ALGO_START + 20].routine = dispatch[ALGO_START + 21].routine =
		dispatch[ALGO_START + 22].routine = dispatch[ALGO_START + 23].routine =
		(synthRoutine) dxAlgo;

	dispatch[ALGO_START + 24].controller = 126;
	dispatch[ALGO_START + 24].operator = 99;
	dispatch[ALGO_START + 25].controller = 126;
	dispatch[ALGO_START + 25].operator = 100;
	dispatch[ALGO_START + 27].routine = (synthRoutine) dxTune;

	/* Main volume */
	dispatch[ALGO_START + 26].controller = 126;
	dispatch[ALGO_START + 26].operator = 102;

	/* operator = 1 */
	dispatch[OP1_START].controller = 126;
	dispatch[OP1_START].operator = 0;
	dispatch[OP1_START + 1].controller = 0;
	dispatch[OP1_START + 1].operator = 1;
	dispatch[OP1_START + 2].controller = 0;
	dispatch[OP1_START + 2].operator = 0;
	/* envelope */
	dispatch[OP1_START + 5].controller = 0;
	dispatch[OP1_START + 5].operator = 2;
	dispatch[OP1_START + 6].controller = 0;
	dispatch[OP1_START + 6].operator = 3;
	dispatch[OP1_START + 7].controller = 0;
	dispatch[OP1_START + 7].operator = 4;
	dispatch[OP1_START + 8].controller = 0;
	dispatch[OP1_START + 8].operator = 5;
	dispatch[OP1_START + 3].controller = 0;
	dispatch[OP1_START + 3].operator = 6;
	dispatch[OP1_START + 9].controller = 0;
	dispatch[OP1_START + 9].operator = 7;
	dispatch[OP1_START + 4].controller = 126;
	dispatch[OP1_START + 4].operator = 1;
	dispatch[OP1_START + 10].controller = 126;
	dispatch[OP1_START + 10].operator = 2;
	dispatch[OP1_START + 11].controller = 126;
	dispatch[OP1_START + 11].operator = 3;
	dispatch[OP1_START + 12].controller = 0;
	dispatch[OP1_START + 12].operator = 8;
	/* L1-A-L2 */
	dispatch[OP1_START + 13].controller = 0;
	dispatch[OP1_START + 13].operator = 9;
	dispatch[OP1_START + 14].controller = 0;
	dispatch[OP1_START + 14].operator = 10;
	dispatch[OP1_START + 15].controller = 0;
	dispatch[OP1_START + 15].operator = 11;

	/* operator = 2 */
	dispatch[OP2_START].controller = 126;
	dispatch[OP2_START].operator = 10;
	dispatch[OP2_START + 1].controller = 1;
	dispatch[OP2_START + 1].operator = 1;
	dispatch[OP2_START + 2].controller = 1;
	dispatch[OP2_START + 2].operator = 0;
	/* envelope */
	dispatch[OP2_START + 5].controller = 1;
	dispatch[OP2_START + 5].operator = 2;
	dispatch[OP2_START + 6].controller = 1;
	dispatch[OP2_START + 6].operator = 3;
	dispatch[OP2_START + 7].controller = 1;
	dispatch[OP2_START + 7].operator = 4;
	dispatch[OP2_START + 8].controller = 1;
	dispatch[OP2_START + 8].operator = 5;
	dispatch[OP2_START + 3].controller = 1;
	dispatch[OP2_START + 3].operator = 6;
	dispatch[OP2_START + 9].controller = 1;
	dispatch[OP2_START + 9].operator = 7;
	dispatch[OP2_START + 4].controller = 126;
	dispatch[OP2_START + 4].operator = 11;
	dispatch[OP2_START + 10].controller = 126;
	dispatch[OP2_START + 10].operator = 12;
	dispatch[OP2_START + 11].controller = 126;
	dispatch[OP2_START + 11].operator = 13;
	dispatch[OP2_START + 12].controller = 1;
	dispatch[OP2_START + 12].operator = 8;
	/* L1-A-L2 */
	dispatch[OP2_START + 13].controller = 1;
	dispatch[OP2_START + 13].operator = 9;
	dispatch[OP2_START + 14].controller = 1;
	dispatch[OP2_START + 14].operator = 10;
	dispatch[OP2_START + 15].controller = 1;
	dispatch[OP2_START + 15].operator = 11;

	/* operator = 3 */
	dispatch[OP3_START].controller = 126;
	dispatch[OP3_START].operator = 20;
	dispatch[OP3_START + 1].controller = 2;
	dispatch[OP3_START + 1].operator = 1;
	dispatch[OP3_START + 2].controller = 2;
	dispatch[OP3_START + 2].operator = 0;
	/* envelope */
	dispatch[OP3_START + 5].controller = 2;
	dispatch[OP3_START + 5].operator = 2;
	dispatch[OP3_START + 6].controller = 2;
	dispatch[OP3_START + 6].operator = 3;
	dispatch[OP3_START + 7].controller = 2;
	dispatch[OP3_START + 7].operator = 4;
	dispatch[OP3_START + 8].controller = 2;
	dispatch[OP3_START + 8].operator = 5;
	dispatch[OP3_START + 3].controller = 2;
	dispatch[OP3_START + 3].operator = 6;
	dispatch[OP3_START + 9].controller = 2;
	dispatch[OP3_START + 9].operator = 7;
	dispatch[OP3_START + 4].controller = 126;
	dispatch[OP3_START + 4].operator = 21;
	dispatch[OP3_START + 10].controller = 126;
	dispatch[OP3_START + 10].operator = 22;
	dispatch[OP3_START + 11].controller = 126;
	dispatch[OP3_START + 11].operator = 23;
	dispatch[OP3_START + 12].controller = 2;
	dispatch[OP3_START + 12].operator = 8;
	/* L1-A-L2 */
	dispatch[OP3_START + 13].controller = 2;
	dispatch[OP3_START + 13].operator = 9;
	dispatch[OP3_START + 14].controller = 2;
	dispatch[OP3_START + 14].operator = 10;
	dispatch[OP3_START + 15].controller = 2;
	dispatch[OP3_START + 15].operator = 11;

	/* operator = 4 */
	dispatch[OP4_START].controller = 126;
	dispatch[OP4_START].operator = 30;
	dispatch[OP4_START + 1].controller = 3;
	dispatch[OP4_START + 1].operator = 1;
	dispatch[OP4_START + 2].controller = 3;
	dispatch[OP4_START + 2].operator = 0;
	/* envelope */
	dispatch[OP4_START + 5].controller = 3;
	dispatch[OP4_START + 5].operator = 2;
	dispatch[OP4_START + 6].controller = 3;
	dispatch[OP4_START + 6].operator = 3;
	dispatch[OP4_START + 7].controller = 3;
	dispatch[OP4_START + 7].operator = 4;
	dispatch[OP4_START + 8].controller = 3;
	dispatch[OP4_START + 8].operator = 5;
	dispatch[OP4_START + 3].controller = 3;
	dispatch[OP4_START + 3].operator = 6;
	dispatch[OP4_START + 9].controller = 3;
	dispatch[OP4_START + 9].operator = 7;
	dispatch[OP4_START + 4].controller = 126;
	dispatch[OP4_START + 4].operator = 31;
	dispatch[OP4_START + 10].controller = 126;
	dispatch[OP4_START + 10].operator = 32;
	dispatch[OP4_START + 11].controller = 126;
	dispatch[OP4_START + 11].operator = 33;
	dispatch[OP4_START + 12].controller = 3;
	dispatch[OP4_START + 12].operator = 8;
	/* L1-A-L2 */
	dispatch[OP4_START + 13].controller = 3;
	dispatch[OP4_START + 13].operator = 9;
	dispatch[OP4_START + 14].controller = 3;
	dispatch[OP4_START + 14].operator = 10;
	dispatch[OP4_START + 15].controller = 3;
	dispatch[OP4_START + 15].operator = 11;

	/* operator = 5 */
	dispatch[OP5_START].controller = 126;
	dispatch[OP5_START].operator = 40;
	dispatch[OP5_START + 1].controller = 4;
	dispatch[OP5_START + 1].operator = 1;
	dispatch[OP5_START + 2].controller = 4;
	dispatch[OP5_START + 2].operator = 0;
	/* envelope */
	dispatch[OP5_START + 5].controller = 4;
	dispatch[OP5_START + 5].operator = 2;
	dispatch[OP5_START + 6].controller = 4;
	dispatch[OP5_START + 6].operator = 3;
	dispatch[OP5_START + 7].controller = 4;
	dispatch[OP5_START + 7].operator = 4;
	dispatch[OP5_START + 8].controller = 4;
	dispatch[OP5_START + 8].operator = 5;
	dispatch[OP5_START + 3].controller = 4;
	dispatch[OP5_START + 3].operator = 6;
	dispatch[OP5_START + 9].controller = 4;
	dispatch[OP5_START + 9].operator = 7;
	dispatch[OP5_START + 4].controller = 126;
	dispatch[OP5_START + 4].operator = 41;
	dispatch[OP5_START + 10].controller = 126;
	dispatch[OP5_START + 10].operator = 42;
	dispatch[OP5_START + 11].controller = 126;
	dispatch[OP5_START + 11].operator = 43;
	dispatch[OP5_START + 12].controller = 4;
	dispatch[OP5_START + 12].operator = 8;
	/* L1-A-L2 */
	dispatch[OP5_START + 13].controller = 4;
	dispatch[OP5_START + 13].operator = 9;
	dispatch[OP5_START + 14].controller = 4;
	dispatch[OP5_START + 14].operator = 10;
	dispatch[OP5_START + 15].controller = 4;
	dispatch[OP5_START + 15].operator = 11;

	/* operator = 6 */
	dispatch[OP6_START].controller = 126;
	dispatch[OP6_START].operator = 50;
	dispatch[OP6_START + 1].controller = 5;
	dispatch[OP6_START + 1].operator = 1;
	dispatch[OP6_START + 2].controller = 5;
	dispatch[OP6_START + 2].operator = 0;
	/* envelope */
	dispatch[OP6_START + 5].controller = 5;
	dispatch[OP6_START + 5].operator = 2;
	dispatch[OP6_START + 6].controller = 5;
	dispatch[OP6_START + 6].operator = 3;
	dispatch[OP6_START + 7].controller = 5;
	dispatch[OP6_START + 7].operator = 4;
	dispatch[OP6_START + 8].controller = 5;
	dispatch[OP6_START + 8].operator = 5;
	dispatch[OP6_START + 3].controller = 5;
	dispatch[OP6_START + 3].operator = 6;
	dispatch[OP6_START + 9].controller = 5;
	dispatch[OP6_START + 9].operator = 7;
	dispatch[OP6_START + 4].controller = 126;
	dispatch[OP6_START + 4].operator = 51;
	dispatch[OP6_START + 10].controller = 126;
	dispatch[OP6_START + 10].operator = 52;
	dispatch[OP6_START + 11].controller = 126;
	dispatch[OP6_START + 11].operator = 53;
	dispatch[OP6_START + 12].controller = 5;
	dispatch[OP6_START + 12].operator = 8;
	/* L1-A-L2 */
	dispatch[OP6_START + 13].controller = 5;
	dispatch[OP6_START + 13].operator = 9;
	dispatch[OP6_START + 14].controller = 5;
	dispatch[OP6_START + 14].operator = 10;
	dispatch[OP6_START + 15].controller = 5;
	dispatch[OP6_START + 15].operator = 11;

	/* memories */
	dispatch[MEM_START].operator = 0;
	dispatch[MEM_START + 1].operator = 1;
	dispatch[MEM_START + 2].operator = 2;
	dispatch[MEM_START + 3].operator = 3;
	dispatch[MEM_START + 4].operator = 4;
	dispatch[MEM_START + 5].operator = 5;
	dispatch[MEM_START + 6].operator = 6;
	dispatch[MEM_START + 7].operator = 7;
	dispatch[MEM_START + 8].operator = 8;
	dispatch[MEM_START + 9].operator = 9;
	dispatch[MEM_START].routine = dispatch[MEM_START + 1].routine =
		dispatch[MEM_START + 2].routine = dispatch[MEM_START + 3].routine =
		dispatch[MEM_START + 4].routine = dispatch[MEM_START + 5].routine =
		dispatch[MEM_START + 6].routine = dispatch[MEM_START + 7].routine =
		dispatch[MEM_START + 8].routine = dispatch[MEM_START + 9].routine =
		dispatch[MEM_START + 10].routine = dispatch[MEM_START + 11].routine =
		dispatch[MEM_START + 15].routine = dispatch[MEM_START + 16].routine =
		(synthRoutine) dxMemory;
	dispatch[MEM_START + 10].controller = 1;
	dispatch[MEM_START + 11].controller = 2;
	dispatch[MEM_START + 15].controller = 3;
	dispatch[MEM_START + 16].controller = 4;

	/* Midi */
	dispatch[MEM_START + 12].controller = 2;
	dispatch[MEM_START + 13].controller = 1;
	dispatch[MEM_START + 12].routine = dispatch[MEM_START + 13].routine =
		(synthRoutine) dxMidi;

	/*
	 * We need to hack in the correct bitmaps for each op. They are all copies
	 * of the same locations structure, but they need a unique image bitmap
	 * for the "OP X" text.
	 */
	if (dxApp.resources[1].devlocn == 0)
	{
		int index;

		for (index = 1; index < 6; index++)
		{
			dxApp.resources[index].devlocn = (brightonLocations *)
				brightonmalloc(sizeof(locations));
			bcopy(dxApp.resources[0].devlocn, dxApp.resources[index].devlocn,
				sizeof(locations));
		}

		dxApp.resources[1].devlocn[PARAM_COUNT - 1].image = OP2XPM;
		dxApp.resources[2].devlocn[PARAM_COUNT - 1].image = OP3XPM;
		dxApp.resources[3].devlocn[PARAM_COUNT - 1].image = OP4XPM;
		dxApp.resources[4].devlocn[PARAM_COUNT - 1].image = OP5XPM;
		dxApp.resources[5].devlocn[PARAM_COUNT - 1].image = OP6XPM;
	}

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
dxConfigure(brightonWindow *win)
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

	printf("going operational %p %p\n", synth, synth->win);

	synth->flags |= OPERATIONAL;
	synth->keypanel = 8;
	synth->keypanel2 = -1;
	synth->transpose = 36;
	synth->dispatch[ALGO_START].other2 = 0;
	synth->dispatch[ALGO_START].other1 = -1;

	dxLoadMem(synth, "dx", 0, 0, synth->mem.active,
		FIRST_DEV, BRISTOL_FORCE);

	brightonPut(win,
		"bitmaps/blueprints/dxshade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	return(0);
}

