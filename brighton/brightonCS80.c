
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
 * Layer params - 12 are reserved:
 *
 * 	Sqr gain level
 * 	Rmp gain level
 *	Osc Brilliance'
 *	Osc Brilliance''
 *	Filter brilliance
 * 	Pan
 * 	Env1 gain
 * 	Nudge
 * 	Switch/level env2 to LPF? Gives two env for filters. Env panned cutover?
 * 	Glide rates - from starting point, not between notes?
 *
 * Global params:
 *
 * 	Detune
 * 	LFO Key Sync
 * 	Midi Channel selection
 * 	Noise Multi
 * 	LFO Multi
 * 	Nudge
 * 	Nudge Depth
 */

#include <fcntl.h>

#include "brighton.h"
#include "bristolmidi.h"
#include "brightonMini.h"
#include "brightoninternals.h"

#include "bristolarpeggiation.h"

static int cs80Init();
static int cs80Configure();
static int cs80Callback(brightonWindow *, int, int, float);
static int cs80panelswitch(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

static int debuglevel = 0;

#include "brightonKeys.h"

/*
 * These are a pair of double click timers and mw is a memory weighting to
 * scale the 64 available memories into banks of banks
 */
static int dc1, mw = 0;

static int width;

/*
 * We need a scratchpad so that we can load single layers out of existing 
 * memories.
 */
static float *scratch = NULL;

#define DEVICE_COUNT 180
/*
 * Need to split up active into the two layers, mods and opts.
 */
#define ACTIVE_DEVS 145
#define MEM_MGT (ACTIVE_DEVS + 2)
#define LAYER_DEVS 120

#define PATCH_BUTTONS MEM_MGT
#define BANK_BUTTONS (MEM_MGT + 9)
#define LOAD_BUTTON (MEM_MGT + 8)

#define KEY_PANEL 2
#define MODS_PANEL 2

#define RADIO_UB 147
#define RADIO_LB 163
#define RADIO_UP 151
#define RADIO_LP 167

#define PANEL_SELECT (ACTIVE_DEVS + 4)

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
 * This example is for a cs80Bristol type synth interface.
 */
#define W1 17
#define W2 16
#define W3 15
#define L1 200
#define L2 50
#define L3 100
#define L4 120

#define R1 53
#define R1a 100
#define R2 346
#define R2a 396
#define R3 618
#define R3a (R3 + 45)
#define R4 (R3 + L2)
#define R5 (R4 + L2)

#define D1 25
#define D2 (W2)
#define D3 20
#define D4 24

#define C1 270
#define C2 (C1 + D1)
#define C3 (C2 + D1)
#define C4 (C3 + D1 + 5)
#define C5 (C4 + D1)
#define C6 (C5 + D1)
#define C7 (C6 + D1  +D3)
#define C8 (C7 + D1)
#define C9 (C8 + D1)
#define C10 (C9 + D1)
#define C11 (C10 + D1)
#define C12 (C11 + D1)
#define C13 (C12 + D1)
#define C14 (C13 + D1)
#define C15 (C14 + D1)
#define C16 (C15 + D1 + D3)
#define C17 (C16 + D1)
#define C18 (C17 + D1)
#define C19 (C18 + D1)
#define C20 (C19 + D1)
#define C21 (C20 + D1)
#define C22 (C21 + D1)
#define C23 (C22 + D1 + D3)
#define C24 (C23 + D1)
#define C25 (C24 + D1)
#define C26 (C25 + D1)

#define CB1 14
#define CB2 (CB1 + D4 + 8)
#define CB3 (CB2 + D4 + 7)
#define CB4 (CB3 + D4)
#define CB5 (CB4 + D4)
#define CB6 (CB5 + D4)
#define CB7 (CB6 + D4)
#define CB8 (CB7 + D4 + 4)
#define CB9 (CB8 + D4 + 16)
#define CB10 (CB9 + D4)
#define CB11 (CB10 + D4)
#define CB12 (CB11 + D4)
#define CB13 (CB12 + D4)
#define CB14 (CB13 + D4)

#define CB15 665
#define CB16 (CB15 + D4)
#define CB17 (CB16 + D4)
#define CB18 (CB17 + D4 + 10)
#define CB19 (CB18 + D4)
#define CB20 (CB19 + D4)
#define CB21 (CB20 + D4 + 1)
#define CB22 (CB21 + D4 + 8)
#define CB23 (CB22 + D4 + 1)
#define CB24 (CB23 + D4)
#define CB25 (CB24 + D4)
#define CB26 (CB25 + D4 + 10)

#define BC1 405
#define BC2 (BC1 + D2)
#define BC3 (BC2 + D2)
#define BC4 (BC3 + D2)
#define BC5 (BC4 + D2)
#define BC6 (BC5 + D2)
#define BC7 (BC6 + D2)
#define BC8 (BC7 + D2)
#define BC9 (BC8 + D2)
#define BC10 (BC9 + D2)
#define BC11 (BC10 + D2)
#define BC12 (BC11 + D2)
#define BC13 (BC12 + D2)
#define BC14 (BC13 + D2)
#define BC15 (BC14 + D2)
#define BC16 (BC15 + D2)

/*
 * two rows of 25 identical controls for the layered synths, then a load for
 * global controls and presets.
 */
static brightonLocations locations[DEVICE_COUNT] = {
	/* First 26 controls for upper layer - 0 */
	{"", 1, C1, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderwhite2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C2, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C3, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 2, C4, R1a, W3, L3, 0, 1, 0, "bitmaps/buttons/rockerblack.xpm", 0, 
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"", 2, C5, R1a, W3, L3, 0, 1, 0, "bitmaps/buttons/rockerblack.xpm", 0, 
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"", 1, C6, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C7, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C8, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderred2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C9, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C10, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderred2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C11, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C12, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C13, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C14, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C15, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slideryellow.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C16, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C17, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C18, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C19, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C20, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C21, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slideryellow.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C22, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C23, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C24, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C25, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C26, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},

	/* Second 25 controls for lower layer - 26 */
	{"", 1, C1, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderwhite2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C2, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C3, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 2, C4, R2a, W3, L3, 0, 1, 0, "bitmaps/buttons/rockerblack.xpm", 0, 
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"", 2, C5, R2a, W3, L3, 0, 1, 0, "bitmaps/buttons/rockerblack.xpm", 0, 
		BRIGHTON_NOSHADOW|BRIGHTON_VERTICAL},
	{"", 1, C6, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C7, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C8, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderred2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C9, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C10, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderred2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C11, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C12, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C13, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C14, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C15, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slideryellow.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C16, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C17, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C18, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C19, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C20, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C21, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slideryellow.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C22, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C23, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C24, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C25, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, C26, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},

	/*
	 * Global controls 1 - 52
	 * The two knobs will eventually be moved outside of the memory area for
	 * global tune and volume control. We will also install about 16 dummies
	 * to cover for the  control panel and a rake more for diverse parameters
	 */
	{"", 10, CB2, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderwhite7_3.xpm", 
		"bitmaps/knobs/sliderwhite7_2.xpm", 
		BRIGHTON_HALFSHADOW|BRIGHTON_NOTCH},
	{"", 10, CB3, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderblack7_1.xpm", 
		"bitmaps/knobs/sliderblack7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB4, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderblack7_1.xpm", 
		"bitmaps/knobs/sliderblack7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB5, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergrey7_1.xpm", 
		"bitmaps/knobs/slidergrey7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB6, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderwhite7_3.xpm",
		"bitmaps/knobs/sliderwhite7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB7, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergrey7_1.xpm", 
		"bitmaps/knobs/slidergrey7_2.xpm", 
		BRIGHTON_HALFSHADOW},

	{"", 1, CB8, R3, W1, L1, 0, 5, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB9, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderwhite7_3.xpm",
		"bitmaps/knobs/sliderwhite7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB10, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderwhite7_3.xpm",
		"bitmaps/knobs/sliderwhite7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB11, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergreen7_1.xpm", 
		"bitmaps/knobs/slidergreen7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB12, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergrey7_1.xpm", 
		"bitmaps/knobs/slidergrey7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 1, CB13    , R3, W1, L1, 0, 5, 0, "bitmaps/knobs/sliderwhite2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, CB14 + 8, R3, W1, L1, 0, 5, 0, "bitmaps/knobs/sliderwhite2.xpm", 0, 
		BRIGHTON_HALFSHADOW},

	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/*
	 * Global controls 2 - 69
	 */
	{"", 10, CB15, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderblack7_1.xpm",
		"bitmaps/knobs/sliderblack7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB16, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergreen7_1.xpm", 
		"bitmaps/knobs/slidergreen7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB17, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderred7_1.xpm", 
		"bitmaps/knobs/sliderred7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB18, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderwhite7_3.xpm",
		"bitmaps/knobs/sliderwhite7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB19, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderwhite7_3.xpm",
		"bitmaps/knobs/sliderwhite7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB20, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/sliderwhite7_3.xpm",
		"bitmaps/knobs/sliderwhite7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB21, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergreen7_1.xpm", 
		"bitmaps/knobs/slidergreen7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB22, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergreen7_1.xpm", 
		"bitmaps/knobs/slidergreen7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB23, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergreen7_1.xpm", 
		"bitmaps/knobs/slidergreen7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB24, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergrey7_1.xpm", 
		"bitmaps/knobs/slidergrey7_2.xpm", 
		BRIGHTON_HALFSHADOW},
	{"", 10, CB25, R3a, W1, L4, 0, 1, 0, "bitmaps/knobs/slidergrey7_1.xpm", 
		"bitmaps/knobs/slidergrey7_2.xpm", 
		BRIGHTON_HALFSHADOW},

	/*
	 * Dummies 65 from index 80
	 *
	 * First 53 for the opts panel, 36 used, final 12 for the opts.
	 */
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* Free 13 May turn into global options so channels can have 20 each */
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* Mods panel, 12 from 133 */
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* Vol and Tune - 145 */
	{"", 0, CB1, R4 + 15, W1 + 8, L1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 
		BRIGHTON_HALFSHADOW|BRIGHTON_NOTCH},
	{"", 0, CB26, R4 + 15, W1 + 8, L1, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	
	/* Memory Buttons */
	{"", 2, BC1, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC2, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC3, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC4, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC5, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlr.xpm", "bitmaps/buttons/touchnlR.xpm", 0},
	{"", 2, BC6, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlr.xpm", "bitmaps/buttons/touchnlR.xpm", 0},
	{"", 2, BC7, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC8, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC9, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC10, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlg.xpm", "bitmaps/buttons/touchnlG.xpm", 0},
	{"", 2, BC11, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlg.xpm", "bitmaps/buttons/touchnlG.xpm", 0},
	{"", 2, BC12, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, BC13, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, BC14, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, BC15, R4, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 
		BRIGHTON_CHECKBUTTON},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 2, BC1, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC2, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC3, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC4, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC5, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlr.xpm", "bitmaps/buttons/touchnlR.xpm", 0},
	{"", 2, BC6, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlr.xpm", "bitmaps/buttons/touchnlR.xpm", 0},
	{"", 2, BC7, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC8, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC9, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm", "bitmaps/buttons/touchnlO.xpm", 0},
	{"", 2, BC10, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlg.xpm", "bitmaps/buttons/touchnlG.xpm", 0},
	{"", 2, BC11, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlg.xpm", "bitmaps/buttons/touchnlG.xpm", 0},
	{"", 2, BC12, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, BC13, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, BC14, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm", "bitmaps/buttons/touchnlW.xpm", 0},
	{"", 2, BC15, R5, W2, L2, 0, 1, 0,
		"bitmaps/buttons/touchnlr.xpm", "bitmaps/buttons/touchnlR.xpm", 
		BRIGHTON_CHECKBUTTON},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* Touch strip - hide the controller */
	{"", 1, 135, 920, 560, 41, 0, 1, 0, "bitmaps/buttons/blue.xpm", 0,
		BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW|BRIGHTON_CENTER|BRIGHTON_REVERSE},

};

/*
 * Mod panel
 */
static brightonLocations csmods[15] = {
	{"", 2, 50, 100, 70, 300, 0, 1, 0, "bitmaps/buttons/dualpushgrey.xpm", 0, 
		BRIGHTON_VERTICAL},

	{"", 1, 305, 50, 80, 400, 0, 1, 0, "bitmaps/knobs/slideryellow.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, 430, 50, 80, 400, 0, 1, 0, "bitmaps/knobs/sliderwhite2.xpm", 0, 
		BRIGHTON_HALFSHADOW},

	{"", 0, 630, 100, 115, 200, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 0, 820, 100, 115, 200, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, 
		BRIGHTON_HALFSHADOW},

	{"", 2, 50, 550, 70, 300, 0, 1, 0, "bitmaps/buttons/rockerblack.xpm", 0, 
		BRIGHTON_VERTICAL},
	{"", 2, 175, 550, 70, 300, 0, 1, 0, "bitmaps/buttons/rockerblack.xpm", 0, 
		BRIGHTON_VERTICAL},
	{"", 2, 300, 550, 70, 300, 0, 1, 0, "bitmaps/buttons/rockerblack.xpm", 0, 
		BRIGHTON_VERTICAL},
	{"", 2, 425, 550, 70, 300, 0, 1, 0, "bitmaps/buttons/rockerblack.xpm", 0, 
		BRIGHTON_VERTICAL},

	{"", 2, 640, 410, 90, 420, 0, 1, 0, "bitmaps/buttons/rockersmoothBB.xpm",
		"bitmaps/buttons/rockersmoothBBd.xpm", 0},
	{"", 2, 740, 410, 90, 420, 0, 1, 0, "bitmaps/buttons/rockersmoothBB.xpm",
		"bitmaps/buttons/rockersmoothBBd.xpm", 0},
	{"", 2, 840, 410, 90, 420, 0, 1, 0, "bitmaps/buttons/rockersmoothBB.xpm",
		"bitmaps/buttons/rockersmoothBBd.xpm", 0},
};

static brightonLocations csmods2[1] = {
	{"", 2, 250, 450, 600, 300, 0, 1, 0, "bitmaps/buttons/rockersmoothBWR.xpm",
		"bitmaps/buttons/rockersmoothBWRd.xpm", 0},
};

#define OPTS_COUNT 48
#define MODS_COUNT 12

#define OW1 50
#define OL1 250

#define OR1 110
#define OR2 390
#define OR3 (OR2 + OR2 - OR1)

#define OD1 73

#define OC1 104
#define OC2 (OC1 + OD1)
#define OC3 (OC2 + OD1)
#define OC4 (OC3 + OD1)
#define OC5 (OC4 + OD1)
#define OC6 (OC5 + OD1)
#define OC7 (OC6 + OD1)
#define OC8 (OC7 + OD1)
#define OC9 (OC8 + OD1)
#define OC10 (OC9 + OD1)
#define OC11 (OC10 + OD1)
#define OC12 (OC11 + OD1)

static brightonLocations csopts[OPTS_COUNT] = {

	{"", 1, OC1, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC2, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC3, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC4, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC5, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderred2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC6, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC7, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC8, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC9, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC10, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC11, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slideryellow.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC12, OR1, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 1, OC1, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC2, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC3, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC4, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC5, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderred2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC6, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC7, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC8, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC9, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC10, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC11, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slideryellow.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC12, OR2, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	{"", 1, OC1, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderwhite2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC2, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC3, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC4, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC5, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC6, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergrey.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC7, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderwhite2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC8, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderwhite2.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC9, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC10, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slidergreen.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC11, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/slideryellow.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 1, OC12, OR3, OW1, OL1, 0, 1, 0, "bitmaps/knobs/sliderblack6.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 900, 900, W1, L1, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

};

static int
cs80DestroySynth(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);

	/*
	 * Since we registered two synths, we now need to remove the upper
	 * manual.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0, BRISTOL_EXIT_ALGO);
	return(0);
}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp cs80App = {
	"cs80",
	0,
	"bitmaps/textures/wood4.xpm",
	BRIGHTON_VERTICAL,
	cs80Init,
	cs80Configure, /* 3 callbacks, unused? */
	midiCallback,
	cs80DestroySynth,
	{8, 150, 8, 2, 5, 520, 0, 0},
	940, 435, 0, 0,
	11,
	{
		{
			"Opts",
			"bitmaps/blueprints/cs80opts.xpm", /* flags */
			"bitmaps/textures/cs80opts.xpm", /* flags */
			BRIGHTON_WITHDRAWN|BRIGHTON_STRETCH,
			0,
			0,
			cs80Callback,
			28, 58, 235, 382,
			OPTS_COUNT,
			csopts
		},
		{
			"CS80",
			"bitmaps/blueprints/cs80.xpm",
			"bitmaps/textures/metal7.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* flags */
			0,
			0,
			cs80Callback,
			23, 47, 957, 677,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/nkbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			208, 724, 758, 248,
			KEY_COUNT,
			keysprofile2
		},
		{
			"Mods",
			"bitmaps/blueprints/cs80mods.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			BRIGHTON_VERTICAL,
			0,
			0,
			cs80Callback,
			23, 724, 186, 248,
			MODS_COUNT,
			csmods
		},
		{
			"Mods2",
			0,
			"bitmaps/textures/metal5.xpm", /* flags */
			BRIGHTON_VERTICAL,
			0,
			0,
			cs80panelswitch,
			943, 724, 38, 248,
			1,
			csmods2
		},
		{
			"Leather Edge",
			0,
			"bitmaps/textures/leather.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 10, 1000,
			0,
			0
		},
		{
			"Leather Cover",
			0,
			"bitmaps/textures/leather.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			10, 830, 13, 160,
			0,
			0
		},
		{
			"Leather Edge",
			0,
			"bitmaps/textures/leather.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			991, 0, 10, 1000,
			0,
			0
		},
		{
			"Leather Cover",
			0,
			"bitmaps/textures/leather.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			979, 830, 13, 160,
			0,
			0
		},
		{
			"Leather Top",
			0,
			"bitmaps/textures/leather.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 1000, 22,
			0,
			0
		},
		{
			"Leather bottom",
			0,
			"bitmaps/textures/leather.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 970, 1000, 33,
			0,
			0
		},
	}
};

static int
cs80panelswitch(brightonWindow *win, int panel, int index, float value)
{
	brightonEvent event;

	event.type = BRIGHTON_EXPOSE;

printf("panel swithc %f\n", value);
	if (value > 0) {
		event.intvalue = 0;
		brightonParamChange(win, 0, -1, &event);
		event.intvalue = 1;
		brightonParamChange(win, 1, -1, &event);
	} else {
		event.intvalue = 1;
		brightonParamChange(win, 0, -1, &event);
	}

	if (width != win->width)
		brightonPut(win,
			"bitmaps/blueprints/cs80shade.xpm", 0, 0, win->width, win->height);

	width = win->width;

	return(0);
}

static int
cs80Debug(guiSynth *synth, int level)
{
	if (debuglevel >= level)
		return(1);
	return(0);
}

static int
cs80MidiSendMsg(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, synth->sid2, c, o, v);
	return(0);
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

	if (cs80Debug(synth, 1))
		printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			if (cs80Debug(synth, 2))
				printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, "cs80", 0, synth->bank + synth->location + mw,
				synth->mem.active, 0, 0);
			modPanelFix(win, synth);
			break;
		case MIDI_BANK_SELECT:
			if (cs80Debug(synth, 2))
				printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static void
cs80ButtonPanel(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;

	if (cs80Debug(synth, 1))
		printf("cs80ButtonPanel(%i, %i, %i)\n", c, o, v);

	switch (c) {
		case 0:
			/* patch selector */
			if (cs80Debug(synth, 2))
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
			if (cs80Debug(synth, 2))
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

#define CS_LOAD 0
#define CS_UB 1
#define CS_LB 2
#define CS_UP 3
#define CS_LP 4
#define CS_SAVE 5

#define UPPER_BANK 0
#define LOWER_BANK 1
#define NO_BANK -1
static int currentLayer = NO_BANK;

/*
 * Loading everything means get the memory with NOCALLS then dispatch the new
 * values manually including the opts and mods panel.
 */
static void
cs80MemoryShim(guiSynth *synth)
{
	brightonEvent event;
	int i;

	event.type = BRIGHTON_FLOAT;

	/* The main panel */
	for (i = 0; i < 80; i++)
	{
		event.value = synth->mem.param[i];
		brightonParamChange(synth->win, 1, i, &event);
	}

	/* The opts panel */
	for (i = 0; i < OPTS_COUNT; i++)
	{
		event.value = synth->mem.param[i + 80];
		brightonParamChange(synth->win, 0, i, &event);
	}

	/* The mods panel */
	for (i = 0; i < MODS_COUNT; i++)
	{
		event.value = synth->mem.param[i + 133];
		brightonParamChange(synth->win, 3, i, &event);
	}

}

/*
 * Load everything
 */
static void
cs80MemoryLoad(guiSynth *synth, int layer, int bank, int patch)
{
	printf("load %i %i %i %i\n", mw, layer, bank, patch);

	loadMemory(synth, "cs80", 0, bank * 10 + patch + mw,
		synth->mem.active, 0, BRISTOL_FORCE|BRISTOL_NOCALLS);
	cs80MemoryShim(synth);
}

/*
 * Loading layer means get the memory with nocalls into scratch memory then
 * dispatch the single layer values manually.
 */
static void
cs80MemoryLoadLayer(guiSynth *synth, int layer, int bank, int patch)
{
	float *hold = synth->mem.param;
	int i, offset = 0;
	brightonEvent event;

	printf("load layer %i %i %i %i\n", mw, layer, bank, patch);

	event.type = BRIGHTON_FLOAT;

	synth->mem.param = scratch;
	loadMemory(synth, "cs80", 0, bank * 10 + patch + mw,
		synth->mem.active, 0, BRISTOL_FORCE|BRISTOL_NOCALLS);
	synth->mem.param = hold;

	/*
	 * So, we now have scratch selected. depending on the selected layer we
	 * need to scan through the channel settings and then the opts for that
	 * channel
	 */
	if (layer == LOWER_BANK)
	{
		printf("load into channel-II\n");
		offset = 26;
	} else
		printf("load into channel-I\n");

	for (i = 0; i < 26; i++)
	{
		event.value = scratch[i + offset];
		brightonParamChange(synth->win, 1, i + offset, &event);
	}

	/*
	 * Then work on the channel opts
	 */
	if (layer == LOWER_BANK) {
		offset = 92;
		layer = 12;
	} else {
		offset = 80;
		layer = 0;
	}

	for (i = 0; i < 12; i++)
	{
		event.value = scratch[i + offset];
		brightonParamChange(synth->win, 0, i + layer, &event);
	}
}

static void
cs80MemorySave(guiSynth *synth, int layer, int bank, int patch)
{
	saveMemory(synth, "cs80", 0, mw + bank * 10 + patch, 0);
}

static void
cs80Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int i, b = 0, p = 0, off = 147;
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;

	if (synth->flags & MEM_LOADING)
		return;

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (cs80Debug(synth, 1))
		printf("cs80Memory(%i, %i, %i)\n", c, o, v);
printf("cs80Memory(%i, %i, %i)\n", c, o, v);

	/*
	 * Six ways here, we need to do button exclusions here, probably in the
	 * routines called.
	 *
	 *	CS_LOAD - single then load last selected layer. Double load all.
	 *	CS_SAVE - double then save all to last selected layer.
	 *
	 *	CS_UB - select upper bank.
	 *	CS_LB - select lower bank.
	 *	CS_UP - select upper bank, double load it.
	 *	CS_LP - select lower bank, double load it.
	 */
	switch (c) {
		case CS_LOAD:
			printf("cs80Memory(load)\n");

			if (currentLayer == LOWER_BANK)
				off = 163;

			for (i = 0; i < 4;i++)
			{
				if (synth->mem.param[i + off] != 0)
				{
					b = i;
					break;
				}
			}

			if (currentLayer == LOWER_BANK)
				off = 167;
			else
				off = 151;

			for (i = 0; i < 10;i++)
			{
				if (synth->mem.param[i + off] != 0)
				{
					p = i;
					break;
				}
			}

			if (brightonDoubleClick(dc1))
				cs80MemoryLoad(synth, currentLayer, b, p);
			else
				cs80MemoryLoadLayer(synth, currentLayer, b, p);
			break;
	 	case CS_SAVE:
			if (currentLayer == NO_BANK)
				return;

			if (brightonDoubleClick(dc1))
			{
				printf("cs80Memory(save)\n");

				if (currentLayer == LOWER_BANK)
					off = 163;

				for (i = 0; i < 4;i++)
				{
					if (synth->mem.param[i + off] != 0)
					{
						b = i;
						break;
					}
				}

				if (currentLayer == LOWER_BANK)
					off = 167;
				else
					off = 151;

				for (i = 0; i < 10;i++)
				{
					if (synth->mem.param[i + off] != 0)
					{
						p = i;
						break;
					}
				}

				cs80MemorySave(synth, currentLayer, b, p);
			}
			break;
	 	case CS_UB:
			printf("cs80Memory(ub, %i, %i)\n", c, o);
			currentLayer = UPPER_BANK;

			/*
			 * Force exclusion
			 */
			if (synth->dispatch[RADIO_UB].other2)
			{
				synth->dispatch[RADIO_UB].other2 = 0;
				return;
			}

			o += RADIO_UB;

			if (v == 0)
			{
				if (synth->dispatch[RADIO_UB].other1 == o)
				{
					event.value = 1;
					brightonParamChange(synth->win, 1, o, &event);
				}
				return;
			}

			if (synth->dispatch[RADIO_UB].other1 != -1)
			{
				synth->dispatch[RADIO_UB].other2 = 1;

				if (synth->dispatch[RADIO_UB].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, 1,
					synth->dispatch[RADIO_UB].other1, &event);
			}
			synth->dispatch[RADIO_UB].other1 = o;

			break;
	 	case CS_LB:
			currentLayer = LOWER_BANK;
			printf("cs80Memory(lb)\n");

			/*
			 * Force exclusion
			 */
			if (synth->dispatch[RADIO_LB].other2)
			{
				synth->dispatch[RADIO_LB].other2 = 0;
				return;
			}

			o += RADIO_LB;

			if (v == 0)
			{
				if (synth->dispatch[RADIO_LB].other1 == o)
				{
					event.value = 1;
					brightonParamChange(synth->win, 1, o, &event);
				}
				return;
			}

			if (synth->dispatch[RADIO_LB].other1 != -1)
			{
				synth->dispatch[RADIO_LB].other2 = 1;

				if (synth->dispatch[RADIO_LB].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, 1,
					synth->dispatch[RADIO_LB].other1, &event);
			}
			synth->dispatch[RADIO_LB].other1 = o;

			break;
	 	case CS_UP:
			currentLayer = UPPER_BANK;
			printf("cs80Memory(up)\n");

			/*
			 * Force exclusion
			 */
			if (synth->dispatch[RADIO_UP].other2)
			{
				synth->dispatch[RADIO_UP].other2 = 0;
				return;
			}

			o += RADIO_UP;

			if (v == 0)
			{
				if (synth->dispatch[RADIO_UP].other1 == o)
				{
					event.value = 1;
					brightonParamChange(synth->win, 1, o, &event);
				}
				return;
			}

			if (synth->dispatch[RADIO_UP].other1 != -1)
			{
				synth->dispatch[RADIO_UP].other2 = 1;

				if (synth->dispatch[RADIO_UP].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, 1,
					synth->dispatch[RADIO_UP].other1, &event);
			}
			synth->dispatch[RADIO_UP].other1 = o;

			break;
	 	case CS_LP:
			currentLayer = LOWER_BANK;
			printf("cs80Memory(lp)\n");

			/*
			 * Force exclusion
			 */
			if (synth->dispatch[RADIO_LP].other2)
			{
				synth->dispatch[RADIO_LP].other2 = 0;
				return;
			}

			o += RADIO_LP;

			if (v == 0)
			{
				if (synth->dispatch[RADIO_LP].other1 == o)
				{
					event.value = 1;
					brightonParamChange(synth->win, 1, o, &event);
				}
				return;
			}

			if (synth->dispatch[RADIO_LP].other1 != -1)
			{
				synth->dispatch[RADIO_LP].other2 = 1;

				if (synth->dispatch[RADIO_LP].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, 1,
					synth->dispatch[RADIO_LP].other1, &event);
			}
			synth->dispatch[RADIO_LP].other1 = o;
			break;
	 	default:
			currentLayer = NO_BANK;
			printf("cs80Memory(?)\n");
			break;
	}
}

static void
cs80Midi(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
cs80Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if ((synth == 0) || (synth->flags & SUPPRESS))
		return(0);

	if (cs80Debug(synth, 2))
		printf("cs80Callback(%i, %i, %f)\n", panel, index, value);
printf("cs80Callback(%i, %i, %f)\n", panel, index, value);

	if (panel == 0) {
		index += 80;
		sendvalue = value * C_RANGE_MIN_1;
	} else if (panel == 3) {
		index += 133;
		sendvalue = value * C_RANGE_MIN_1;
	} else {
		if (cs80App.resources[panel].devlocn[index].to == 1.0)
			sendvalue = value * C_RANGE_MIN_1;
		else
			sendvalue = value;
	}

	/*
	 * If the index is less than LAYER_DEVS then write the value to the desired
	 * layer only, otherwise write the value to both/top layers?
	 */
	synth->mem.param[index] = value;

/*
	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);
*/

	synth->dispatch[index].routine(synth,
		global.controlfd, synth->sid,
		synth->dispatch[index].controller,
		synth->dispatch[index].operator,
		sendvalue);

	return(0);
}

static void
cs80WaveLevel(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (v != 0)
	{
		if (c == 1)
		{
			/* CH-I */
			if (o == 0) {
				if (synth->mem.param[3] == 0)
					v = 0;
				else
					v = synth->mem.param[80] * C_RANGE_MIN_1;
			} else {
				if (synth->mem.param[4] == 0)
					v = 0;
				else
					v = synth->mem.param[81] * C_RANGE_MIN_1;
			}
		} else {
			/* CH-II */
			if (o == 0) {
				if (synth->mem.param[29] == 0)
					v = 0;
				else
					v = synth->mem.param[96] * C_RANGE_MIN_1;
			} else {
				if (synth->mem.param[30] == 0)
					v = 0;
				else
					v = synth->mem.param[97] * C_RANGE_MIN_1;
			}
		}
	}

printf("%i %i %i\n", c, o, v);
	bristolMidiSendMsg(global.controlfd, synth->sid, c, o, v);
}

static void
cs80Release(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	switch (c) {
		case 4:
			v = synth->mem.param[134] * synth->mem.param[14] * C_RANGE_MIN_1;
			break;
		case 5:
			v = synth->mem.param[134] * synth->mem.param[20] * C_RANGE_MIN_1;
			break;
		case 10:
			v = synth->mem.param[134] * synth->mem.param[40] * C_RANGE_MIN_1;
			break;
		case 11:
			v = synth->mem.param[134] * synth->mem.param[46] * C_RANGE_MIN_1;
			break;
		case 126:
			bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, (int)
				(synth->mem.param[134] * synth->mem.param[14] * C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 5, 3, (int)
				(synth->mem.param[134] * synth->mem.param[20] * C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 10, 4, (int)
				(synth->mem.param[134] * synth->mem.param[40] * C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 11, 3, (int)
				(synth->mem.param[134] * synth->mem.param[46] * C_RANGE_MIN_1));
			return;
	}

	bristolMidiSendMsg(global.controlfd, synth->sid, c, o, v);
}

static void
cs80Resonance(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	switch (c) {
		case 2:
			v = synth->mem.param[71] * synth->mem.param[7] * C_RANGE_MIN_1;
			break;
		case 3:
			v = synth->mem.param[71] * synth->mem.param[9] * C_RANGE_MIN_1;
			break;
		case 8:
			v = synth->mem.param[71] * synth->mem.param[33] * C_RANGE_MIN_1;
			break;
		case 9:
			v = synth->mem.param[71] * synth->mem.param[35] * C_RANGE_MIN_1;
			break;
		case 126:
			bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, (int)
				(synth->mem.param[71] * synth->mem.param[7] * C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, (int)
				(synth->mem.param[71] * synth->mem.param[9] * C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, (int)
				(synth->mem.param[71] * synth->mem.param[33] * C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 9, 1, (int)
				(synth->mem.param[71] * synth->mem.param[35] * C_RANGE_MIN_1));
			return;
	}

	bristolMidiSendMsg(global.controlfd, synth->sid, c, o, v);
}

static void
cs80Pitchbend(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (global.libtest)
		return;

	bristolMidiSendMsg(global.controlfd, synth->sid, BRISTOL_EVENT_PITCH, 0, v);
}

static void
cs80Detune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int value = v / 50;

	if (o == 101) {
		if (!global.libtest) {
			bristolMidiSendNRP(global.controlfd, synth->sid,
				BRISTOL_NRP_DETUNE, value);
		}
	} else {
		/*
		 * Channel 2 detune
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 7, 6, v);
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
cs80Init(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
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

	printf("Initialise the cs80 link to bristol: %p\n", synth->win);

	scratch = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
	synth->dispatch = (dispatcher *)
		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher));

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets.
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);
	}
printf("	The SID are %i and %i\n", synth->sid, synth->sid2);

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		/* Default all controllers to send a message that is ignored */
		synth->dispatch[i].controller = 126;
		synth->dispatch[i].operator = 101;
		synth->dispatch[i].routine = (synthRoutine) cs80MidiSendMsg;
	}

	/* LFO */
	synth->dispatch[0].controller = 0;
	synth->dispatch[0].operator = 0;
	synth->dispatch[1].controller = 126;
	synth->dispatch[1].operator = 24;
	synth->dispatch[2].controller = 1;
	synth->dispatch[2].operator = 5;

	/* Square on/off, ramp on/off - will need shim for level later */
	synth->dispatch[3].controller = 1;
	synth->dispatch[3].operator = 0;
	synth->dispatch[3].routine = (synthRoutine) cs80WaveLevel;
	synth->dispatch[4].controller = 1;
	synth->dispatch[4].operator = 1;
	synth->dispatch[4].routine = (synthRoutine) cs80WaveLevel;

	synth->dispatch[5].controller = 126;
	synth->dispatch[5].operator = 26; /* Noise level */
	synth->dispatch[6].controller = 2;
	synth->dispatch[6].operator = 0;
	synth->dispatch[7].controller = 2;
	synth->dispatch[7].operator = 1;
	synth->dispatch[7].routine = (synthRoutine) cs80Resonance;
	synth->dispatch[8].controller = 3;
	synth->dispatch[8].operator = 0;
	synth->dispatch[9].controller = 3;
	synth->dispatch[9].operator = 1;
	synth->dispatch[9].routine = (synthRoutine) cs80Resonance;
	synth->dispatch[10].controller = 4;
	synth->dispatch[10].operator = 0;
	synth->dispatch[11].controller = 4;
	synth->dispatch[11].operator = 1;
	synth->dispatch[12].controller = 4;
	synth->dispatch[12].operator = 2;
	synth->dispatch[13].controller = 4;
	synth->dispatch[13].operator = 3;
	synth->dispatch[14].controller = 4;
	synth->dispatch[14].operator = 4;
	synth->dispatch[14].routine = (synthRoutine) cs80Release;
	synth->dispatch[15].controller = 126;
	synth->dispatch[15].operator = 28;
	/* Sine level */
	synth->dispatch[16].controller = 1;
	synth->dispatch[16].operator = 2;
	synth->dispatch[17].controller = 5;
	synth->dispatch[17].operator = 0;
	synth->dispatch[18].controller = 5;
	synth->dispatch[18].operator = 1;
	synth->dispatch[19].controller = 5;
	synth->dispatch[19].operator = 2;
	synth->dispatch[20].controller = 5;
	synth->dispatch[20].operator = 3;
	synth->dispatch[20].routine = (synthRoutine) cs80Release;
	synth->dispatch[21].controller = 5;
	synth->dispatch[21].operator = 4;

	/* Brill/Vol */
	synth->dispatch[22].controller = 126;
	synth->dispatch[22].operator = 11;
	synth->dispatch[23].controller = 126;
	synth->dispatch[23].operator = 12;
	synth->dispatch[24].controller = 126;
	synth->dispatch[24].operator = 13;
	synth->dispatch[25].controller = 126;
	synth->dispatch[25].operator = 14;

	synth->dispatch[26].controller = 6;
	synth->dispatch[26].operator = 0;
	synth->dispatch[27].controller = 126;
	synth->dispatch[27].operator = 25;
	/* PW */
	synth->dispatch[28].controller = 7;
	synth->dispatch[28].operator = 5;

	/* Sqr/Rmp on off will need shim for level */
	synth->dispatch[29].controller = 7;
	synth->dispatch[29].operator = 0;
	synth->dispatch[29].routine = (synthRoutine) cs80WaveLevel;
	synth->dispatch[30].controller = 7;
	synth->dispatch[30].operator = 1;
	synth->dispatch[30].routine = (synthRoutine) cs80WaveLevel;
	synth->dispatch[31].controller = 126;
	synth->dispatch[31].operator = 27; /* Noise level */
	synth->dispatch[32].controller = 8;
	synth->dispatch[32].operator = 0;
	synth->dispatch[33].controller = 8;
	synth->dispatch[33].operator = 1;
	synth->dispatch[33].routine = (synthRoutine) cs80Resonance;
	synth->dispatch[34].controller = 9;
	synth->dispatch[34].operator = 0;
	synth->dispatch[35].controller = 9;
	synth->dispatch[35].operator = 1;
	synth->dispatch[35].routine = (synthRoutine) cs80Resonance;
	synth->dispatch[36].controller = 10;
	synth->dispatch[36].operator = 0;
	synth->dispatch[37].controller = 10;
	synth->dispatch[37].operator = 1;
	synth->dispatch[38].controller = 10;
	synth->dispatch[38].operator = 2;
	synth->dispatch[39].controller = 10;
	synth->dispatch[39].operator = 3;
	synth->dispatch[40].controller = 10;
	synth->dispatch[40].operator = 4;
	synth->dispatch[40].routine = (synthRoutine) cs80Release;
	synth->dispatch[41].controller = 126;
	synth->dispatch[41].operator = 29;
	/* CH-II Sine */
	synth->dispatch[42].controller = 7;
	synth->dispatch[42].operator = 2;
	synth->dispatch[43].controller = 11;
	synth->dispatch[43].operator = 0;
	synth->dispatch[44].controller = 11;
	synth->dispatch[44].operator = 1;
	synth->dispatch[45].controller = 11;
	synth->dispatch[45].operator = 2;
	synth->dispatch[46].controller = 11;
	synth->dispatch[46].operator = 3;
	synth->dispatch[46].routine = (synthRoutine) cs80Release;
	synth->dispatch[47].controller = 11;
	synth->dispatch[47].operator = 4;

	/* Brill/Vol */
	synth->dispatch[48].controller = 126;
	synth->dispatch[48].operator = 15;
	synth->dispatch[49].controller = 126;
	synth->dispatch[49].operator = 16;
	synth->dispatch[50].controller = 126;
	synth->dispatch[50].operator = 17;
	synth->dispatch[51].controller = 126;
	synth->dispatch[51].operator = 18;
	/* Channel-2 detune */
	synth->dispatch[52].controller = 126;
	synth->dispatch[52].operator = 100;
	synth->dispatch[52].routine = (synthRoutine) cs80Detune;
	synth->dispatch[53].controller = 126;
	synth->dispatch[53].operator = 101;
	synth->dispatch[54].controller = 126;
	synth->dispatch[54].operator = 101;
	synth->dispatch[55].controller = 126;
	synth->dispatch[55].operator = 101;
	synth->dispatch[56].controller = 126;
	synth->dispatch[56].operator = 101;
	synth->dispatch[57].controller = 126;
	synth->dispatch[57].operator = 101;
	/* LFO */
	synth->dispatch[58].controller = 126;
	synth->dispatch[58].operator = 20;
	/* Rate. Add in Key sync as global option if desired */
	synth->dispatch[59].controller = 15;
	synth->dispatch[59].operator = 0;
	synth->dispatch[60].controller = 126;
	synth->dispatch[60].operator = 21;
	synth->dispatch[61].controller = 126;
	synth->dispatch[61].operator = 22;
	synth->dispatch[62].controller = 126;
	synth->dispatch[62].operator = 23;

	/* Osc transpose */
	synth->dispatch[63].controller = 1;
	synth->dispatch[63].operator = 7;
	synth->dispatch[64].controller = 7;
	synth->dispatch[64].operator = 7;
	synth->dispatch[65].controller = 126;
	synth->dispatch[65].operator = 101;
	synth->dispatch[66].controller = 126;
	synth->dispatch[66].operator = 101;
	synth->dispatch[67].controller = 126;
	synth->dispatch[67].operator = 101;
	synth->dispatch[68].controller = 126;
	synth->dispatch[68].operator = 101;

	/* Channel level */
	synth->dispatch[69].controller = 126;
	synth->dispatch[69].operator = 5;

	/* Overall brilliance */
	synth->dispatch[70].controller = 126;
	synth->dispatch[70].operator = 6;
	synth->dispatch[71].controller = 126;
	synth->dispatch[71].operator = 101;
	synth->dispatch[71].routine = (synthRoutine) cs80Resonance;
	synth->dispatch[72].controller = 126;
	synth->dispatch[72].operator = 101;
	synth->dispatch[73].controller = 126;
	synth->dispatch[73].operator = 101;
	synth->dispatch[74].controller = 126;
	synth->dispatch[74].operator = 101;
	synth->dispatch[75].controller = 126;
	synth->dispatch[75].operator = 101;

	/* Key spreading Brill/Level */
	synth->dispatch[76].controller = 126;
	synth->dispatch[76].operator = 7;
	synth->dispatch[77].controller = 126;
	synth->dispatch[77].operator = 8;
	synth->dispatch[78].controller = 126;
	synth->dispatch[78].operator = 9;
	synth->dispatch[79].controller = 126;
	synth->dispatch[79].operator = 10;

	/* 16x CH-I controls starting with Square gain, Tri gain and two brill */
	synth->dispatch[80].controller = 1;
	synth->dispatch[80].operator = 0;
	synth->dispatch[80].routine = (synthRoutine) cs80WaveLevel;
	synth->dispatch[81].controller = 1;
	synth->dispatch[81].operator = 1;
	synth->dispatch[81].routine = (synthRoutine) cs80WaveLevel;
	synth->dispatch[82].controller = 1;
	synth->dispatch[82].operator = 3;
	synth->dispatch[83].controller = 1;
	synth->dispatch[83].operator = 4;
	synth->dispatch[84].controller = 126;
	synth->dispatch[84].operator = 30; /* filter brill */
	synth->dispatch[85].controller = 126;
	synth->dispatch[85].operator = 3; /* Pan */
	synth->dispatch[86].controller = 126;
	synth->dispatch[86].operator = 101;
	synth->dispatch[87].controller = 126;
	synth->dispatch[87].operator = 101;
	synth->dispatch[88].controller = 126;
	synth->dispatch[88].operator = 101;
	synth->dispatch[89].controller = 126;
	synth->dispatch[89].operator = 101;
	synth->dispatch[90].controller = 126;
	synth->dispatch[90].operator = 101;
	synth->dispatch[91].controller = 126;
	synth->dispatch[91].operator = 101;
	synth->dispatch[92].controller = 126;
	synth->dispatch[92].operator = 101;
	synth->dispatch[93].controller = 126;
	synth->dispatch[93].operator = 101;
	synth->dispatch[94].controller = 126;
	synth->dispatch[94].operator = 101;
	synth->dispatch[95].controller = 126;
	synth->dispatch[95].operator = 101;

	/* 16x CH-II controls starting with Square gain, Tri gain and two brill */
	synth->dispatch[96].controller = 7;
	synth->dispatch[96].operator = 0;
	synth->dispatch[96].routine = (synthRoutine) cs80WaveLevel;
	synth->dispatch[97].controller = 7;
	synth->dispatch[97].operator = 1;
	synth->dispatch[97].routine = (synthRoutine) cs80WaveLevel;
	synth->dispatch[98].controller = 7;
	synth->dispatch[98].operator = 3;
	synth->dispatch[99].controller = 7;
	synth->dispatch[99].operator = 4;
	synth->dispatch[100].controller = 126;
	synth->dispatch[100].operator = 31; /* Filter brill */
	synth->dispatch[101].controller = 126;
	synth->dispatch[101].operator = 4; /* Pan */
	synth->dispatch[102].controller = 126;
	synth->dispatch[102].operator = 101;
	synth->dispatch[103].controller = 126;
	synth->dispatch[103].operator = 101;
	synth->dispatch[104].controller = 126;
	synth->dispatch[104].operator = 101;
	synth->dispatch[105].controller = 126;
	synth->dispatch[105].operator = 101;
	synth->dispatch[106].controller = 126;
	synth->dispatch[106].operator = 101;
	synth->dispatch[107].controller = 126;
	synth->dispatch[107].operator = 101;
	synth->dispatch[108].controller = 126;
	synth->dispatch[108].operator = 101;
	synth->dispatch[109].controller = 126;
	synth->dispatch[109].operator = 101;
	synth->dispatch[110].controller = 126;
	synth->dispatch[110].operator = 101;
	synth->dispatch[111].controller = 126;
	synth->dispatch[111].operator = 101;

	/* 16x global controls */
	synth->dispatch[112].controller = 126;
	synth->dispatch[112].operator = 101;
	synth->dispatch[112].routine = (synthRoutine) cs80Detune;
	synth->dispatch[113].controller = 126;
	synth->dispatch[113].operator = 101;
	synth->dispatch[114].controller = 126;
	synth->dispatch[114].operator = 101;
	synth->dispatch[115].controller = 126;
	synth->dispatch[115].operator = 101;
	synth->dispatch[116].controller = 126;
	synth->dispatch[116].operator = 101;
	synth->dispatch[117].controller = 126;
	synth->dispatch[117].operator = 101;
	synth->dispatch[118].controller = 126;
	synth->dispatch[118].operator = 101;
	synth->dispatch[119].controller = 126;
	synth->dispatch[119].operator = 101;
	synth->dispatch[120].controller = 126;
	synth->dispatch[120].operator = 101;
	synth->dispatch[121].controller = 126;
	synth->dispatch[121].operator = 101;
	synth->dispatch[122].controller = 126;
	synth->dispatch[122].operator = 101;
	synth->dispatch[123].controller = 126;
	synth->dispatch[123].operator = 101;
	synth->dispatch[124].controller = 126;
	synth->dispatch[124].operator = 101;
	synth->dispatch[125].controller = 126;
	synth->dispatch[125].operator = 101;
	synth->dispatch[126].controller = 126;
	synth->dispatch[126].operator = 101;
	synth->dispatch[127].controller = 126;
	synth->dispatch[127].operator = 101;

	/* A few dummies */
	synth->dispatch[128].controller = 126;
	synth->dispatch[128].operator = 101;
	synth->dispatch[129].controller = 126;
	synth->dispatch[129].operator = 101;
	synth->dispatch[130].controller = 126;
	synth->dispatch[130].operator = 101;
	synth->dispatch[131].controller = 126;
	synth->dispatch[131].operator = 101;
	synth->dispatch[132].controller = 126;
	synth->dispatch[132].operator = 101;

	/* Mods panel */
	synth->dispatch[133].controller = 126;
	synth->dispatch[133].operator = 101;
	synth->dispatch[134].controller = 126;
	synth->dispatch[134].operator = 101;
	synth->dispatch[134].routine = (synthRoutine) cs80Release;
	synth->dispatch[135].controller = 126;
	synth->dispatch[135].operator = 101;
	synth->dispatch[136].controller = 126;
	synth->dispatch[136].operator = 101;
	synth->dispatch[137].controller = 126;
	synth->dispatch[137].operator = 101;
	synth->dispatch[138].controller = 126;
	synth->dispatch[138].operator = 101;
	synth->dispatch[139].controller = 126;
	synth->dispatch[139].operator = 101;
	synth->dispatch[140].controller = 126;
	synth->dispatch[140].operator = 101;
	synth->dispatch[141].controller = 126;
	synth->dispatch[141].operator = 101;
	synth->dispatch[142].controller = 126;
	synth->dispatch[142].operator = 101;
	synth->dispatch[143].controller = 126;
	synth->dispatch[143].operator = 101;
	synth->dispatch[144].controller = 126;
	synth->dispatch[144].operator = 101;

	/* Global tune and gain */
	synth->dispatch[145].controller = 126;
	synth->dispatch[145].operator = 1;
	synth->dispatch[146].controller = 126;
	synth->dispatch[146].operator = 2;

	synth->dispatch[147].controller = CS_UB;
	synth->dispatch[147].operator = 0;
	synth->dispatch[147].routine = (synthRoutine) cs80Memory;
	synth->dispatch[148].controller = CS_UB;
	synth->dispatch[148].operator = 1;
	synth->dispatch[148].routine = (synthRoutine) cs80Memory;
	synth->dispatch[149].controller = CS_UB;
	synth->dispatch[149].operator = 2;
	synth->dispatch[149].routine = (synthRoutine) cs80Memory;
	synth->dispatch[150].controller = CS_UB;
	synth->dispatch[150].operator = 3;
	synth->dispatch[150].routine = (synthRoutine) cs80Memory;

	synth->dispatch[151].controller = CS_UP;
	synth->dispatch[151].operator = 0;
	synth->dispatch[151].routine = (synthRoutine) cs80Memory;
	synth->dispatch[152].controller = CS_UP;
	synth->dispatch[152].operator = 1;
	synth->dispatch[152].routine = (synthRoutine) cs80Memory;
	synth->dispatch[153].controller = CS_UP;
	synth->dispatch[153].operator = 2;
	synth->dispatch[153].routine = (synthRoutine) cs80Memory;
	synth->dispatch[154].controller = CS_UP;
	synth->dispatch[154].operator = 3;
	synth->dispatch[154].routine = (synthRoutine) cs80Memory;
	synth->dispatch[155].controller = CS_UP;
	synth->dispatch[155].operator = 4;
	synth->dispatch[155].routine = (synthRoutine) cs80Memory;
	synth->dispatch[156].controller = CS_UP;
	synth->dispatch[156].operator = 5;
	synth->dispatch[156].routine = (synthRoutine) cs80Memory;
	synth->dispatch[157].controller = CS_UP;
	synth->dispatch[157].operator = 6;
	synth->dispatch[157].routine = (synthRoutine) cs80Memory;
	synth->dispatch[158].controller = CS_UP;
	synth->dispatch[158].operator = 7;
	synth->dispatch[158].routine = (synthRoutine) cs80Memory;
	synth->dispatch[159].controller = CS_UP;
	synth->dispatch[159].operator = 8;
	synth->dispatch[159].routine = (synthRoutine) cs80Memory;
	synth->dispatch[160].controller = CS_UP;
	synth->dispatch[160].operator = 9;
	synth->dispatch[160].routine = (synthRoutine) cs80Memory;
	synth->dispatch[161].controller = CS_LOAD;
	synth->dispatch[161].operator = 0;
	synth->dispatch[161].routine = (synthRoutine) cs80Memory;

	synth->dispatch[163].controller = CS_LB;
	synth->dispatch[163].operator = 0;
	synth->dispatch[163].routine = (synthRoutine) cs80Memory;
	synth->dispatch[164].controller = CS_LB;
	synth->dispatch[164].operator = 1;
	synth->dispatch[164].routine = (synthRoutine) cs80Memory;
	synth->dispatch[165].controller = CS_LB;
	synth->dispatch[165].operator = 2;
	synth->dispatch[165].routine = (synthRoutine) cs80Memory;
	synth->dispatch[166].controller = CS_LB;
	synth->dispatch[166].operator = 3;
	synth->dispatch[166].routine = (synthRoutine) cs80Memory;

	synth->dispatch[167].controller = CS_LP;
	synth->dispatch[167].operator = 0;
	synth->dispatch[167].routine = (synthRoutine) cs80Memory;
	synth->dispatch[168].controller = CS_LP;
	synth->dispatch[168].operator = 1;
	synth->dispatch[168].routine = (synthRoutine) cs80Memory;
	synth->dispatch[169].controller = CS_LP;
	synth->dispatch[169].operator = 2;
	synth->dispatch[169].routine = (synthRoutine) cs80Memory;
	synth->dispatch[170].controller = CS_LP;
	synth->dispatch[170].operator = 3;
	synth->dispatch[170].routine = (synthRoutine) cs80Memory;
	synth->dispatch[171].controller = CS_LP;
	synth->dispatch[171].operator = 4;
	synth->dispatch[171].routine = (synthRoutine) cs80Memory;
	synth->dispatch[172].controller = CS_LP;
	synth->dispatch[172].operator = 5;
	synth->dispatch[172].routine = (synthRoutine) cs80Memory;
	synth->dispatch[173].controller = CS_LP;
	synth->dispatch[173].operator = 6;
	synth->dispatch[173].routine = (synthRoutine) cs80Memory;
	synth->dispatch[174].controller = CS_LP;
	synth->dispatch[174].operator = 7;
	synth->dispatch[174].routine = (synthRoutine) cs80Memory;
	synth->dispatch[175].controller = CS_LP;
	synth->dispatch[175].operator = 8;
	synth->dispatch[175].routine = (synthRoutine) cs80Memory;
	synth->dispatch[176].controller = CS_LP;
	synth->dispatch[176].operator = 9;
	synth->dispatch[176].routine = (synthRoutine) cs80Memory;

	synth->dispatch[177].controller = CS_SAVE;
	synth->dispatch[177].operator = 0;
	synth->dispatch[177].routine = (synthRoutine) cs80Memory;

	synth->dispatch[178].controller = 126;
	synth->dispatch[178].operator = 101;

	synth->dispatch[179].routine = (synthRoutine) cs80Pitchbend;

	/* Tune CH-1 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 6, CONTROLLER_RANGE/2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 6, CONTROLLER_RANGE/2);
	/* PWM LFO Key Sync */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 1, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1, 1);
	/* No touch on Filter env, HP filt, mods */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 5, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 5, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 6, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 6, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 6, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 9, 6, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 2, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 2, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 9, 2, C_RANGE_MIN_1);
	/* Noise gain */
	bristolMidiSendMsg(global.controlfd, synth->sid, 15, 0, 8192);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
cs80Configure(brightonWindow *win)
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

	synth->keypanel = KEY_PANEL;

	if (synth->location == 0)
	{
		synth->bank = 10;
		synth->location = 1;
	} else if (synth->location > 0) {
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

	if (cs80Debug(synth, 2))
		printf("weight %i, bank %i, mem %i\n", mw, synth->bank,synth->location);

	configureGlobals(synth);

	synth->keypanel2 = -1;
	synth->transpose = 24;

	synth->flags |= OPERATIONAL;

	dc1 = brightonGetDCTimer(win->dcTimeout);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	brightonParamChange(synth->win, KEY_PANEL + 2, -1, &event);

	/* Push some radio buttons to set their starting point */
	event.value = 1.0;
	synth->dispatch[RADIO_UB].other1 = RADIO_UB;
	synth->dispatch[RADIO_LB].other1 = RADIO_LB;
	synth->dispatch[RADIO_UP].other1 = RADIO_UP;
	synth->dispatch[RADIO_LP].other1 = RADIO_LP;
	brightonParamChange(synth->win, 1, RADIO_UB, &event);
	brightonParamChange(synth->win, 1, RADIO_LB, &event);
	brightonParamChange(synth->win, 1, RADIO_UP, &event);
	brightonParamChange(synth->win, 1, RADIO_LP, &event);

	brightonPut(win,
		"bitmaps/blueprints/cs80shade.xpm", 0, 0, win->width, win->height);
	width = win->width;

	/* Then set some defaults for volume, balance and tuning */
	event.type = BRIGHTON_FLOAT;
	event.value = 0.9;
	brightonParamChange(synth->win, 1, 146, &event);
	event.value = 0.5;
	brightonParamChange(synth->win, 1, 145, &event);
	event.value = 1.0; /* On/Off switch for Opts panel */
	brightonParamChange(synth->win, 4, 0, &event);

	/* Then set the load button twice to get a memory loaded */
	cs80MemoryLoad(synth, 0, 0, 0);

	if (cs80Debug(synth, 3))
		printf("dct = %i\n", win->dcTimeout);

	return(0);
}

