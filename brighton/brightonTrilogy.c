
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
 * This file is just the visible presentation code for the trilogy. The stratus
 * was already architected to support the extra parameterisation for the trilogy
 * so this just now needed a different layout of a few of the parameters.
 *
 * Check String options.
 *
 *	P12 String pan
 *	P13 String harmonics
 *	P14 String spacialisation
 *	P15 String mod level
 *	P16 String TBD
 */

#include "brighton.h"
#include "brightonMini.h"

extern int stratusInit();
extern int stratusConfigure();
extern int stratusCallback(brightonWindow *, int, int, float);
extern int stratusModCallback(brightonWindow *, int, int, float);
extern int stratusMidiCallback(brightonWindow *, int, int, float);

#include "brightonKeys.h"
#include "brightoninternals.h"

#define DEVICE_COUNT 87

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
 * This example is for a trilogyBristol type synth interface.
 */
#define S1 35
#define S2 110
#define S2b 15
#define S2c 80
#define S3 70
#define S4 600

#define BO 10
#define B2 6

#define R1 190
#define R2 560
#define R2a 545
#define R2b 685

#define D1 51
#define D2 43
#define D3 30

#define C1 26
#define C2 (C1 + D1)
#define C3 (C2 + D1)
#define C4 (C3 + D1)
#define C5 (C4 + D1)
#define C6 (C5 + D1)
#define C7 (C6 + D1)
#define C8 (C7 + D1)
#define C9 (C8 + D1)
#define C10 (C9 + D1)
#define C11 (C10 + D1)
#define C12 (C11 + D1)
#define C13 (C12 + D1)
#define C14 (C13 + D1)
#define C15 (C14 + D1)
#define C16 (C15 + D1)
#define C17 (C16 + D1)
#define C18 (C17 + D1)
#define C19 (C18 + D1)
#define C20 (C19 + D1)

static brightonLocations trilogyLocations[DEVICE_COUNT] = {
	/* Organ */
	{"Organ-2'", 1, C1-10, 110, 90, 40, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm", 0,
        BRIGHTON_REVERSE|BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW},
	{"Organ-4'", 1, C1-10, 180, 90, 40, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm", 0,
        BRIGHTON_REVERSE|BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW},
	{"Organ-8'", 1, C1-10, 250, 90, 40, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm", 0,
        BRIGHTON_REVERSE|BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW},
	{"Organ-15'", 1, C1-10, 320, 90, 40, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm", 0,
        BRIGHTON_REVERSE|BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW},

	/* Filter - 4 */
	{"Filter-Env", 0, C3, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"Filter-Cutoff", 0, C4, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Filter-Res", 0, C5, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Filter-Pedal", 0, C6, R1, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},

	/* - 8 */
	{"Glide-Amout", 0, C7, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Glide-Speed", 0, C8, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Glide-Multi", 0, C9, R1, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},
	{"Glide-Dir", 0, C10, R1, S1, S2, 0, 3, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},

	/* - 12 */
	{"Osc1-Tuning", 0, C11, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"Osc1-Sync", 2, C11 + (D2*1), R1, S2b+4, S2-10, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Osc1-Octave", 2, C11 + (D2*2), R1, S2b+4, S2-10, 0, 12, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},

	/* - 15 */
	{"Osc2-Trill", 2, C11 + (D2*4) - 5, R1, S2b+4, S2-10, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Osc2-Octave", 2, C11 + (D2*3) - 3, R1, S2b+4, S2-10, 0, 12, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Osc2-Tuning", 0, C15, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},

	/* - 18 */
	{"LFO-Routing", 0, C16+5, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED|BRIGHTON_NOTCH},
	{"LFO-Muli", 0, C17 + D1 / 2, R1, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},
	{"LFO-Waveform", 0, C19-5, R1, S1, S2, 0, 3, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},

	/* - 21 */
	{"Mix-Synth", 0, C1-5, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Mix-Organ", 0, C2-5, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Mix-String", 0, C3-5, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* - 24 String section */
	{"String-footage", 0, C4, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"String-Timbre", 0, C5, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"String-Attack", 0, C6, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"String-Decay", 0, C7, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* - 28 */
	{"Osc-Waveform", 0, C8, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Osc-WaveAlt", 0, C9, R2, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},
	{"Osc-WaveLegato", 0, C10, R2, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},

	/* - 31 */
	{"LFO-Rate", 0, C16, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-Attack", 0, C17, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-Delay", 0, C18, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-Depth", 0, C19, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* Dummies 12 - 35 */
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, 0, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* These are shadows for the opts parameters - 47 */
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* These are shadows for the envelope parameters from the mod panel - 66 */
	{"Attack", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"Decay", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"Sustain", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"Release", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* Memory - 40 */
	{"", 2, C11+(D3*1), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C11+(D3*2), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C11+(D3*3), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C11+(D3*4), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, C11+(D3*1), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C11+(D3*2), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C11+(D3*3), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C11+(D3*4), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	/* Load and Save */
	{"", 2, C11+(D3*5), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C11+(D3*5), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	/* Bank Sel */
	{"", 2, C11+(D3*0), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* Mem search buttons Down then Up */
	{"", 2, C11+(D3*6), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C11+(D3*6), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	/* Search Free */
	{"", 2, C11+(D3*0), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

/* Midi, perhaps eventually file import/export buttons */
	{"", 2, C11+(D3*7), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C11+(D3*7), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

	{"", 3, C11 + 30, R2a - 90, 165, 70, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}
};

static brightonLocations trilogyOpts[19] = {
	/* First one is index 47 */
	{"", 0, C1, 100, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm",// Master vol 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, C1, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // organ pan
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, C2 - 12, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // o wave
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, C3, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // o space
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C4 - 25, 470, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // o mod
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 0, 285, 32, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // SynHarm
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 53 */
	{"", 2, C1 - 15, 500, 20, 110, 0, 1, 0, // clicky
		"bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_NOSHADOW},

	{"", 0, C1 - 18, 650, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // O tune
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},

	{"", 0, C7, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // syn pan
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, C7+10, 670, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // syn tune
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, 285, 606, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // not used
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* 58 */
	{"", 2, C8, 380, 20, 110, 0, 1, 0,
		"bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_NOSHADOW}, // Env touch
	{"", 2, C9 + 10, 380, 20, 110, 0, 4, 0,
		"bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_NOSHADOW}, // filter type
	{"", 0, C10, 475, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 61 - string options */
	{"", 0, C13+8, 190, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C13+8, 490, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C13+20, 790, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 0, C14+18, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C16-20, 450, S1, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
};

#define TRILOGY_MODCOUNT 7
static brightonLocations trilogyMods[TRILOGY_MODCOUNT] = {
	{"Attack", 1, 440, 180, 80, 700, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Decay", 1, 580, 180, 80, 700, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Sustain", 1, 720, 180, 80, 700, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},
	{"Release", 1, 860, 180, 80, 700, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	/* View Opts button */
	{"", 2, 150, 800, 98, 170, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Joystick */
	{"", 5, 40, 160, 310, 500, 0, 1, 0, "bitmaps/images/sphere.xpm", 
		0, BRIGHTON_WIDE},
	/* Joystick dummy */
	{"", 0, 0, 0, 10, 10, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp trilogyApp = {
	"trilogy",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH, /*flags */
	stratusInit,
	stratusConfigure, /* 3 callbacks, unused? */
	stratusMidiCallback,
	destroySynth,
	{5, 100, 1, 2, 5, 520, 0, 0},
	900, 385, 0, 0,
	7, /* Panel count */
	{
		{
			"TRILOGY",
			"bitmaps/blueprints/trilogy.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* flags */
			0,
			0,
			stratusCallback,
			20, 100, 960, 550,
			DEVICE_COUNT,
			trilogyLocations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			200, 690, 785, 290,
			KEY_COUNT,
			keys
		},
		{
			"Mods",
			"bitmaps/blueprints/trilogymods.xpm",
			"bitmaps/blueprints/trilogymods.xpm",
			BRIGHTON_STRETCH,
			0,
			0,
			stratusModCallback,
			30, 710, 160, 250,
			TRILOGY_MODCOUNT,
			trilogyMods
		},
		{
			"Options",
			"bitmaps/blueprints/trilogyopts.xpm",
			"bitmaps/images/pcb.xpm",
			BRIGHTON_WITHDRAWN,
			0,
			0,
			stratusModCallback,
			25, 105, 950, 540,
			19,
			trilogyOpts
		},
		{
			"Logo",
			"bitmaps/blueprints/trilogylogo.xpm",
			0,
			BRIGHTON_STRETCH, /*flags */
			0,
			0,
			0,
			20, 0, 960, 100,
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
			0, 0, 20, 1000,
			0,
			0
		},
		{
			"Edge",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /*flags */
			0,
			0,
			0,
			980, 0, 20, 1000,
			0,
			0
		},
	}
};

