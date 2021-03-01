
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

static int axxeInit();
static int axxeConfigure();
static int axxeCallback(brightonWindow *, int, int, float);
/*static int keyCallback(void *, int, int, float); */
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define DEVICE_COUNT 50
#define ACTIVE_DEVS 30
#define MEM_START (ACTIVE_DEVS)
#define MIDI_START (MEM_START + 18)
#define RADIOSET_1 MEM_START
#define RADIOSET_2 (MEM_START + 1)

#define KEY_PANEL 1

#define CDIFF 32
#define CDIFF2 12

#define PC0 118
#define PC1 (PC0 + CDIFF)
#define PC2 (PC1 + CDIFF)
#define PC3 (PC2 + CDIFF)
#define PC4 (PC3 + CDIFF)
#define PC5 (PC4 + CDIFF)
#define PC6 (PC5 + CDIFF + CDIFF2)
#define PC7 (PC6 + CDIFF)
#define PC8 (PC7 + CDIFF)
#define PC9 (PC8 + CDIFF + CDIFF2)
#define PC10 (PC9 + CDIFF + CDIFF2)
#define PC11 (PC10 + CDIFF)
#define PC12 (PC11 + CDIFF)
#define PC13 (PC12 + CDIFF + CDIFF2)
#define PC14 (PC13 + CDIFF)
#define PC15 (PC14 + CDIFF + CDIFF2)
#define PC16 (PC15 + CDIFF)
#define PC17 (PC16 + CDIFF)
#define PC18 (PC17 + CDIFF + CDIFF2)
#define PC19 (PC18 + CDIFF)
#define PC20 (PC19 + CDIFF + CDIFF2)
#define PC21 (PC20 + CDIFF)
#define PC22 (PC21 + CDIFF)
#define PC23 (PC22 + CDIFF)

#define C0 126
#define C1 (C0 + CDIFF)
#define C2 (C1 + CDIFF)
#define C3 (C2 + CDIFF)
#define C4 (C3 + CDIFF)
#define C5 (C4 + CDIFF)
#define C6 (C5 + CDIFF + CDIFF2)
#define C7 (C6 + CDIFF)
#define C8 (C7 + CDIFF)
#define C9 (C8 + CDIFF + CDIFF2)
#define C10 (C9 + CDIFF + CDIFF2)
#define C11 (C10 + CDIFF)
#define C12 (C11 + CDIFF)
#define C13 (C12 + CDIFF + CDIFF2)
#define C14 (C13 + CDIFF)
#define C15 (C14 + CDIFF + CDIFF2)
#define C16 (C15 + CDIFF)
#define C17 (C16 + CDIFF)
#define C18 (C17 + CDIFF + CDIFF2)
#define C19 (C18 + CDIFF)
#define C20 (C19 + CDIFF + CDIFF2)
#define C21 (C20 + CDIFF)
#define C22 (C21 + CDIFF)
#define C23 (C22 + CDIFF)

#define RP0 414
#define RP1 (RP0 + 205)
#define RP2 (RP0 + 100)
#define RP3 880

#define R0 500
#define R1 (R0 + 205)
#define R2 (R0 + 75)
#define R3 900

#define W1 12
#define PW1 26
#define L1 300
#define LP1 363

#define BDIFF (CDIFF + 2)
#define BDIFF2 (CDIFF2 + 2)

#define B0 170
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

#define B18 (BDIFF)
#define B19 (B18 + BDIFF + BDIFF)

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
 * This example is for a axxeBristol type synth interface.
 */
static brightonLocations phatlocations[DEVICE_COUNT] = {
	/* Glide/Tranpose - 0 */
	{"Glide", 1, PC0, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Transpose", 2, C1, R2 - 30, 15, LP1/2 - 60, 0, 2, 0, 0, 0,
		BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},

	/* VCO - 2 */
	{"VCO LFO Sine Mod", 1, PC2, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO LFO Square Mod", 1, PC3, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO S/H Mod", 1, PC4, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO ADSR Mod", 1, PC5, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO PW", 1, PC6, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO PWM LFO", 1, PC7, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO PWM ADSR", 1, PC8, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* LFO - 9 */
	{"LFO Freq", 1, PC9, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* Mixer - 10 */
	{"Mix Noise", 1, PC10, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Mix VCO Ramp", 1, PC11, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Mix VCO Pulse", 1, PC12, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* VCF - 13 */
	{"VCF Cutoff", 1, PC13, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO Resonance", 1, PC14, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO KBD", 1, PC15, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCF LFO Sine", 1, PC16, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCF ADSR", 1, PC17, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* VCA - 18 */
	{"VCA Gain", 1, PC18, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCA ADSR", 1, PC19, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* ADSR - 20 */
	{"Attack", 1, PC20, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Decay", 1, PC21, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Sustain", 1, PC22, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Release", 1, PC23, RP0, PW1, LP1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* Pink/White noise - 24 */
	{"Pink/White", 2, C1, 110, 14, 100, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* LFO mode - 25 */
	{"LFO S/M", 2, C22, 110, 14, 100, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* LFO trigger - gate/auto - 26 */
	{"LFO Trig", 2, C20, 110, 14, 100, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Pitch Control - 27 */
	{"Pitch Flat", 2, 25, RP2, 30, 80, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"Pitch Trill", 2, 53, RP2, 30, 80, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"Pitch Sharp", 2, 80, RP2, 30, 80, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},

	/* Memory Load, mem, Save - 30 */
	{"", 2, B0, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, B1, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B2, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B3, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B4, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B5, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B6, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B7, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B8, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, B9, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B10, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B11, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B12, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B13, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B14, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B15, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B16, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, B17, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},

	/* MIDI - or android up/down memories */
	{"", 2, B18, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, B19, RP3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},
};
static brightonLocations locations[DEVICE_COUNT] = {
	/* Glide/Tranpose - 0 */
	{"Glide", 1, C0, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Transpose", 2, C1, R2 + 30, 15, L1/2 - 60, 0, 2, 0, 0, 0,
		BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},

	/* VCO - 2 */
	{"VCO LFO Sine Mod", 1, C2, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO LFO Square Mod", 1, C3, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO S/H Mod", 1, C4, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO ADSR Mod", 1, C5, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO PW", 1, C6, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO PWM LFO", 1, C7, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO PWM ADSR", 1, C8, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* LFO - 9 */
	{"LFO Freq", 1, C9, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* Mixer - 10 */
	{"Mix Noise", 1, C10, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Mix VCO Ramp", 1, C11, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Mix VCO Pulse", 1, C12, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* VCF - 13 */
	{"VCF Cutoff", 1, C13, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO Resonance", 1, C14, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCO KBD", 1, C15, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCF LFO Sine", 1, C16, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCF ADSR", 1, C17, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* VCA - 18 */
	{"VCA Gain", 1, C18, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"VCA ADSR", 1, C19, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* ADSR - 20 */
	{"Attack", 1, C20, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Decay", 1, C21, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Sustain", 1, C22, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Release", 1, C23, R0, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* Pink/White noise - 24 */
	{"Pink/White", 2, C1, 220, 14, 100, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* LFO mode - 25 */
	{"LFO S/M", 2, C22, 220, 14, 100, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* LFO trigger - gate/auto - 26 */
	{"LFO Trig", 2, C20, 220, 14, 100, 0, 1, 0,
		"bitmaps/buttons/klunk.xpm", 0, BRIGHTON_VERTICAL},

	/* Pitch Control - 27 */
	{"Pitch Flat", 2, 25, R2, 30, 80, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"Pitch Trill", 2, 53, R2, 30, 80, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"Pitch Sharp", 2, 80, R2, 30, 80, 0, 1, 0, "bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},

	/* Memory Load, mem, Save - 30 */
	{"", 2, B0, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, B1, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B2, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B3, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B4, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B5, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B6, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B7, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B8, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, B9, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B10, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B11, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B12, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B13, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B14, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B15, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, B16, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, B17, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},

	/* MIDI */
	{"", 2, B18, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, B19, R3, 20, 60, 0, 1, 0, "bitmaps/buttons/pressoffw.xpm",
		"bitmaps/buttons/pressonw.xpm", BRIGHTON_CHECKBUTTON},
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp axxePhatApp = {
	"axxe",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal4.xpm",
	0, /* BRIGHTON_STRETCH, //flags */
	axxeInit,
	axxeConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	600, 350, 0, 0,
	4, /* Panels */
	{
		{
			"Axxe",
			"bitmaps/blueprints/axxephat.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			axxeCallback,
			15, 0, 970, 620,
			DEVICE_COUNT,
			phatlocations
		},
#ifdef RIBBON_KEYBOARD
		{
			"RibbonKeyboard",
			0,
			"bitmaps/newkeys/ribbonKeys.xpm",
			BRIGHTON_STRETCH,
			0,
			ribbonCallBack,
			ribbonCallBack,
			80, 625, 840, 360,
			1,
			ribbonkeyboard,
		},
#else
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/fkbg.xpm", /* flags */
			BRIGHTON_STRETCH|BRIGHTON_KEY_PANEL,
			0,
			0,
			keyCallback,
			80, 625, 840, 360,
			KEY_COUNT_2OCTAVE2,
			keys2octave2
		},
#endif
		{
			"Axxe",
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
			"Axxe",
			0,
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		}
	}
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp axxeApp = {
	"axxe",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal4.xpm",
	0, /* BRIGHTON_STRETCH, //flags */
	axxeInit,
	axxeConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{1, 100, 2, 2, 5, 520, 0, 0},
	600, 350, 0, 0,
	4, /* Panels */
	{
		{
			"Axxe",
			"bitmaps/blueprints/axxe.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			axxeCallback,
			15, 0, 970, 700,
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
			120, 700, 800, 300,
			KEY_COUNT_3OCTAVE,
			keys3octave
		},
		{
			"Axxe",
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
			"Axxe",
			0,
			"bitmaps/textures/metal4.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
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
			loadMemory(synth, synth->resources->name, 0,
				synth->location, synth->mem.active, 0, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value * 10;

			synth->location = (synth->location % 10) + value * 10;
			loadMemory(synth, synth->resources->name, 0,
				synth->location, synth->mem.active, 0, 0);
			break;
	}
	return(0);
}

static int
axxeMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static int
axxeMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if ((synth->flags | OPERATIONAL) == 0)
		return(0);

/*	printf("	axxeMemory(B: %i L %i: %i, %i)\n", */
/*		synth->bank, synth->location, c, o); */

	switch(c) {
		case 0:
			saveMemory(synth, "axxe", 0, synth->bank + synth->location, 0);
			return(0);
			break;
		case 17:
			loadMemory(synth, "axxe", 0, synth->bank + synth->location,
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

				if (synth->dispatch[RADIOSET_1].other1 != c)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_1].other1 + MEM_START, &event);
			}
			synth->dispatch[RADIOSET_1].other1 = c;
			synth->bank = c * 10;
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

				if (synth->dispatch[RADIOSET_2].other1 != c)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_2].other1 + MEM_START, &event);
			}
			synth->dispatch[RADIOSET_2].other1 = c;
			/* Do radio buttons */
			synth->location = c - 8;
			break;
	}
	return(0);
}

static int
axxeMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
#warning if we do not check for ack then socket might hang on exit
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

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
axxeCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*printf("axxeCallback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (axxeApp.resources[0].devlocn[index].to == 1.0)
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

static int
axxeAutoTrig(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * This takes the LFO value and sends it to the LFO, and if we have auto
	 * trig configured will also send a value to 126/19 for trigger repeat rate.
	 */
	if (synth->mem.param[26] == 0)
	{
		/*
		 * This is measured in buffer sizes, so it not exactly accurate.
		 *
		 * Fastest rate is going to be 10Hz (1.0), slowest rate 0.1Hz (0.0).
		 */
		bristolMidiSendMsg(fd, chan, 126, 19,
			(int) (synth->mem.param[9] * C_RANGE_MIN_1));
	} else {
		bristolMidiSendMsg(fd, chan, 126, 19, 0);
	}

	bristolMidiSendMsg(fd, chan, 2, 0,
		(int) (synth->mem.param[9] * C_RANGE_MIN_1));
	return(0);
}

static int
axxeTranspose(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
axxeInit(brightonWindow *win)
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

	/*
	 * We need to take a copy here since the value may change as the synth
	 * is drawn
	 */
	if ((initmem = synth->location) == 0)
		initmem = 11;

	synth->win = win;

	printf("Initialise the axxe link to bristol: %p\n", synth->win);

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
	{
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);
	}

	for (i = 0; i < DEVICE_COUNT; i++)
	{
		synth->dispatch[i].routine = axxeMidiSendMsg;
	}

	dispatch[0].controller = 126;
	dispatch[0].operator = 0;

	dispatch[1].controller = 126;
	dispatch[1].operator = 0;
	dispatch[1].routine = (synthRoutine) axxeTranspose;

	dispatch[2].controller = 126;
	dispatch[2].operator = 2;
	dispatch[3].controller = 126;
	dispatch[3].operator = 3;
	dispatch[4].controller = 126;
	dispatch[4].operator = 4;
	dispatch[5].controller = 126;
	dispatch[5].operator = 5;

	dispatch[6].controller = 1;
	dispatch[6].operator = 0;

	dispatch[7].controller = 126;
	dispatch[7].operator = 6;
	dispatch[8].controller = 126;
	dispatch[8].operator = 7;

	dispatch[9].controller = 2;
	dispatch[9].operator = 0;
	dispatch[9].routine = (synthRoutine) axxeAutoTrig;

	dispatch[10].controller = 126;
	dispatch[10].operator = 8;
	dispatch[11].controller = 126;
	dispatch[11].operator = 9;
	dispatch[12].controller = 126;
	dispatch[12].operator = 10;

	dispatch[13].controller = 3;
	dispatch[13].operator = 0;
	dispatch[14].controller = 3;
	dispatch[14].operator = 1;
	dispatch[15].controller = 3;
	dispatch[15].operator = 3;

	dispatch[16].controller = 126;
	dispatch[16].operator = 11;
	dispatch[17].controller = 126;
	dispatch[17].operator = 12;

	dispatch[18].controller = 126;
	dispatch[18].operator = 13;
	dispatch[19].controller = 126;
	dispatch[19].operator = 14;

	dispatch[20].controller = 4;
	dispatch[20].operator = 0;
	dispatch[21].controller = 4;
	dispatch[21].operator = 1;
	dispatch[22].controller = 4;
	dispatch[22].operator = 2;
	dispatch[23].controller = 4;
	dispatch[23].operator = 3;

	dispatch[24].controller = 6;
	dispatch[24].operator = 1;

	dispatch[25].controller = 126;
	dispatch[25].operator = 15;
	dispatch[26].controller = 126;
	dispatch[26].operator = 19;
	dispatch[26].routine = (synthRoutine) axxeAutoTrig;

	dispatch[27].controller = 126;
	dispatch[27].operator = 16;
	dispatch[28].controller = 126;
	dispatch[28].operator = 17;
	dispatch[29].controller = 126;
	dispatch[29].operator = 18;

	dispatch[MEM_START + 0].controller = 17;
	dispatch[MEM_START + 1].controller = 1;
	dispatch[MEM_START + 2].controller = 2;
	dispatch[MEM_START + 3].controller = 3;
	dispatch[MEM_START + 4].controller = 4;
	dispatch[MEM_START + 5].controller = 5;
	dispatch[MEM_START + 6].controller = 6;
	dispatch[MEM_START + 7].controller = 7;
	dispatch[MEM_START + 8].controller = 8;
	dispatch[MEM_START + 9].controller = 9;
	dispatch[MEM_START + 10].controller = 10;
	dispatch[MEM_START + 11].controller = 11;
	dispatch[MEM_START + 12].controller = 12;
	dispatch[MEM_START + 13].controller = 13;
	dispatch[MEM_START + 14].controller = 14;
	dispatch[MEM_START + 15].controller = 15;
	dispatch[MEM_START + 16].controller = 16;
	dispatch[MEM_START + 17].controller = 0;

	dispatch[MEM_START + 0].routine =
		dispatch[MEM_START + 1].routine =
		dispatch[MEM_START + 2].routine =
		dispatch[MEM_START + 3].routine =
		dispatch[MEM_START + 4].routine =
		dispatch[MEM_START + 5].routine =
		dispatch[MEM_START + 6].routine =
		dispatch[MEM_START + 7].routine =
		dispatch[MEM_START + 8].routine =
		dispatch[MEM_START + 9].routine =
		dispatch[MEM_START + 10].routine =
		dispatch[MEM_START + 11].routine =
		dispatch[MEM_START + 12].routine =
		dispatch[MEM_START + 13].routine =
		dispatch[MEM_START + 14].routine =
		dispatch[MEM_START + 15].routine =
		dispatch[MEM_START + 16].routine =
		dispatch[MEM_START + 17].routine
			= (synthRoutine) axxeMemory;

	dispatch[MIDI_START].controller = 1;
	dispatch[MIDI_START + 1].controller = 2;
	dispatch[MIDI_START].routine = dispatch[MIDI_START + 1].routine =
		(synthRoutine) axxeMidi;

	dispatch[RADIOSET_1].other1 = -1;
	dispatch[RADIOSET_2].other1 = -1;

	/*
	 * Grooming env for 'gain' to amp. Short but not choppy attack, slightly
	 * larger decay since the main env can shorten that if really desired.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 0, 20);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 1, 1000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 3, 50);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 5, 0); /* no velocity */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 6, 0); /* no rezero */

	/*
	 * Need to specify env gain fixed, filter mod on, osc waveform.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 6, 16383);

	/* sync LFO to keyon */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, 16383);
	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
axxeConfigure(brightonWindow* win)
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

	synth->bank = 10;
	if (synth->location == 0)
		synth->location = 11;

	synth->flags |= OPERATIONAL;
	synth->keypanel = 1;
	synth->keypanel2 = -1;
	synth->transpose = 36;

	loadMemory(synth, "axxe", 0, initmem, synth->mem.active, 0, 0);

	brightonPut(win,
		"bitmaps/blueprints/axxeshade.xpm", 0, 0, win->width, win->height);

	event.value = 1;
	brightonParamChange(synth->win, synth->panel, MEM_START + 1, &event);
	brightonParamChange(synth->win, synth->panel, MEM_START + 9, &event);

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

