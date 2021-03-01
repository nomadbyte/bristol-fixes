
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
 * Variables:
 *	Some LFO
 *	Some ADSR (7 stage?)
 *	Osc: F/V/S, G/V/S, D/V/S, Waveforms.
 */


#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int granularInit();
static int granularConfigure();
static int granularCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;
static int dc1, dc2;

#include "brightonKeys.h"

#define MOD_PANEL 0
#define KEY_PANEL 1
#define MEM_PANEL 2

#define MOD_COUNT 150
#define MEM_COUNT 21

#define MOD_START 0
#define MEM_START (MOD_COUNT - MEM_COUNT)

#define ACTIVE_DEVS (MOD_COUNT - MEM_COUNT)
#define DEVICE_COUNT MOD_COUNT

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
 * This example is for a granularBristol type synth interface.
 */

#define SBXS 15
#define SBYS 50
#define SBXD 15
#define SBYD 50

#define SELECTBUS(x, y) \
	{"", 2, x + 0 * SBXD, y + 0 * SBYD, SBXS, SBYS, 0, 1.01, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 0 * SBXD, y + 1 * SBYD, SBXS, SBYS, 0, 4, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 0 * SBXD, y + 2 * SBYD, SBXS, SBYS, 0, 7, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 1 * SBXD, y + 0 * SBYD, SBXS, SBYS, 0, 2, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 1 * SBXD, y + 1 * SBYD, SBXS, SBYS, 0, 5, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 1 * SBXD, y + 2 * SBYD, SBXS, SBYS, 0, 8, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 2 * SBXD, y + 0 * SBYD, SBXS, SBYS, 0, 3, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 2 * SBXD, y + 1 * SBYD, SBXS, SBYS, 0, 6, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 2 * SBXD, y + 2 * SBYD, SBXS, SBYS, 0, 9, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}

#define MRXS 15
#define MRYS 50
#define MRXD 15
#define MRYD 54

#define MODROUTING(x, y) \
	{"", 2, x + 0 * MRXD, y + 0 * MRYD, MRXS, MRYS, 0, 1.01, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 0 * MRXD, y + 1 * MRYD, MRXS, MRYS, 0, 4, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 1 * MRXD, y + 0 * MRYD, MRXS, MRYS, 0, 7, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 1 * MRXD, y + 1 * MRYD, MRXS, MRYS, 0, 2, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 2 * MRXD, y + 0 * MRYD, MRXS, MRYS, 0, 5, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 2 * MRXD, y + 1 * MRYD, MRXS, MRYS, 0, 8, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 3 * MRXD, y + 0 * MRYD, MRXS, MRYS, 0, 3, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, x + 3 * MRXD, y + 1 * MRYD, MRXS, MRYS, 0, 6, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0}, \
	{"", 2, 0 + 2 * MRXD, 0 + 2 * MRYD, MRXS, MRYS, 0, 9, 0, \
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", \
		BRIGHTON_WITHDRAWN}

#define S1 200

#define D1 65

#define oW1 120
#define oL1 120

#define oC1 39

#define oC2 180
#define oC3 (oC2 + D1)
#define oC4 (oC3 + D1)
#define oC5 (oC4 + D1)
#define oC6 (oC5 + D1)
#define oC62 (oC6 + D1 - 5)

#define oC7 600
#define oC8 (oC7 + D1)
#define oC9 (oC8 + D1)
#define oC10 (oC9 + D1)

#define oC15 900
#define oC16 950

#define oR1 350
#define oR2 550
#define oR3 750

#define oR4 400
#define oR6 600

static brightonLocations options[MOD_COUNT] = {
	/* Env ADSRG 0 */
	{"", 0, oC2,  oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC3,  oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC4,  oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC5,  oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC6,  oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},

	/* Second env 5 */
	{"", 0, oC2,  oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC3,  oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC4,  oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC5,  oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC6,  oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},

	/* Third Env 10 */
	{"", 0, oC2, oR3, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC3, oR3, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC4, oR3, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC5, oR3, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC6, oR3, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},

	/* Osc Depth 15 */
	{"", 0, oC7,  oR1, oW1, oL1, 0, 1, 0,  "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC8,  oR1, oW1, oL1, 0, 1, 0,  "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, oC9,  oR1, oW1, oL1, 0, 1, 0,  "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC10,  oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	/* Osc Scatter */
	{"", 0, oC7,  oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC8,  oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC9,  oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC10, oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	/* Osc params, Fine tune, glide 23/24/25 */
	{"", 0, oC7,  oR3, oW1, oL1, 0, 9, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC8,  oR3, oW1, oL1, 0, 8, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, oC9,  oR3, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},

	MODROUTING(oC62, oR1 + 12), /* 26 - Env */
	MODROUTING(oC62, oR2 + 12), /* 35 - Env */
	MODROUTING(oC62, oR3 + 12), /* 44 - Env */
	SELECTBUS(oC10 - 2, oR3), /* 53 */

	/* LFO - 62 */
	{"", 0, oC1,  oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	MODROUTING(85, oR1 + 12),
	/* Noise 72 */
	{"", 0, oC1, oR2, oW1, oL1, 0, 1, 0, "bitmaps/knobs/grotary.xpm", "bitmaps/knobs/alpharotary.xpm", 0},
	MODROUTING(85, oR2 + 12),

	/* Dummies for later implementation */
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},
	{"", 0, 0, 0, 25, 25, 0, 1, 0, "bitmaps/knobs/grotary.xpm",
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN},

	/* Memories */
	SELECTBUS(900, oR1),
	SELECTBUS(900, oR2),

	/* Midi up/down, save */
	{"", 2, 900, oR3, MRXS, MRYS, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 900 + MRXD, oR3, MRXS, MRYS, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 900 + 2 * MRXD, oR3, MRXS, MRYS, 0, 1, 0,
		"bitmaps/buttons/pressoffo.xpm", 
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
};

#define mR1 200
#define mR2 450
#define mR3 700

#define mC1 100
#define mC2 283
#define mC3 466
#define mC4 649
#define mC5 835

#define mC11 100
#define mC12 255
#define mC13 410
#define mC14 565
#define mC15 720
#define mC16 874

#define S3 100
#define S4 80
#define S5 120
#define S6 150

/*
 * Should try and make this one as generic as possible, and try to use it as
 * a general memory routine. has Midi u/d, mem u/d, load/save and a display.
 */
static int
memCallback(brightonWindow* win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (synth->flags & SUPPRESS)
		return(0);

	/*
	 * The first ten buttons are exclusive highlighting, we use the first mem
	 * pointer to handle this.
	 */
	if (synth->dispatch[MEM_START].other2)
	{
		synth->dispatch[MEM_START].other2 = 0;
		return(0);
	}
	if (synth->dispatch[MEM_START + 1].other2)
	{
		synth->dispatch[MEM_START + 1].other2 = 0;
		return(0);
	}

printf("memCallback(%i, %i, %f) %i, %s\n", panel, index, value,
synth->mem.active, synth->resources->name);

	if (index == (DEVICE_COUNT - 1))
	{
		if (brightonDoubleClick(dc1)) {
			saveMemory(synth, "granular", 0, synth->bank * 10
				+ synth->location, 0);
		}
		return(0);
	}

	/* First bank selections */
	if (index < (MEM_START + 9))
	{
		brightonEvent event;

		event.command = BRIGHTON_PARAMCHANGE;
		event.type = BRIGHTON_FLOAT;
		event.value = 0;

		/*
		 * This is a numeric. We need to force exclusion.
		 */
		if (synth->dispatch[MEM_START].other1 != -1)
		{
			synth->dispatch[MEM_START].other2 = 1;

			if (synth->dispatch[MEM_START].other1 != index)
				event.value = 0;
			else
				event.value = 1;

			brightonParamChange(synth->win, panel,
				synth->dispatch[MEM_START].other1, &event);
		}

		synth->bank = index - MEM_START;

		if (brightonDoubleClick(dc2)) {
printf("bank %i, mem %i\n", synth->bank, synth->location);
			loadMemory(synth, "granular", 0, synth->bank * 10 + synth->location,
				synth->mem.active, 0, BRISTOL_FORCE);
		}

		synth->dispatch[MEM_START].other1 = index;
	} else if (index < (MEM_START + 18)) {
		brightonEvent event;

		event.command = BRIGHTON_PARAMCHANGE;
		event.type = BRIGHTON_FLOAT;
		event.value = 0;

		/*
		 * This is a numeric. We need to force exclusion.
		 */
		if (synth->dispatch[MEM_START + 1].other1 != -1)
		{
			synth->dispatch[MEM_START + 1].other2 = 1;

			if (synth->dispatch[MEM_START + 1].other1 != index)
				event.value = 0;
			else
				event.value = 1;

			brightonParamChange(synth->win, panel,
				synth->dispatch[MEM_START + 1].other1, &event);
		}

		synth->location = index - MEM_START - 9;

		if (brightonDoubleClick(dc2)) {
printf("bank %i, mem %i\n", synth->bank, synth->location);
			loadMemory(synth, "granular", 0, synth->bank * 10 + synth->location,
				synth->mem.active, 0, BRISTOL_FORCE);
		}

		synth->dispatch[MEM_START + 1].other1 = index;
	} else {
		int newchan;

		/*
		 * This is a control button.
		 */
		switch(index) {
			case MOD_COUNT - 3:
				/*
				 * Midi Down
				 */
				if ((newchan = synth->midichannel - 1) < 0)
				{
					synth->midichannel = 0;
					return(0);
				}

				if (global.libtest)
				{
					printf("midi chan %i\n", newchan);
					synth->midichannel = newchan;
					return(0);
				}

				bristolMidiSendMsg(global.controlfd, synth->sid,
					127, 0, BRISTOL_MIDICHANNEL|newchan);

				synth->midichannel = newchan;
				break;
			case MOD_COUNT - 2:
				/*
				 * Midi Up
				 */
				if ((newchan = synth->midichannel + 1) > 15)
				{
					synth->midichannel = 15;
					return(0);
				}

				if (global.libtest)
				{
					printf("midi chan %i\n", newchan);
					synth->midichannel = newchan;
					return(0);
				}

				bristolMidiSendMsg(global.controlfd, synth->sid,
					127, 0, BRISTOL_MIDICHANNEL|newchan);

				synth->midichannel = newchan;
				break;
		}
	}

	return(0);
}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp granularApp = {
	"granular",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/granular.xpm",
	BRIGHTON_STRETCH,
	granularInit,
	granularConfigure, /* 3 callbacks */
	midiCallback,
	destroySynth,
	{16, 0, 2, 2, 5, 520, 0, 0},
	740, 400, 0, 0,
	4, /* panel count */
	{
		{
			"Mods",
			0,
			0, /* flags */
			0,
			0,
			0,
			granularCallback,
			//30, 50, 940, 365,
			5, 5, 990, 560,
			MOD_COUNT,
			options
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			100, 710, 900, 250,
			KEY_COUNT,
			keysprofile
		},
		{
			"Mods",
			"bitmaps/blueprints/mods.xpm",
			0, /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			modCallback,
			0, 710, 100, 250,
			2,
			mods
		},
	}
};

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, synth->resources->name, 0,
				synth->bank + synth->location, synth->mem.active, 0, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
granularMods(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	unsigned int i, flags = 0;

	for (i = 0; i < 9; i++)
	{
		if (synth->mem.param[o + i] != 0)
			flags |= (0x01 << i);
	}

	if (c == 15)
		bristolMidiSendMsg(fd, chan, 0, 10, flags);
	else
		bristolMidiSendMsg(fd, chan, 126, c, flags);

	return(0);
}

/*
static int
granularOscWaveform(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (synth->dispatch[53].other2)
	{
		synth->dispatch[53].other2 = 0;
		return(0);
	}

	if (synth->dispatch[53].other1 != -1)
	{
		event.command = BRIGHTON_PARAMCHANGE;
		event.type = BRIGHTON_FLOAT;

		synth->dispatch[53].other2 = o;

		if (synth->dispatch[53].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, 0, synth->dispatch[53].other1, &event);
	}

	if (v != 0)
		bristolMidiSendMsg(fd, chan, 0, 10, v - 1);

	synth->dispatch[53].other1 = o;

	return(0);
}
*/

static int
granularMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
/*	printf("%i, %i, %i\n", c, o, v); */
	bristolMidiSendMsg(fd, chan, c, o, v);
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
granularCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (granularApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	if (index >= MEM_START)
		return(memCallback(win, panel, index, value));

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
granularInit(brightonWindow* win)
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

	printf("Initialise the granular link to bristol: %p\n", synth->win);

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
	{
		synth->dispatch[i].controller = 88;
		synth->dispatch[i].routine = granularMidiSendMsg;
	}

	synth->dispatch[MOD_START + 0].controller = 1; /* Env */
	synth->dispatch[MOD_START + 0].operator = 0;
	synth->dispatch[MOD_START + 1].controller = 1; /* Env */
	synth->dispatch[MOD_START + 1].operator = 1;
	synth->dispatch[MOD_START + 2].controller = 1; /* Env */
	synth->dispatch[MOD_START + 2].operator = 2;
	synth->dispatch[MOD_START + 3].controller = 1; /* Env */
	synth->dispatch[MOD_START + 3].operator = 3;
	synth->dispatch[MOD_START + 4].controller = 1; /* Env */
	synth->dispatch[MOD_START + 4].operator = 4;

	synth->dispatch[MOD_START + 5].controller = 2; /* Env2 */
	synth->dispatch[MOD_START + 5].operator = 0;
	synth->dispatch[MOD_START + 6].controller = 2; /* Env2 */
	synth->dispatch[MOD_START + 6].operator = 1;
	synth->dispatch[MOD_START + 7].controller = 2; /* Env2 */
	synth->dispatch[MOD_START + 7].operator = 2;
	synth->dispatch[MOD_START + 8].controller = 2; /* Env2 */
	synth->dispatch[MOD_START + 8].operator = 3;
	synth->dispatch[MOD_START + 9].controller = 2; /* Env2 */
	synth->dispatch[MOD_START + 9].operator = 4;

	synth->dispatch[MOD_START + 10].controller = 3; /* Env3 */
	synth->dispatch[MOD_START + 10].operator = 0;
	synth->dispatch[MOD_START + 11].controller = 3; /* Env3 */
	synth->dispatch[MOD_START + 11].operator = 1;
	synth->dispatch[MOD_START + 12].controller = 3; /* Env3 */
	synth->dispatch[MOD_START + 12].operator = 2;
	synth->dispatch[MOD_START + 13].controller = 3; /* Env3 */
	synth->dispatch[MOD_START + 13].operator = 3;
	synth->dispatch[MOD_START + 14].controller = 3; /* Env3 */
	synth->dispatch[MOD_START + 14].operator = 4;

	synth->dispatch[MOD_START + 15].controller = 0; /* Gain */
	synth->dispatch[MOD_START + 15].operator = 0;
	synth->dispatch[MOD_START + 16].controller = 0; /* Frequency */
	synth->dispatch[MOD_START + 16].operator = 2;
	synth->dispatch[MOD_START + 17].controller = 0; /* Delay */
	synth->dispatch[MOD_START + 17].operator = 4;
	synth->dispatch[MOD_START + 18].controller = 0; /* Duration */
	synth->dispatch[MOD_START + 18].operator = 6;
	synth->dispatch[MOD_START + 19].controller = 0; /* Gain Scatter */
	synth->dispatch[MOD_START + 19].operator = 1;
	synth->dispatch[MOD_START + 20].controller = 0; /* Frequency scatter */
	synth->dispatch[MOD_START + 20].operator = 3;
	synth->dispatch[MOD_START + 21].controller = 0; /* Delay scatter */
	synth->dispatch[MOD_START + 21].operator = 5;
	synth->dispatch[MOD_START + 22].controller = 0; /* Duration scatter */
	synth->dispatch[MOD_START + 22].operator = 7;
	/* Transpose and graincount */
	synth->dispatch[MOD_START + 23].controller = 0; /* Osc */
	synth->dispatch[MOD_START + 23].operator = 8;
	synth->dispatch[MOD_START + 24].controller = 0; /* Osc */
	synth->dispatch[MOD_START + 24].operator = 9;
	/* Glide */
	synth->dispatch[MOD_START + 25].controller = 126; /* Osc */
	synth->dispatch[MOD_START + 25].operator = 0;

	/* Env routing */
	for (i = 0; i < 9; i++)
	{
		synth->dispatch[MOD_START + 26 + i].controller = 10; /* Mod */
		synth->dispatch[MOD_START + 26 + i].operator = 26;
		synth->dispatch[MOD_START + 26 + i].routine =
			(synthRoutine) granularMods;
	}
	for (i = 0; i < 9; i++)
	{
		synth->dispatch[MOD_START + 35 + i].controller = 11; /* Mod */
		synth->dispatch[MOD_START + 35 + i].operator = 35;
		synth->dispatch[MOD_START + 35 + i].routine =
			(synthRoutine) granularMods;
	}
	for (i = 0; i < 9; i++)
	{
		synth->dispatch[MOD_START + 44 + i].controller = 12; /* Mod */
		synth->dispatch[MOD_START + 44 + i].operator = 44;
		synth->dispatch[MOD_START + 44 + i].routine =
			(synthRoutine) granularMods;
	}

	/* Waveform
	synth->dispatch[MOD_START + 53].controller =
		synth->dispatch[MOD_START + 54].controller =
		synth->dispatch[MOD_START + 55].controller =
		synth->dispatch[MOD_START + 56].controller =
		synth->dispatch[MOD_START + 57].controller =
		synth->dispatch[MOD_START + 58].controller =
		synth->dispatch[MOD_START + 59].controller =
		synth->dispatch[MOD_START + 60].controller =
		synth->dispatch[MOD_START + 61].controller = 0;
	synth->dispatch[MOD_START + 53].operator = 53;
	synth->dispatch[MOD_START + 54].operator = 54;
	synth->dispatch[MOD_START + 55].operator = 55;
	synth->dispatch[MOD_START + 56].operator = 56;
	synth->dispatch[MOD_START + 57].operator = 57;
	synth->dispatch[MOD_START + 58].operator = 58;
	synth->dispatch[MOD_START + 59].operator = 59;
	synth->dispatch[MOD_START + 60].operator = 60;
	synth->dispatch[MOD_START + 61].operator = 61;
	synth->dispatch[MOD_START + 53].routine =
		synth->dispatch[MOD_START + 54].routine =
		synth->dispatch[MOD_START + 55].routine =
		synth->dispatch[MOD_START + 56].routine =
		synth->dispatch[MOD_START + 57].routine =
		synth->dispatch[MOD_START + 58].routine =
		synth->dispatch[MOD_START + 59].routine =
		synth->dispatch[MOD_START + 60].routine =
		synth->dispatch[MOD_START + 61].routine =
			(synthRoutine) granularOscWaveform;
	*/
	for (i = 0; i < 9; i++)
	{
		synth->dispatch[MOD_START + 53 + i].controller = 15; /* Mod */
		synth->dispatch[MOD_START + 53 + i].operator = 53;
		synth->dispatch[MOD_START + 53 + i].routine =
			(synthRoutine) granularMods;
	}


	synth->dispatch[MOD_START + 62].controller = 4; /* LFO */
	synth->dispatch[MOD_START + 62].operator = 0;
	for (i = 0; i < 9; i++)
	{
		synth->dispatch[MOD_START + 63 + i].controller = 13; /* LFO */
		synth->dispatch[MOD_START + 63 + i].operator = 63;
		synth->dispatch[MOD_START + 63 + i].routine =
			(synthRoutine) granularMods;
	}

	synth->dispatch[MOD_START + 72].controller = 5; /* Noise */
	synth->dispatch[MOD_START + 72].operator = 0;
	for (i = 0; i < 9; i++)
	{
		synth->dispatch[MOD_START + 73 + i].controller = 14; /* NOISE */
		synth->dispatch[MOD_START + 73 + i].operator = 73;
		synth->dispatch[MOD_START + 73 + i].routine =
			(synthRoutine) granularMods;
	}

	/* 
	 * These will be replaced by some opts controllers. We need to tie the
	 * envelope parameters for decay, sustain. We need to fix a few parameters
	 * of the oscillators too - transpose, tune and gain.
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, 10);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 16382);
	 */

	/* Velocity tracking to zero on second envelope */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 5, 1);
	/* Enable key tracking and multi LFO with sync to keyOn */
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 3, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 4, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 1, 1);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
granularConfigure(brightonWindow *win)
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
	synth->keypanel = KEY_PANEL;
	synth->keypanel2 = -1;
	synth->transpose = 36;
	loadMemory(synth, "granular", 0, synth->location, synth->mem.active, 0, 0);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	brightonPut(win,
		"bitmaps/blueprints/granular.xpm", 0, 0, win->width, win->height);
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	event.command = BRIGHTON_PARAMCHANGE;
	event.type = BRIGHTON_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, 0, MEM_START, &event);
	brightonParamChange(synth->win, 0, MEM_START + 9, &event);

	dc1 = brightonGetDCTimer(win->dcTimeout);
	dc2 = brightonGetDCTimer(win->dcTimeout);

	return(0);
}

