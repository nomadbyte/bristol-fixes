
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
#include "brightonKeys.h"
#include "brightonMixer.h"
#include "brightonMixerMemory.h"

static int mixInit();
static int mixConfigure();
static int mixCallback(brightonWindow *, int, int, float);

extern guimain global;

extern int functionOp(guiSynth *, int, int, int, float);
extern void mixerMemory(guiSynth *, int, int, int, float *);
extern int loadMixerMemory(guiSynth *, char *, int);
extern int setMixerMemory(mixerMem *, int, int, float *, char *);


static void mixSendMsg(guiSynth *, int, int, int, int, int);

/* brightonMixerMemory.c */
extern void *initMixerMemory(int count);
	
/*
 * The last parameter, device count, was used to define a memory structure.
 * This is used by the GUI parameter changes to store the parameter values. From
 * here we should be able to save the structure.
 *
 * Personally I think we should define a memory structure that is overlayed 
 * onto the array of floats that defines the actual memory space. To allow for
 * extensability then bussing should come first, then tracks listed from '1'.
 * The function section which includes master volume is not saved.
 */

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
 * This example is for a mixBristol type synth interface.
 */
#define CD1 37
#define R1 46
#define R2 (R1 + CD1)
#define R3 (R2 + CD1)
#define R4 (R3 + CD1)
#define R5 (R4 + CD1)
#define R6 (R5 + CD1)
#define R7 (R6 + CD1)
#define R8 (R7 + CD1)
#define R9 (R8 + CD1)
#define R10 (R9 + CD1)
#define R11 (R10 + CD1)
#define R12 (R11 + CD1)
#define R13 (R12 + CD1)
#define R14 (R13 + CD1)
#define R15 (R14 + CD1)
#define R16 (R15 + CD1)
#define R17 (R16 + CD1)
#define R18 (R17 + CD1)

#define RSL 741
#define RTX (RSL + 50)

#define D1 180

#define C0 120
#define C1 275
#define C2 100
#define C3 600
#define C4 350

#define C5 75
#define C6 400
#define C7 725

#define C8 30
#define C9 275
#define C10 525
#define C11 760

#define C12 770

#define S1 450
#define S2 40
#define SB3 225
#define SB4 11
#define SB5 20
#define SB6 200
#define SB7 9

#define BH 10
#define BH1 9

#define BB1 2
#define BB2 (BB1 + BH)
#define BB3 (BB2 + BH)
#define BB4 (BB3 + BH)

#define BBR1 685
#define BBR2 (BBR1 + BH)
#define BBR3 (BBR2 + BH)
#define BBR4 (BBR3 + BH)
#define BBR5 (BBR4 + BH)

/*
 * This should be organised such that continuous controllers are located first,
 * then buttons? The benefits of such an organisation is that the continuous 
 * controllers are likely to be extended, and the button groups tend to be
 * exclusive, so whilst we have 16 IO selectors, for example, these are only
 * represented by one parameter, the channel number.
 */
static brightonLocations locations[MEM_COUNT] = {
	/*
	 * Input channel select buttons
	 */
	{"", 2, C8, BB1, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BB1, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BB1, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BB1, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C8, BB2, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BB2, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BB2, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BB2, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C8, BB3, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BB3, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BB3, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BB3, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C8, BB4, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BB4, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BB4, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BB4, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},

	/*
	 * presend select
	 */
	{"", 2, C12, R2, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R2 + 10, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R2 + 19, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R2 + 29, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW},

	/*
	 * Dynamics select
	{"", 2, C12, R3, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	 */
	{"", 2, C12, R5 + 0, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R5 + 10, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R5 + 20, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},

	/*
	 * Filter select
	 */
	{"", 2, C12, R10 + 1, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R10 + 11, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R10 + 21, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R10 + 31, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},

	/*
	 * postsend select
	 */
	{"", 2, C12, R13 + 0, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R13 + 9, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R13 + 19, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R13 + 29, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW},

	/*
	 * Effects select: reverb, echo, chorus, flanger, tremelo, vibrato, delay
	 */
	{"", 2, C12, R14 + 6, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R14 + 16, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R14 + 26, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R14 + 35, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R14 + 45, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R14 + 55, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C12, R14 + 65, SB3, SB7, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},

	{"", 0, C1, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobred.xpm", 0, 0},
	{"", 0, C0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobyellow.xpm", 0, 0},
	{"", 0, C0, R3, S1, S2, 0, 1, 0, "bitmaps/knobs/knob.xpm", 0, 0},
	{"", 0, C0, R4, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, C0, R5, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, C0, R6, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, C0, R7, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, C0, R8, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, C0, R9, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, C0, R10, S1, S2, 0, 1, 0, "bitmaps/knobs/knob.xpm", 0, 0},
	{"", 0, C0, R11, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, C0, R12, S1, S2, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, C0, R13, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, C0, R14, S1, S2, 0, 1, 0, "bitmaps/knobs/knoborange.xpm", 0, 0},
	{"", 0, C0, R15, S1, S2, 0, 1, 0, "bitmaps/knobs/knoborange.xpm", 0, 0},
	{"", 0, C0, R16, S1, S2, 0, 1, 0, "bitmaps/knobs/knoborange.xpm", 0, 0},
	{"", 0, C1, R17, S1, S2, 0, 1, 0, "bitmaps/knobs/knobyellow.xpm", 0, BRIGHTON_NOTCH},

	/*
	 * Mute/Solo/Boost
	 */
	{"", 2, C5, R18, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C6, R18, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C7, R18, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffS.xpm", "bitmaps/buttons/pressonS.xpm", BRIGHTON_NOSHADOW},

	/*
	 * Output channel select buttons
	 */
	{"", 2, C8, BBR1, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BBR1, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BBR1, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BBR1, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C8, BBR2, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BBR2, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BBR2, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BBR2, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C8, BBR3, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BBR3, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BBR3, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BBR3, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C8, BBR4, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BBR4, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BBR4, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BBR4, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW},

	/*
	 * Bus select buttons
	 */
	{"", 2, C8, BBR5, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSb.xpm", "bitmaps/buttons/pressonSb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C9, BBR5, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSb.xpm", "bitmaps/buttons/pressonSb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C10, BBR5, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSb.xpm", "bitmaps/buttons/pressonSb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, C11, BBR5, SB3, SB4, 0, 1.01, 0,
		"bitmaps/buttons/pressoffSb.xpm", "bitmaps/buttons/pressonSb.xpm", BRIGHTON_NOSHADOW},

	{"", 1, C4 - 25, RSL, 360, 226, 0, 1, 0, "bitmaps/knobs/sliderblack2.xpm", 0, 0},
	/* Finally the display. This is the 80th location (0..79). */
	{"", 3, 0, 977, 1000, 13, 0, 1, 0, "bitmaps/digits/display2.xpm", 0, 0}
};

#define BS1 80
#define BS2 70

#define BRD1 100
#define BRD2 72

#define BR1 11
#define BR2 (BR1 + BRD2)
#define BR3 (BR2 + BRD2)
#define BR4 (BR3 + BRD2)
#define BR5 (BR4 + BRD2)
#define BR6 (BR5 + BRD2)
#define BR7 (BR6 + BRD2)
#define BR8 (BR7 + BRD2)
#define BR9 (BR8 + BRD2)

#define BR10 (BR9 + BRD2)

#define BCD1 100
#define BCD2 125

#define BC1 35
#define BC2 200
#define BC3 350
#define BC4 550
#define BC5 700
#define BC6 850

#define BC7 (BC1 + BCD2)
#define BC8 (BC7 + BCD2)
#define BC9 (BC8 + BCD2)
#define BC10 (BC9 + BCD2)
#define BC11 (BC10 + BCD2)
#define BC12 (BC11 + BCD2)
#define BC13 (BC12 + BCD2)

#define FRS1 125

#define FBC0 5
#define FBC1 (115)
#define FBC2 (FBC1 + FRS1)
#define FBC3 (FBC2 + FRS1)
#define FBC4 (FBC3 + FRS1)
#define FBC5 (FBC4 + FRS1)
#define FBC6 (FBC5 + FRS1)
#define FBC7 860

#define FBW1 35
#define FBW2 32

#define FBH1 13

#define FBS1 70

/*
 * One effect - FX select, some params, output channels selection.
 */

#define FXBUS(off) \
	{"", 0, FBC1, off, FBS1, FBS1, 0, 1, 0, \
		"bitmaps/knobs/knobgreen.xpm", 0, 0}, \
	{"", 0, FBC2, off, FBS1, FBS1, 0, 1, 0, \
		"bitmaps/knobs/knoborange.xpm", 0, 0}, \
	{"", 0, FBC3, off, FBS1, FBS1, 0, 1, 0, \
		"bitmaps/knobs/knoborange.xpm", 0, 0}, \
	{"", 0, FBC4, off, FBS1, FBS1, 0, 1, 0, \
		"bitmaps/knobs/knoborange.xpm", 0, 0}, \
	{"", 0, FBC5, off, FBS1, FBS1, 0, 1, 0, \
		"bitmaps/knobs/knobyellow.xpm", 0, BRIGHTON_NOTCH}, \
	{"", 0, FBC6, off, FBS1, FBS1, 0, 1, 0, \
		"bitmaps/knobs/knobred.xpm", 0, 0}, \
	{"", 2, FBC0, off + 0, FBW1, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC0, off + 13, FBW1, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC0, off + 26, FBW1, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC0, off + 39, FBW1, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC0 + FBW1 + 5, off + 0, FBW1, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC0 + FBW1 + 5, off + 13, FBW1, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC0 + FBW1 + 5, off + 26, FBW1, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC0 + FBW1 + 5, off + 39, FBW1, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSy.xpm", "bitmaps/buttons/pressonSy.xpm", BRIGHTON_NOSHADOW}, \
	\
	{"", 2, FBC7 + FBW1 * 0 + 3, off + 0, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 1 + 1, off + 0, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 2, off + 0, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 3, off + 0, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 0 + 3, off + 13, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 1 + 1, off + 13, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 2, off + 13, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 3, off + 13, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 0 + 3, off + 26, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 1 + 1, off + 26, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 2, off + 26, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 3, off + 26, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 0 + 3, off + 39, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 1 + 1, off + 39, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 2, off + 39, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}, \
	{"", 2, FBC7 + FBW1 * 3, off + 39, FBW2, FBH1, 0, 1.01, 0, \
		"bitmaps/buttons/pressoffSg.xpm", "bitmaps/buttons/pressonSg.xpm", BRIGHTON_NOSHADOW}
/*16 = 30 */

/* FX Bus definitions: */
static brightonLocations busses[BUS_COUNT] = {
	/* Bus groups */
	FXBUS(BR1),
	FXBUS(BR2),
	FXBUS(BR3),
	FXBUS(BR4),
	FXBUS(BR5),
	FXBUS(BR6),
	FXBUS(BR7),
	FXBUS(BR8),
/* 40 * 8 = 320 */
#define BSL 299
	{"", 0, BC1 + 40, BR9, BS1, BS2, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, BRIGHTON_NOTCH},
	{"", 1, BC1, BR10, 55, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 1, BC7, BR10, 55, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 0, BC8 + 40, BR9, BS1, BS2, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, BRIGHTON_NOTCH},
	{"", 1, BC8, BR10, 55, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 1, BC9, BR10, 55, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 0, BC10 + 40, BR9, BS1, BS2, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, BRIGHTON_NOTCH},
	{"", 1, BC10, BR10, 55, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 1, BC11, BR10, 55, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 0, BC12 + 40, BR9, BS1, BS2, 0, 1, 0, "bitmaps/knobs/knob2.xpm", 0, BRIGHTON_NOTCH},
	{"", 1, BC12, BR10, 55, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 1, BC13, BR10, 55, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
/* total = 12 + 320 = 332 */
};

#define MC1 150
#define MC2 200
#define MC3 650

#define MS1 240

/* Don't think we pack this?
static brightonLocations master[MASTER_COUNT] = {
	{"", 0, MC2, BR8, 600, 600, 0, 1, 0, 0, 0, 0},
	{"", 1, MC1 + 50, BR10, 130, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 1, MC3, BR10, 130, BSL, 0, 1, 0,
		"bitmaps/knobs/sliderblack2.xpm", 0, 0},
	{"", 0, MC1, BR1, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knob.xpm", 0, 0},
	{"", 0, MC1, BR2, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, MC1, BR3, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, MC1, BR4, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, MC1, BR5, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, MC1, BR6, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, MC1, BR7, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, MC3, BR1, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knob.xpm", 0, 0},
	{"", 0, MC3, BR2, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, MC3, BR3, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, MC3, BR4, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 0, 0},
	{"", 0, MC3, BR5, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, MC3, BR6, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
	{"", 0, MC3, BR7, MS1, MS1, 0, 1, 0, "bitmaps/knobs/knobgreen.xpm", 0, 0},
};
*/

#define FD1 8
#define FD2 67
#define FD3 120

#define FPS1 83
#define FPS2 650
#define FPS3 875

#define FR1 25

#define FR2 (FR1 + FPS1 * 5)
#define FR3 (FR2 + FPS1 - FD1)
#define FR4 (FR3 + FPS1 - FD1)
#define FR5 (FR4 + FPS1 - FD1)
#define FR6 (FR5 + FPS1 - FD1)
#define FR7 (FR6 + FPS1 - FD1)
#define FR8 (FR7 + FPS1 + FD1)

#define FR10 (FR10 + 120)

#define FC1 10

#define FC2 (FC1 + FC1 + FD2)
#define FC3 (FC2 + FD2)
#define FC4 (FC3 + FD2)
#define FC5 (FC4 + FD2)
#define FC6 (FC5 + FD2)
#define FC7 (FC6 + FD2)
#define FC8 (FC7 + FD2)
#define FC9 (FC8 + FD2)
#define FC10 (FC9 + FD2)
#define FC11 (FC10 + FD2)
#define FC12 (FC11 + FD2)
#define FC13 (FC12 + FD2)
#define FC14 (FC13 + FD2)

#define FC15 (FC14 + FD2 - 5)

#define DISPLAY1 (FUNCTION_COUNT - 6)
#define DISPLAY2 (FUNCTION_COUNT - 5)
#define DISPLAY3 (FUNCTION_COUNT - 4)
#define DISPLAY4 (FUNCTION_COUNT - 3)
#define DISPLAY5 (FUNCTION_COUNT - 2)
#define DISPLAY6 (FUNCTION_COUNT - 1)

#define FBBW 47
#define FBBH 70

/*
 * Display and memory select buttons.
 * Should memory selection be panel operation
 */
static brightonLocations functions[FUNCTION_COUNT] = {
	/* Master gain */
	{"", 0, 750, 75, 250, 250, 0, 1, 0, 0, 0, 0},
	/* Page left and right */
	{"", 2, FC1, FR2 - 5, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnlb.xpm", "bitmaps/buttons/touchnlb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC15, FR2 - 5, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnrb.xpm", "bitmaps/buttons/touchnrb.xpm", BRIGHTON_NOSHADOW},
	/* left functions */
	{"", 2, FC1, FR3, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnrb.xpm", "bitmaps/buttons/touchnrb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC1, FR4, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnrb.xpm", "bitmaps/buttons/touchnrb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC1, FR5, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnrb.xpm", "bitmaps/buttons/touchnrb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC1, FR6, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnrb.xpm", "bitmaps/buttons/touchnrb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC1, FR7, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnrb.xpm", "bitmaps/buttons/touchnrb.xpm", BRIGHTON_NOSHADOW},
	/* right functions */
	{"", 2, FC15, FR3, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnlb.xpm", "bitmaps/buttons/touchnlb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC15, FR4, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnlb.xpm", "bitmaps/buttons/touchnlb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC15, FR5, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnlb.xpm", "bitmaps/buttons/touchnlb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC15, FR6, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnlb.xpm", "bitmaps/buttons/touchnlb.xpm", BRIGHTON_NOSHADOW},
	{"", 2, FC15, FR7, FBBW, FBBH, 0, 1.01, 0,
		"bitmaps/buttons/touchnlb.xpm", "bitmaps/buttons/touchnlb.xpm", BRIGHTON_NOSHADOW},
	/*
	 * Some more buttons for base, 0-9, up/down, etc.
	 *
	 * First the load function
	 */
	{"", 2, FC1, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffo.xpm",
			"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	/* 0 to nine */
	{"", 2, FC3, FR8, 40, 75, 0, 1.01, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC4, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC5, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC6, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC7, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC8, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC9, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC10, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC11, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC12, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffg.xpm",
			"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* Memory load/save */
	{"", 2, FC14, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffo.xpm",
			"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, FC15, FR8, 40, 75, 0, 1.01, 0,
		"bitmaps/buttons/pressoffo.xpm",
			"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},

/*	{"", 3, FC1, FR1, FPS2, FPS1 * 5 - 25, 0, 1, 0, */
	{"", 6, FC1 + 25, FR1 + 20, FPS2 - 14, FPS1 * 4 - 26, 0, 1, 0,
		"bitmaps/images/vu.xpm", "bitmaps/images/vumask.xpm", 0},

	{"", 3, FC2 - FD1 * 2, FR2, FPS3, FPS1 - 10, 0, 1, 0, 0, 
		"bitmaps/images/alphadisplay3.xpm", 0},

	{"", 3, FC2 - FD1 * 2, FR3, FPS3, FPS1 - 1, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay4.xpm", 0},
	{"", 3, FC2 - FD1 * 2, FR4, FPS3, FPS1, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay5.xpm", 0},
	{"", 3, FC2 - FD1 * 2, FR5, FPS3, FPS1, 0, 1, 0, 0, 0, 0},
	{"", 3, FC2 - FD1 * 2, FR6, FPS3, FPS1, 0, 1, 0, 0, 0, 0},
	{"", 3, FC2 - FD1 * 2, FR7, FPS3, FPS1, 0, 1, 0, 0, 0, 0},
/*		"bitmaps/images/alphadisplay2.xpm", 0}, */
};

/*
 * Width is a function of track count. With 16 tracks this is about 965, which
 * is equivalent to 16 + 5.5 tracks - this is used to scale to different widths.
 */
#define WIDTH (965 * (CHAN_COUNT + 5.5) / 21.5)
#define PS1 ((int) (4 * 965 / WIDTH))
#define PV1 4

#define PWB ((int) (20 * 965 / WIDTH))

#define PW1 ((int) (((1000 - PWB * 2) / (CHAN_COUNT + 5.5)) - PS1))
#define PW2 ((int) (PW1 + PS1))

#define PBH 760

#define M1CHAN(offset) \
	{\
		"Mix",\
		"bitmaps/blueprints/m1chan.xpm",\
		"bitmaps/textures/metal5.xpm",\
		0,\
		0,\
		0,\
		mixCallback,\
		PWB + PS1 + PW2 * offset, PV1, PW1, 1000 - PV1 * 2,\
		PARAM_COUNT,\
		locations\
	}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp mixApp = {
	"mixer",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH,
	mixInit,
	mixConfigure, /* 3 callbacks, unused? */
	0,
	destroySynth,
	{1, 0, 2, 1, 5, 520, 0, 0},
	WIDTH,
	720, 0, 0,
	CHAN_COUNT + 4, /* panel count == CHAN_COUNT + 4 */
	{
		{
			"Border Left",
			0,
			"bitmaps/textures/wood2.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, PWB, 1000,
			0,
			0
		},
		{
			"Border Right",
			0,
			"bitmaps/textures/wood2.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			1000 - PWB, 0, PWB, 1000,
			0,
			0
		},
		{
			"Function Panel",
			"bitmaps/blueprints/display5.xpm",
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			mixCallback,
				PWB + PS1 + PW2 * CHAN_COUNT,
				PV1, /*PS1, */
				1000 - PWB - PS1 * 2 - PWB - PW2 * CHAN_COUNT,
				1000 - PBH - PV1,
			FUNCTION_COUNT,
			functions
		},
		{
			"Bussing",
			"bitmaps/blueprints/m1bus.xpm",
			"bitmaps/textures/metal5.xpm",
			0, /* flags */
			0,
			0,
			mixCallback,
				PWB + PS1 + PW2 * CHAN_COUNT,
				1000 - PBH + PV1, /*PS1, */
				1000 - PWB - PS1 * 2 - PWB - PW2 * CHAN_COUNT,
				PBH - PV1 * 2,/*1000 - PS1 * 2, */
			BUS_COUNT,
			busses
		},
		M1CHAN(0), M1CHAN(1), M1CHAN(2), M1CHAN(3),
		M1CHAN(4), M1CHAN(5), M1CHAN(6), M1CHAN(7),
		M1CHAN(8), M1CHAN(9), M1CHAN(10), M1CHAN(11),
		M1CHAN(12), M1CHAN(13), M1CHAN(14), M1CHAN(15),
		M1CHAN(16), M1CHAN(17), M1CHAN(18), M1CHAN(19),
		M1CHAN(20), M1CHAN(21), M1CHAN(22), M1CHAN(23),
		M1CHAN(24), M1CHAN(25), M1CHAN(26), M1CHAN(27),
		M1CHAN(28), M1CHAN(29), M1CHAN(30), M1CHAN(31),
		M1CHAN(32), M1CHAN(33), M1CHAN(34), M1CHAN(35),
		M1CHAN(36), M1CHAN(37), M1CHAN(38), M1CHAN(39),
		M1CHAN(40), M1CHAN(41), M1CHAN(42), M1CHAN(43),
		M1CHAN(44), M1CHAN(45), M1CHAN(46), M1CHAN(47),
		M1CHAN(48), M1CHAN(49), M1CHAN(50), M1CHAN(51),
		M1CHAN(52), M1CHAN(53), M1CHAN(54), M1CHAN(55),
		M1CHAN(56), M1CHAN(57), M1CHAN(58), M1CHAN(59),
	}
};

/*static dispatcher dispatch[DEVICE_COUNT];

static void
mixMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
printf("mixMemory()\n");

	switch (c) {
		default:
		case 0:
			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;
			displayPanelText(synth, "PRG", synth->location,
				DISPLAY_PANEL, DISPLAY_DEV);
			break;
		case 1:
			loadMemory(synth, "mix", 0, synth->location,
				synth->mem.active, FIRST_DEV, 0);
			displayPanelText(synth, "PRG", synth->bank + synth->location,
				DISPLAY_PANEL, DISPLAY_DEV);
			break;
		case 2:
			saveMemory(synth, "mix", 0, synth->location, FIRST_DEV);
			displayPanelText(synth, "PRG", synth->bank + synth->location,
				DISPLAY_PANEL, DISPLAY_DEV);
			break;
	}
}

static void
mixMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (c == 1) {
		if (--synth->midichannel < 0)
			synth->midichannel = 0;
	} else {
		if (++synth->midichannel >= 16)
			synth->midichannel = 15;
	}

	bristolMidiSendMsg(global.controlfd, synth->sid,
		127, 0, BRISTOL_MIDICHANNEL|synth->midichannel);

	displayPanelText(synth, "MIDI", synth->midichannel + 1,
		DISPLAY_PANEL, DISPLAY_DEV);
}
*/

/*
 * Having these located here implies only one instance of the mixer. Not sure
 * if that is reasonable, but if we implement multiple then this can be buried
 * in a dispatch structure instead. I have not specified implementation of
 * multiple mixers as this is realtime from the audio device. Multiple mixers
 * would be in separate engines?
 *
 * Oops, should change since it is very non-conducive to memory selections.
 * This is non-trivial, since the GUI does not support radio buttons, and the
 * memory subsystem should not know about them. Consequently they have to be
 * handled here, and it can create issues where the GUI, the mixer and its
 * memory subsystem are not in agreament.
MARK
 */
int currentInSelect[MAX_CHAN_COUNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};
int currentDynSelect[MAX_CHAN_COUNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};
int currentFiltSelect[MAX_CHAN_COUNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};
int currentFXSelect[MAX_CHAN_COUNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};
int currentBusSelect[MAX_CHAN_COUNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};
int currentBusFXSelect[MAX_CHAN_COUNT] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};

/*
 * May pull all this track/bus/function logic into a separate file, along with
 * mixCallback() as access point. From a logical perspective the GUI should
 * feed into the memory structures and to the engine. The memory save should
 * push the memory structures out to storage, and the load routines should 
 * just feed into the GUI. This makes the memory rather bespoke, but there are
 * a few issues that have to be taken care of for the mixer memories that will
 * probably require such coding.
 *
 * Currently busOp is called from mixCallback.
 */
int
busOp(guiSynth *synth, int panel, int track, int operator, float value)
{
	int param = operator % 30;

	if ((synth->flags & OPERATIONAL) == 0)
		return(-1);

/*	printf("busOp(%x, %i, %i, %i, (%i) %f)\n", */
/*		synth, panel, track, operator, param, value); */

	/*
	 * For now just try and get the first 16 devices to operate as radio
	 * buttons.
	 */
	if ((param < 14) && (param > 5))
	{
		/*
		 * See if we need to clear last value. Logic is:
		 *	last value = -1 -> send request for this track.
		 *	last value = other -> clear other bitmap, request this track.
		 *	last value = this -> clear this bitmap, clear track request.
		 */
		if (currentBusFXSelect[track] == -1)
		{
			currentBusFXSelect[track] = operator;
		} else if (currentBusFXSelect[track] == operator) {
			currentBusFXSelect[track] = -1;
		} else {
			brightonEvent event;

			/*
			 * We now have to alter the bitmap for the previous element.
			 * This is a call to libbrighton....
			 * guiSynth has brightonApp *resources:
			 *	brightonResources[] resources = panel.
			 *  resources.callback()
			 *
			 * However, it is wrong that we need to know about all the brighton
			 * internals, so need to add a routine to libbrighton to allow me
			 * to set a value, and have the display reflect that change.
			 */
			event.type = BRIGHTON_FLOAT;
			event.value = 0.0;

			brightonParamChange(synth->win, panel,
				currentBusFXSelect[track], &event);

			currentBusFXSelect[track] = operator;
		}

		return(0);
	}
	return(0);
}

#define DYN_START (CHAN_COUNT + 4)
#define DYN_END (DYN_START + 3)
#define FILT_START (DYN_START + 3)
#define FILT_END (FILT_START + 4)
#define FX_START (FILT_START + 8)
#define FX_END (FX_START + 7)
#define BUS_START (FX_START + 43)
#define BUS_END (BUS_START + 4)

/*
 * May pull all this track/bus/function logic into a separate file, along with
 * mixCallback() as access point. This takes calls from the bussing section and
 * the mixing channels and covers all parameters. It does not receive calls 
 * from the control panel which includes the master volume, and I think I will
 * keep it that way, ie, master volume is a manual configuration?
 *
 * This will put the configured option into its associated structure and send
 * the value through to the engine, and the code will be bespoke for the mixer.
 * Memory load and save will also be bespoke, they will save the values to a
 * disk file and refill them from the file for loading, sending engine updates
 * for changes.
 *
 * trackOp is called from the generic mixcallback.
 */
static void
trackOp(guiSynth *synth, int panel, int track, int operator, float value)
{
	if ((synth->flags & OPERATIONAL) == 0)
		return;

/*	printf("trackOp(%x, %i, %i, %i, %f)\n", */
/*		synth, panel, track, operator, value); */

	/*
	 * For now just try and get the first 16 devices to operate as radio
	 * buttons.
	 */
	if (operator < CHAN_COUNT)
	{
		/*
		 * See if we need to clear last value. Logic is:
		 *	last value = -1 -> send request for this track.
		 *	last value = other -> clear other bitmap, request this track.
		 *	last value = this -> clear this bitmap, clear track request.
		 */
		if (currentInSelect[track] == -1) {
			currentInSelect[track] = operator;
		} else if (currentInSelect[track] == operator) {
			currentInSelect[track] = -1;
		} else {
			brightonEvent event;

			/*
			 * We now have to alter the bitmap for the previous element.
			 * This is a call to libbrighton....
			 * guiSynth has brightonApp *resources:
			 *	brightonResources[] resources = panel.
			 *  resources.callback()
			 *
			 * However, it is wrong that we need to know about all the brighton
			 * internals, so need to add a routine to libbrighton to allow me
			 * to set a value, and have the display reflect that change.
			 */
			event.type = BRIGHTON_FLOAT;
			event.value = 0.0;

			brightonParamChange(synth->win, panel,
				currentInSelect[track], &event);

			currentInSelect[track] = operator;
		}

		return;
	}

	/*
	 * Dynamics radio buttons
	 */
	if ((operator < DYN_END) && (operator >= DYN_START))
	{
		if (currentDynSelect[track] == -1)
		{
			currentDynSelect[track] = operator;
		} else if (currentDynSelect[track] == operator) {
			currentDynSelect[track] = -1;
		} else {
			brightonEvent event;

			/*
			 * We now have to alter the bitmap for the previous element.
			 * This is a call to libbrighton....
			 * guiSynth has brightonApp *resources:
			 *	brightonResources[] resources = panel.
			 *  resources.callback()
			 *
			 * However, it is wrong that we need to know about all the brighton
			 * internals, so need to add a routine to libbrighton to allow me
			 * to set a value, and have the display reflect that change.
			 */
			event.type = BRIGHTON_FLOAT;
			event.value = 0.0;

			brightonParamChange(synth->win, panel,
				currentDynSelect[track], &event);

			currentDynSelect[track] = operator;
		}

		return;
	}
	/*
	 * Filter select radio buttons
	 */
	if ((operator < FILT_END) && (operator >= FILT_START))
	{
		if (currentFiltSelect[track] == -1)
		{
			currentFiltSelect[track] = operator;
		} else if (currentFiltSelect[track] == operator) {
			currentFiltSelect[track] = -1;
		} else {
			brightonEvent event;

			/*
			 * We now have to alter the bitmap for the previous element.
			 * This is a call to libbrighton....
			 * guiSynth has brightonApp *resources:
			 *	brightonResources[] resources = panel.
			 *  resources.callback()
			 *
			 * However, it is wrong that we need to know about all the brighton
			 * internals, so need to add a routine to libbrighton to allow me
			 * to set a value, and have the display reflect that change.
			 */
			event.type = BRIGHTON_FLOAT;
			event.value = 0.0;

			brightonParamChange(synth->win, panel,
				currentFiltSelect[track], &event);

			currentFiltSelect[track] = operator;
		}

		return;
	}
	/*
	 * FX radio buttons
	 */
	if ((operator < FX_END) && (operator >= FX_START))
	{
		if (currentFXSelect[track] == -1)
		{
			currentFXSelect[track] = operator;
		} else if (currentFXSelect[track] == operator) {
			currentFXSelect[track] = -1;
		} else {
			brightonEvent event;

			/*
			 * We now have to alter the bitmap for the previous element.
			 * This is a call to libbrighton....
			 * guiSynth has brightonApp *resources:
			 *	brightonResources[] resources = panel.
			 *  resources.callback()
			 *
			 * However, it is wrong that we need to know about all the brighton
			 * internals, so need to add a routine to libbrighton to allow me
			 * to set a value, and have the display reflect that change.
			 */
			event.type = BRIGHTON_FLOAT;
			event.value = 0.0;

			brightonParamChange(synth->win, panel,
				currentFXSelect[track], &event);

			currentFXSelect[track] = operator;
		}
		return;
	}
	/*
	 * Bus select radio buttons
	 */
	if ((operator < BUS_END) && (operator >= BUS_START))
	{
		if (currentBusSelect[track] == -1)
		{
			currentBusSelect[track] = operator;
		} else if (currentBusSelect[track] == operator) {
			currentBusSelect[track] = -1;
		} else {
			brightonEvent event;

			/*
			 * We now have to alter the bitmap for the previous element.
			 * This is a call to libbrighton....
			 * guiSynth has brightonApp *resources:
			 *	brightonResources[] resources = panel.
			 *  resources.callback()
			 *
			 * However, it is wrong that we need to know about all the brighton
			 * internals, so need to add a routine to libbrighton to allow me
			 * to set a value, and have the display reflect that change.
			 */
			event.type = BRIGHTON_FLOAT;
			event.value = 0.0;

			brightonParamChange(synth->win, panel,
				currentBusSelect[track], &event);

			currentBusSelect[track] = operator;
		}
		return;
	}
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.

 * The memory structure is going to be large. To cater for future extensions
 * each memory component (track, bus, etc) is going to have extra memory 
 * allocated. New parameters will occupy the extra space allowing for 
 * compatability of memories over releases.
 */
static int
mixCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (synth == 0)
		return(0);

/* printf("1: mixCallback(%i, %i, %f): %x\n", panel, index, value, synth); */

	if (mixApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	switch (panel) {
		case FUNCTION_PANEL:
			functionOp(synth, panel, index, index - 1, value);

			break;
		case BUS_PANEL:
			if (index < 240) {
				int chan, op;

				chan = index / 30;
				op = index % 30;

				busOp(synth, panel, chan, index, value);
				mixerMemory(synth, MM_GET, BUS_PANEL, index, &value);

				mixSendMsg(synth, global.controlfd, synth->sid,
					64 + chan, op, sendvalue);
			} else {
				/* WTF is VBUS_PANEL?  not an op... */
				mixerMemory(synth, MM_GET, VBUS_PANEL, index - 240, &value);

				mixSendMsg(synth, global.controlfd, synth->sid,
					80, index - 240, sendvalue);
			}
/*			index += panel * PARAM_COUNT; */
			/*
			 * Corrected to:
			 */
			index += FUNCTION_COUNT;

			break;
		default:
			/*
			 * Here I want to call a track function, it will be given the 
			 * track number, parameter and value, and will set that value in
			 * an internal memory space. For radio buttons it may also need
			 * to send requests to the GUI to alter the display.
			 */
			trackOp(synth, panel, panel - 4, index, value);
			setMixerMemory((mixerMem *) synth->mem.param, panel, index, &value, NULL);

			mixSendMsg(synth,
				global.controlfd, synth->sid, panel - 4, index, sendvalue);

			break;
	}

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

/* printf("2: mixCallback(%i, %f): %x\n", index, value, synth); */

	/*
	 * Parameters are now handled through brightonMixerMemory.c
	if (debug) printf("p1 %x %i %i\n", synth->dispatch[index].routine,
		synth->sid,
		synth->dispatch[index].controller,
		synth->dispatch[index].operator);

	synth->dispatch[index].routine(synth,
		global.controlfd, synth->sid,
		synth->dispatch[index].controller,
		synth->dispatch[index].operator,
		sendvalue);

#ifdef DEBUG
	printf("dispatch[%x,%i](%i, %i, %i, %i, %i)\n", synth, index,
		global.controlfd, synth->sid,
		synth->dispatch[index].controller,
		synth->dispatch[index].operator,
		sendvalue);
#endif
	 */

	return(0);
}

static void
mixSendMsg(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
/*printf("mixSendMsg(%i, %i, %i, %i, %f\n", fd, chan, c, o, v); */
/*	int value; */
/*	 */
/*	value = v < 1.0? v * 16383: v; */
	bristolMidiSendMsg(fd, chan, c, o, v);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 *
 * For the mixer it is also reasonable to build the application structures, 
 * rather than have them defined in advance?
 */
static int
mixInit(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
/*	dispatcher *dispatch; */
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

	printf("Initialise the mix link to bristol: %p\n", synth->win);

/* Alloc device_count. This is wrong, should be 'abs max'. */
	synth->mem.param = (float *) initMixerMemory(16);
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
/*	synth->dispatch = (dispatcher *) */
/*		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher)); */
/*	dispatch = synth->dispatch; */

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
/*		synth->dispatch[i].routine = (synthRoutine) mixSendMsg; */

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
mixConfigure(brightonWindow *win)
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
	synth->keypanel = -1;
	synth->keypanel2 = -1;

	functionOp(synth, FUNCTION_PANEL, 1, 1, 0.0);

	displayPanelText(synth, "TRK", 1, 4, PARAM_COUNT - 1);
	displayPanelText(synth, "TRK", 2, 5, PARAM_COUNT - 1);
	displayPanelText(synth, "TRK", 3, 6, PARAM_COUNT - 1);
	displayPanelText(synth, "TRK", 4, 7, PARAM_COUNT - 1);
	displayPanelText(synth, "TRK", 5, 8, PARAM_COUNT - 1);
	displayPanelText(synth, "TRK", 6, 9, PARAM_COUNT - 1);
	displayPanelText(synth, "TRK", 7, 10, PARAM_COUNT - 1);
	displayPanelText(synth, "TRK", 8, 11, PARAM_COUNT - 1);

	if (CHAN_COUNT > 7)
	{
		displayPanelText(synth, "TRK", 9, 12, PARAM_COUNT - 1);
		displayPanelText(synth, "TRK", 10, 13, PARAM_COUNT - 1);
		displayPanelText(synth, "TRK", 11, 14, PARAM_COUNT - 1);
		displayPanelText(synth, "TRK", 12, 15, PARAM_COUNT - 1);
		displayPanelText(synth, "TRK", 13, 16, PARAM_COUNT - 1);
		displayPanelText(synth, "TRK", 14, 17, PARAM_COUNT - 1);
		displayPanelText(synth, "TRK", 15, 18, PARAM_COUNT - 1);
		displayPanelText(synth, "TRK", 16, 19, PARAM_COUNT - 1);
	}

	loadMixerMemory(synth, "default", 0);

	event.type = BRIGHTON_FLOAT;
	event.value = 0.5;
	brightonParamChange(synth->win, 2, 0, &event);

	return(0);
}

