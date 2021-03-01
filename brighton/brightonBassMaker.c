
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
 * The 'Bass' Maker, 4 stage 16 step sequencer.
 *
 * Crashes after BM is stopped. Issues with voice->baudio was NULL.
 * Apparent loss of pitch bend.
 * Mod controller mapping does not appear to work.
 *
 *	Global transpose. Test.
 *	Mem search DONE
 *	Midi channel DONE
 *	Need to make sure transpose and channel selections are in the memories and
 *	recovered. Tested ctype, cc, transpose
 *
 *	Controller options
 *	Controller to note (13 steps) + channel
 *	Controller to tune
 *	Controller to pitchwheel
 *	Controller to mod wheel
 *	Controller to other continuous controller
 *	Controller to operator parameter
 *
 *	Copy page
 *	Fill values
 *
 *	Midi send and recieve clock. ffs.
 */

#include <fcntl.h>

#include "brighton.h"
#include "bristolmidi.h"
#include "brightonMini.h"
#include "brightoninternals.h"
#include "brightonledstates.h"

static int bmInit();
static int bmConfigure();
static int bmCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

static int dc;

#define BM_TRANSPOSE 24

#define KEY_PANEL 1

#define PAGE_COUNT 4 /* This would require rebuilding the control panel */
#define OP_COUNT 8
#define PAGE_STEP 16
#define STEP_COUNT (PAGE_STEP * PAGE_COUNT)

#define TOTAL_DEVS (PAGE_STEP * OP_COUNT)
#define CONTROL_COUNT 60 /* controls less memomry selectors/entry */
#define CONTROL_ACTIVE 20
#define COFF (TOTAL_DEVS * PAGE_COUNT)
#define ACTIVE_DEVS (COFF + CONTROL_ACTIVE)
#define DEVICE_COUNT (COFF + CONTROL_COUNT)
#define DISPLAY_DEV (CONTROL_COUNT - 1)

#define BM_C_CONTROL	0 /* or greater */
#define BM_C_NOTE		-1
#define BM_C_TUNE		-2
#define BM_C_GLIDE		-3 /* ffs */
#define BM_C_MOD		-4 /* ffs */
#define BM_C_OPERATOR	-5 /* ffs */

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
 * This example is for a bmBristol type synth interface.
 */

typedef struct BmNote {
	float note;
	float transp;
	float velocity;
	float cont;
	float led;
	float button;
	float pad1;
	float pad2;
} bmNote;

typedef struct BmPage {
	bmNote step[PAGE_STEP];
} bmPage;

/* A/B/C/D active: 0 4 4 */
/* A/B/C/D select: 4 4 8 */
/* Speed, Duty, 4xdirection: 8 6 14 */
/* 6 pad: 14 6 20 */
/* ABCD led * 2: 20 8 28  */
/* Dir led * 4: 28 4 32 */
/* Memory 12: 32 12 44 */
/* Display 4: 44 4 48 */
/* 12 pad: 48 12 60 */

typedef struct BmControl {
	float speed;
	float duty;
	float up;
	float down;
	float updown;
	float rand;
	float active[PAGE_COUNT];
	float select[PAGE_COUNT];
	float transpose;
	float ctype;
	float cc;
	float cop;
	float pad1[2];
	/* 20 Following parameters are NOT in the memories */
	float activeled[PAGE_COUNT];
	float selectled[PAGE_COUNT];
	float dirled[PAGE_COUNT];
	float mem[12];
	float start;
	float stop;
	float pad2[10];
	float display[PAGE_COUNT];
} bmControl;

typedef struct BmMem {
	bmPage page[PAGE_COUNT];
	bmControl control;
	/* These are transient parameters used during operation */
	int lastnote;
	int lastpage;
	int laststep;
	int clearstep;
	int lastcnote;
	int cMemPage;
	int cMemEntry;
	int pageSelect;
} bmMem;

struct {
	synthRoutine control[OP_COUNT];
} call;

#define DR1 194
#define R1 60
#define R2 (R1 + DR1 + 10)
#define R3 (R2 + DR1 - 10)
#define R4 (R3 + DR1)
#define R5 (R4 + DR1)
#define R6 (R5 + DR1/3 - 15)

#define DC1 60
#define C1 30
#define C2 (C1 + DC1)
#define C3 (C2 + DC1)
#define C4 (C3 + DC1)
#define C5 (C4 + DC1)
#define C6 (C5 + DC1)
#define C7 (C6 + DC1)
#define C8 (C7 + DC1)
#define C9 (C8 + DC1)
#define C10 (C9 + DC1)
#define C11 (C10 + DC1)
#define C12 (C11 + DC1)
#define C13 (C12 + DC1)
#define C14 (C13 + DC1)
#define C15 (C14 + DC1)
#define C16 (C15 + DC1)

#define W1 35
#define H1 100
#define W2 20
#define H2 60
#define W3 12
#define H3 27
#define H4 75
#define O1 (W1 / 3)
#define O2 ((W1 - W2) / 2 + 4)
#define O3 ((W1 - W3) / 2 + 3)

static brightonLocations locations[TOTAL_DEVS] = {

	{"", 0, C1, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 2, C1 + O3, R2, 10, H4, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY},
	{"", 0, C1, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 0, C1, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C1 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C1 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm", BRIGHTON_CHECKBUTTON},
	{"", 0, C1 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C1 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C2, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 2, C2 + O3, R2, 10, H4, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY},
	{"", 0, C2, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 0, C2, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C2 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C2 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm", BRIGHTON_CHECKBUTTON},
	{"", 0, C2 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C2 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C3, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 2, C3 + O3, R2, 10, H4, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY},
	{"", 0, C3, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 0, C3, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C3 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C3 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C3 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C3 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C4, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 2, C4 + O3, R2, 10, H4, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY},
	{"", 0, C4, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 0, C4, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C4 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C4 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C4 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C4 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C5, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 2, C5 + O3, R2, 10, H4, 0, 2, 0, 0, 0, BRIGHTON_THREEWAY},
	{"", 0, C5, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 0, C5, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C5 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C5 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C5 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C5 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C6, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C6 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C6, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C6, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C6 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C6 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C6 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C6 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C7, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C7 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C7, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C7, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C7 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C7 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C7 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C7 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C8, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C8 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C8, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C8, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C8 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C8 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C8 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C8 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C9, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C9 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C9, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C9, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C9 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C9 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C9 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C9 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C10, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C10 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C10, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C10, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C10 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C10 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C10 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C10 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C11, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C11 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C11, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C11, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C11 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C11 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C11 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C11 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C12, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C12 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C12, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C12, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C12 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C12 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C12 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C12 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C13, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C13 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C13, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C13, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C13 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C13 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C13 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C13 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C14, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C14 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C14, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C14, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C14 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C14 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C14 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C14 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C15, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C15 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C15, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C15, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C15 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C15 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C15 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C15 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, C16, R1, W1, H1, 0, 12, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 2, C16 + O3, R2, 10, H4, 0, 2, 0, 0,
		0, BRIGHTON_THREEWAY},
	{"", 0, C16, R3, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		0},
	{"", 0, C16, R4, W1, H1, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0,
		BRIGHTON_NOTCH},
	{"", 12, C16 + O3, R5, W3, H3, 0, 4, 0, 0, 0, 0},
	{"", 2, C16 + O2, R6, W2, H2, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 0, C16 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C16 + O2, R6, W2, H2, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

};

/*
 *	start/pause + flash, stop=return to start.			2 + led
 *	4 screen selectors + 4 active selectors a/b/c/d		8 + 8 led
 *
 *	speed, up, down, u/d, rand, trig, duty cycle.
 *		rotary * 2 + 5.
 *
 *	memories		12
 *
 *	controller mapping - tuning, mod, etc.
 *	display + option + up/down
 *
 *	8+7(+5pad+)8+3+12+4(+12pad)=60
 */
#define CR1 300
#define CR2 500
#define CW1 30
#define CH1 200

static brightonLocations bm_controls[CONTROL_COUNT] = {
	/* Speed, Duty, 4xdirection: 0 6 6 */
	{"", 0, 20, 255, 50, 500, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 0, 100, 255, 50, 500, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 0, 0},
	{"", 2, 230, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_u.xpm",
		"bitmaps/digits/kbd_u.xpm", BRIGHTON_NOSHADOW},
	{"", 2, 230, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_down.xpm",
		"bitmaps/digits/kbd_down.xpm",	0},
	{"", 2, 260, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_ud.xpm",
		"bitmaps/digits/kbd_ud.xpm",	0},
	{"", 2, 260, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_r.xpm",
		"bitmaps/digits/kbd_r.xpm",	0},
	/* A/B/C/D active: 6 4 10 */
	{"", 2, 340, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_a.xpm",
		"bitmaps/digits/kbd_a.xpm",	BRIGHTON_NOSHADOW},
	{"", 2, 370, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_b.xpm",
		"bitmaps/digits/kbd_b.xpm",	BRIGHTON_NOSHADOW},
	{"", 2, 400, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_c.xpm",
		"bitmaps/digits/kbd_c.xpm",	BRIGHTON_NOSHADOW},
	{"", 2, 430, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_d.xpm",
		"bitmaps/digits/kbd_d.xpm",	0},
	/* A/B/C/D select: 10 4 14 */
	{"", 2, 340, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_a.xpm",
		"bitmaps/buttons/touchnlW.xpm", BRIGHTON_CHECKBUTTON|0},
	{"", 2, 370, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_b.xpm",
		"bitmaps/buttons/touchnlW.xpm", BRIGHTON_CHECKBUTTON|0},
	{"", 2, 400, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_c.xpm",
		"bitmaps/buttons/touchnlW.xpm", BRIGHTON_CHECKBUTTON|0},
	{"", 2, 430, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_d.xpm",
		"bitmaps/buttons/touchnlW.xpm", BRIGHTON_CHECKBUTTON|0},
	/* 6 pad: 14 6 20 */
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	/* ABCD led * 2: 20 8 28  */
	{"", 12, 350, 200, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 380, 200, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 410, 200, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 440, 200, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 350, 730, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 380, 730, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 410, 730, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 440, 730, 11, 85, 0, 4, 0, 0, 0, 0},
	/* Dir led * 4: 28 4 32 */
	{"", 12, 240, 200, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 240, 730, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 270, 200, 11, 85, 0, 4, 0, 0, 0, 0},
	{"", 12, 270, 730, 11, 85, 0, 4, 0, 0, 0, 0},
	/* Memory 12: 32 12 44 */
	{"", 2, 530, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_0.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 560, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_1.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 590, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_2.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 620, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_3.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, 650, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_4.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},

	{"", 2, 530, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_5.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},
	{"", 2, 560, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_6.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},
	{"", 2, 590, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_7.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},
	{"", 2, 620, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_8.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},
	{"", 2, 650, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_9.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},

	{"", 2, 680, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_ld.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},
	{"", 2, 680, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_s.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},
	/* Start/Stop + LED */
	{"", 2, 175, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_st.xpm",
		"bitmaps/digits/kbd_st.xpm", 0},
	{"", 2, 175, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_stop.xpm",
		"bitmaps/digits/kbd_stop.xpm", 0},
	{"", 12, 185, 200, 11, 80, 0, 4, 0, 0, 0, 0},
	{"", 12, 185, 730, 11, 80, 0, 4, 0, 0, 0, 0},
	/* 8 pad: 52 8 60 */
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 9, 9, 9, 9, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	/* Display 5: 55 5 60 */
	{"", 2, 740, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_ua.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},
	{"", 2, 740, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_da.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 2, 950, CR1, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_fn.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON|0},
	{"", 2, 950, CR2, CW1, CH1, 0, 1, 0, "bitmaps/digits/kbd_ret.xpm",
		"bitmaps/buttons/touchnlW.xpm",BRIGHTON_CHECKBUTTON},
	{"", 3, 775, 415, 170, 123, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}
};

static int
bmMidiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->bank = value - (value % 8);
			synth->location = value % 8;
			loadMemory(synth, "bassmaker", 0, synth->bank * 10 + synth->location,
				synth->mem.active, 0, 0);
			break;
		/*
		 * This needs a case statement for midi clock which can then be used
		 * to drive the fast timers.
		 */
	}
	return(0);
}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp bmApp = {
	"bassmaker",
	0,
	"bitmaps/textures/metal2.xpm",
	BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* default is tesselate */
	bmInit,
	bmConfigure, /* 3 callbacks, unused? */
	bmMidiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	820, 410, 0, 0,
	8,
	{
		{
			"BM-20",
			"bitmaps/blueprints/bm.xpm",
			"bitmaps/textures/metal2.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* flags */
			0,
			bmConfigure,
			bmCallback,
			11, 243, 976, 690,
			TOTAL_DEVS,
			locations
		},
		{
			"BM-20",
			"bitmaps/blueprints/bm.xpm",
			"bitmaps/textures/metal2.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN, /* flags */
			0,
			bmConfigure,
			bmCallback,
			11, 243, 976, 690,
			TOTAL_DEVS,
			locations
		},
		{
			"BM-20",
			"bitmaps/blueprints/bm.xpm",
			"bitmaps/textures/metal2.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN, /* flags */
			0,
			bmConfigure,
			bmCallback,
			11, 243, 976, 690,
			TOTAL_DEVS,
			locations
		},
		{
			"BM-20",
			"bitmaps/blueprints/bm.xpm",
			"bitmaps/textures/metal2.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN, /* flags */
			0,
			bmConfigure,
			bmCallback,
			11, 243, 976, 690,
			TOTAL_DEVS,
			locations
		},
		{
			"Mods",
			"bitmaps/blueprints/bmmods.xpm",
			//"bitmaps/textures/metal7.xpm", /* flags */
			"bitmaps/textures/metal2.xpm",
			BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_STRETCH,
			0,
			0,
			bmCallback,
			12, 0, 977, 240,
			CONTROL_COUNT,
			bm_controls
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood6.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			14, 930, 974, 60,
			0,
			0
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

int
bmMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
//	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
bmMemShim(guiSynth *synth)
{
	int i;
	bmMem *bm = (bmMem *) synth->mem.param;
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;

	/*
	 * We need to review the button state and from that set the per note LED to
	 * the desired color
	 */
	for (i = 0; i < STEP_COUNT; i++)
	{
		event.value = bm->page[i / PAGE_STEP].step[i % PAGE_STEP].button;
//printf("Shim %i: %i %i = %f\n", i, i / PAGE_STEP, (i % PAGE_STEP) * OP_COUNT + 4, event.value);
		brightonParamChange(synth->win, i / PAGE_STEP,
			(i % PAGE_STEP) * OP_COUNT + 4, &event);
	}

	synth->transpose = bm->control.transpose - 12;
}

#define BM_DIR_UP 0
#define BM_DIR_DOWN 1
#define BM_DIR_UPDOWN 2
#define BM_DIR_RAND 3

#define BM_INC_A	0x01
#define BM_INC_B	0x02
#define BM_INC_C	0x04
#define BM_INC_D	0x08

static void
bmStuffPage(guiSynth *synth, int page, int dir)
{
	if (dir == BM_DIR_UP)
	{
		brightonFastTimer(synth->win, page, 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 12, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 20, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 28, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 36, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 44, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 52, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 60, BRIGHTON_FT_REQ, 0);

		brightonFastTimer(synth->win, page, 68, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 76, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 84, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 92, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 100, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 108, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 116, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 124, BRIGHTON_FT_REQ, 0);
	} else if (dir == BM_DIR_DOWN) {
		brightonFastTimer(synth->win, page, 124, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 116, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 108, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 100, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 92, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 84, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 76, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 68, BRIGHTON_FT_REQ, 0);

		brightonFastTimer(synth->win, page, 60, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 52, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 44, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 36, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 28, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 20, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 12, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page, 4, BRIGHTON_FT_REQ, 0);
	} else if (dir == BM_DIR_RAND) {
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
		brightonFastTimer(synth->win, page,
			(rand() & 0x0f) * 8 + 4, BRIGHTON_FT_REQ, 0);
	}
}

static void
bmStuffCallbacks(guiSynth *synth)
{
	bmMem *bm = (bmMem *) synth->mem.param;

	int seq, inc = 0;

	if (synth->flags & MEM_LOADING)
		return;

	/*
	 * The next two changes would disable the sequence if we change it, that
	 * is arguably correct?
	event.value = BRIGHTON_LED_OFF;
	brightonParamChange(synth->win, 4, 46, &event);
	event.value = BRIGHTON_LED_RED;
	brightonParamChange(synth->win, 4, 47, &event);
	 */

	/*
	 * We have to consider which pages we are going to include, the page and
	 * note ordering (U, D, UD, R) and perhaps eventually a time code 3/4, 4/4?
	brightonFastTimer(synth->win, 0, 4, BRIGHTON_FT_REQ, 0);
	 */

	if (bm->control.up != 0) {
		printf("Sequence Up: include page");
		seq = BM_DIR_UP;
	}
	if (bm->control.down != 0) {
		printf("Sequence down: include page");
		seq = BM_DIR_DOWN;
	}
	if (bm->control.updown != 0) {
		printf("Sequence updown: include page");
		seq = BM_DIR_UPDOWN;
	}
	if (bm->control.rand != 0) {
		printf("Sequence rand: include page");
		seq = BM_DIR_RAND;
	}

	if (bm->control.active[0] != 0) {
		printf(" A");
		inc |= BM_INC_A;
	}
	if (bm->control.active[1] != 0) {
		printf(" B");
		inc |= BM_INC_B;
	}
	if (bm->control.active[2] != 0) {
		printf(" C");
		inc |= BM_INC_C;
	}
	if (bm->control.active[3] != 0) {
		printf(" D");
		inc |= BM_INC_D;
	}

	brightonFastTimer(0, 0, 0, BRIGHTON_FT_CANCEL, 0);

	if (bm->control.up != 0) {
		/* Stuff the straight sequence */
		if (bm->control.active[0] != 0)
			bmStuffPage(synth, 0, BM_DIR_UP);
		if (bm->control.active[1] != 0)
			bmStuffPage(synth, 1, BM_DIR_UP);
		if (bm->control.active[2] != 0)
			bmStuffPage(synth, 2, BM_DIR_UP);
		if (bm->control.active[3] != 0)
			bmStuffPage(synth, 3, BM_DIR_UP);
	} else if (bm->control.down != 0) {
		if (bm->control.active[0] != 0)
			bmStuffPage(synth, 0, BM_DIR_DOWN);
		if (bm->control.active[1] != 0)
			bmStuffPage(synth, 1, BM_DIR_DOWN);
		if (bm->control.active[2] != 0)
			bmStuffPage(synth, 2, BM_DIR_DOWN);
		if (bm->control.active[3] != 0)
			bmStuffPage(synth, 3, BM_DIR_DOWN);
	} else if (bm->control.updown != 0) {
		if (bm->control.active[0] != 0)
			bmStuffPage(synth, 0, BM_DIR_UP);
		if (bm->control.active[1] != 0)
			bmStuffPage(synth, 1, BM_DIR_UP);
		if (bm->control.active[2] != 0)
			bmStuffPage(synth, 2, BM_DIR_UP);
		if (bm->control.active[3] != 0)
			bmStuffPage(synth, 3, BM_DIR_UP);
		if (bm->control.active[3] != 0)
			bmStuffPage(synth, 3, BM_DIR_DOWN);
		if (bm->control.active[2] != 0)
			bmStuffPage(synth, 2, BM_DIR_DOWN);
		if (bm->control.active[1] != 0)
			bmStuffPage(synth, 1, BM_DIR_DOWN);
		if (bm->control.active[0] != 0)
			bmStuffPage(synth, 0, BM_DIR_DOWN);
	} else if (bm->control.rand != 0) {
		/*
		 * Choose a random page, then request a random order of the notes.
		 */
		bmStuffPage(synth, rand() & 0x03, BM_DIR_RAND);
		bmStuffPage(synth, rand() & 0x03, BM_DIR_RAND);
		bmStuffPage(synth, rand() & 0x03, BM_DIR_RAND);
		bmStuffPage(synth, rand() & 0x03, BM_DIR_RAND);
	}

	printf("\n");

	if (bm->control.start != 0)
		brightonFastTimer(0, 0, 0, BRIGHTON_FT_START, 0);
}

static int rmi = 0;

static int
bmMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	printf("bmMemory(%i %i %i %i %i)\n", fd, chan, c, o, v);

	if (synth->flags & MEM_LOADING)
		return(0);
	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	/*
	 * We have 12 buttons, 0..9, load, save.
	 */
	switch (c) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			if (rmi) {
				rmi = 0;
				synth->location = c;
			} else if ((synth->location = synth->location * 10 + c) >= 1000)
				synth->location = c;
			displayPanelText(synth, "MEM", synth->location, 4, DISPLAY_DEV);
			break;
		case 10:
			/*
			 * We have to send a notes off before reloading a sequence otherwise
			 * we might leave notes dangling.
			 */
			bristolMidiSendMsg(fd, synth->sid, 127, 0, BRISTOL_ALL_NOTES_OFF);
			if (loadMemory(synth, "bassmaker", 0, synth->location,
				synth->mem.active, 0, BRISTOL_FORCE) < 0)
				displayPanelText(synth, "FREE", synth->location, 4,DISPLAY_DEV);
			else
				displayPanelText(synth, "LOAD", synth->location, 4,DISPLAY_DEV);
			bmStuffCallbacks(synth);
			bmMemShim(synth);
			rmi = 1;
			break;
		case 11:
			if (brightonDoubleClick(dc)) {
				displayPanelText(synth, "SAVE", synth->location, 4,DISPLAY_DEV);
				saveMemory(synth, "bassmaker", 0, synth->location, 0);
				rmi = 1;
			}
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
bmCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

//printf("bmCallback(%i, %i, %f) %x\n", panel, index, value, synth);

	if (synth == 0)
		return(0);

	switch (panel) {
		case 1:
			index += TOTAL_DEVS;
			break;
		case 2:
			index += TOTAL_DEVS * 2;
			break;
		case 3:
			index += TOTAL_DEVS * 3;
			break;
		case 4:
			index += TOTAL_DEVS * 4;
			break;
	}

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (bmApp.resources[panel].devlocn[index].to == 1.0)
		sendvalue = value * (CONTROLLER_RANGE - 1);
	else
		sendvalue = value;

	synth->mem.param[index] = value;

/*	if ((!global.libtest) || (index >= ACTIVE_DEVS)) */
//	if ((!global.libtest) || (index >= 40))
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
/*
//	else
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
//
*/
	return(0);
}

static void
bmPanelSwitch(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	bmMem *bm = (bmMem *) synth->mem.param;

	event.type = BRIGHTON_FLOAT;
	event.value = BRIGHTON_LED_OFF;
	brightonParamChange(synth->win, 4, 24, &event);
	brightonParamChange(synth->win, 4, 25, &event);
	brightonParamChange(synth->win, 4, 26, &event);
	brightonParamChange(synth->win, 4, 27, &event);

	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 0;
	brightonParamChange(synth->win, 0, -1, &event);
//	event.type = BRIGHTON_EXPOSE;
//	event.intvalue = 0;
	brightonParamChange(synth->win, 1, -1, &event);
//	event.type = BRIGHTON_EXPOSE;
//	event.intvalue = 0;
	brightonParamChange(synth->win, 2, -1, &event);
//	event.type = BRIGHTON_EXPOSE;
//	event.intvalue = 0;
	brightonParamChange(synth->win, 3, -1, &event);

	event.type = BRIGHTON_FLOAT;
	event.value = BRIGHTON_LED_YELLOW_FLASHING;
	brightonParamChange(synth->win, 4, 24+c, &event);

	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, c, -1, &event);

	switch (c) {
		case 0:
			displayPanelText(synth, "panel A", c + 1, 4, DISPLAY_DEV);
			bm->pageSelect = 0;
			break;
		case 1:
			displayPanelText(synth, "panel B", c + 1, 4, DISPLAY_DEV);
			bm->pageSelect = 1;
			break;
		case 2:
			displayPanelText(synth, "panel C", c + 1, 4, DISPLAY_DEV);
			bm->pageSelect = 2;
			break;
		case 3:
			displayPanelText(synth, "panel D", c + 1, 4, DISPLAY_DEV);
			bm->pageSelect = 3;
			break;
	}
}

static void
bmSendControl(guiSynth *synth, bmMem *bm, int c, int o)
{
	/*
	 * Control can have various possibilities ranging from a basic controller
	 * change to note events and targetted device events.
	 */
	switch ((int) bm->control.ctype) {
		case BM_C_CONTROL:
			/*
			 * Because of the way control.cc is implemented, this is actually
			 * the second MIDI channel: this gives access to the first 15 CC
			 */
			bristolMidiSendMsg(global.controlfd, synth->midichannel, 0,
				bm->control.cc, (int)
					(bm->page[c].step[o].cont * C_RANGE_MIN_1));
			break;
		case BM_C_NOTE:
		{
			int note, chan, velocity;

			/*
			 * Convert the setting into 12 steps, take cc to be midi channel
			 * Channel is the 'cc' setting here.
			 */
			if ((chan = bm->control.cc) > 15)
				chan = 15;
			note = (int) (bm->page[c].step[o].cont * 12);
			velocity = (int) (bm->page[c].step[o].velocity * 127);

			note += synth->transpose
				+ bm->page[c].step[o].transp * 12;

			bristolMidiSendKeyMsg(global.controlfd, chan,
				BRISTOL_EVENT_KEYOFF, bm->lastcnote, velocity);
			bristolMidiSendKeyMsg(global.controlfd, chan,
				BRISTOL_EVENT_KEYON, note, velocity);

			bm->lastcnote = note;

			break;
		}
		case BM_C_GLIDE:
			/*
			 * This is 126/0 for most emulators and that may need to be 
			 * enforced later.
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 0,
				(int) (bm->page[c].step[o].cont * C_RANGE_MIN_1));
			break;
		case BM_C_TUNE:
			/*
			 * This is 126/1 for most emulators and that may need to be 
			 * enforced later.
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 1,
				(int) (bm->page[c].step[o].cont * C_RANGE_MIN_1));
			break;
		case BM_C_MOD:
			/*
			 * This is CC #1 for most emulators and that may need to be 
			 * enforced later.
			 */
			bristolMidiSendMsg(global.controlfd, synth->midichannel, 0,
				1, (int) (bm->page[c].step[o].cont * C_RANGE_MIN_1));
			break;
		case BM_C_OPERATOR:
			/*
			 * cop = operator, 126 will be typical use.
			 * cc = controller of operator
			 */
			break;
	}
}

static void
bmCallNote(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bmMem *bm = (bmMem *) synth->mem.param;
	int note = bm->page[c].step[o].note + synth->transpose
		+ bm->page[c].step[o].transp * 12;
	int velocity = (int) (bm->page[c].step[o].velocity * 127);
//printf("bmCallNote(%i, %i, %i)\n", c, o, v);
	/*
	 * If we are stopped then send the note, otherwise do nothing
	 */
	if (bm->control.stop != 0) {
		bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, bm->lastnote, velocity);
		bmSendControl(synth, bm, c, o);
		bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, note, velocity);
	}
	bm->lastnote = note;
	bm->lastpage = c;
	bm->laststep = o;
}

static void
bmCallTranspose(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bmMem *bm = (bmMem *) synth->mem.param;
	int note = bm->page[c].step[o].note + synth->transpose
		+ bm->page[c].step[o].transp * 12;
	int velocity = (int) (bm->page[c].step[o].velocity * 127);
	/*
	 * If we are stopped then send the note, otherwise do nothing
	 */
	if (bm->control.stop != 0) {
		bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, bm->lastnote, velocity);
		bmSendControl(synth, bm, c, o);
		bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, note, velocity);
	}
	bm->lastnote = note;
	bm->lastpage = c;
	bm->laststep = o;
//printf("bmCallTranspose(%i, %i, %i)\n", c, o, v);
}

static void
bmCallVolume(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bmMem *bm = (bmMem *) synth->mem.param;
	int note = bm->page[c].step[o].note + synth->transpose
		+ bm->page[c].step[o].transp * 12;
	int velocity = (int) (bm->page[c].step[o].velocity * 127);
	/*
	 * If we are stopped then send the note, otherwise do nothing
	 */
	if (bm->control.stop != 0) {
		bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, bm->lastnote, velocity);
		bmSendControl(synth, bm, c, o);
		bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, note, velocity);
	}
	bm->lastnote = note;
	bm->lastpage = c;
	bm->laststep = o;
//printf("bmCallVolume(%i, %i, %i)\n", c, o, v);
}

static void
bmCallController(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * If we are stopped then send the controller, otherwise do nothing
	 */
//printf("bmCallController(%i, %i, %i)\n", c, o, v);
}

static void
bmCallLed(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bmMem *bm = (bmMem *) synth->mem.param;
	int note;
	int velocity;

	/*
	 * If we are playing then send the note on/off + control. Depending on the
	 * configured LED state we may skip the note, skip the note off event, etc.
	 */
//synth->mem.param[c * TOTAL_DEVS + o * OP_COUNT + 5]);

	note = bm->page[c].step[o].note + synth->transpose
		+ bm->page[c].step[o].transp * 12;
	velocity = (int) (bm->page[c].step[o].velocity * 127);

//printf("bmCallLed(%i, %i, %i) note %i, v %i \n", c, o, v, note, velocity);

	switch ((int) bm->page[c].step[o].button) {
		case 0:
		default:
			/* Send note on/off according to value */
			if (v)
			{
				bmSendControl(synth, bm, c, o);
				bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
					BRISTOL_EVENT_KEYON, note, velocity);
			} else
				bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
					BRISTOL_EVENT_KEYOFF, note, velocity);
			break;
		case 1:
			/* Dont' send note off */
			if (v)
				{
					bmSendControl(synth, bm, c, o);
					bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
						BRISTOL_EVENT_KEYON, note, velocity);
				}
			break;
		case 2:
			/* Dont' send note on, only off */
			bristolMidiSendKeyMsg(global.controlfd, synth->midichannel,
				BRISTOL_EVENT_KEYOFF, note, velocity);
			return;
		case 3:
			/*
			 * all notes off? Nothing?
			bristolMidiSendMsg(fd, synth->sid, 127, 0, BRISTOL_ALL_NOTES_OFF);
			 */
			return;
	}
	bm->lastnote = note;
	bm->lastpage = c;
	bm->laststep = o;
}

static void
bmCallButton(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	printf("bmCallButton(%i, %i, %i)\n", c, o, c * TOTAL_DEVS + 0);

	event.type = BRIGHTON_FLOAT;

	if (++synth->mem.param[c * TOTAL_DEVS + o * OP_COUNT + 4] > 3)
		synth->mem.param[c * TOTAL_DEVS + o * OP_COUNT + 4] = 0;

	event.value = synth->mem.param[c * TOTAL_DEVS + o * OP_COUNT + 4];

	brightonParamChange(synth->win, c, o * OP_COUNT + 4, &event);

	synth->mem.param[c * TOTAL_DEVS + o * OP_COUNT + 5]
		= synth->mem.param[c * TOTAL_DEVS + o * OP_COUNT + 4];
}

static void
bmCallPad1(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
}

static void
bmCallPad2(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
}

static void
bmSpeed(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * We need to stuff both speed and duty cycle
	printf("speed %f, duty %f\n",
		10 + synth->mem.param[COFF + 0] * synth->mem.param[COFF + 0] * 990,
		(10 + synth->mem.param[COFF + 0] * synth->mem.param[COFF + 0] * 990) *
			synth->mem.param[COFF + 1]);
	 */

	/*
	 * Speed should go from 10 to 1000ms? Can change that later.
	 */
	brightonFastTimer(0, 0, 0, BRIGHTON_FT_TICKTIME,
		10 + synth->mem.param[COFF + 0] * synth->mem.param[COFF + 0] * 990);
	brightonFastTimer(0, 0, 0, BRIGHTON_FT_DUTYCYCLE,
		(10 + synth->mem.param[COFF + 0] * synth->mem.param[COFF + 0] * 990)
			* synth->mem.param[COFF + 1]);
}

static int ssEx = 0;

static void
bmDirection(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (v ==0)
		return;

	/*
	 * Clear all the LED, light this one, stuff the callback table
	 */
	event.type = BRIGHTON_FLOAT;
	event.value = BRIGHTON_LED_OFF;
	brightonParamChange(synth->win, 4, 28, &event);
	brightonParamChange(synth->win, 4, 29, &event);
	brightonParamChange(synth->win, 4, 30, &event);
	brightonParamChange(synth->win, 4, 31, &event);

	switch (c) {
		case 0:
			synth->mem.param[COFF + 3] = synth->mem.param[COFF + 4]
				= synth->mem.param[COFF + 5] = 0;
			break;
		case 1:
			synth->mem.param[COFF + 2] = synth->mem.param[COFF + 4]
				= synth->mem.param[COFF + 5] = 0;
			break;
		case 2:
			synth->mem.param[COFF + 2] = synth->mem.param[COFF + 3]
				= synth->mem.param[COFF + 5] = 0;
			break;
		case 3:
			synth->mem.param[COFF + 2] = synth->mem.param[COFF + 3]
				= synth->mem.param[COFF + 4] = 0;
			break;
	}

	event.value = BRIGHTON_LED_GREEN;
	brightonParamChange(synth->win, 4, 28 + c, &event);

	bmStuffCallbacks(synth);
}

/*
 *	Controller options
 *		Controller to mod wheel
 *		Controller to tune
 *		Controller to pitchwheel
 *		Controller to note (13 steps) + channel
 *		Controller to other continuous controller
 *		Controller to operator parameter (later)
 *	Copy page
 *	Save
 *	Load
 */

static void
bmCopyPage(guiSynth *synth, int fd, int chan, bmMem *bm, int from, int to)
{
	int i;
	brightonEvent event;

	if (from == to)
		return;

	event.type = BRIGHTON_FLOAT;
	for (i = 0; i < PAGE_STEP; i++)
	{
		event.value = bm->page[from].step[i].note;
		brightonParamChange(synth->win, to, i * OP_COUNT + 0, &event);
		event.value = bm->page[from].step[i].transp;
		brightonParamChange(synth->win, to, i * OP_COUNT + 1, &event);
		event.value = bm->page[from].step[i].velocity;
		brightonParamChange(synth->win, to, i * OP_COUNT + 2, &event);
		event.value = bm->page[from].step[i].cont;
		brightonParamChange(synth->win, to, i * OP_COUNT + 3, &event);
		event.value = bm->page[from].step[i].led;
		brightonParamChange(synth->win, to, i * OP_COUNT + 4, &event);
	}
}

static char * copyMenu[5] = {
	"copy to A",
	"copy to B",
	"copy to C",
	"copy to D",
	0
};

static char * ctlMenu[6] = {
	"CTL CC",
	"CTL NOTE",
	"CTL TUNE",
	"CTL Glide",
	"CTL MOD",
	0
};

static char *transpMenu[26] = {
	"-12 Semitones",
	"-11 Semitones",
	"-10 Semitones",
	"-9 Semitones",
	"-8 Semitones",
	"-7 Semitones",
	"-6 Semitones",
	"-5 Semitones",
	"-4 Semitones",
	"-3 Semitones",
	"-2 Semitones",
	"-1 Semitone",
	"0 Semitones",
	"+1 Semitone",
	"+2 Semitones",
	"+3 Semitones",
	"+4 Semitones",
	"+5 Semitones",
	"+6 Semitones",
	"+7 Semitones",
	"+8 Semitones",
	"+9 Semitones",
	"+10 Semitones",
	"+11 Semitones",
	"+12 Semitones",
	0
};

static char *midiChan[17] = {
	"MIDI CH 1", /* Bass channel selection. These should be submenu */
	"MIDI CH 2",
	"MIDI CH 3",
	"MIDI CH 4",
	"MIDI CH 5",
	"MIDI CH 6",
	"MIDI CH 7",
	"MIDI CH 8",
	"MIDI CH 9",
	"MIDI CH 10",
	"MIDI CH 11",
	"MIDI CH 12",
	"MIDI CH 13",
	"MIDI CH 14",
	"MIDI CH 15",
	"MIDI CH 16",
	0
};

static char *clearMenu[6] = {
	"Clear Note",
	"Clear Tranpose",
	"Clear Volume",
	"Clear Cont",
	"Clear Trigger",
	0
};

static char *memMenu[4] = {
	"Find Up",
	"Mem Down",
	"Mem Up",
	0
};

typedef struct bMenuList {
	char *title;
	int count;
	char **content;
} bMenuList;

#define B_M_C 7

typedef struct BMenu {
	bMenuList item[B_M_C];
	int count;
	int current;
	int sub;
} bMenu;

bMenu mainMenu = {
	{
		{"Memory", 3, memMenu},
		{"Copy", 4, copyMenu},
		{"Controllers", 5, ctlMenu},
		{"First MIDI", 16, midiChan},
		{"Second MIDI", 16, midiChan},
		{"Clear", 5, clearMenu},
		{"Transpose", 25, transpMenu},
	},
	B_M_C, 0, -1
};

static void
bmMultiMenuMove(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * We could be in the main menu or a submenu, need to check the submenu
	 * setting then move through the text strings.
	 */
	if (mainMenu.sub == -1) {
		/*
		 * Main menu
		 */
		if (v == 0)
		{
			if (--mainMenu.current < 0)
				mainMenu.current = mainMenu.count - 1;

			displayPanel(synth, mainMenu.item[mainMenu.current].title,
				synth->location, 4, DISPLAY_DEV);
		} else {
			if (++mainMenu.current >= mainMenu.count)
				mainMenu.current = 0;

			displayPanel(synth, mainMenu.item[mainMenu.current].title,
				synth->location, 4, DISPLAY_DEV);
		}
	} else {
		/*
		 * Sub menu
		 */
		if (v == 0)
		{
			if (--mainMenu.sub < 0)
				mainMenu.sub = mainMenu.item[mainMenu.current].count - 1;

			displayPanel(synth,
				mainMenu.item[mainMenu.current].content[mainMenu.sub],
				synth->location, 4, DISPLAY_DEV);
		} else {
			if (++mainMenu.sub >= mainMenu.item[mainMenu.current].count)
				mainMenu.sub = 0;

			displayPanel(synth,
				mainMenu.item[mainMenu.current].content[mainMenu.sub],
				synth->location, 4, DISPLAY_DEV);
		}
	}
}

static void
bmMenuFunction(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * For now just return to start of main menu
	 */
	mainMenu.sub = -1;

	displayPanel(synth, mainMenu.item[mainMenu.current].title,
		synth->location, 4, DISPLAY_DEV);
}

static void
bmMultiMenuEnter(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bmMem *bm = (bmMem *) synth->mem.param;

	/*
	 * Execute the desired function and save to a memory dummy location if
	 * necessary (controller functions).
	 */
	if (mainMenu.sub < 0)
	{
		/*
		 * We should preload a value here which is the current selected value
		 */
		switch (mainMenu.current)
		{
			case 3:
				mainMenu.sub = synth->midichannel;
				break;
			case 4:
				mainMenu.sub = bm->control.cc;
				break;
			case 6:
				mainMenu.sub = synth->transpose + 12;
				break;
			default:
				mainMenu.sub = 0;
				break;
		}

		displayPanel(synth,
			mainMenu.item[mainMenu.current].content[mainMenu.sub],
			synth->location, 4, DISPLAY_DEV);
		return;
	}

	switch (mainMenu.current) {
		case 0:
			/* Memory */
			switch (mainMenu.sub) {
				case 2:
				{
					/* Mem Up */
					int location = synth->location;

					while (loadMemory(synth, "bassmaker", 0, ++location,
						synth->mem.active, 0, BRISTOL_STAT) < 0)
					{
						if (location == synth->location)
							break;
						if (location == 1023)
							location = -1;
					}
					synth->location = location;
					displayPanelText(synth, "Mem", synth->location,
						4, DISPLAY_DEV);
					break;
				}
				case 1:
				{
					/* Mem Down */
					int location;

					if ((location = synth->location) == 0)
						location = 1024;

					while (loadMemory(synth, "bassmaker", 0, --location,
						synth->mem.active, 0, BRISTOL_STAT) < 0)
					{
						if (location == synth->location)
							break;
						if (location == 0)
							location = 1024;
					}
					synth->location = location;
					displayPanelText(synth, "Mem", synth->location,
						4, DISPLAY_DEV);
					break;
				}
				case 0:
				{
					/* Mem Find Up */
					int location = synth->location;

					while (loadMemory(synth, "bassmaker", 0, ++location,
						synth->mem.active, 0, BRISTOL_STAT) >= 0)
					{
						if (location == synth->location)
							break;
						if (location == 1023)
							location = 0;
					}
					synth->location = location;
					displayPanelText(synth, "Free", synth->location,
						4, DISPLAY_DEV);
					break;
				}
			}
			break;
		case 1:
			/* Copy */
			switch (mainMenu.sub) {
				case 0:
					/* Copy page */
					bmCopyPage(synth, fd, chan, bm, bm->pageSelect, 0);
					break;
				case 1:
					/* Copy page */
					bmCopyPage(synth, fd, chan, bm, bm->pageSelect, 1);
					break;
				case 2:
					/* Copy page */
					bmCopyPage(synth, fd, chan, bm, bm->pageSelect, 2);
					break;
				case 3:
					/* Copy page */
					bmCopyPage(synth, fd, chan, bm, bm->pageSelect, 3);
					break;
			}
			break;
		case 2:
			/*
			 * Controllers
			 */
			switch (mainMenu.sub) {
				case 0:
					/* CC */
					bm->control.ctype = 11;
					break;
				case 1:
					bm->control.ctype = -1;
					break;
				case 2:
					/* Set control function Pitch */
					bm->control.ctype = -2;
					break;
				case 3:
					bm->control.ctype = -2;
					break;
				case 4:
					bm->control.ctype = -4;
					break;
			}

			break;
		case 3:
			/* Fist MIDI */
			synth->midichannel = mainMenu.sub;
			break;
		case 4:
			/* Second MIDI */
			bm->control.cc = mainMenu.sub;
			break;
		case 5:
			/* Clear Menu ops */
			{
				int i, page, offset;
				brightonEvent event;

				event.type = BRIGHTON_FLOAT;

				page = bm->pageSelect;
				offset = mainMenu.sub;

				switch (offset)
				{
					case 0:
					default:
						event.value = 0;
						break;
					case 1:
						event.value = 1.0;
						break;
					case 2:
						event.value = 0.8;
						break;
					case 3:
						event.value = 0.5;
						break;
				}

				for (i = 0; i < PAGE_STEP; i++)
				{
					brightonParamChange(synth->win, page,
						i * OP_COUNT + offset, &event);
				}
			}
			break;
		case 6:
			/* Transpose */
			bm->control.transpose = BM_TRANSPOSE + mainMenu.sub - 12;
			synth->transpose = BM_TRANSPOSE + mainMenu.sub - 12;
			break;
	}
}

static void
bmMenu(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	printf("bmMenu(%i, %i, %i)\n", c, o, v);

	switch(c) {
		case 0:
			/* Up menu */
			bmMultiMenuMove(synth, fd, chan, c, o, 1);
			break;
		case 1:
			/* Down menu */
			bmMultiMenuMove(synth, fd, chan, c, o, 0);
			break;
		case 2:
			/* Function */
			bmMenuFunction(synth, fd, chan, c, o, v);
			break;
		case 3:
			/* Enter */
			bmMultiMenuEnter(synth, fd, chan, c, o, v);
			break;
	}
}

static void
bmPageSelect(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;
	if (v == 0)
		event.value = BRIGHTON_LED_OFF;
	else
		event.value = BRIGHTON_LED_GREEN;
	brightonParamChange(synth->win, 4, 20 + c, &event);

	bmStuffCallbacks(synth);
}

static void
bmStartStop(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	bmMem *bm = (bmMem *) synth->mem.param;

	if (ssEx) {
		ssEx = 0;
		return;
	}

	event.type = BRIGHTON_FLOAT;

	bristolMidiSendMsg(fd, synth->sid, 127, 0, BRISTOL_ALL_NOTES_OFF);

	if (c == 1) {
		/* Stop */
		event.value = BRIGHTON_LED_OFF;
		brightonParamChange(synth->win, 4, 46, &event);
		event.value = BRIGHTON_LED_RED;
		brightonParamChange(synth->win, 4, 47, &event);

		((bmMem *) synth->mem.param)->lastnote = -1;;

		brightonFastTimer(0, 0, 0, BRIGHTON_FT_STOP, 0);
		/* And send a MIDI all notes off */
		bristolMidiSendMsg(fd, synth->sid, 127, 0, BRISTOL_ALL_NOTES_OFF);

		ssEx = 1;
		event.value = 0.0;
		brightonParamChange(synth->win, 4, 44, &event);
		bm->clearstep = bm->laststep;
		bm->laststep = 0;
	} else {
		/*
		 * There are two cases here, first press and second press. First press
		 * will start the sequence, second will pause the sequence.
		 *
		 * When it is first press we need to clock the first element in the
		 * list.
		 */
		if (bm->clearstep >= 0)
		{
			event.value = BRIGHTON_LED_OFF;
			brightonParamChange(synth->win, bm->lastpage,
				4 + bm->clearstep * 8, &event);
		}
		bm->clearstep = -1;

		/* Turn off Stop LED */
		event.value = BRIGHTON_LED_OFF;
		brightonParamChange(synth->win, 4, 47, &event);

		if (synth->mem.param[COFF + 45] != 0) {
			/* Stopped - toggle to green and run */
			event.value = BRIGHTON_LED_GREEN;
		} else {
			/* If we were not stopped then were we running */
			if (synth->mem.param[COFF + 44] != 0) {
				event.value = BRIGHTON_LED_GREEN;
			} else {
				event.value = BRIGHTON_LED_YELLOW_FLASHING;
			}
		}
		brightonParamChange(synth->win, 4, 46, &event);

		brightonFastTimer(0, 0, 0, BRIGHTON_FT_START, 0);

		event.command = BRIGHTON_FAST_TIMER;
		event.value = 1;
		brightonParamChange(synth->win, bm->lastpage,
			4 + bm->laststep * 8, &event);

		ssEx = 1;
		event.value = 0.0;
		brightonParamChange(synth->win, 4, 45, &event);
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
bmInit(brightonWindow *win)
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

	call.control[0] = (synthRoutine) bmCallNote;
	call.control[1] = (synthRoutine) bmCallTranspose;
	call.control[2] = (synthRoutine) bmCallVolume;
	call.control[3] = (synthRoutine) bmCallController;
	call.control[4] = (synthRoutine) bmCallLed;
	call.control[5] = (synthRoutine) bmCallButton;
	call.control[6] = (synthRoutine) bmCallPad1;
	call.control[7] = (synthRoutine) bmCallPad2;

	synth->win = win;

	printf("Initialise the bm link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(sizeof(bmMem));
	synth->mem.count = sizeof(bmMem) / sizeof(float);
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
		synth->dispatch[i].routine = bmMidiSendMsg;

	for (i = 0; i < PAGE_STEP * PAGE_COUNT * OP_COUNT; i++)
	{
		/* Page */
		synth->dispatch[i].controller = i / (PAGE_STEP * OP_COUNT);
		/* Step */
		synth->dispatch[i].operator = (i / OP_COUNT) % PAGE_STEP;
		/* Control index */
		synth->dispatch[i].routine = call.control[i % OP_COUNT];
	}

	/* Control Panel Speed/Duty */
	synth->dispatch[COFF + 0].controller = 1;
	synth->dispatch[COFF + 1].controller = 2;
	synth->dispatch[COFF + 0].routine
		= synth->dispatch[COFF + 1].routine
			= (synthRoutine) bmSpeed;

	/* Dir */
	synth->dispatch[COFF + 2].controller = 0;
	synth->dispatch[COFF + 3].controller = 1;
	synth->dispatch[COFF + 4].controller = 2;
	synth->dispatch[COFF + 5].controller = 3;
	synth->dispatch[COFF + 2].routine
		= synth->dispatch[COFF + 3].routine
		= synth->dispatch[COFF + 4].routine
		= synth->dispatch[COFF + 5].routine
			= (synthRoutine) bmDirection;

	/* Select/Include */
	synth->dispatch[COFF + 6].controller = 0;
	synth->dispatch[COFF + 7].controller = 1;
	synth->dispatch[COFF + 8].controller = 2;
	synth->dispatch[COFF + 9].controller = 3;
	synth->dispatch[COFF + 6].routine
		= synth->dispatch[COFF + 7].routine
		= synth->dispatch[COFF + 8].routine
		= synth->dispatch[COFF + 9].routine
			= (synthRoutine) bmPageSelect;

	/* Edit */
	synth->dispatch[COFF + 10].controller = 0;
	synth->dispatch[COFF + 11].controller = 1;
	synth->dispatch[COFF + 12].controller = 2;
	synth->dispatch[COFF + 13].controller = 3;
	synth->dispatch[COFF + 10].routine
		= synth->dispatch[COFF + 11].routine
		= synth->dispatch[COFF + 12].routine
		= synth->dispatch[COFF + 13].routine
			= (synthRoutine) bmPanelSwitch;

	synth->dispatch[COFF + 32].controller = 0;
	synth->dispatch[COFF + 33].controller = 1;
	synth->dispatch[COFF + 34].controller = 2;
	synth->dispatch[COFF + 35].controller = 3;
	synth->dispatch[COFF + 36].controller = 4;
	synth->dispatch[COFF + 37].controller = 5;
	synth->dispatch[COFF + 38].controller = 6;
	synth->dispatch[COFF + 39].controller = 7;
	synth->dispatch[COFF + 40].controller = 8;
	synth->dispatch[COFF + 41].controller = 9;

	synth->dispatch[COFF + 42].controller = 10;
	synth->dispatch[COFF + 43].controller = 11;

	/* Start/Stop */
	synth->dispatch[COFF + 44].controller = 0;
	synth->dispatch[COFF + 45].controller = 1;
	synth->dispatch[COFF + 44].routine = synth->dispatch[COFF + 45].routine
		= (synthRoutine) bmStartStop;

	synth->dispatch[COFF + 32].routine
		= synth->dispatch[COFF + 33].routine
		= synth->dispatch[COFF + 34].routine
		= synth->dispatch[COFF + 35].routine
		= synth->dispatch[COFF + 36].routine
		= synth->dispatch[COFF + 37].routine
		= synth->dispatch[COFF + 38].routine
		= synth->dispatch[COFF + 39].routine
		= synth->dispatch[COFF + 40].routine
		= synth->dispatch[COFF + 41].routine
		= synth->dispatch[COFF + 42].routine
		= synth->dispatch[COFF + 43].routine
			= (synthRoutine) bmMemory;

	synth->dispatch[COFF + 55].controller = 0;
	synth->dispatch[COFF + 56].controller = 1;
	synth->dispatch[COFF + 57].controller = 2;
	synth->dispatch[COFF + 58].controller = 3;
	synth->dispatch[COFF + 55].routine
		= synth->dispatch[COFF + 56].routine
		= synth->dispatch[COFF + 57].routine
		= synth->dispatch[COFF + 58].routine
			= (synthRoutine) bmMenu;

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
bmConfigure(brightonWindow *win)
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
	synth->transpose = BM_TRANSPOSE;

	brightonPut(win,
		"bitmaps/blueprints/bmshade.xpm", 0, 0, win->width, win->height);

	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, 1, -1, &event);
	brightonParamChange(synth->win, 2, -1, &event);
	brightonParamChange(synth->win, 3, -1, &event);
	event.intvalue = 0;
	brightonParamChange(synth->win, 1, -1, &event);
	brightonParamChange(synth->win, 2, -1, &event);
	brightonParamChange(synth->win, 3, -1, &event);
	event.intvalue = 1;
	brightonParamChange(synth->win, 0, -1, &event);

	event.type = BRIGHTON_FLOAT;
	event.value = BRIGHTON_LED_YELLOW_FLASHING;
	brightonParamChange(synth->win, 4, 24, &event);
	/* Stop button */
	event.value = BRIGHTON_LED_RED;
	brightonParamChange(synth->win, 4, 47, &event);
	event.value = 1.0;
	brightonParamChange(synth->win, 4, 45, &event);

	event.type = BRIGHTON_INT;
	event.intvalue = 1;
	event.value = 1;
	brightonParamChange(synth->win, 0, 4, &event);
	event.value = 18;
	brightonParamChange(synth->win, 0, 11, &event);
	event.value = 3;
	brightonParamChange(synth->win, 0, 18, &event);
	event.value = 1;
	brightonParamChange(synth->win, 0, 25, &event);

	loadMemory(synth, "bassmaker", 0, synth->location, synth->mem.active, 0, 
		BRISTOL_FORCE);
	bmStuffCallbacks(synth);
	bmMemShim(synth);

	displayPanel(synth, "BASSMAKER", synth->location, 4, DISPLAY_DEV);

	dc = brightonGetDCTimer(win->dcTimeout);

	return(0);
}

