
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

static int hammondB3Init();
static int hammondB3Configure();
static int hammondB3MidiCallback(brightonWindow *, int, int, float);
static int hammondB3Callback(brightonWindow *, int, int, float);
static int hammondB3destroy();
/*static int keyCallback(void *, int, int, float); */
/*static int modCallback(void *, int, int, float); */
static void doClick();

static int dc = 0, shade_id;

extern guimain global;
static guimain manual;

#include "brightonKeys.h"

#define FIRST_DEV 0
#define C_COUNT (35 + FIRST_DEV)
#define MEM_COUNT 16
#define VOL_COUNT 1
#define LESLIE_COUNT 2
#define OPTS_COUNT 40

#define ACTIVE_DEVS (C_COUNT + OPTS_COUNT + VOL_COUNT + LESLIE_COUNT - 1)
#define DEVICE_COUNT (C_COUNT + OPTS_COUNT + VOL_COUNT + LESLIE_COUNT + MEM_COUNT)

#define OPTS_PANEL 1

#define DRAWBAR_START 9

#define DEVICE_START FIRST_DEV
#define OPTS_START (DEVICE_START + C_COUNT)
#define VOL_START (OPTS_START + OPTS_COUNT)
#define LESLIE_START (VOL_START + VOL_COUNT)
#define MEM_START (LESLIE_START + LESLIE_COUNT)

#define DISPLAY_PAN 4
#define DISPLAY_DEV (MEM_COUNT - 1)

#define KEY_PANEL1 6
#define KEY_PANEL2 7

#define RADIOSET_1 MEM_START

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
 * This example is for a hammondB3Bristol type synth interface.
 */
#define SD1 28
#define SD2 50

#define W1 20
#define L1 770

#define R1 0

#define Cm1 225
#define Cm2 (Cm1 + SD1)

#define C1 290
#define C2 (C1 + SD1)
#define C3 (C2 + SD1)
#define C4 (C3 + SD1)
#define C5 (C4 + SD1)
#define C6 (C5 + SD1)
#define C7 (C6 + SD1)
#define C8 (C7 + SD1)
#define C9 (C8 + SD1)

#define C11 556
#define C12 (C11 + SD1)
#define C13 (C12 + SD1)
#define C14 (C13 + SD1)
#define C15 (C14 + SD1)
#define C16 (C15 + SD1)
#define C17 (C16 + SD1)
#define C18 (C17 + SD1)
#define C19 (C18 + SD1)

#define MC1 9
#define MC2 55
#define MC3 183

#define MC4 810
#define MC5 (MC4 + SD2)
#define MC6 (MC5 + SD2)
#define MC7 (MC6 + SD2)

#define MR1 360
#define MR2 650
#define MR3 700

#define MW1 36
#define MW2 50

#define MH1 525
#define MH2 200

#include <brightondevflags.h>

/*
 * Drawbars were 0 to 7 due to encoding them in a single 7bit value - 3 bits
 * were for gain and 4 bits required for the drawbar. This was always in need
 * of a change and will implement it finally. It's actually quite trivial, it
 * uses sequential encoding rather than reserving bits, the latter is too
 * lossy.
 */
static brightonLocations locations[C_COUNT] = {
	{"Lower Drawbar 16", BRIGHTON_HAMMOND, C1, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Drawbar 5 1/3", BRIGHTON_HAMMOND, C2, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Drawbar 8", BRIGHTON_HAMMOND, C3, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Drawbar 4", BRIGHTON_HAMMOND, C4, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Drawbar 2 2/3", BRIGHTON_HAMMOND, C5, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Drawbar 2", BRIGHTON_HAMMOND, C6, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Drawbar 1 3/5", BRIGHTON_HAMMOND, C7, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Drawbar 1 1/3", BRIGHTON_HAMMOND, C8, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Drawbar 1", BRIGHTON_HAMMOND, C9, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},

	{"Upper Drawbar 16", BRIGHTON_HAMMOND, C11, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Drawbar 5 1/3", BRIGHTON_HAMMOND, C12, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Drawbar 8", BRIGHTON_HAMMOND, C13, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Drawbar 4", BRIGHTON_HAMMOND, C14, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Drawbar 2 2/3", BRIGHTON_HAMMOND, C15, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Drawbar 2", BRIGHTON_HAMMOND, C16, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Drawbar 1 3/5", BRIGHTON_HAMMOND, C17, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Drawbar 1 1/3", BRIGHTON_HAMMOND, C18, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Drawbar 1", BRIGHTON_HAMMOND, C19, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},

	/* 18 - switches, etc */
	{"Leslie", 2, MC1, MR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockersmooth.xpm",
		"bitmaps/buttons/rockersmoothd.xpm", 0},
	{"Reverb", 2, MC2, MR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockersmooth.xpm",
		"bitmaps/buttons/rockersmoothd.xpm", 0},
	{"VibraChorus", 0, MC2 + 57, MR1 + 70, MW1 + 10, MH1, 0, 6, 0, 0, 0, 0},
	{"Bright", 2, MC3, MR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockersmooth.xpm",
		"bitmaps/buttons/rockersmoothd.xpm", 0},

	/* 22 - perc switches, etc */
	{"Perc 4", 2, MC4, MR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockersmooth.xpm",
		"bitmaps/buttons/rockersmoothd.xpm", 0},
	{"Perc 2 2/3", 2, MC5, MR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockersmooth.xpm",
		"bitmaps/buttons/rockersmoothd.xpm", 0},
	{"Perc Slow", 2, MC6, MR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockersmooth.xpm",
		"bitmaps/buttons/rockersmoothd.xpm", 0},
	{"Perc Soft", 2, MC7, MR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockersmooth.xpm",
		"bitmaps/buttons/rockersmoothd.xpm", 0},

	/* 26 - Bass drawbars */
	{"Bass Drawbar 16", BRIGHTON_HAMMOND, Cm1, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Bass Drawbar 8", BRIGHTON_HAMMOND, Cm2, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},

	/*
	 * 28 - Dummies. May one day extend the bass drawbar configuration as per
	 * some other emulations. It is extremely easy to do, just not convinved it
	 * really make sense.
	 */
	{"", BRIGHTON_HAMMOND, Cm2, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", BRIGHTON_HAMMOND, Cm1, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", BRIGHTON_HAMMOND, Cm2, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", BRIGHTON_HAMMOND, Cm1, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", BRIGHTON_HAMMOND, Cm2, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", BRIGHTON_HAMMOND, Cm1, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", BRIGHTON_HAMMOND, Cm2, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
};

static brightonLocations volume[1] = {
	{"MasterVolume", 0, 125, 125, 750, 750, 0, 1, 0, 0, 0, 0},
};

static brightonLocations leslie[2] = {
	{"Leslie On/Off", 2, 100, 600, 800, 200, 0, 1, 0,
		"bitmaps/buttons/sw3.xpm",
		"bitmaps/buttons/sw5.xpm", BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW},
	{"", 2, 100, 100, 800, 200, 0, 1, 0,
	/*
	 * Use a slighly older rocker than the plastic white, looks mildly more
	 * authentic.
		"bitmaps/buttons/rockerwhite.xpm", 0, 0}
	 */
		"bitmaps/buttons/sw3.xpm",
		"bitmaps/buttons/sw5.xpm", BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW}
};

#define mR1 100
#define mR2 300
#define mR3 550
#define mR4 800

#define mC1 50
#define mC2 213
#define mC3 376
#define mC4 539
#define mC5 705

#define S4 100
#define S5 150

static brightonLocations mem[MEM_COUNT] = {
	/* memories */
	{"", 2, mC5, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC1, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC2, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC3, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC4, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC5, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC1, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC2, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC3, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, mC4, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	/* midi U, D, Load, Save */
	{"", 2, mC1, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC2, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC4, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC3, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffo.xpm", 
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC5, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	/* display */
	{"", 3, mC1, mR1, 750, 125, 0, 1, 0, 0, 0, 0},
};

#define OD1 90
#define OD2 92
#define OD3 70

#define OC1 65
#define OC2 (OC1 + OD1 + 50)
#define OC3 (OC2 + OD1)
#define OC4 (OC3 + OD1)
#define OC5 (OC4 + OD1)
#define OC6 (OC5 + OD1)
#define OC7 (OC6 + OD1)
#define OC8 (OC7 + OD1)
#define OC9 (OC8 + OD1)
#define OC10 (OC9 + OD1)

#define OR1 23
#define OR2 (OR1 + OD2)
#define OR3 (OR2 + OD2)
#define OR4 (OR3 + OD2)

#define OR5 (OR1 + 250)
#define OR6 (OR5 + 250)
#define OR7 (OR6 + 250)

#define OS1 20
#define OS2 88
#define OS3 150
static brightonLocations opts[OPTS_COUNT] = {
	/* Leslie type - 26 */
	{"Leslie Separate", 2, OC1, OR1, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Leslie Sync", 2, OC1, OR2, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Leslie NoBass", 2, OC1, OR3, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Leslie Break", 2, OC1, OR4, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	/* Leslie global - 30 */
	{"Leslie X-Over", 0, OC2, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Leslie Inertia", 0, OC3, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm",
		0, BRIGHTON_NOSHADOW},
	/* Leslie HS - 32 */
	{"Leslie HighFreq", 0, OC5, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Leslie HighDepth", 0, OC6, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Leslie HighPhase", 0, OC7, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm",
		0, BRIGHTON_NOSHADOW},
	/* Leslie global - 35 */
	{"Leslie Overdrive", 0, OC4, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm",
		0, BRIGHTON_NOSHADOW},
	/* Leslie LS - 37 */
	{"Leslie LowFreq", 0, OC8, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Leslie LowDepth", 0, OC9, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Leslie LowPhase", 0, OC10, OR1, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm",
		0, BRIGHTON_NOSHADOW},
	/* VC Params - 40 */
	{"VibraChorus Phase", 0, OC2, OR5, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"VibraChorus LC", 0, OC3, OR5, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"VibraChorus Depth", 0, OC4, OR5, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_NOSHADOW},
	/* Envelopes 43 */
	{"Perc Fast Decay", 0, OC2, OR6, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Perc Slow Decay", 0, OC3, OR6, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Perc Fast Attack", 0, OC5, OR6, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Perc Slow Attack", 0, OC6, OR6, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm",
		0, BRIGHTON_NOSHADOW},
	/* Diverse - 47 */
	{"Preacher", 2, OC1, OR7 - 10, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Compression", 2, OC1, OR7 + OD2 - 10, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"Bright", 0, OC2, OR7, OS3, OS3, 0, 7, 0, "bitmaps/knobs/knobyellownew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Click", 0, OC3, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Leslie T Phase", 0, OC10, OR5, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Damping", 0, OC4, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm",
		0, BRIGHTON_NOSHADOW},
	/* Hammond chorus phase delay 53 */
	{"VibraChorus PD", 0, OC5, OR5, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobgreynew.xpm",
		0, BRIGHTON_NOSHADOW},
/* Five reverb controls from 27 */
	{"Reverb Decay", 0, OC6, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Reverb Drive", 0, OC7, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Reverb Crossover", 0, OC8, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Reverb Feedback", 0, OC9, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_NOSHADOW},
	{"Reverb Wet", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_NOSHADOW},

	{"", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, OC10, OR7, OS3, OS3, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
};

static int commonPreset = -1;

static void
commonPresets(brightonWindow *win, guiSynth *synth, int index, float value)
{
	if (value == 0.0) {
		if (index != 23)
			commonPreset = -1;
		return;
	}

	if (commonPreset != -1)
	{
		brightonEvent event;

		event.command = BRIGHTON_PARAMCHANGE;
		event.type = BRIGHTON_FLOAT;
		event.value = 0;

/*	printf("commonPresets(): %i %i\n", index, commonPreset); */
		if (index == 23)
		{
			if (brightonDoubleClick(dc))
				saveMemory(synth, "hammondB3", 0, synth->bank + synth->location,
					FIRST_DEV);

			brightonParamChange(synth->win, KEY_PANEL2, index - 12, &event);

			return;
		}

		if (commonPreset < 12)
			/* Key release upper manual */
			brightonParamChange(synth->win, KEY_PANEL1, commonPreset, &event);
		else
			/* Key release lower manual, commonPreset - 12 */
			brightonParamChange(synth->win, KEY_PANEL2, commonPreset - 12,
				&event);
	}

	synth->location = commonPreset = index;

	loadMemory(synth, "hammondB3", 0, synth->bank + synth->location,
		C_COUNT, FIRST_DEV, 0);
}

static int
umCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (global.libtest)
		return(0);

	if (synth->flags & REQ_MIDI_DEBUG)
		printf("umCallback(%i, %i, %f)\n", global.libtest, index, value);

	/*
	 * If the code from 0 to 11 then this will become the preset section and
	 * be used to load the first 24 memories.
	 */
	if (index < 12)
	{
/*		printf("umCallback(%x, %i, %i, %f): %i %i\n", synth, panel, index, */
/*			value, synth->transpose, global.controlfd); */
		commonPresets(win, synth, index, value);
		return(0);
	}

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 */
	if (value)
		bristolMidiSendMsg(manual.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
	else
		bristolMidiSendMsg(manual.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);

	return(0);
}

static int
lmCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (global.libtest)
		return(0);

	if (synth->flags & REQ_MIDI_DEBUG)
		printf("lmCallback(%i, %i, %f)\n", panel, index, value);

	/*
	 * If the code from 0 to 11 then this will become the preset section and
	 * be used to load the first 24 memories.
	 */
	if (index < 12)
	{
/*		printf("lmCallback(%x, %i, %i, %f): %i %i\n", synth, panel, index, */
/*			value, synth->transpose, global.controlfd); */

		commonPresets(win, synth, index + 12, value);
		return(0);
	}

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 */
	if (value)
		bristolMidiSendMsg(manual.controlfd, synth->midichannel + 1,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose);
	else
		bristolMidiSendMsg(manual.controlfd, synth->midichannel + 1,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose);

	return(0);
}

static int
hammondB3destroy(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("hammondB3destroy(%p): %i\n", win, synth->midichannel);

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

int
b3PedalCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 */
	if (value > 0)
		bristolMidiSendMsg(global.controlfd, synth->midichannel + 1,
			BRISTOL_EVENT_KEYON, 0, index);
	else
		bristolMidiSendMsg(global.controlfd, synth->midichannel + 1,
			BRISTOL_EVENT_KEYOFF, 0, index);

	return(0);
}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp hammondB3App = {
	"hammondB3",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/wood3.xpm",
	0, /* BRIGHTON_STRETCH, //flags */
	hammondB3Init,
	hammondB3Configure, /* 3 callbacks, unused? */
	hammondB3MidiCallback,
	hammondB3destroy,
	{-1, 0, 2, 2, 5, 175, 0, 0},
	765, 400, 0, 0,
	15,
	{
		{
			"HammondB3",
			"bitmaps/blueprints/hammondB3.xpm",
			"bitmaps/textures/metal5.xpm",
			0, /*BRIGHTON_STRETCH, // flags */
			0,
			0,
			hammondB3Callback,
			25, 122, 950, 238,
			C_COUNT,
			locations
		},
		{
			"Opts",
			"bitmaps/blueprints/hammondB3opts.xpm",
			"bitmaps/textures/wood3.xpm",
			0x020, /*BRIGHTON_STRETCH, // flags */
			0,
			0,
			hammondB3Callback,
			74, 357, 632, 440,
			/*70, 440, 470, 500, */
			OPTS_COUNT,
			opts
		},
		{
			"volume",
			0,
			"bitmaps/textures/metal5.xpm",
			0, /*BRIGHTON_STRETCH, // flags */
			0,
			0,
			hammondB3Callback,
			25, 427, 47, 89,
			1,
			volume
		},
		{
			"leslie",
			0,
			"bitmaps/textures/metal5.xpm",
			0, /*BRIGHTON_STRETCH, // flags */
			0,
			0,
			hammondB3Callback,
			30, 707, 40, 89,
			2,
			leslie
		},
		{
			"Memory",
			"bitmaps/blueprints/hammondB3mem.xpm",
			"bitmaps/textures/wood3.xpm",
			0x020, /*BRIGHTON_STRETCH, // flags */
			0,
			0,
			hammondB3Callback,
			704, 358, 270, 440,
			/*70, 440, 470, 500, */
			MEM_COUNT,
			mem
		},
		{
			"HammondB3bar",
			0,
			"bitmaps/textures/metal5.xpm",
			0, /*flags */
			0,
			0,
			0,
			25, 561, 950, 29,
			0,
			0
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/hkbg.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			umCallback,
			70, 357, 878, 211,
			KEY_COUNT_6_OCTAVE,
			keys6hammond
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/hkbg.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			lmCallback,
			74, 589, 870, 211,
			KEY_COUNT_6_OCTAVE,
			keys6hammond
		},
		{
			"Pedalboard",
			0,
			"bitmaps/textures/wood6.xpm",
			BRIGHTON_STRETCH,
			0,
			0,
			b3PedalCallback,
			260, 823, 500, 167,
			KEY_COUNT_PEDAL,
			pedalBoard
		},
		{
			"Wood Edge",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 25, 813,
			0,
			0
		},
		{
			"Wood Edge",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /*flags */
			0,
			0,
			0,
			975, 0, 25, 813,
			0,
			0
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood4.xpm",
			0,
			0,
			0,
			0,
			25, 24, 950, 57,
			0,
			0
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood4.xpm",
			BRIGHTON_STRETCH,
			0,
			0,
			0,
			25, 49, 950, 73,
			0,
			0
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood5.xpm",
			BRIGHTON_STRETCH, /*flags */
			0,
			0,
			0,
			25, 49, 950, 122,
			0,
			0
		},
		{
			"Pedalboard",
			0,
			"bitmaps/keys/vkbg.xpm",
			BRIGHTON_STRETCH,
			0,
			0,
			0,
			0, 813, 1000, 187,
			0,
			0
		},
	}
};

static void
b3SendMsg(int fd, int chan, int c, int o, int v)
{
/*printf("b3SendMsg(%i, %i, %i, %i, %i) [%i, %i]\n", fd, chan, c, o, v, */
/*global.controlfd, manual.controlfd); */
	bristolMidiSendMsg(global.controlfd, chan, c, o, v);
	bristolMidiSendMsg(manual.controlfd, chan + 1, c, o, v);
}

static void
hammondB3Memory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

/*printf("hammondB3Memory(%i, %i)\n", c, o); */

	if (synth->dispatch[RADIOSET_1].other2)
	{
		synth->dispatch[RADIOSET_1].other2 = 0;
		return;
	}

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

				brightonParamChange(synth->win, DISPLAY_PAN,
					synth->dispatch[RADIOSET_1].other1, &event);
			}
			synth->dispatch[RADIOSET_1].other1 = o;

			if (synth->flags & BANK_SELECT) {
				if ((synth->bank * 10 + o * 10) >= 1000)
				{
					synth->location = o;
					synth->flags &= ~BANK_SELECT;
					if (loadMemory(synth, "hammondB3", 0,
						synth->bank + synth->location,
						synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
						displayPanelText(synth, "FRE",
							synth->bank + synth->location,
							DISPLAY_PAN, DISPLAY_DEV);
					else
						displayPanelText(synth, "PRG",
							synth->bank + synth->location,
							DISPLAY_PAN, DISPLAY_DEV);
				} else {
					synth->bank = synth->bank * 10 + o * 10;
					if (loadMemory(synth, "hammondB3", 0,
						synth->bank + synth->location,
						synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
						displayPanelText(synth, "FRE",
							synth->bank + synth->location,
							DISPLAY_PAN, DISPLAY_DEV);
					else
						displayPanelText(synth, "BANK",
							synth->bank + synth->location,
							DISPLAY_PAN, DISPLAY_DEV);
				}
			} else {
				synth->location = o;
				if (loadMemory(synth, "hammondB3", 0,
					synth->bank + synth->location,
					synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
					displayPanelText(synth, "FRE",
						synth->bank + synth->location,
						DISPLAY_PAN, DISPLAY_DEV);
				else
					displayPanelText(synth, "PRG",
						synth->bank + synth->location,
						DISPLAY_PAN, DISPLAY_DEV);
			}
			break;
		case 1:
			synth->flags |= MEM_LOADING;
			if (loadMemory(synth, "hammondB3", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayPanelText(synth, "FRE", synth->bank + synth->location,
					DISPLAY_PAN, DISPLAY_DEV);
			else
				displayPanelText(synth, "PRG", synth->bank + synth->location,
					DISPLAY_PAN, DISPLAY_DEV);
			synth->flags &= ~MEM_LOADING;
			synth->flags &= ~BANK_SELECT;
			break;
		case 2:
			saveMemory(synth, "hammondB3", 0, synth->bank + synth->location,
				FIRST_DEV);
			displayPanelText(synth, "PRG", synth->bank + synth->location,
				DISPLAY_PAN, DISPLAY_DEV);
			synth->flags &= ~BANK_SELECT;
			break;
		case 3:
			if (synth->flags & BANK_SELECT) {
				synth->flags &= ~BANK_SELECT;
				if (loadMemory(synth, "hammondB3", 0,
					synth->bank + synth->location,
					synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
					displayPanelText(synth, "FRE",
						synth->bank + synth->location,
						DISPLAY_PAN, DISPLAY_DEV);
				else
					displayPanelText(synth, "PRG",
						synth->bank + synth->location,
						DISPLAY_PAN, DISPLAY_DEV);
			} else {
				synth->bank = 0;
				displayPanelText(synth, "BANK", synth->bank, DISPLAY_PAN,
					DISPLAY_DEV);
				synth->flags |= BANK_SELECT;
			}
			break;
	}
/*	printf("	hammondB3Memory(B: %i L %i: %i)\n", */
/*		synth->bank, synth->location, o); */
}

static void
hammondB3Midi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;
/*	printf("hammondB3Midi(%i %i %i %i %i)\n", fd, chan, c, o, v); */

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			return;
		}
	} else {
		/*
		 * On the upper side we need to reserve two midi channels
		 */
		if ((newchan = synth->midichannel + 1) >= 15)
		{
			synth->midichannel = 14;
			return;
		}
	}

	/*
	 * Lower manual midi channel selection first
	 */
	bristolMidiSendMsg(manual.controlfd, synth->sid2,
		127, 0, (BRISTOL_MIDICHANNEL|newchan) + 1);

	bristolMidiSendMsg(global.controlfd, synth->sid,
		127, 0, BRISTOL_MIDICHANNEL|newchan);

	synth->midichannel = newchan;

	displayPanelText(synth, "MIDI", synth->midichannel + 1,
		DISPLAY_PAN, DISPLAY_DEV);

}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
hammondB3Callback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*printf("hammondB3Callback(%i, %i, %f): %x\n", panel, index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (hammondB3App.resources[panel].devlocn[index].to == 1.0)
		sendvalue = value * (CONTROLLER_RANGE - 1);
	else
		sendvalue = value;

	switch (panel) {
		default:
		case 0:
			break;
		case 1:
			index += OPTS_START;
			break;
		case 2:
			index += VOL_START;
			break;
		case 3:
			index += LESLIE_START;
			break;
		case 4:
			index += MEM_START;
			break;
	}
/*printf("index is now %i, %i %i\n", index, DEVICE_COUNT, ACTIVE_DEVS); */

	synth->mem.param[index] = value;

	if ((!global.libtest) || (index >= ACTIVE_DEVS) || (index == 18))
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

#ifdef DEBUG
	else
		printf("dispatch[%x,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#endif

	return(0);
}

static void
hammondB3Passthrough(float value)
{
/*	printf("hammondB3Passthrough\n"); */
}

static void
doVolume(guiSynth *synth)
{
	b3SendMsg(global.controlfd, synth->sid, 0, 4,
		(int) (synth->mem.param[VOL_START] * C_RANGE_MIN_1));
	b3SendMsg(global.controlfd, synth->sid2, 0, 4,
		(int) (synth->mem.param[VOL_START] * C_RANGE_MIN_1));
	b3SendMsg(global.controlfd, synth->sid2, 3, 4,
		(int) (synth->mem.param[VOL_START] * C_RANGE_MIN_1));
}

static void
doLMSlider(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int slidervalue;

/*printf("doLMSlider(%x, %i, %i, %i, %i, %i)\n", */
/*synth, fd, chan, cont, op, value); */

	slidervalue = cont * 9 + value;

	bristolMidiSendMsg(manual.controlfd, synth->sid2, 0, 2, slidervalue);
}

static void
doPedalSlider(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int slidervalue;

	slidervalue = cont * 9 + value;

	bristolMidiSendMsg(manual.controlfd, synth->sid2, 3, 2, slidervalue);
}

static void
doDrawbar(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int slidervalue;

	/*printf("doDrawbar(%x, %i, %i, %i, %i, %i)\n", */
	/*	synth, fd, chan, cont, op, value); */

	slidervalue = cont * 9 + value;

	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, slidervalue);
}

static void
hammondB3Switch(guiSynth *id, int fd, int chan, int cont, int op, int value)
{
	brightonEvent event;

/*	printf("hammondB3Switch(%x, %i, %i, %i, %i, %i)\n", */
/*		id, fd, chan, cont, op, value); */

	id->flags &= ~OPERATIONAL;
	/* 
	 * If the sendvalue is zero, then withdraw the opts window, draw the
	 * keyboard window, and vice versa.
	 */
	if (value == 0)
	{
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 1, -1, &event);
		event.intvalue = 0;
		brightonParamChange(id->win, 4, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 5, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 6, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 7, -1, &event);

		shade_id = brightonPut(id->win,
			"bitmaps/blueprints/hammondB3shade.xpm", 0, 0, id->win->width,
				id->win->height);
	} else {
		brightonRemove(id->win, shade_id);

		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 5, -1, &event);
		event.intvalue = 0;
		brightonParamChange(id->win, 6, -1, &event);
		event.intvalue = 0;
		brightonParamChange(id->win, 7, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 1, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 4, -1, &event);
	}
	id->flags |= OPERATIONAL;
}

static void
doVibra(guiSynth *synth, int fd, int chan, int cont, int op, int v)
{
	/*
	printf("doVibra(%i, %i, %i)\n", cont, op, v);
	 * We get a value from 0 to 6.
	 * 0 1 2 are vibra only, with speeds from the opts panel.
	 * 3 is off
	 * 4 5 6 are vibrachorus, with same respective speeds
	 */
	switch (op) {
		case 0: /* Tap delay */
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0, v);
			break;
		case 1: /* Delay line progressive filtering */
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1, v);
			break;
		case 2:
			doClick(synth);
			if (v == 3) {
				bristolMidiSendMsg(global.controlfd, synth->sid, 126, 0, 0);
				return;
			}
			/*
			 * Turn it on, set a depth and a VC.
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 0, 1);
			if (v < 3) {
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 2, 3 - v);
				bristolMidiSendMsg(global.controlfd, synth->sid, 6, 4, 0);
				return;
			}
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 2, v - 3);
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 4, 1);
			break;
		case 3: /* Direct signal gain */
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 3, v);
			break;
		case 4: /* Rate */
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 5, v);
			break;
	}
}

#define LESLIE_ONOFF 18

static void
doLeslieSpeed(guiSynth *synth)
{
	int speed, depth, phase;

	if (synth->mem.param[LESLIE_ONOFF] == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 100, 7, 0);
		return;
	}

	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 4,
		(int) (synth->mem.param[OPTS_START + 24] * C_RANGE_MIN_1));

	/*
	 * If we do not yet have another option, configure this as default
	 */
	if (synth->dispatch[OPTS_START + 1].other1 == 0)
		synth->dispatch[OPTS_START + 1].other1 = 1;

	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 7,
		(int) (synth->dispatch[OPTS_START + 1].other1));

	if (synth->mem.param[LESLIE_START] == 0)
	{
		speed = synth->mem.param[OPTS_START + 6] * C_RANGE_MIN_1;
		depth = synth->mem.param[OPTS_START + 7] * C_RANGE_MIN_1;
		phase = synth->mem.param[OPTS_START + 8] * C_RANGE_MIN_1;
	} else {
		speed = synth->mem.param[OPTS_START + 10] * C_RANGE_MIN_1;
		depth = synth->mem.param[OPTS_START + 11] * C_RANGE_MIN_1;
		phase = synth->mem.param[OPTS_START + 12] * C_RANGE_MIN_1;
	}
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 0, speed);
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 3, depth);
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 2, phase);
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 6,
		(int) (synth->mem.param[OPTS_START + 4] * C_RANGE_MIN_1));
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 1,
		(int) (synth->mem.param[OPTS_START + 5] * C_RANGE_MIN_1));
}

static void
doReverbParam(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
/*printf("doReverbParam(%i, %i, %i)\n", cont, op, value); */
	if (op == 4)
		bristolMidiSendMsg(global.controlfd, synth->sid, 101, 5, value);
	bristolMidiSendMsg(global.controlfd, synth->sid, 101, op, value);
}

static void
doReverb(guiSynth *synth)
{
    if (synth->mem.param[DEVICE_START + 19] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 101, 3, 0);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 101, 3,
			(int) (synth->mem.param[OPTS_START + 30] * C_RANGE_MIN_1));
}

static void
doDamping(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 1, value);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 1, value);
}

static void
doClick(guiSynth *synth)
{
	float scale = 1.0;

	if (synth->mem.param[DEVICE_START + 20] != 3)
		scale = 0.1;

	/*
	 * If we have VC enabled then reduce the click volume by half, otherwise
	 * it is full strength.
	 */
	b3SendMsg(global.controlfd, synth->sid, 0, 6,
		(int) (synth->mem.param[OPTS_START + 23] * C_RANGE_MIN_1 * scale));
	b3SendMsg(global.controlfd, synth->sid2, 0, 6,
		(int) (synth->mem.param[OPTS_START + 23] * C_RANGE_MIN_1 * scale));
}

static void
doPreacher(guiSynth *synth)
{
	if (synth->mem.param[OPTS_START + 20] == 0)
	{
		b3SendMsg(global.controlfd, synth->sid, 126, 3, 0);
		b3SendMsg(global.controlfd, synth->sid, 0, 7, 0);
		b3SendMsg(global.controlfd, synth->sid2, 0, 7, 0);
		b3SendMsg(global.controlfd, synth->sid2, 3, 7, 0);
	} else {
		b3SendMsg(global.controlfd, synth->sid, 126, 3, 1);
		b3SendMsg(global.controlfd, synth->sid, 0, 7, 1);
		b3SendMsg(global.controlfd, synth->sid2, 0, 7, 1);
		b3SendMsg(global.controlfd, synth->sid2, 3, 7, 1);
	}
}

static void
doBright(guiSynth *synth)
{
/*printf("doBright()\n"); */
	if (synth->mem.param[DEVICE_START + 21] == 0)
	{
		/*
		 * Once to the hammond manager
		 */
		b3SendMsg(global.controlfd, synth->sid, 126, 1, 0);
		/*
		 * And to the sine oscillator
		 */
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 0);
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 8);
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 16);
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 24);
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 32);
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 40);
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 48);
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 56);
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 64);
	} else {
		b3SendMsg(global.controlfd, synth->sid, 126, 1,
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 0 +
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 8 +
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 16 +
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 24 +
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 32 +
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 40 +
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 48 +
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 56 +
			(int) (synth->mem.param[OPTS_START + 22]));
		b3SendMsg(global.controlfd, synth->sid, 0, 0, 64 +
			(int) (synth->mem.param[OPTS_START + 22]));
	}
}

static void
doCompress(guiSynth *synth)
{
/*printf("doCompress()\n"); */
	if (synth->mem.param[OPTS_START + 21] == 0)
	{
		b3SendMsg(global.controlfd, synth->sid, 126, 2, 0);
	} else {
		b3SendMsg(global.controlfd, synth->sid, 126, 2, 1);
	}
}

static void
doGrooming(guiSynth *synth)
{
/*printf("doGrooming()\n"); */
	if (synth->mem.param[DEVICE_START + 25] != 0)
	{
		b3SendMsg(global.controlfd, synth->sid, 1, 0,
			(int) (synth->mem.param[OPTS_START + 18] * C_RANGE_MIN_1));
	} else {
		b3SendMsg(global.controlfd, synth->sid, 1, 0,
			(int) (synth->mem.param[OPTS_START + 19] * C_RANGE_MIN_1));
	}
}

static void
doPerc(guiSynth *synth)
{
/*printf("doPerc()\n"); */
	/*
	 * 4 foot percussive
	 */
	if (synth->mem.param[DEVICE_START + 22] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 24);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 25);

	/*
	 * 2 1/3 foot percussive
	 */
	if (synth->mem.param[DEVICE_START + 23] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 32);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 33);

	/*
	 * Slow fast decay
	 */
	if (synth->mem.param[DEVICE_START + 24] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 1,
			(int) (synth->mem.param[OPTS_START + 16] * C_RANGE_MIN_1));
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 1,
			(int) (synth->mem.param[OPTS_START + 17] * C_RANGE_MIN_1));
}

static void
hammondOption(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	brightonEvent event;

/*printf("hammondOption(%x, %i, %i, %i, %i, %i)\n", */
/*	synth, fd, chan, cont, op, value); */

	switch (cont) {
		case OPTS_START:
			/*
			 * Rotation type. Send 100.7 becomes op;
			 */
			if ((synth->flags & MEM_LOADING) == 0)
			{
				if (synth->dispatch[OPTS_START].other2)
				{
					synth->dispatch[OPTS_START].other2 = 0;
					return;
				}
				synth->dispatch[OPTS_START].other2 = 1;
				if (synth->dispatch[OPTS_START].other1 >= 0)
				{
					event.command = BRIGHTON_PARAMCHANGE;
					if (synth->dispatch[OPTS_START].other1 != (op - 1))
						event.value = 0;
					else
						event.value = 1;
					brightonParamChange(synth->win,
						OPTS_PANEL, synth->dispatch[OPTS_START].other1, &event);
				}
			}

			if (synth->mem.param[OPTS_START + op - 1] != 0)
			{
				synth->dispatch[OPTS_START].other1 = op - 1;
				synth->dispatch[OPTS_START + 1].other1 = op;

				if (synth->mem.param[LESLIE_ONOFF] == 0)
					bristolMidiSendMsg(global.controlfd, chan, 100, 7, 0);
				else {
					bristolMidiSendMsg(global.controlfd, chan, 100, 7, op);
					bristolMidiSendMsg(global.controlfd, chan, 100, 1,
						(int) (synth->mem.param[OPTS_START + 5]
							* C_RANGE_MIN_1));
				}
			}
			break;
		case OPTS_START + 3:
			/*
			 * Rotor break. Send 100.7 = 4 off, 100.7 = 5 on.
			 */
			if (value == 0)
				bristolMidiSendMsg(global.controlfd, chan, 100, 7, 4);
			else
				bristolMidiSendMsg(global.controlfd, chan, 100, 7, 5);
			break;
		case OPTS_START + 4:
		case OPTS_START + 5:
		case OPTS_START + 6:
		case OPTS_START + 7:
		case OPTS_START + 8:
		case OPTS_START + 10:
		case OPTS_START + 11:
		case OPTS_START + 12:
			doLeslieSpeed(synth);
			break;
		case OPTS_START + 9:
			/* overdrive */
			bristolMidiSendMsg(global.controlfd, synth->sid, 100, 5,
				(int) (synth->mem.param[OPTS_START + 9] * C_RANGE_MIN_1));
			break;
		case OPTS_START + 13:
		case OPTS_START + 14:
		case OPTS_START + 15:
			doVibra(synth, fd, chan, cont, op, value);
			break;
		case OPTS_START + 16:
		case OPTS_START + 17:
			doPerc(synth);
			break;
		case OPTS_START + 18:
		case OPTS_START + 19:
			doGrooming(synth);
			break;
		case OPTS_START + 20:
			doPreacher(synth);
			break;
		case OPTS_START + 21:
			doCompress(synth);
			break;
		case OPTS_START + 22:
			doBright(synth);
			break;
		case OPTS_START + 23:
			doClick(synth);
			break;
		case OPTS_START + 24:
			doLeslieSpeed(synth);
			break;
		case OPTS_START + 25:
			doDamping(synth, fd, chan, cont, op, value);
			break;
/*		case OPTS_START + 26: */
		default:
			break;
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
hammondB3Init(brightonWindow *win)
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

	printf("Initialise the hammondB3 link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
	synth->dispatch = (dispatcher *)
		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher));
	dispatch = synth->dispatch;

	if (!global.libtest)
	{
		if (synth->synthtype == BRISTOL_HAMMONDB3)
		{
			bcopy(&global, &manual, sizeof(guimain));
			manual.home = 0;

			synth->synthtype = BRISTOL_HAMMOND;
			if ((synth->sid = initConnection(&global, synth)) < 0)
				return(-1);

			/*
			 * send a hello, with a voice count, then request starting of the
			 * synthtype. All of these will require an ACK, and we should wait
			 * here and read that ack before proceeding with each next step.
			 */
			manual.flags |= BRISTOL_CONN_FORCE|BRIGHTON_NOENGINE;
			manual.port = global.port;
			manual.host = global.host;

			synth->midichannel++;
			synth->synthtype = BRISTOL_HAMMONDB3;
			if ((synth->sid2 = initConnection(&manual, synth)) < 0)
				return(-1);
			synth->midichannel--;
			global.manualfd = manual.controlfd;
			global.manual = &manual;
			manual.manual = &global;
		} else {
			if ((synth->sid2 = initConnection(&manual, synth)) < 0)
				return(-1);
		}
	}

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		synth->dispatch[i].routine = (synthRoutine) hammondB3Passthrough;
	}

	/*
	 * Put in the drawbar configurations, upper manual
	 */
	dispatch[0].routine =
	dispatch[1].routine =
	dispatch[2].routine =
	dispatch[3].routine =
	dispatch[4].routine =
	dispatch[5].routine =
	dispatch[6].routine =
	dispatch[7].routine =
	dispatch[8].routine = (synthRoutine) doLMSlider;

	dispatch[0].controller = 0;
	dispatch[1].controller = 1;
	dispatch[2].controller = 2;
	dispatch[3].controller = 3;
	dispatch[4].controller = 4;
	dispatch[5].controller = 5;
	dispatch[6].controller = 6;
	dispatch[7].controller = 7;
	dispatch[8].controller = 8;

	/*
	 * Put in the drawbar configurations, upper manual
	 */
	dispatch[DRAWBAR_START].routine =
	dispatch[DRAWBAR_START + 1].routine =
	dispatch[DRAWBAR_START + 2].routine =
	dispatch[DRAWBAR_START + 3].routine =
	dispatch[DRAWBAR_START + 4].routine =
	dispatch[DRAWBAR_START + 5].routine =
	dispatch[DRAWBAR_START + 6].routine =
	dispatch[DRAWBAR_START + 7].routine =
	dispatch[DRAWBAR_START + 8].routine = (synthRoutine) doDrawbar;

	dispatch[DRAWBAR_START].controller = 0;
	dispatch[DRAWBAR_START + 1].controller = 1;
	dispatch[DRAWBAR_START + 2].controller = 2;
	dispatch[DRAWBAR_START + 3].controller = 3;
	dispatch[DRAWBAR_START + 4].controller = 4;
	dispatch[DRAWBAR_START + 5].controller = 5;
	dispatch[DRAWBAR_START + 6].controller = 6;
	dispatch[DRAWBAR_START + 7].controller = 7;
	dispatch[DRAWBAR_START + 8].controller = 8;

	/* Options controllers */
	dispatch[OPTS_START].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START].controller = OPTS_START;
	dispatch[OPTS_START].operator = 1;
	dispatch[OPTS_START + 1].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 1].controller = OPTS_START;
	dispatch[OPTS_START + 1].operator = 2;
	dispatch[OPTS_START + 2].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 2].controller = OPTS_START;
	dispatch[OPTS_START + 2].operator = 3;
	dispatch[OPTS_START].other1 = -1;
	dispatch[OPTS_START].other2 = 0;
	dispatch[OPTS_START + 3].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 3].controller = OPTS_START + 3;

	dispatch[OPTS_START + 4].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 4].controller = OPTS_START + 4;
	dispatch[OPTS_START + 5].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 5].controller = OPTS_START + 5;
	dispatch[OPTS_START + 6].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 6].controller = OPTS_START + 6;
	dispatch[OPTS_START + 7].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 7].controller = OPTS_START + 7;
	dispatch[OPTS_START + 8].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 8].controller = OPTS_START + 8;
	dispatch[OPTS_START + 9].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 9].controller = OPTS_START + 9;
	dispatch[OPTS_START + 10].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 10].controller = OPTS_START + 10;
	dispatch[OPTS_START + 11].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 11].controller = OPTS_START + 11;
	dispatch[OPTS_START + 12].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 12].controller = OPTS_START + 12;

	/* vibra */
	dispatch[OPTS_START + 13].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 13].controller = OPTS_START + 13;
	dispatch[OPTS_START + 13].operator = 0;
	dispatch[OPTS_START + 14].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 14].controller = OPTS_START + 14;
	dispatch[OPTS_START + 14].operator = 1;
	dispatch[OPTS_START + 15].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 15].controller = OPTS_START + 15;
	dispatch[OPTS_START + 15].operator = 3;
	dispatch[DEVICE_START + 20].controller = 6;
	dispatch[DEVICE_START + 20].operator = 2;
	dispatch[DEVICE_START + 20].routine = (synthRoutine) doVibra;

	/* Percussives */
	dispatch[OPTS_START + 16].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 16].controller = OPTS_START + 16;
	dispatch[OPTS_START + 17].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 17].controller = OPTS_START + 17;
	dispatch[22].routine = (synthRoutine) doPerc;
	dispatch[23].routine = (synthRoutine) doPerc;
	dispatch[24].routine = (synthRoutine) doPerc;

	/* Soft attack - grooming */
	dispatch[OPTS_START + 18].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 18].controller = OPTS_START + 18;
	dispatch[OPTS_START + 19].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 19].controller = OPTS_START + 19;
	dispatch[DEVICE_START + 25].routine = (synthRoutine) doGrooming;

	/* Bass Drawbars */
	dispatch[26].routine = dispatch[27].routine = (synthRoutine) doPedalSlider;
	dispatch[26].controller = 0;
	dispatch[27].controller = 1;

	/* preacher */
	dispatch[OPTS_START + 20].routine = (synthRoutine) doPreacher;
	dispatch[OPTS_START + 20].controller = OPTS_START + 20;
	dispatch[OPTS_START + 21].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 21].controller = OPTS_START + 21;
	dispatch[OPTS_START + 22].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 22].controller = OPTS_START + 22;
	dispatch[DEVICE_START + 21].routine = (synthRoutine) doBright;
	dispatch[OPTS_START + 23].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 23].controller = OPTS_START + 23;

	/* reverb */
	dispatch[OPTS_START + 24].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 24].controller = OPTS_START + 24;
	dispatch[OPTS_START + 25].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 25].controller = OPTS_START + 25;
/*	dispatch[OPTS_START + 26].routine = (synthRoutine) hammondOption; */
	dispatch[OPTS_START + 26].controller = 6;
	dispatch[OPTS_START + 26].operator = 4;
	dispatch[OPTS_START + 26].routine = (synthRoutine) doVibra;

	/* Reverb */
	dispatch[OPTS_START + 27].controller = 101;
	dispatch[OPTS_START + 27].operator = 0;
	dispatch[OPTS_START + 27].routine = (synthRoutine) doReverbParam;
	dispatch[OPTS_START + 28].controller = 101;
	dispatch[OPTS_START + 28].operator = 1;
	dispatch[OPTS_START + 28].routine = (synthRoutine) doReverbParam;
	dispatch[OPTS_START + 29].controller = 101;
	dispatch[OPTS_START + 29].operator = 2;
	dispatch[OPTS_START + 29].routine = (synthRoutine) doReverbParam;
	dispatch[OPTS_START + 30].controller = 101;
	dispatch[OPTS_START + 30].operator = 3;
	dispatch[OPTS_START + 30].routine = (synthRoutine) doReverbParam;
	dispatch[OPTS_START + 31].controller = 101;
	dispatch[OPTS_START + 31].operator = 4;
	dispatch[OPTS_START + 31].routine = (synthRoutine) doReverbParam;
	dispatch[DEVICE_START + 19].routine = (synthRoutine) doReverb;

	dispatch[DEVICE_START + 18].routine = (synthRoutine) doLeslieSpeed;
	dispatch[LESLIE_START].routine = (synthRoutine) doLeslieSpeed;

	dispatch[VOL_START].routine = (synthRoutine) doVolume;

	/* Memory/Midi buttons */
	dispatch[MEM_START + 10].controller = 12;
	dispatch[MEM_START + 10].operator = 0;

	/*
	 * These are for the memory radio buttons
	 */
	dispatch[MEM_START].other1 = 0;
	dispatch[MEM_START].other2 = 0;

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
		(synthRoutine) hammondB3Memory;

	/*
	 * Mem load and save
	 */
	dispatch[MEM_START + 12].controller = 1;
	dispatch[MEM_START + 13].controller = 2;
	dispatch[MEM_START + 14].controller = 3;
	dispatch[MEM_START + 12].routine = dispatch[MEM_START + 13].routine =
		dispatch[MEM_START + 14].routine = (synthRoutine) hammondB3Memory;

	/*
	 * Midi up/down
	 */
	dispatch[MEM_START + 10].controller = 2;
	dispatch[MEM_START + 11].controller = 1;
	dispatch[MEM_START + 10].routine = dispatch[MEM_START + 11].routine =
		(synthRoutine) hammondB3Midi;

	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, 3);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 3, 400);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 4, 13000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 5, 0);

	bristolMidiSendMsg(global.controlfd, synth->sid, 12, 7, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 0, 4);

	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 0, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, 10);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 13000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, 400);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 13000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 5, 0);

	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 0, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 1, 1200);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 3, 20);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 15500);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 5, 0);

	bristolMidiSendMsg(manual.controlfd, synth->sid2, 1, 0, 2);
	bristolMidiSendMsg(manual.controlfd, synth->sid2, 1, 1, 3);
	bristolMidiSendMsg(manual.controlfd, synth->sid2, 1, 2, 16383);
	bristolMidiSendMsg(manual.controlfd, synth->sid2, 1, 3, 1000);
	bristolMidiSendMsg(manual.controlfd, synth->sid2, 1, 4, 13000);
	bristolMidiSendMsg(manual.controlfd, synth->sid2, 1, 5, 0);

	dispatch[LESLIE_START + 1].routine = (synthRoutine) hammondB3Switch;

	return(0);
}

static int
hammondB3MidiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, "hammondB3", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
hammondB3Configure(brightonWindow *win)
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
	synth->keypanel = 6;
	synth->keypanel2 = 7;
	synth->transpose = 24;

	synth->bank = 0;
	synth->location = 0;

	hammondB3Switch(synth, 0, 0, 0, 0, 1);
	loadMemory(synth, "hammondB3", 0, 0, synth->mem.active, FIRST_DEV, 0);
	hammondB3Switch(synth, 0, 0, 0, 0, 0);

	/*
	 * We also want to configure the mod wheel so that it is half way up, this
	 * affects the speed of the leslie.
	 */
	if (global.libtest == 0)
	{
		bristolMidiControl(global.controlfd, synth->midichannel,
			0, 1, C_RANGE_MIN_1 >> 1);
		bristolMidiControl(global.controlfd, synth->midichannel + 1,
			0, 1, C_RANGE_MIN_1 >> 1);
		/*
		 * And the expression (foot) pedal to full on as it governs the gain
		 */
		bristolMidiControl(global.controlfd, synth->midichannel,
			0, 4, C_RANGE_MIN_1);
		bristolMidiControl(global.controlfd, synth->midichannel + 1,
			0, 4, C_RANGE_MIN_1);
	}

	synth->mem.param[LESLIE_ONOFF] = 1;

	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, 8, -1, &event);

/*	event.value = 1.0; */
/*	brightonParamChange(synth->win, 1, 2, &event); */
/*	brightonParamChange(synth->win, 1, 0, &event); */
	configureGlobals(synth);

	dc = brightonGetDCTimer(win->dcTimeout);

	bristolMidiSendMsg(global.controlfd, synth->midichannel,
		BRISTOL_EVENT_KEYON, 0, 10 + synth->transpose);
	bristolMidiSendMsg(global.controlfd, synth->midichannel,
		BRISTOL_EVENT_KEYOFF, 0, 10 + synth->transpose);

	return(0);
}

