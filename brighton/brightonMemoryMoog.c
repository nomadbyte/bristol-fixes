
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

/* Korg MemoryMoog UI */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int memMoogInit();
static int memMoogConfigure();
static int memMoogCallback(brightonWindow *, int, int, float);
static int memMoogModCallback(brightonWindow *, int, int, float);
static int MMmidiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define DEVICE_COUNT 113
#define ACTIVE_DEVS 91
#define DISPLAY (DEVICE_COUNT - 1)

#define MEM_START (ACTIVE_DEVS + 4)
#define MIDI_START (MEM_START + 14)

#define KEY_PANEL 1

#define OSC_1_RADIO 0
#define OSC_2_RADIO 1
#define OSC_3_RADIO 2

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
 * This example is for a memMoogBristol type synth interface.
 */

#define RD1 210
#define C4D0 30
#define C4D1 27
#define C4D2 44
#define C4D3 25

#define R0 140
#define R1 (R0 + RD1)
#define R2 (R1 + RD1)
#define R3 (R2 + RD1)

#define R1_1 (R0 + (R3 - R0) / 2)
#define R1_0 (R0 + (R1_1 - R0) / 2)
#define R1_2 (R3 - (R1_1 - R0) / 2)

#define C10 35
#define C11 (C10 + C4D0)

#define C20 (C11 + C4D1 + 10)
#define C21 (C20 + C4D1)
#define C22 (C21 + C4D1)
#define C23 (C22 + C4D1)

#define R21_0 (R1_1 + (R3 - R1_1) / 3)
#define R21_1 (R1_1 + (R3 - R1_1) * 2 / 3)

#define C30 (C23 + C4D1 + 10)

#define C40 (C30 + C4D1 + 30)
#define C41 (C40 + C4D3)
#define C42 (C41 + C4D3)
#define C43 (C42 + C4D3)
#define C44 (C43 + C4D3)
#define C45 (C44 + C4D3)
#define C46 (C45 + C4D3)

#define C50 (C46 + C4D3 + 10)
#define C51 (C50 + C4D3)
#define C52 (C51 + C4D3)
#define C53 (C52 + C4D3)
#define C54 (C53 + C4D3 + 10)
#define C55 (C54 + C4D3 - 10)
#define C56 (C55 + C4D3 - 10)
#define C57 (C56 + C4D3 + 15)
#define C58 (C57 + C4D3)
#define C59 (C58 + C4D3)

#define C60 (C59 + C4D1 + 5)

#define C70 (C60 + C4D2 + 15)
#define C71 (C70 + C4D2)
#define C72 (C71 + C4D2)
#define C73 (C72 + C4D2)

#define C80 (C73 + C4D2)

#define S1 90
#define S2 14
#define S3 40
#define S4 100

static brightonLocations locations[DEVICE_COUNT] = {
	/* Oscillators - 0 */
	{"Osc1-16", 2, C50, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc1-8", 2, C51, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc1-4", 2, C52, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc1-2", 2, C53, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc1-Sync", 2, C54, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc1-PW", 0, C56, R0, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Osc1-Square", 2, C57, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc1-Ramp", 2, C58, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc1-Tri", 2, C59, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	/* Osc-2 9 */
	{"Osc2-16", 2, C50, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc2-8", 2, C51, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc2-4", 2, C52, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc2-2", 2, C53, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc2-Tuning", 0, C54 - 5, R1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, BRIGHTON_NOTCH},
	{"Osc2-PW", 0, C56, R1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Osc2-Square", 2, C57, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc2-Ramp", 2, C58, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc2-Tri", 2, C59, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	/* Osc-3 - 18 */
	{"Osc3-16", 2, C50, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc3-8", 2, C51, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc3-4", 2, C52, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc3-2", 2, C53, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc3-Tuning", 0, C54 - 5, R2, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, BRIGHTON_NOTCH},
	{"Osc3-PW", 0, C56, R2, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Osc3-Square", 2, C57, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc3-Ramp", 2, C58, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc3-Tri", 2, C59, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	/* Osc-3 options - 27 */
	{"Osc3-LFO", 2, C51, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Osc3-KBD", 2, C53, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	/* Mixer - 29 */
	{"Mix-Osc1", 0, C60, R0, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Mix-Osc2", 0, C60, R1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Mix-Osc3", 0, C60, R2, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Mix-Noise", 0, C60, R3, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* Filter/Env - 33 */
	{"VCF-Cutoff", 0, C71, R0, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Resonance", 0, C72, R0, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Env", 0, C73, R0, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Attack", 0, C70, R1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Decay", 0, C71, R1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Sustain", 0, C72, R1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-Release", 0, C73, R1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCF-KBD-1/3", 2, C70 - 5, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"VCF-KBD-2/3", 2, C70 + 15, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	/* Env - 42 */
	{"VCA-Attack", 0, C70, R3, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCA-Decay", 0, C71, R3, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCA-Sustain", 0, C72, R3, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"VCA-Release", 0, C73, R3, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Env-ReZero", 2, C70, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Env-Conditional", 2, C71, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Env-KBD", 2, C72, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Env-Release", 2, C73, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	/* Outputs - 50 */
	{"ProgVolume", 0, C80, R1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Chorus-Depth", 0, C80, R3, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* Mods - 52 */
	{"LFO-Freq", 0, C40 + 10, R0, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"LFO-Tri", 2, C42, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"LFO-Saw", 2, C43, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"LFO-Ramp", 2, C44, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"LFO-Square", 2, C45, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"LFO-S/H", 2, C46, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	{"Mod-FM1", 2, C40, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Mod-FM2", 2, C41, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Mod-FM3", 2, C42, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Mod-PW1", 2, C43, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Mod-PW2", 2, C44, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Mod-PW3", 2, C45, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Mod-Filter", 2, C46, R1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchog.xpm",
		"bitmaps/buttons/touchg.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	{"PolyMod-Osc3", 0, C40 + 10, R2, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"PolyMod-Env", 0, C41 + 30, R2, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"PolyMod-Cont", 2, C44, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"PolyMod-Inv", 2, C46, R2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	{"PolyMod-FM1", 2, C41, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"PolyMod-FM2", 2, C42, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"PolyMod-PM1", 2, C43, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"PolyMod-PM2", 2, C44, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"PolyMod-Filt", 2, C45, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	/* Pedals - 74 */
	{"Pedal1-AMT", 0, C30, R0 + 45, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Pedal1-Pitch", 2, C30 - 7, R1_0 - 20, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Pedal1-Filt", 2, C30 + 13, R1_0 - 20, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Pedal1-Vol", 2, C30 + 3, R1_1 - 30, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Pedal2-AMT", 0, C30, R1_2 + 60, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Pedal2-Mod", 2, C30 - 7, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Pedal2-Osc2", 2, C30 + 13, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},

	/* Main - 81 */
	{"G-Mono", 2, C10 - 10, R1_0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"G-Hold", 2, C10 + 10, R1_0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"G-Multi", 2, C11, R1_0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Glide", 0, C10 - 10, R1_1, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"Glide-On", 2, C11, R1_1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Transpose1", 2, C10 - 10, R1_2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Transpose2", 2, C10 + 10, R1_2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Transpose3", 2, C11, R1_2, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_RADIOBUTTON|BRIGHTON_NOSHADOW},
	{"Tuning", 0, C10 - 10, R3, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	{"ModDepth", 0, C11 - 5, R3, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* Effect option */
	{"", 0, C80, R2, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},

	/* Master vol - 92 */
	{"", 0, C80, R0, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, 0},
	/* Tuning */
	{"", 2, C10 - 10, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 0, C11 - 5, R0, S3, S4, 0, 1, 0, "bitmaps/knobs/knob6.xpm", 0, BRIGHTON_NOTCH},

	/* Memory/Midi - 94 */
	/* Load/Save */
	{"", 2, C20 - 4, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C23 + 3, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* 1 - 3 */
	{"", 2, C20 - 4, R1_1 - 70, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C21 + S2, R1_1 - 70, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C23 + 3, R1_1 - 70, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* 4 - 6 */
	{"", 2, C20 - 4, R21_0 - 46, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C21 + S2, R21_0 - 46, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C23 + 3, R21_0 - 46, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* 7 - 9 */
	{"", 2, C20 - 4, R21_1 - 23, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C21 + S2, R21_1 - 23, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C23 + 3, R21_1 - 23, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	/* 0 */
	{"", 2, C21  + S2, R3, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},

/*	{"", 2, C23, R1_1, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm", */
/*		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW}, */
/*	{"", 2, C23, R21_0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm", */
/*		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW}, */
	{"", 2, C23 + 3, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, C20 - 4, R0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},

	{"", 2, 0, 0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},
	{"", 2, 0, 0, S2, S1, 0, 1, 0, "bitmaps/buttons/touchogb.xpm",
		"bitmaps/buttons/touchgb.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_WITHDRAWN},

	{"", 4, 790, 1020, 180, 140, 0, 1, 0, "bitmaps/images/memorymoog.xpm",0, 0},

	{"", 3, C20 - 6, R1_0, 106, S1 - 10, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0},
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp memMoogApp = {
	"memoryMoog",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	0,
	memMoogInit,
	memMoogConfigure, /* 3 callbacks, unused? */
	MMmidiCallback,
	destroySynth,
	{6, 100, 4, 2, 5, 520, 0, 0},
	817, 310, 0, 0,
	8, /* Panels */
	{
		{
			"MemoryMoog",
			"bitmaps/blueprints/memMoog.xpm",
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			memMoogCallback,
			12, 0, 980, 550,
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
			100, 650, 913, 340,
			KEY_COUNT_5OCTAVE,
			keysprofile2
		},
		{
			"Mods",
			"bitmaps/blueprints/mmmods.xpm",
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			memMoogModCallback,
			14, 656, 86, 334,
			2,
			mods
		},
		{
			"Edge",
			0,
			"bitmaps/textures/wood4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			988, 0, 13, 1000,
			0,
			0
		},
		{
			"Bar",
			0,
			"bitmaps/textures/wood6.xpm",
			BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			13, 550, 975, 100,
			0,
			0
		},
		{
			"Edge",
			0,
			"bitmaps/textures/wood4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 12, 1000,
			0,
			0
		},
		{
			"Edge",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			2, 4, 9, 988,
			0,
			0
		},
		{
			"Edge",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			990, 4, 10, 988,
			0,
			0
		},
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

static void
postMemory(guiSynth *synth)
{
	brightonEvent event;
	int i;

	/*
	 * We have a couple of settings to force into their targetted values,
	 * namely the radio buttons.
	 *	Each Osc transpose
	 *	Synth transpose
	 *	LFO waveform
	 */
	event.value = 1;

	if (synth->mem.param[0])
		i = 0;
	else if (synth->mem.param[1])
		i = 1;
	else if (synth->mem.param[2])
		i = 2;
	else
		i = 3;
	brightonParamChange(synth->win, 0, i, &event);

	if (synth->mem.param[9])
		i = 9;
	else if (synth->mem.param[10])
		i = 10;
	else if (synth->mem.param[11])
		i = 11;
	else
		i = 12;
	brightonParamChange(synth->win, 0, i, &event);

	if (synth->mem.param[18])
		i = 18;
	else if (synth->mem.param[19])
		i = 19;
	else if (synth->mem.param[20])
		i = 20;
	else
		i = 21;
	brightonParamChange(synth->win, 0, i, &event);

	if (synth->mem.param[53])
		i = 53;
	else if (synth->mem.param[54])
		i = 54;
	else if (synth->mem.param[55])
		i = 55;
	else if (synth->mem.param[56])
		i = 56;
	else
		i = 57;
	brightonParamChange(synth->win, 0, i, &event);

	if (synth->mem.param[86])
		i = 86;
	else if (synth->mem.param[87])
		i = 87;
	else
		i = 88;
	brightonParamChange(synth->win, 0, i, &event);
}

int
memoogloadMemory(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	loadMemory(synth, "memoryMoog", 0, location, synth->mem.active, 0, 0);
	postMemory(synth);

	return(0);
}

static int
MMmidiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value + synth->bank;
			loadMemory(synth, "memoryMoog", 0, synth->location,
				synth->mem.active, 0, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value + 10;
			synth->location = (synth->location % 10) + synth->bank;
			loadMemory(synth, "memoryMoog", 0, synth->location,
				synth->mem.active, 0, 0);
			break;
	}
	postMemory(synth);

	return(0);
}

static int
memMoogModCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

/*printf("memMoogModCallback(%x, %i, %i, %f)\n", synth, panel, index, value); */

	/*
	 * If this is controller 0 it is the frequency control, otherwise a 
	 * generic controller 1. The depth of the bend wheel is a parameter, so
	 * we need to scale the value here.
	 */
	if (index == 0)
	{
		float bend = synth->mem.param[89];
		/*
		 * Value is between 0 and 1 latching at 0.5. Have to scale the value
		 * and subtrace if from the mid point
		 */
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_PITCH, 0,
			(int) (((0.5 - bend / 2) + (value * bend)) * C_RANGE_MIN_1));
	} else {
		bristolMidiControl(global.controlfd, synth->midichannel,
			0, 1, ((int) (value * C_RANGE_MIN_1)) >> 7);
	}
	return(0);
}

static int
memMoogMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
/*printf("%i, %i, %i\n", c, o, v); */
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
memMoogMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int cmem = synth->location;

	if (synth->flags & MEM_LOADING)
		return;

	switch (c) {
		default:
		case 0:
			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;
			if (loadMemory(synth, "memoryMoog", 0, synth->location,
				synth->mem.active, 0, BRISTOL_STAT) < 0)
				displayPanelText(synth, "FRE", synth->location, 0, DISPLAY);
			else
				displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			break;
		case 1:
			loadMemory(synth, "memoryMoog", 0, synth->location,
				synth->mem.active, 0, 0);
			postMemory(synth);
			displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			synth->location = 0;
			break;
		case 2:
			saveMemory(synth, "memoryMoog", 0, synth->location, 0);
			displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			synth->location = 0;
			break;
		case 3:
			while (loadMemory(synth, "memoryMoog", 0, --synth->location,
				synth->mem.active, 0, 0) < 0)
			{
				if (synth->location < 0)
					synth->location = 1000;
				if (synth->location == cmem)
					break;
			}
			postMemory(synth);
			displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			break;
		case 4:
			while (loadMemory(synth, "memoryMoog", 0, ++synth->location,
				synth->mem.active, 0, 0) < 0)
			{
				if (synth->location > 999)
					synth->location = -1;
				if (synth->location == cmem)
					break;
			}
			postMemory(synth);
			displayPanelText(synth, "PRG", synth->location, 0, DISPLAY);
			break;
	}
}

static void
memMoogMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			displayPanelText(synth, "MIDI", synth->midichannel + 1, 0, DISPLAY);
			return;
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 15)
		{
			synth->midichannel = 15;
			displayPanelText(synth, "MIDI", synth->midichannel + 1, 0, DISPLAY);
			return;
		}
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

	displayPanelText(synth, "MIDI", synth->midichannel + 1, 0, DISPLAY);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
memMoogCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*printf("memMoogCallback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (memMoogApp.resources[panel].devlocn[index].to == 1)
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
		printf("dispatch[%x,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#endif

	return(0);
}

static void
memMoogOsc3LFO(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int value = 0;

	if (v == 0)
	{ 
		/*
		 * LFO off, find the desired transpose
		 */
		if (synth->mem.param[18])
			value = 0;
		else if (synth->mem.param[19])
			value = 1;
		else if (synth->mem.param[20])
			value = 2;
		else if (synth->mem.param[21])
			value = 3;
	} else {
		/*
		 * LFO on
		 */
		bristolMidiSendMsg(fd, chan, 2, 1, 10);
		return;
	}
	bristolMidiSendMsg(fd, chan, 2, 1, value);
}

static void
memMoogOscTranspose(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int value;

	if (synth->flags & MEM_LOADING)
		return;

	/*
	 * We need to configure the radio set and then request the tranpose value.
	 */
	if (synth->dispatch[c].other2)
	{
		synth->dispatch[c].other2 = 0;
		return;
	}

	if (synth->dispatch[c].other1 != -1)
	{
		event.type = BRIGHTON_FLOAT;

		synth->dispatch[c].other2 = 1;

		if (synth->dispatch[c].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[c].other1, &event);
	}
	synth->dispatch[c].other1 = o;

	if (o > 17) {
		/*
		 * Osc 3 - tranpose depends on mem[27]
		 */
		if (synth->mem.param[27])
			return;
		else
			value = o - 18;
	} else if (o > 4)
		value = o - 9;
	else
		value = o;

	bristolMidiSendMsg(fd, chan, c, 1, value);
}

static void
memMoogLFOWave(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (synth->flags & MEM_LOADING)
		return;

	/*
	 * We need to configure the radio set and then request the tranpose value.
	 */
	if (synth->dispatch[c].other2)
	{
		synth->dispatch[c].other2 = 0;
		return;
	}

	if (synth->dispatch[c].other1 != -1)
	{
		event.type = BRIGHTON_FLOAT;

		synth->dispatch[c].other2 = 1;

		if (synth->dispatch[c].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[c].other1, &event);
	}
	synth->dispatch[c].other1 = o;

	bristolMidiSendMsg(fd, chan, 126, 4, o - 53);
}

static void
memMoogTranspose(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (synth->flags & MEM_LOADING)
		return;

	/*
	 * We need to configure the radio set and then request the tranpose value.
	 */
	if (synth->dispatch[c].other2)
	{
		synth->dispatch[c].other2 = 0;
		return;
	}

	if (synth->dispatch[c].other1 != -1)
	{
		event.type = BRIGHTON_FLOAT;

		synth->dispatch[c].other2 = 1;

		if (synth->dispatch[c].other1 != o)
			event.value = 0;
		else
			event.value = 1;

		brightonParamChange(synth->win, synth->panel,
			synth->dispatch[c].other1, &event);
	}
	synth->dispatch[c].other1 = o;

	synth->transpose = 24 + (o - 86) * 12;
}

static void
multiTune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	/*
	 * Configures all the tune settings to zero (ie, 0.5). Do the Osc-A first,
	 * since it is not visible, and then request the GUI to configure Osc-B.
	 */
/*	bristolMidiSendMsg(fd, synth->sid, 0, 2, 8192); */
	event.type = BRIGHTON_FLOAT;
	event.value = 0.5;
	brightonParamChange(synth->win, synth->panel, 13, &event);
	brightonParamChange(synth->win, synth->panel, 22, &event);
	brightonParamChange(synth->win, synth->panel, 94, &event);
}

static void
keyHold(guiSynth *synth)
{
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->mem.param[82])
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|1);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|0);
}

static void
memoryGlide(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (c == 126)
	{
		if (synth->mem.param[85] != 0)
			bristolMidiSendMsg(fd, chan, 126, 0, v);
		return;
	}
	if (v != 0)
		bristolMidiSendMsg(fd, chan, 126, 0,
			(int) (synth->mem.param[84] * C_RANGE_MIN_1));
	else
		bristolMidiSendMsg(fd, chan, 126, 0, 0);
}

static void
filterTrack(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	float value;

	value = (synth->mem.param[40] + synth->mem.param[41] * 2)
		* C_RANGE_MIN_1 / 3;

	bristolMidiSendMsg(fd, chan, 3, 3, (int) value);
}

static void
jointRelease(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (c == 45)
	{
		if (synth->mem.param[49] == 0)
			bristolMidiSendMsg(fd, chan, 6, 3, v);
		return;
	}
	if (v == 0)
		bristolMidiSendMsg(fd, chan, 6, 3,
			(int) (synth->mem.param[45] * C_RANGE_MIN_1));
	else
		bristolMidiSendMsg(fd, chan, 6, 3, 10);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
memMoogInit(brightonWindow *win)
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

	printf("Initialise the memMoog link to bristol: %p\n", synth->win);

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
	synth->voices = 6;
	if (!global.libtest)
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = memMoogMidiSendMsg;

	/* Osc - first transpose options */
	dispatch[0].controller = 0;
	dispatch[0].operator = 0;
	dispatch[1].controller = 0;
	dispatch[1].operator = 1;
	dispatch[2].controller = 0;
	dispatch[2].operator = 2;
	dispatch[3].controller = 0;
	dispatch[3].operator = 3;
	dispatch[9].controller = 1;
	dispatch[9].operator = 9;
	dispatch[10].controller = 1;
	dispatch[10].operator = 10;
	dispatch[11].controller = 1;
	dispatch[11].operator = 11;
	dispatch[12].controller = 1;
	dispatch[12].operator = 12;
	dispatch[18].controller = 2;
	dispatch[18].operator = 18;
	dispatch[19].controller = 2;
	dispatch[19].operator = 19;
	dispatch[20].controller = 2;
	dispatch[20].operator = 20;
	dispatch[21].controller = 2;
	dispatch[21].operator = 21;
	dispatch[0].routine = dispatch[1].routine =
	dispatch[2].routine = dispatch[3].routine =
	dispatch[9].routine = dispatch[10].routine =
	dispatch[11].routine = dispatch[12].routine =
	dispatch[18].routine = dispatch[19].routine =
	dispatch[20].routine = dispatch[21].routine =
		(synthRoutine) memMoogOscTranspose;
	/* Osc Options. */
	dispatch[4].controller = 1;
	dispatch[4].operator = 7;
	dispatch[5].controller = 0;
	dispatch[5].operator = 0;
	dispatch[6].controller = 0;
	dispatch[6].operator = 6;
	dispatch[7].controller = 0;
	dispatch[7].operator = 4;
	dispatch[8].controller = 0;
	dispatch[8].operator = 5;
	/* Osc 2 */
	dispatch[13].controller = 1;
	dispatch[13].operator = 2;
	dispatch[14].controller = 1;
	dispatch[14].operator = 0;
	dispatch[15].controller = 1;
	dispatch[15].operator = 6;
	dispatch[16].controller = 1;
	dispatch[16].operator = 4;
	dispatch[17].controller = 1;
	dispatch[17].operator = 5;
	/* Osc 3 */
	dispatch[22].controller = 2;
	dispatch[22].operator = 2;
	dispatch[23].controller = 2;
	dispatch[23].operator = 0;
	dispatch[24].controller = 2;
	dispatch[24].operator = 6;
	dispatch[25].controller = 2;
	dispatch[25].operator = 4;
	dispatch[26].controller = 2;
	dispatch[26].operator = 5;
	/* LFO/KBD */
	dispatch[27].controller = 126;
	dispatch[27].operator = 1;
	dispatch[27].routine = (synthRoutine) memMoogOsc3LFO;
	dispatch[28].controller = 126;
	dispatch[28].operator = 7;

	/* Mixer */
	dispatch[29].controller = 126;
	dispatch[29].operator = 30;
	dispatch[30].controller = 126;
	dispatch[30].operator = 31;
	dispatch[31].controller = 126;
	dispatch[31].operator = 32;
	dispatch[32].controller = 126;
	dispatch[32].operator = 33;

	/* Filter */
	dispatch[33].controller = 3;
	dispatch[33].operator = 0;
	dispatch[34].controller = 3;
	dispatch[34].operator = 1;
	dispatch[35].controller = 126;
	dispatch[35].operator = 17;
	/* Filter Env */
	dispatch[36].controller = 4;
	dispatch[36].operator = 0;
	dispatch[37].controller = 4;
	dispatch[37].operator = 1;
	dispatch[38].controller = 4;
	dispatch[38].operator = 2;
	dispatch[39].controller = 4;
	dispatch[39].operator = 3;
	/* Filter Key tracking */
	dispatch[40].controller = 126;
	dispatch[40].operator = 1;
	dispatch[41].controller = 126;
	dispatch[41].operator = 2;
	dispatch[40].routine = dispatch[41].routine = (synthRoutine) filterTrack;

	/* AMP ENV */
	dispatch[42].controller = 6;
	dispatch[42].operator = 0;
	dispatch[43].controller = 6;
	dispatch[43].operator = 1;
	dispatch[44].controller = 6;
	dispatch[44].operator = 2;
	dispatch[45].controller = 45;
	dispatch[45].operator = 3;
	dispatch[45].routine = (synthRoutine) jointRelease;
	/* AMP ENV Options */
	dispatch[46].controller = 6;
	dispatch[46].operator = 6;
	dispatch[47].controller = 6;
	dispatch[47].operator = 7;
	dispatch[48].controller = 6;
	dispatch[48].operator = 8;
	dispatch[49].controller = 126;
	dispatch[49].operator = 101;
	dispatch[49].routine = (synthRoutine) jointRelease;
	/* Prog volume */
	dispatch[50].controller = 6;
	dispatch[50].operator = 4;
	/* Aux? We don't have aux out or headphones, so use this for panning. */
	dispatch[51].controller = 100;
	dispatch[51].operator = 0;

	/* Mods - LFO freq first */
	dispatch[52].controller = 8;
	dispatch[52].operator = 0;
	dispatch[53].controller = 3;
	dispatch[53].operator = 53;
	dispatch[54].controller = 3;
	dispatch[54].operator = 54;
	dispatch[55].controller = 3;
	dispatch[55].operator = 55;
	dispatch[56].controller = 3;
	dispatch[56].operator = 56;
	dispatch[57].controller = 3;
	dispatch[57].operator = 57;
	dispatch[53].routine = dispatch[54].routine =
		dispatch[55].routine = dispatch[56].routine =
		dispatch[57].routine = (synthRoutine) memMoogLFOWave;
	/* LFO Routing: */
	dispatch[58].controller = 126;
	dispatch[58].operator = 8;
	dispatch[59].controller = 126;
	dispatch[59].operator = 9;
	dispatch[60].controller = 126;
	dispatch[60].operator = 10;
	dispatch[61].controller = 126;
	dispatch[61].operator = 11;
	dispatch[62].controller = 126;
	dispatch[62].operator = 12;
	dispatch[63].controller = 126;
	dispatch[63].operator = 13;
	dispatch[64].controller = 126;
	dispatch[64].operator = 14;

	/* Poly mods */
	dispatch[65].controller = 126;
	dispatch[65].operator = 20;
	dispatch[66].controller = 126;
	dispatch[66].operator = 21;
	dispatch[67].controller = 126;
	dispatch[67].operator = 27;
	dispatch[68].controller = 126;
	dispatch[68].operator = 28;
	/* Poly Routing */
	dispatch[69].controller = 126;
	dispatch[69].operator = 22;
	dispatch[70].controller = 126;
	dispatch[70].operator = 23;
	dispatch[71].controller = 126;
	dispatch[71].operator = 24;
	dispatch[72].controller = 126;
	dispatch[72].operator = 25;
	dispatch[73].controller = 126;
	dispatch[73].operator = 26;

	/* Pedals */
	dispatch[74].controller = 126;
	dispatch[74].operator = 40;
	dispatch[75].controller = 126;
	dispatch[75].operator = 41;
	dispatch[76].controller = 126;
	dispatch[76].operator = 42;
	dispatch[77].controller = 126;
	dispatch[77].operator = 43;
	dispatch[78].controller = 126;
	dispatch[78].operator = 44;
	dispatch[79].controller = 126;
	dispatch[79].operator = 45;
	dispatch[80].controller = 126;
	dispatch[80].operator = 46;

	/* Main - Mono/Hold/Multi */
	dispatch[81].controller = 126;
	dispatch[81].operator = 2;
	dispatch[82].routine = (synthRoutine) keyHold;
	dispatch[83].controller = 126;
	dispatch[83].operator = 6;
	/* Glide */
	dispatch[84].controller = 126;
	dispatch[84].operator = 0;
	dispatch[85].controller = 127;
	dispatch[85].operator = 0;
	dispatch[84].routine = dispatch[85].routine = (synthRoutine) memoryGlide;

	/* Transpose */
	dispatch[86].controller = 4;
	dispatch[86].operator = 86;
	dispatch[87].controller = 4;
	dispatch[87].operator = 87;
	dispatch[88].controller = 4;
	dispatch[88].operator = 88;
	dispatch[86].routine = dispatch[87].routine = dispatch[88].routine
		= (synthRoutine) memMoogTranspose;

	/* Depth controls */
	dispatch[89].controller = 126;
	dispatch[89].operator = 47;
	dispatch[90].controller = 126;
	dispatch[90].operator = 3;

	/* Effects depth */
	dispatch[91].controller = 100;
	dispatch[91].operator = 1;

	/* Master, Autotune and tune */
	dispatch[92].controller = 126;
	dispatch[92].operator = 16;
	dispatch[93].routine = (synthRoutine) multiTune;
	dispatch[94].controller = 126;
	dispatch[94].operator = 1;

	dispatch[MEM_START +0].routine = dispatch[MEM_START +1].routine =
		dispatch[MEM_START +2].routine = dispatch[MEM_START +3].routine =
		dispatch[MEM_START +4].routine = dispatch[MEM_START +5].routine =
		dispatch[MEM_START +6].routine = dispatch[MEM_START +7].routine =
		dispatch[MEM_START +8].routine = dispatch[MEM_START +9].routine =
		dispatch[MEM_START +10].routine = dispatch[MEM_START +11].routine =
		dispatch[MEM_START +12].routine = dispatch[MEM_START +13].routine
			= (synthRoutine) memMoogMemory;

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
		(synthRoutine) memMoogMidi;

	dispatch[OSC_1_RADIO].other1 = -1;
	dispatch[OSC_2_RADIO].other1 = -1;
	dispatch[OSC_3_RADIO].other1 = -1;

	/* Put in diverse defaults settings */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 0, 16383);
	/* Tune osc-1 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	/* Gain on filter env - changed to fixed MOD on filter, variable Env. Then */
	/* changed to fixed env with in-algo filter gain... */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, C_RANGE_MIN_1);
	/* Filter env does not track velocity */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 5, 0);
	/* Waveform subosc */
/*	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, C_RANGE_MIN_1); */
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
memMoogConfigure(brightonWindow *win)
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
	synth->transpose = 48;

	synth->bank = 0;
	event.value = 1;
	/* Poly */
	loadMemory(synth, "memoryMoog", 0,
		synth->location, synth->mem.active, 0, 0);
	postMemory(synth);

	brightonPut(win,
		"bitmaps/blueprints/memMoogshade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	event.type = BRIGHTON_FLOAT;
	event.value = 0.5;
	brightonParamChange(synth->win, 0, 93, &event);
	event.value = 1.0;
	brightonParamChange(synth->win, 0, 92, &event); /* Volume */
	brightonParamChange(synth->win, 0, 74, &event); /* pedalamt1 */
	brightonParamChange(synth->win, 0, 78, &event); /* pedalamt2 */
	configureGlobals(synth);

	synth->loadMemory = (loadRoutine) memoogloadMemory;

	return(0);
}

