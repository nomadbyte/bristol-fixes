
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

static int ms20Init();
static int ms20Configure();
static int ms20Callback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define KEY_PANEL 1

#define FIRST_DEV 0

/* 36 controllers, 4 idle, 35 patches, 5 idle, then memory. */
#define DEVICE_COUNT 98
#define ACTIVE_DEVS 79 /* 35 of 40 jack connectors active. */
#define MEM_START (ACTIVE_DEVS + 1)
#define MS20_OUTPUTS 20
#define MS20_INPUTS 20
#define OUTPUT_START 40
#define INPUT_START 60

#define DISPLAY1 (DEVICE_COUNT - 2)
#define DISPLAY2 (DEVICE_COUNT - 1)

#define S1 60
#define S2 74

#define R1 150
#define R2 383
#define R3 616
#define R4 800

#define RD1 ((R4 - R1) / 4)

#define RR1 R1
#define RR2 (RR1 + RD1)
#define RR3 (RR2 + RD1)
#define RR4 (RR3 + RD1)
#define RR5 R4

#define CD1 67
#define C1 17
#define C2 (C1 + CD1)
#define C3 (C2 + CD1)
#define C4 (C3 + CD1 + 1)
#define C5 (C4 + CD1 - 1)
#define C6 (C5 + CD1 + 1)
#define C7 (C6 + CD1)
#define C8 (C7 + CD1)
#define C9 (C8 + CD1 + 4)
#define C10 (C9 + CD1 + 1)
#define C11 (C10 + CD1 + 3)
#define C12 (C11 + CD1)
#define C13 (C12 + CD1)
#define C14 (C13 + CD1)

static int oselect;
static struct {
	int input;
	int id; /* From graphical interface. */
} links[MS20_OUTPUTS];

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
 * This example is for a ms20Bristol type synth interface.
 */
static brightonLocations locations[DEVICE_COUNT] = {
/* LFO and control */
	{"", 0, C1 - 1, R1 - 12, S2, S2, 0, 3, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C1, R2 - 30, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C1 - 1, R3 - 72, S2, S2, 0, 3, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C1, R4, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C2 - 5, R1 - 12, S2, S2, 0, 3, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C2, R2 - 30, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C2 - 5, R3 - 72, S2, S2, 0, 3, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C2, R4, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C3, R1, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C3, R2, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C3, RR4, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C3, RR5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C4 - 3, R1 + 3, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C4, R2 + 6, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C4, RR4 + 6, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C4, RR5 + 6, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C5 - 3, R1 + 3, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C5, R2 + 6, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C5, RR4 + 6, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C5, RR5 + 6, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C6, RR4 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C6, RR5 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C7, RR3 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C7, RR4 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C7, RR5 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C8, RR1 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C8, RR2 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C8, RR3 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C8, RR4 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C8, RR5 + 5, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C9, R4 + 6, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C10, R4 + 6, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C11 - 2, R4 + 4, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C12 - 1, R4 + 2, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},
	{"", 0, C13, R4, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	{"", 0, C14 + 2, R1 - 30, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_NOSHADOW|BRIGHTON_REDRAW},

	/* Dummies */
	{"", 0, C10, R1, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_WITHDRAWN},
	{"", 0, C11, R1, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_WITHDRAWN},
	{"", 0, C12, R1, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_WITHDRAWN},
	{"", 0, C13, R1, S1, S1, 0, 1, 0, 
		"bitmaps/knobs/line.xpm", 0, BRIGHTON_NOTCH|BRIGHTON_WITHDRAWN},

	/*
	 * First the Outputs. Start with clocks at 40.
	 */
	{"", 2, 549, 297, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 627, 297, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	/* Env-1 out */
	{"", 2, 680, 297, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 680, 401, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	/* Env-2 rev out */
	{"", 2, 849, 297, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	/* KBD CV and TRIG out */
	{"", 2, 964, 297, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 964, 401, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	/* S&H Out */
	{"", 2, 627, 527, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 756, 527, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 798, 527, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 877, 527, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	/* Ext Sig Proc outputs */
	{"", 2, 627, 690, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 735, 690, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 820, 690, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 923, 690, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 964, 690, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_NOSHADOW},
	/* Dummies */
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoff.xpm",
		"bitmaps/buttons/patchonl.xpm", BRIGHTON_WITHDRAWN},

	/*
	 * Then the inputs. The primary is a dummy. I will put in 15 inputs and
	 * 25 outputs, there are 353 in total with some dummies and a couple that
	 * are not active.
	 */
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},

	/* The top line */
	{"", 2, 549, 180, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 611, 180, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 664, 180, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 725, 180, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 777, 180, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 832, 180, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* Control and trig */
	{"", 2, 735, 401, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 797, 401, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* KBD Control and trig */
	{"", 2, 923, 297, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 923, 401, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 885, 347, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* S&H and its clock, VCA */
	{"", 2, 586, 412, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 549, 527, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 680, 527, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* Wheels */
	{"", 2, 923, 527, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 964, 527, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* Sig In */
	{"", 2, 549, 690, 12, 30, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* Dummy ins */
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 900, 900, 12, 15, 0, 1, 0, "bitmaps/buttons/patchoffb.xpm",
		"bitmaps/buttons/patchon.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},

/* memories */
#define MH1 53
#define MH2 50
#define MW1 11
#define MW2 10

#define MR1 225
#define MR2 308

#define MCD 14
#define MC1 338
#define MC2 (MC1 + MCD)
#define MC3 (MC2 + MCD)
#define MC4 (MC3 + MCD)
#define MC5 (MC4 + MCD)
#define MC6 (MC5 + MCD)
#define MC7 (MC6 + MCD)
#define MC8 (MC7 + MCD + MCD)

/* Should be put into parameters, fixed values does not work... */
	{"", 2, MC2, MR1, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC3, MR1, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC4, MR1, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC5, MR1, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC6, MR1, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC2, MR2, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC3, MR2, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC4, MR2, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC5, MR2, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC6, MR2, MW1, MH1, 0, 1.01, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, MC1, MR1, MW2, MH2, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC1, MR2, MW2, MH2, 0, 1.01, 0,
		"bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	/* mem up down */
	{"", 2, MC7, MR1, MW2, MH2, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC7, MR2, MW2, MH2, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* MIDI up down */
	{"", 2, MC8, MR1, MW2, MH2, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC8, MR2, MW2, MH2, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* displays */
	{"", 3, 343, 90, 110, 45, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0},
	{"", 3, 343, 132, 110, 45, 0, 1, 0, 0, 0, 0}
};

/*
 * These are new mod controls for the Pro-1. Used here for the MS-20.
 */
static brightonLocations newmods[2] = {
	{"", BRIGHTON_MODWHEEL, 280, 180, 140, 550, 0, 1, 0,
		"bitmaps/knobs/modwheel.xpm", 0,
		BRIGHTON_REVERSE|BRIGHTON_HSCALE|BRIGHTON_NOSHADOW|BRIGHTON_CENTER|BRIGHTON_NOTCH},
	{"", BRIGHTON_MODWHEEL, 620, 180, 140, 550, 0, 1, 0,
		"bitmaps/knobs/modwheel.xpm", 0,
		BRIGHTON_REVERSE|BRIGHTON_HSCALE|BRIGHTON_NOSHADOW},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp ms20App = {
	"ms20",
	0,
	"bitmaps/textures/metal7.xpm",
	BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* default is tesselate */
	ms20Init,
	ms20Configure, /* 3 callbacks, unused? */
	0,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	865, 440, 0, 0,
	5, /* one panel only */
	{
		{
			"ms-20",
			"bitmaps/blueprints/ms20.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* flags */
			0,
			ms20Configure,
			ms20Callback,
			12, 25, 980, 677,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/ekbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			140, 700, 840, 300,
			KEY_COUNT_3OCTAVE,
			keys3octave
		},
		{
			"Mods",
			0, //"bitmaps/blueprints/mods.xpm",
			"bitmaps/textures/metal7.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			modCallback,
			12, 700, 128, 300,
			2,
			newmods
		},
		{
			"SP1",
			0,
			"bitmaps/images/rhodes.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 12, 1000,
			0,
			0
		},
		{
			"SP2",
			0,
			"bitmaps/images/rhodes.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			988, 0, 12, 1000,
			0,
			0
		},
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

static int
ms20MidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static int
ms20Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
/*	printf("ms20Memory(%i %i %i %i %i)\n", fd, chan, c, o, v); */

	if (synth->flags & MEM_LOADING)
		return(0);
	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	if (synth->dispatch[MEM_START].other2)
	{
		synth->dispatch[MEM_START].other2 = 0;
		return(0);
	}

	switch (c) {
		default:
		case 0:
			if (synth->dispatch[MEM_START].other1 != -1)
			{
				brightonEvent event;
				synth->dispatch[MEM_START].other2 = 1;

				if (synth->dispatch[MEM_START].other1 != c)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[MEM_START].other1, &event);
			}
			synth->dispatch[MEM_START].other1 = c;

			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;

			if (loadMemory(synth, "ms20", 0, synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY1);
			else
				displayText(synth, "PRG", synth->location, DISPLAY1);
			break;
		case 1:
			if (loadMemory(synth, "ms20", 0, synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY2);
			else
				displayText(synth, "PRG", synth->location, DISPLAY2);
/*			synth->location = 0; */
			break;
		case 2:
			saveMemory(synth, "ms20", 0, synth->location, FIRST_DEV);
			displayText(synth, "PRG", synth->location, DISPLAY2);
/*			synth->location = 0; */
			break;
		case 3:
			while (loadMemory(synth, "ms20", 0, ++synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location > 999)
					synth->location = -1;
			}
			displayText(synth, "PRG", synth->location, DISPLAY2);
			break;
		case 4:
			while (loadMemory(synth, "ms20", 0, --synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location < 0)
					synth->location = 999;
			}
			displayText(synth, "PRG", synth->location, DISPLAY2);
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
ms20Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*	printf("ms20Callback(%i, %i, %f): %i %x\n", panel, index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);
	if (index >= DEVICE_COUNT)
		return(0);

	if (ms20App.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * (CONTROLLER_RANGE - 1);
	else
		sendvalue = value;

	synth->mem.param[index] = value;

/*	if ((!global.libtest) || (index >= ACTIVE_DEVS)) */
	if ((!global.libtest) || (index >= 40))
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
	else
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

	return(0);
}

static void
ms20Midi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	printf("ms20Midi(%i %i %i %i %i)\n", fd, chan, c, o, v);

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[MEM_START + 14].other1 == MEM_START + 14)
	{
		int newchan;

		if (c == 1) {
			if ((newchan = synth->midichannel - 1) < 0)
			{
				synth->midichannel = 0;
				return;
			}
		} else {
			if ((newchan = synth->midichannel + 1) >= 16)
			{
				synth->midichannel = 15;
				return;
			}
		}

		if (global.libtest == 0)
		{
			bristolMidiSendMsg(global.controlfd, synth->sid,
				127, 0, BRISTOL_MIDICHANNEL|newchan);
		}

		synth->midichannel = newchan;

		displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY1);
	} else {
		if (c == 1) {
			while (locations[--synth->dispatch[MEM_START + 15].other2].name[0]
				== '\0')
			{
				if (synth->dispatch[MEM_START + 15].other2 < 0)
					synth->dispatch[MEM_START + 15].other2 = 49;
			}
		} else { 
			while (locations[++synth->dispatch[MEM_START + 15].other2].name[0]
				== '\0')
			{
				if (synth->dispatch[MEM_START + 15].other2 >= 49)
					synth->dispatch[MEM_START + 15].other2 = 0;
			}
		}
		displayText(synth,
			locations[synth->dispatch[MEM_START + 15].other2].name,
			synth->dispatch[MEM_START + 15].other2,
			DISPLAY2);
		if (synth->dispatch[MEM_START + 15].other1 == 1)
		{
/*printf("PANEL X SELECTOR %i %i = %i %i\n", */
/*synth->dispatch[MEM_START + 15].other1, */
/*synth->dispatch[MEM_START + 15].other2, */
/*synth->dispatch[synth->dispatch[MEM_START + 15].other2].controller, */
/*synth->dispatch[synth->dispatch[MEM_START + 15].other2].operator); */
			synth->dispatch[54].controller =
				synth->dispatch[
					synth->dispatch[MEM_START + 15].other2].controller;
			synth->dispatch[54].operator =
				synth->dispatch[
					synth->dispatch[MEM_START + 15].other2].operator;
		} else {
/*printf("PANEL Y SELECTOR %i %i = %i %i\n", */
/*synth->dispatch[MEM_START + 15].other1, */
/*synth->dispatch[MEM_START + 15].other2, */
/*synth->dispatch[synth->dispatch[MEM_START + 15].other2].controller, */
/*synth->dispatch[synth->dispatch[MEM_START + 15].other2].operator); */
			synth->dispatch[55].controller =
				synth->dispatch[
					synth->dispatch[MEM_START + 15].other2].controller;
			synth->dispatch[55].operator =
				synth->dispatch[
					synth->dispatch[MEM_START + 15].other2].operator;
		}
	}
}

static void
ms20PanelSelect(guiSynth *synth, int fd, int chan, int c, int o, int v)
{

/*	printf("ms20PanelSelect(%x, %i, %i, %i, %i, %i\n", */
/*		synth, fd, chan, c, o, v); */
/* dispatch[MEM_START + 14] = dispatch[MEM_START + 15] */

	if (synth->dispatch[MEM_START + 14].other2)
	{
		synth->dispatch[MEM_START + 14].other2 = 0;
		return;
	}

	if (synth->dispatch[MEM_START + 14].other1 != -1)
	{
		brightonEvent event;
		synth->dispatch[MEM_START + 14].other2 = 1;

		if (synth->dispatch[MEM_START + 14].other1 != c)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[MEM_START + 14].other1, &event);
	}
	displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY1);

	if (c == MEM_START + 15)
	{
		if (synth->dispatch[MEM_START + 15].other1 > 1)
			synth->dispatch[MEM_START + 15].other1 = 0;

		switch (synth->dispatch[MEM_START + 15].other1) {
			case 0:
			default:
				displayText(synth, "PAN-X",
					synth->dispatch[MEM_START + 15].other2, DISPLAY1);
				break;
			case 1:
				displayText(synth, "PAN-Y",
					synth->dispatch[MEM_START + 15].other2, DISPLAY1);
				break;
		}
		synth->dispatch[MEM_START + 15].other1++;
	}
	synth->dispatch[MEM_START + 14].other1 = c;
}

static void
ms20IOSelect(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

/*	printf("ms20IOSelect(%i, %i, %i)\n", c, o, v); */

	if ((synth->flags & OPERATIONAL) == 0)
		return;

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
				101, o, links[o].input);
			links[o].input = 0;
			event.type = BRIGHTON_UNLINK;
			event.intvalue = links[o].id;
			brightonParamChange(synth->win, 0, OUTPUT_START + o, &event);
		}
	} else if (c == 2) {
		int i;

		if (oselect >= 0) {
			for (i = 0; i < MS20_OUTPUTS; i++)
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
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 100, oselect, o);
			event.type = BRIGHTON_LINK;
			event.intvalue = INPUT_START + o;
			links[oselect].id =
				brightonParamChange(synth->win, 0, OUTPUT_START + oselect,
					&event);
		} else {
			/*
			 * See if this input is connected, if so clear it.
			 */
			for (i = 0; i < MS20_OUTPUTS; i++)
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
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
ms20Init(brightonWindow *win)
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

	printf("Initialise the ms20 link to bristol: %p\n", synth->win);

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

/*	for (i = 0; i < DEVICE_COUNT; i++) */
/*		synth->dispatch[i].routine = ms20MidiSendMsg; */

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
			synth->dispatch[i].operator = i - INPUT_START;
			synth->dispatch[i].routine = (synthRoutine) ms20IOSelect;
		} else if (i >= OUTPUT_START) {
			/* Outputs */
			synth->dispatch[i].controller = 1;
			synth->dispatch[i].operator = i - OUTPUT_START;
			synth->dispatch[i].routine = (synthRoutine) ms20IOSelect;
		} else
			synth->dispatch[i].routine = ms20MidiSendMsg;
	}

	/* memory */
	dispatch[MEM_START].operator = 0;
	dispatch[MEM_START].controller = MEM_START;
	dispatch[MEM_START + 1].operator = 1;
	dispatch[MEM_START + 1].controller = MEM_START + 1;
	dispatch[MEM_START + 2].operator = 2;
	dispatch[MEM_START + 2].controller = MEM_START + 2;
	dispatch[MEM_START + 3].operator = 3;
	dispatch[MEM_START + 3].controller = MEM_START + 3;
	dispatch[MEM_START + 4].operator = 4;
	dispatch[MEM_START + 4].controller = MEM_START + 4;
	dispatch[MEM_START + 5].operator = 5;
	dispatch[MEM_START + 5].controller = MEM_START + 5;
	dispatch[MEM_START + 6].operator = 6;
	dispatch[MEM_START + 6].controller = MEM_START + 6;
	dispatch[MEM_START + 7].operator = 7;
	dispatch[MEM_START + 7].controller = MEM_START + 7;
	dispatch[MEM_START + 8].operator = 8;
	dispatch[MEM_START + 8].controller = MEM_START + 8;
	dispatch[MEM_START + 9].operator = 9;
	dispatch[MEM_START + 9].controller = MEM_START + 9;

	dispatch[MEM_START + 10].controller = 1;
	dispatch[MEM_START + 11].controller = 2;
	dispatch[MEM_START + 16].controller = 3;
	dispatch[MEM_START + 17].controller = 4;

	dispatch[MEM_START + 0].routine = dispatch[MEM_START + 1].routine =
	dispatch[MEM_START + 2].routine = dispatch[MEM_START + 3].routine =
	dispatch[MEM_START + 4].routine = dispatch[MEM_START + 5].routine =
	dispatch[MEM_START + 6].routine = dispatch[MEM_START + 7].routine =
	dispatch[MEM_START + 8].routine = dispatch[MEM_START + 9].routine =
	dispatch[MEM_START + 10].routine = dispatch[MEM_START + 11].routine =
	dispatch[MEM_START + 16].routine = dispatch[MEM_START + 17].routine =
		(synthRoutine) ms20Memory;

	/* midi */
	dispatch[MEM_START + 12].controller = 2;
	dispatch[MEM_START + 13].controller = 1;
	dispatch[MEM_START + 12].routine = dispatch[MEM_START + 13].routine =
		(synthRoutine) ms20Midi;

	/* midi/panel selectors */
	dispatch[MEM_START + 14].controller = MEM_START + 14;
	dispatch[MEM_START + 15].controller = MEM_START + 15;
	dispatch[MEM_START + 14].routine = dispatch[MEM_START + 15].routine =
		(synthRoutine) ms20PanelSelect;
	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
ms20Configure(brightonWindow *win)
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
	synth->keypanel = 1;
	synth->keypanel2 = -1;
	synth->transpose = 36;

	displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY1);

	loadMemory(synth, "ms20", 0, synth->location, synth->mem.active,
		FIRST_DEV, 0);
	displayText(synth, "PRG", synth->location, DISPLAY2);

	brightonPut(win,
		"bitmaps/blueprints/ms20shade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);

	return(0);
}

