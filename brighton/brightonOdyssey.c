
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

/* ARP Axxe */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int initmem;

static int odysseyInit();
static int odysseyConfigure();
static int odysseyCallback(brightonWindow *, int, int, float);
/*static int keyCallback(void *, int, int, float); */
static int odysseyMemory(brightonWindow *, int, int, float);
static void odysseyPanelSwitch(brightonWindow *);
static int odysseyMidi(guiSynth *, int);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define DEVICE_COUNT 61
#define ACTIVE_DEVS 61 /* Actually 57 - should not sve pitch control. */
#define MEM_START 0
#define MIDI_START (MEM_START + 18)

#define RADIOSET_1 (ACTIVE_DEVS - 2)
#define RADIOSET_2 (RADIOSET_1 + 1)

#define KEY_PANEL 1

#define CDIFF 34
#define CDIFF2 15

#define C0 35
#define C1 85

#define C2 132
#define C3 (C2 + CDIFF)
#define C4 (C3 + CDIFF)
#define C5 (C4 + CDIFF)
#define C6 (C5 + CDIFF + CDIFF2)
#define C7 (C6 + CDIFF)
#define C8 (C7 + CDIFF)
#define C9 (C8 + CDIFF)
#define C10 (C9 + CDIFF + CDIFF2)
#define C11 (C10 + CDIFF)
#define C12 (C11 + CDIFF + CDIFF)
#define C13 (C12 + CDIFF)
#define C14 (C13 + CDIFF)
#define C15 (C14 + CDIFF)
#define C16 (C15 + CDIFF)
#define C17 (C16 + CDIFF)
#define C18 (C17 + CDIFF)
#define C19 (C18 + CDIFF + CDIFF + CDIFF2)
#define C20 (C19 + CDIFF)
#define C21 (C20 + CDIFF)
#define C22 (C21 + CDIFF)
#define C23 (C22 + CDIFF)

#define R4 98
#define R0 (R4 + 428)
#define R1 (R0 + 205)
#define R2 (R0 + 75)
#define R5 (R0 + 332)

#define W1 12
#define W2 15
#define L1 242

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
 * This example is for a odysseyBristol type synth interface.
 */

static brightonLocations locations[DEVICE_COUNT] = {
	/* Glide/Tranpose - 0 */
	{"Glide", 1, C0 - 4, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"Transpose", 2, C1, R2 + 20, 15, L1/2 - 40, 0, 2, 0, 0, 0,
		BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},

	/* VCO - 2 */
	{"VCO1-FM-1", 1, C2, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlue.xpm", 0, 0},
	{"VCO1-FM-2", 1, C3, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"VCO1-PW", 1, C4, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"VCO1-PWM", 1, C5, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpYellow.xpm", 0, 0},
	{"VCO1-Coarse", 1, C2, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlue.xpm", 0, BRIGHTON_NOTCH},
	{"VCO1-Fine", 1, C3, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, BRIGHTON_NOTCH},

	{"VCO1-Mod1-SinSqr", 2, C2, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO1-Mod2-SH/Env", 2, C3, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO1-PWM-Sine/Env", 2, C5, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO1-KBD/LFO", 2, C4, R4, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* VCO2 - 12 */
	{"VCO2-FM-1", 1, C6, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlue.xpm", 0, 0},
	{"VCO2-FM-2", 1, C7, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"VCO2-PW", 1, C8, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"VCO2-PWM", 1, C9, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpYellow.xpm", 0, 0},
	{"VCO2-Coarse", 1, C6, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlue.xpm", 0, BRIGHTON_NOTCH},
	{"VCO2-Fine", 1, C7, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, BRIGHTON_NOTCH},

	{"VCO2-Mod1-SinS/H", 2, C6, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO2-Mod2-SH/Env", 2, C7, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO2-PWM-Sine/Env", 2, C9, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO2-Sync", 2, C8, R4, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Mods - 22 */
	{"S/H-Mix-1", 1, C10, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpWhite.xpm", 0, 0},
	{"S/H-Mix-2", 1, C11, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpWhite.xpm", 0, 0},
	{"S/H-Gain", 1, C12, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},
	{"LFO-Freq", 1, C12, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlack.xpm", 0, 0},

	{"S/H-Mix1-RampSqr", 2, C10, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"S/H-Mix2-NoiseVCO2", 2, C11, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"S/H-Trig-KBD/LFO", 2, C12 - CDIFF, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Filter, etc - 29 */
	{"Mix-1-Lvl", 1, C13, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpYellow.xpm", 0, 0},
	{"Mix-2-Lvl", 1, C14, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpBlue.xpm", 0, 0},
	{"Mix-3-Lvl", 1, C15, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"VCF-Mod-1", 1, C16, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpGreen.xpm", 0, 0},
	{"VCF-Mod-2", 1, C17, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpGreen.xpm", 0, 0},
	{"VCF-Mod-3", 1, C18, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpGreen.xpm", 0, 0},
	{"VCA-EnvLvl", 1, C19, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"VCF-Cutoff", 1, C13, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpWhite.xpm", 0, 0},
	{"VCF-Res", 1, C14, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpWhite.xpm", 0, 0},
	{"HPF-Cutoff", 1, C18, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpGreen.xpm", 0, 0},
	{"VCA-Gain", 1, C19, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	/* And its routing - 40 */
	{"MIX-Noise/RM", 2, C13, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"MIX-VCO1-RampSqr", 2, C14, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"MIX-VCO2-RampSqr", 2, C15, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO-Mod1-KBD/SH", 2, C16, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO-Mod2-SH/LFO", 2, C17, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCO-Mod3-ADSR/SR", 2, C18, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"VCA-ADSR/AR", 2, C19, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* ADSR - 47 */
	{"Attack", 1, C20, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"Decay", 1, C21, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"Sustain", 1, C22, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"Release", 1, C23, R0, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"AR-Attack", 1, C20, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},
	{"AR-Release", 1, C21, R4, W2, L1, 0, 1, 0,
		"bitmaps/knobs/sliderArpRed.xpm", 0, 0},

	{"ADSR-KBD/REPEAT", 2, C20, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"ADSR-KBD/AUTO", 2, C22, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"AR-KBD/REPEAT", 2, C23, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Pink/White noise - 56 */
	{"Noise-White/Pink", 2, C1, R4, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Pitch Control - 57 */
	{"Pitch-Flat", 2, 25, R5, 30, 60, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"Pitch-Trill", 2, 53, R5, 30, 60, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"Pitch-Sharp", 2, 80, R5, 30, 60, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},

	/* Dual osc frequencies */
	{"VCO2-Dual", 2, C8, R4 + 160, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
};

static brightonLocations locations2[DEVICE_COUNT] = {
	/* Glide/Tranpose - 0 */
	{"", 1, C0, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
//	{"", 2, C1, R2, 15, L1/2 - 10, 0, 2, 0, 0, 0,
	{"Transpose", 2, C1, R2 + 20, 15, L1/2 - 40, 0, 2, 0, 0, 0,
		BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},

	/* VCO - 2 */
	{"", 1, C2, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C3, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C4, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C5, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C2, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, BRIGHTON_NOTCH},
	{"", 1, C3, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, BRIGHTON_NOTCH},

	{"", 2, C2, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C3, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C5, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C4, R4, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* VCO2 - 12 */
	{"", 1, C6, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C7, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C8, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C9, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C6, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, BRIGHTON_NOTCH},
	{"", 1, C7, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, BRIGHTON_NOTCH},

	{"", 2, C6, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C7, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C9, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C8, R4, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Mods - 22 */
	{"", 1, C10, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C11, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C12, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C12, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},

	{"", 2, C10, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C11, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C12 - CDIFF, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Filter, etc - 29 */
	{"", 1, C13, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C14, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C15, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C16, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C17, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C18, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C19, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C13, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C14, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C18, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C19, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	/* And its routing - 40 */
	{"", 2, C13, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C14, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C15, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C16, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C17, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C18, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C19, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* ADSR - 47 */
	{"", 1, C20, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C21, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C22, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C23, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C20, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},
	{"", 1, C21, R4, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack3.xpm", 0, 0},

	{"", 2, C20, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C22, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
	{"", 2, C23, R5, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Pink/White noise - 56 */
	{"", 2, C1, R4, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Pitch Control - 57 */
	{"", 2, 25, R5, 30, 60, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, 53, R5, 30, 60, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, 80, R5, 30, 60, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},

	/* Dual osc frequencies */
	{"VCO2-Dual", 2, C8, R4 + 160, W1 * 3/2, 60, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},
};

#define BDIFF (CDIFF)
#define BDIFF2 (CDIFF2)

#define B0 190
#define B1 (B0 + BDIFF + BDIFF2)
#define B2 (B1 + BDIFF)
#define B3 (B2 + BDIFF)
#define B4 (B3 + BDIFF)
#define B5 (B4 + BDIFF)
#define B6 (B5 + BDIFF)
#define B7 (B6 + BDIFF)
#define B8 (B7 + BDIFF)
#define B9 (B8 + BDIFF + BDIFF2)
#define B10 (B9 + BDIFF)
#define B11 (B10 + BDIFF)
#define B12 (B11 + BDIFF)
#define B13 (B12 + BDIFF)
#define B14 (B13 + BDIFF)
#define B15 (B14 + BDIFF)
#define B16 (B15 + BDIFF)
#define B17 (B16 + BDIFF + BDIFF2)

#define B20 40
#define B18 (B20 + BDIFF + BDIFF2 + 10)
#define B19 (B18 + BDIFF + BDIFF2)

#define BL1 500

#define R3 200

static brightonLocations memories[21] = {
	/* Memory Load, mem, Save - 61 */
	{"", 2, B0, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, B1, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B2, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B3, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B4, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B5, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B6, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B7, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B8, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, B9, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B10, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B11, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B12, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B13, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B14, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B15, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B16, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, B17, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},

	/* MIDI */
	{"", 2, B18, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, B19, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},

	/* Panel switch */
	{"", 2, B20, R3, 20, BL1, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp odysseyApp = {
	"odyssey",
	0, /* no blueprint on background. */
	"bitmaps/textures/metal4.xpm",
	0, /* BRIGHTON_STRETCH, //flags */
	odysseyInit,
	odysseyConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	600, 450, 0, 0,
	6, /* Panels */
	{
		{
			"Odyssey",
			"bitmaps/blueprints/odyssey2.xpm",
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_REVERSE|BRIGHTON_STRETCH, /* flags */
			0,
			0,
			odysseyCallback,
			15, 0, 970, 680,
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
			60, 750, 900, 250,
			KEY_COUNT_3OCTAVE,
			keys3octave
		},
		{
			"Odyssey",
			0,
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 15, 1000,
			0,
			0
		},
		{
			"Odyssey",
			0,
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		},
		{
			"Odyssey",
			"bitmaps/blueprints/odyssey.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			odysseyCallback,
			15, 0, 970, 680,
			DEVICE_COUNT,
			locations2
		},
		{
			"Odyssey",
			"bitmaps/blueprints/odysseymem.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			odysseyMemory,
			15, 680, 970, 70,
			21,
			memories
		}
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
			loadMemory(synth, "odyssey", 0, synth->bank + synth->location,
				synth->mem.active, 0, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
odysseyMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

/*
 * This will be called back with memories, midi and panel switch.
 */
static int
odysseyMemory(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

/*printf("odysseyMemory(%i, %f)\n", index, value); */

	switch(index) {
		case 17:
			saveMemory(synth, "odyssey", 0, synth->bank + synth->location, 0);
			return(0);
			break;
		case 0:
			loadMemory(synth, "odyssey", 0, synth->bank + synth->location,
				synth->mem.active, 0, 0);
			return(0);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			if (synth->dispatch[RADIOSET_1].other2)
			{
				synth->dispatch[RADIOSET_1].other2 = 0;
				return(0);
			}
			if (synth->dispatch[RADIOSET_1].other1 != -1)
			{
				synth->dispatch[RADIOSET_1].other2 = 1;

				if (synth->dispatch[RADIOSET_1].other1 != index)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, 5,
					synth->dispatch[RADIOSET_1].other1 + MEM_START, &event);
			}
			synth->dispatch[RADIOSET_1].other1 = index;
			synth->bank = index * 10;
			break;
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			if (synth->dispatch[RADIOSET_2].other2)
			{
				synth->dispatch[RADIOSET_2].other2 = 0;
				return(0);
			}
			if (synth->dispatch[RADIOSET_2].other1 != -1)
			{
				synth->dispatch[RADIOSET_2].other2 = 1;

				if (synth->dispatch[RADIOSET_2].other1 != index)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, 5,
					synth->dispatch[RADIOSET_2].other1 + MEM_START, &event);
			}
			synth->dispatch[RADIOSET_2].other1 = index;
			/* Do radio buttons */
			synth->location = index - 8;
			break;
		case 18:
			/*
			 * Midi down
			 */
			odysseyMidi(synth, 1);
			break;
		case 19:
			/*
			 * Midi Up
			 */
			odysseyMidi(synth, 0);
			break;
		case 20:
			/*
			 * Midi Panel Switch
			 */
			odysseyPanelSwitch(win);
			break;
	}
	return(0);
}

static int
odysseyMidi(guiSynth *synth, int c)
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
	{
		/*
		 * To overcome that we should consider checking a sequence number in
		 * the message library? That is non trivial since it requires that
		 * our midi messges have a 'ack' flag included - we cannot check for
		 * ack here (actually, we could, and in the app is probably the right
		 * place to do it rather than the lib however both would have to be
		 * changed to suppor this - nc).
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

	return(0);
}

static int oMark = 4;
static float *bs;

static void
odysseyPanelSwitch(brightonWindow * win)
{
	brightonEvent event;
	guiSynth *synth = findSynth(global.synths, win);

/*printf("odysseyPanelSwitch()\n"); */
/*	bcopy(synth->mem.param, bs, DEVICE_COUNT * sizeof(float)); */

	if (oMark == 0)
	{
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(synth->win, 0, -1, &event);
		event.intvalue = 1;
		brightonParamChange(synth->win, 4, -1, &event);
		oMark = 4;
		/* And toggle the filter type - rooney. */
		/*bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 3); */
		bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);
	} else {
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(synth->win, 4, -1, &event);
		event.intvalue = 1;
		brightonParamChange(synth->win, 0, -1, &event);
		oMark = 0;
		/* And toggle the filter type - chamberlain. */
		bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 0);
	}

/*
printf("odysseyPanelSwitch(%i)\n", oMark);
	event.type = BRIGHTON_FLOAT;
	for (i = 0; i < ACTIVE_DEVS; i++)
	{
		event.value = bs[i];
		synth->mem.param[i] = bs[i] - 1;
		brightonParamChange(synth->win, oMark, i, &event);
	}
*/
}

int calling = 0;
/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
odysseyCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*printf("odysseyCallback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if (panel != 0)
	{
		brightonEvent event;

		/*
		 * We are in the second panel, alternative skin. The GUI will have
		 * changed, but call ourselves back in panel 0 for all other changes.
		 */
		event.type = BRIGHTON_FLOAT;
		event.value = value;
		brightonParamChange(synth->win, 0, index, &event);

		return(0);
	} else {
		brightonEvent event;
		/*
		 * We were in panel 0, we need to adjust the parameter in the next
		 * layer
		 */
		if (calling)
			return(0);

		calling = 1;

		event.type = BRIGHTON_FLOAT;
		event.value = value;
		brightonParamChange(synth->win, 4, index, &event);

		calling = 0;
	}

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (odysseyApp.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * (CONTROLLER_RANGE - 1);
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
odysseyLFO(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (v == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 1, 10);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 1, 2);
}

static void
odysseyTranspose(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	switch (v)
	{
		case 0:
			synth->transpose = 36;
			break;
		case 1:
			synth->transpose = 48;
			break;
		case 2:
			synth->transpose = 60;
			break;
	}
}

static void
odysseyFiltOpt(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * This filter mod is either SH MIX or Keyboard tracking. One causes a 
	 * buffer selection, the other is a filter option.
	 */
/*printf("odysseyFiltOpt(%i, %i)\n", o, v); */
	if (o == 23)
	{
		/*
		 * Continuous.
		 */
		if (synth->mem.param[43] == 0)
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 23, v);
		else
			bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, v);
	} else {
		if (v == 0)
		{
			/*
			 * Key tracking switch.
			 */
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 31, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 23, 0);
			bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3,
				synth->mem.param[32] * C_RANGE_MIN_1);
		} else {
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 31, 1);
			bristolMidiSendMsg(global.controlfd, synth->sid, 126, 23,
				synth->mem.param[32] * C_RANGE_MIN_1);
			bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, 0);
		}
	}
}

static void
arAutoTrig(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * This takes the LFO value and sends it to the LFO, and if we have auto
	 * trig configured will also send a value to 126/19 for trigger repeat rate.
	 */
	if (o == 55)
	{
		/*
		 * Repeat switch
		 */
		if (v == 0)
		{
			/*
			 * Repeat. Send request and configure AR values
			 */
			bristolMidiSendMsg(fd, chan, 126, 55, 1);

			bristolMidiSendMsg(global.controlfd, synth->sid, 7, 2, 100);
		} else {
			bristolMidiSendMsg(fd, chan, 126, 55, 0);

			bristolMidiSendMsg(global.controlfd, synth->sid, 7, 2, 16383);
		}
		return;
	}
	if (o == 3) {
		bristolMidiSendMsg(global.controlfd, synth->sid, 7, 1,
			(int) (synth->mem.param[52] * C_RANGE_MIN_1));
		bristolMidiSendMsg(global.controlfd, synth->sid, 7, 3,
			(int) (synth->mem.param[52] * C_RANGE_MIN_1));
		return;
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
odysseyInit(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	dispatcher *dispatch;
	int i;

	if (synth == 0)
	{
		synth = findSynth(global.synths, (int) 0);
		if (synth == 0)
		{
			printf("cannot init\n");
			return(0);
		}
	}

	if ((initmem = synth->location) == 0)
		initmem = 11;

	synth->win = win;

	printf("Initialise the odyssey link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	bs = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
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
	{
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);
	}

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		synth->dispatch[i].routine = odysseyMidiSendMsg;
	}

	/* Glide/Octave */
	dispatch[0].controller = 126;
	dispatch[0].operator = 0;
	dispatch[1].controller = 126;
	dispatch[1].operator = 0;
	dispatch[1].routine = (synthRoutine) odysseyTranspose;

	/* Osc-1 FM MOD */
	dispatch[2].controller = 126;
	dispatch[2].operator = 2;
	dispatch[3].controller = 126;
	dispatch[3].operator = 3;
	/* Osc-1 PW MOD */
	dispatch[4].controller = 0;
	dispatch[4].operator = 0;
	dispatch[5].controller = 126;
	dispatch[5].operator = 4;
	/* Osc-1 Freq coarse/fine */
	dispatch[6].controller = 0;
	dispatch[6].operator = 2;
	dispatch[7].controller = 0;
	dispatch[7].operator = 10;
	/* Osc1 lfo/sh/etc */
	dispatch[8].controller = 126;
	dispatch[8].operator = 5;
	dispatch[9].controller = 126;
	dispatch[9].operator = 6;
	dispatch[10].controller = 126;
	dispatch[10].operator = 7;
	/* Osc-1 key/lfo - not done..... */
	dispatch[11].controller = 126;
	dispatch[11].operator = 100;
	dispatch[11].routine = (synthRoutine) odysseyLFO;

	/* Osc-2 FM */
	dispatch[12].controller = 126;
	dispatch[12].operator = 8;
	dispatch[13].controller = 126;
	dispatch[13].operator = 9;
	/* Osc-2 PW */
	dispatch[14].controller = 1;
	dispatch[14].operator = 0;
	dispatch[15].controller = 126;
	dispatch[15].operator = 10;
	/* Osc-2 Freq coarse/fine */
	dispatch[16].controller = 1;
	dispatch[16].operator = 2;
	dispatch[17].controller = 1;
	dispatch[17].operator = 10;
	/* Osc-2 lfo/sh/etc */
	dispatch[18].controller = 126;
	dispatch[18].operator = 11;
	dispatch[19].controller = 126;
	dispatch[19].operator = 12;
	dispatch[20].controller = 126;
	dispatch[20].operator = 13;
	/* Osc-2 Sync */
	dispatch[21].controller = 1;
	dispatch[21].operator = 7;

	/* SH mixer */
	dispatch[22].controller = 126;
	dispatch[22].operator = 14;
	dispatch[23].controller = 126;
	dispatch[23].operator = 15;
	/* SH Output? */
	dispatch[24].controller = 126;
	dispatch[24].operator = 16;
	/* LFO freq */
	dispatch[25].controller = 2;
	dispatch[25].operator = 0;
	/* SH Mods */
	dispatch[26].controller = 126;
	dispatch[26].operator = 17;
	dispatch[27].controller = 126;
	dispatch[27].operator = 18;
	dispatch[28].controller = 126;
	dispatch[28].operator = 19;

	/* Audio Mixer */
	dispatch[29].controller = 126;
	dispatch[29].operator = 20;
	dispatch[30].controller = 126;
	dispatch[30].operator = 21;
	dispatch[31].controller = 126;
	dispatch[31].operator = 22;
	/* VCF Mods */
	dispatch[32].controller = 126;
	dispatch[32].operator = 23;
	dispatch[32].routine = (synthRoutine) odysseyFiltOpt;
	dispatch[33].controller = 126;
	dispatch[33].operator = 24;
	dispatch[34].controller = 126;
	dispatch[34].operator = 25;
	/* VCA Env */
	dispatch[35].controller = 126;
	dispatch[35].operator = 26;
	/* VCF Cutoff/Req */
	dispatch[36].controller = 3;
	dispatch[36].operator = 0;
	dispatch[37].controller = 3;
	dispatch[37].operator = 1;
	/* HPF Cutoff */
	dispatch[38].controller = 9;
	dispatch[38].operator = 0;
	/* VCA Gain */
	dispatch[39].controller = 126;
	dispatch[39].operator = 27;
	/* Diverse Mods */
	dispatch[40].controller = 126;
	dispatch[40].operator = 28;
	dispatch[41].controller = 126;
	dispatch[41].operator = 29;
	dispatch[42].controller = 126;
	dispatch[42].operator = 30;
	dispatch[43].controller = 126;
	dispatch[43].operator = 31;
	dispatch[43].routine = (synthRoutine) odysseyFiltOpt;
	dispatch[44].controller = 126;
	dispatch[44].operator = 32;
	dispatch[45].controller = 126;
	dispatch[45].operator = 33;
	dispatch[46].controller = 126;
	dispatch[46].operator = 34;

	/* ADSR */
	dispatch[47].controller = 4;
	dispatch[47].operator = 0;
	dispatch[48].controller = 4;
	dispatch[48].operator = 1;
	dispatch[49].controller = 4;
	dispatch[49].operator = 2;
	dispatch[50].controller = 4;
	dispatch[50].operator = 3;
	/* AR */
	dispatch[51].controller = 7;
	dispatch[51].operator = 0;
	dispatch[52].controller = 7;
	dispatch[52].operator = 3;
	dispatch[52].routine = (synthRoutine) arAutoTrig;
	/* Env Mods */
	dispatch[53].controller = 126;
	dispatch[53].operator = 53;
	dispatch[54].controller = 126;
	dispatch[54].operator = 49;
	dispatch[55].controller = 126;
	dispatch[55].operator = 55;
	dispatch[55].routine = (synthRoutine) arAutoTrig;

	/* Noise white/pink */
	dispatch[56].controller = 6;
	dispatch[56].operator = 1;

	/* PPC */
	dispatch[57].controller = 126;
	dispatch[57].operator = 57;
	dispatch[58].controller = 126;
	dispatch[58].operator = 58;
	dispatch[59].controller = 126;
	dispatch[59].operator = 59;
	dispatch[60].controller = 126;
	dispatch[60].operator = 60;

	/*
	 * Need to specify env gain fixed, filter mod on, osc waveform.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);

	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 49, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, 0);

	/* These will change the ringmod osc */
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, 5);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 8, 16383);

	/* AR envelope fixed parameters. */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 1, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 4, 16383);

	/* Ringmodd parameters. */
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 0, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 1, 128);
	bristolMidiSendMsg(global.controlfd, synth->sid, 8, 2, 16383);

	/*
	 * Grooming env for 'gain' to amp. Short but not choppy attack, slightly
	 * larger decay since the main env can shorten that if really desired.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 0, 20);
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 1, 1000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 3, 50);
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 5, 0); /* - velocity */
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 6, 0); /* - rezero */

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
odysseyConfigure(brightonWindow* win)
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

	if (synth->location == 1)
	{
		synth->bank = 10;
		synth->location = 11;
	}

	loadMemory(synth, "odyssey", 0, initmem, synth->mem.active, 0, 0);

	event.value = 1;
	brightonParamChange(synth->win, 5, MEM_START + 1, &event);
	brightonParamChange(synth->win, 5, MEM_START + 9, &event);

	brightonPut(win,
		"bitmaps/blueprints/odysseyshade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);

	/*
	 * Touch a key on/off
	 */
/*	bristolMidiSendMsg(global.controlfd, synth->midichannel, */
/*		BRISTOL_EVENT_KEYON, 0, 10 + synth->transpose); */
/*	bristolMidiSendMsg(global.controlfd, synth->midichannel, */
/*		BRISTOL_EVENT_KEYOFF, 0, 10 + synth->transpose); */
	configureGlobals(synth);
	return(0);
}

