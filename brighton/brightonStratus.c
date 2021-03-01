
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
 * Filter type selector DONE
 * 
 * Synth Pan DONE
 *
 * Env touch sense - env to filter should not have touch? Maybe it should to
 * add harmonics with touch hence an option. The option goes to the envelope
 * and to the emulator since they have to both recognise when to use touch?
 * DONE TEST
 *
 * Fixed Legato waveforms and waveform selection (alternate osc1/osc2). DONE
 *
 * Fixed Osc2 trill - Osc-1 trill is a function of joystick plus mod routing,
 * Osc-2 trill is a funtion of this button? DONE
 *
 * Fixed glides DONE
 *
 * Fixed pedal DONE
 *
 * Corrected tuning DONE
 *
 * Added options information: DONE
 *
 *	P1 Master volume
 *
 *	P2  Organ pan
 *	P3  Organ waveform distorts
 *	P4  Organ spacialisation
 *	P5  Organ mod level
 *	J1  Organ key grooming 
 *	P6  Organ tuning
 *
 *	P7  Synth pan
 *	P8  Synth tuning
 *	P9  Synth osc1 harmonics
 *	P10 Synth osc2 harmonics
 *	J2  Synth velocity sensitivity
 *	J3  Synth filter type
 *	P11 Synth tracking
 *
 *	P12 String pan
 *	P13 String harmonics
 *	P14 String spacialisation
 *	P15 String mod level
 *	P16 String waveform - dropped?
 *
 * Continuous controllers and joystick DONE
 *
 * Consider the VCO LFO options. Could perhaps be improved however they are 
 * flexible and without more data on the original this will remain. DONE
 *
 * Fix LFO mono/multi for nosync/sync of lfo to key DONE TEST
 *
 * Fix envelope touch sense, emulation touch sense DONE
 *
 * Doublecheck options DONE
 *
 * Fix sync. This could be postponed but can try and look into whether the 
 * noise comes from the syncing waveform or the synced waveform.
 *
 * Added filter keyboard tracking. Reworked tuning needs testing. This could
 * also be postponed however it is not far off.
 */

#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"

static int initmem = 0;

int stratusInit();
int stratusConfigure();
int stratusCallback(brightonWindow *, int, int, float);
/*static int keyCallback(void *, int, int, float); */
int stratusModCallback(brightonWindow *, int, int, float);
int stratusMidiCallback(brightonWindow *, int, int, float);
static void panelSwitch(guiSynth *, int, int, int, int, int);
static void stratusLoadMemory(guiSynth *, int, int, int, int, int);

static int dc;

extern guimain global;
static guimain manual;

#include "brightonKeys.h"
#include "brightoninternals.h"

#define DEVICE_COUNT 87
#define STRATUS_DEVS 47
#define ACTIVE_DEVS 70
#define MEM_START (ACTIVE_DEVS)
#define RADIOSET_1 MEM_START
#define RADIOSET_2 (MEM_START + 1)
#define RADIOSET_3 (MEM_START + 2)
#define DISPLAY_DEV (DEVICE_COUNT - 1)

#define KEY_PANEL 1
#define OPTS_PANEL 3
#define OPTS_OFFSET 47
#define MODS_OFFSET 66

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
 * This example is for a stratusBristol type synth interface.
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

#define R2a 260
#define R2b 380
#define R2c 500
#define R2d 620

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
#define C13 (C12 + D1 + 23)
#define C14 (C13 + D1)
#define C15 (C14 + D1)
#define C16 (C15 + D1)
#define C17 (C16 + D1 - 23)
#define C18 (C17 + D1)
#define C19 (C18 + D1)
#define C20 (C19 + D1)

static brightonLocations stratusLocations[DEVICE_COUNT] = {
	/* Organ */
	{"Organ-2'", 1, C1-10, 110, 90, 40, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW|BRIGHTON_REVERSE},
	{"Organ-4'", 1, C1-10, 180, 90, 40, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW|BRIGHTON_REVERSE},
	{"Organ-8'", 1, C1-10, 250, 90, 40, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW|BRIGHTON_REVERSE},
	{"Organ-16'", 1, C1-10, 320, 90, 40, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm", 0,
        BRIGHTON_VERTICAL|BRIGHTON_NOSHADOW|BRIGHTON_REVERSE},

	/* Filter - 4 */
	{"VCF-Env", 0, C3, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"VCF-Cutoff", 0, C4, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Res", 0, C5, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Pedal", 0, C6, R1, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},

	/* - 8 */
	{"Glide-Amount", 0, C7, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
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
	{"Osc1-Sync", 2, C11 + (D2*1) + 3, R1, S2b+4, S2-10, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Osc1-Octave", 2, C11 + (D2*2), R1, S2b+4, S2-10, 0, 12, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},

	/* - 15 */
	{"Osc1-Trill", 2, C11 + (D2*1) + 3, R2, S2b+4, S2-10, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Osc2-Octave", 2, C11 + (D2*2), R2, S2b+4, S2-10, 0, 12, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"Osc2-Tuning", 0, C11 + (D2*0), R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},

	/* - 18 */
	{"LFO-Routing", 0, C13+10, R1, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED|BRIGHTON_NOTCH},
	{"LFO-Multi", 0, C14 + D1/2 + 3, R1, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},
	{"LFO-Shape", 0, C16-5, R1, S1, S2, 0, 3, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},

	/* - 21 */
	{"Mix-Synth", 0, C1-5, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Mix-Organ", 0, C2-5, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Mix-X", 0, C3-5, 0, 0, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* - 24 String section */
	{"X1", 0, C4, 0, 0, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"X2", 0, C5, 0, 0, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"X3", 0, C6, 0, 0, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"X4", 0, C7, 0, 0, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* - 28 */
	{"Osc-Waveform", 0, C7+10, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},
	{"Osc-WaveAlt", 0, C8 + D1/2 + 3, R2, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},
	{"Osc-WaveLegato", 0, C10-5, R2, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_STEPPED},

	/* - 31 */
	{"LFO-Rate", 0, C13, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-Attack", 0, C14, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-Delay", 0, C15, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-Depth", 0, C16, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* Dummies 12 - 39 */
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
	/* 57 */
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, R2, S1, S2, 0, 1.01, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 2, 0, R2, S1, S2, 0, 4, 0, "bitmaps/knobs/knob4.xpm", 
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
	{"Attack", 0, C3, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Decay", 0, C4, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Sustain", 0, C5, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Release", 0, C6, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knobblue.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

/*
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, 0, R2, S1, S2, 0, 1, 0, "bitmaps/knobs/knob4.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
*/

	/* Memory - 40 */
	{"", 2, C17+(D3*1), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C17+(D3*2), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C17+(D3*3), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C17+(D3*4), R2a, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, C17+(D3*1), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C17+(D3*2), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C17+(D3*3), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C17+(D3*4), R2b, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	/* Load and Save */
	{"", 2, C17+(D3*1), R2c, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C17+(D3*1), R2d, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm",
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	/* Bank Sel */
	{"", 2, C17+(D3*2), R2c, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* Mem search buttons Down then Up */
	{"", 2, C17+(D3*3), R2d, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C17+(D3*3), R2c, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	/* Search Free */
	{"", 2, C17+(D3*2), R2d, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

/* Midi, perhaps eventually file import/export buttons */
	{"", 2, C17+(D3*4), R2d, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C17+(D3*4), R2c, S2b, S2c, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},

	{"", 3, C17 + 30, R2a - 120, 106, 60, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0}
};

/*
 * Need more tuning, there is a global for both layers, then one each for the
 * three components (not synth)
 */
static brightonLocations stratusOpts[19] = {
	/* First one is index 47 */
	{"", 0, C1, 100, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm",// Master vol 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, C1, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // organ pan
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, C2 -4, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // o wave
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, C3 + 10, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // o space
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, C4 - 10, 490, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // o mod
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 0, 312, 30, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // SynHarm
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* 53 */
	{"", 2, C1 - 15, 500, 20, 110, 0, 1, 0, // clicky
		"bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_NOSHADOW},

	{"", 0, C1 - 18, 650, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // O tune
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},

	{"", 0, C8 - 8, 380, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // syn pan
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, C8 - 8, 670, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // syn tune
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
	{"", 0, 312, 620, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", // not used
		"bitmaps/knobs/alpharotary.xpm", 0},
	/* 58 */
	{"", 2, C9 - 3, 380, 20, 110, 0, 1, 0,
		"bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_NOSHADOW}, // Env touch
	{"", 2, C10, 380, 20, 110, 0, 4, 0,
		"bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_NOSHADOW}, // filter type
	{"", 0, C11 - 22, 480, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"", 0, C15, R1, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C16, R1, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C17, R1, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C18, R1, 100, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 0, C19, R1, S1, 100, 0, 1, 0, "bitmaps/knobs/smp.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
};

#define STRATUS_MODCOUNT 7
static brightonLocations stratusMods[STRATUS_MODCOUNT] = {
	{"", 1, 440, 180, 80, 700, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 1, 580, 180, 80, 700, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 1, 720, 180, 80, 700, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
	{"", 1, 860, 180, 80, 700, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},

	/* View Opts button */
	{"", 2, 420, 800, 150, 170, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	/* Joystick - we need to keep the dummy since it dispatches X/Y events */
	{"", 5, 200, 160, 600, 500, 0, 1, 0, "bitmaps/images/sphere.xpm", 
		0, BRIGHTON_WIDE},
	{"", 0, 0, 0, 10, 10, 0, 1, 0, "bitmaps/knobs/sliderblack.xpm",
		0, BRIGHTON_WITHDRAWN|BRIGHTON_NOSHADOW},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp stratusApp = {
	"stratus",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH, /*flags */
	stratusInit,
	stratusConfigure, /* 3 callbacks, unused? */
	stratusMidiCallback,
	destroySynth,
	{5, 100, 1, 2, 5, 520, 0, 0},
	840, 385, 0, 0,
	7, /* Panel count */
	{
		{
			"Stratus",
			"bitmaps/blueprints/stratus.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* flags */
			0,
			0,
			stratusCallback,
			22, 100, 956, 550,
			DEVICE_COUNT,
			stratusLocations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			140, 690, 845, 290,
			KEY_COUNT,
			keys
		},
		{
			"Mods",
			"bitmaps/blueprints/stratusmods.xpm",
			"bitmaps/blueprints/stratusmods.xpm",
			BRIGHTON_STRETCH,
			0,
			0,
			stratusModCallback,
			30, 710, 100, 250,
			STRATUS_MODCOUNT,
			stratusMods
		},
		{
			"Options",
			"bitmaps/blueprints/stratusopts.xpm",
			"bitmaps/images/pcb.xpm",
			BRIGHTON_WITHDRAWN,
			0,
			0,
			stratusModCallback,
			32, 110, 940, 530,
			19,
			stratusOpts
		},
		{
			"Logo",
			"bitmaps/blueprints/stratuslogo.xpm",
			0,
			BRIGHTON_STRETCH, /*flags */
			0,
			0,
			0,
			22, 0, 956, 100,
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
			0, 0, 22, 1000,
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
			978, 0, 22, 1000,
			0,
			0
		},
	}
};

int
stratusMidiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, synth->resources->name, 0, synth->bank + synth->location,
				synth->mem.active, 0, 0);
//			stratusLoadMemory(synth, global.controlfd, synth->midichannel,
//				0, 0, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static void
stratusLoadMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int i;
	brightonEvent event;

	/*
	 * This is now a shim to map the opts and mods, we have to send the events
	 * twice since these controls are in the basic set and the shadow set since
	 * the different emulations have the envelope parameters in differents
	 * panels.
	 */
	event.type = BRISTOL_FLOAT;

	event.value = synth->mem.param[MODS_OFFSET];
	brightonParamChange(synth->win, 0, MODS_OFFSET + 0, &event);
	brightonParamChange(synth->win, 2, 0, &event);

	event.value = synth->mem.param[MODS_OFFSET + 1];
	brightonParamChange(synth->win, 0, MODS_OFFSET + 1, &event);
	brightonParamChange(synth->win, 2, 1, &event);

	event.value = synth->mem.param[MODS_OFFSET + 2];
	brightonParamChange(synth->win, 0, MODS_OFFSET + 2, &event);
	brightonParamChange(synth->win, 2, 2, &event);

	event.value = synth->mem.param[MODS_OFFSET + 3];
	brightonParamChange(synth->win, 0, MODS_OFFSET + 3, &event);
	brightonParamChange(synth->win, 2, 3, &event);

	for (i = 0; i < 19; i++)
	{
		event.type = BRISTOL_FLOAT;
		event.value = synth->mem.param[OPTS_OFFSET + i];

		brightonParamChange(synth->win, 3, i, &event);
	}
}

int
stratusModCallback(brightonWindow *win, int panel, int dev, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue, index;

	if (synth == NULL)
		return(0);

/*
printf("stratusModCallback(%i, %i, %f) %x\n", panel, dev, value,
(size_t) synth);
*/

	/*
	 * If this is controller 0-3 it is the Envelope and we need to bury the
	 * values in our memory and dispatch the request.
	 * Then we have the panel switch and two more parameters which are combined
	 * callbacks for the Joystick X and Y.
	 */
	if (panel == 2) {
		switch (dev) {
			case 0:
			case 1:
			case 2:
			case 3:
				/* Envelope parameters, we need to send these */
				synth->mem.param[MODS_OFFSET + dev] = value;
				if (!global.libtest)
					bristolMidiSendMsg(global.controlfd, synth->sid,
						2, dev, (int) (value * C_RANGE_MIN_1));
				break;
			case 4:
				/* Panelswitch */
				panelSwitch(synth, 0, 0, 0, 0, value);
				break;
			case 5:
				/*
				 * Joystick Y motion, below 0.5 = VCA, above = VCF. We just 
				 * send controller-1 and let the engine decipher it.
				 */
				if (!global.libtest)
					bristolMidiSendControlMsg(global.controlfd,
						synth->midichannel,
						1,
						((int) (value * C_RANGE_MIN_1)) >> 7);
				break;
			case 6:
				/* Joystick X motion - pitchbend */
				if (!global.libtest)
					bristolMidiSendMsg(global.controlfd, synth->midichannel,
						BRISTOL_EVENT_PITCH, 0, (int) (value * C_RANGE_MIN_1));
				break;
		}
		return(0);
	}

	if (stratusApp.resources[panel].devlocn[dev].to == 1.0)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	index = OPTS_OFFSET + dev;

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	synth->mem.param[index] = value;

	if ((!global.libtest) || (index >= ACTIVE_DEVS)) {
		if (synth->dispatch[index].other1 < 0) {
			synth->dispatch[index].routine(synth,
				global.controlfd,
				synth->sid,
				synth->dispatch[index].controller,
				synth->dispatch[index].operator,
				sendvalue);
			synth->dispatch[index].routine(synth,
				global.controlfd,
				synth->sid2,
				synth->dispatch[index].controller,
				synth->dispatch[index].operator,
				sendvalue);
		} else
			synth->dispatch[index].routine(synth,
				global.controlfd,
				synth->dispatch[index].other1, /* The SID is buried here */
				synth->dispatch[index].controller,
				synth->dispatch[index].operator,
				sendvalue);
	} else
		printf("dispatch2[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd,
			synth->dispatch[index].other1, /* The SID is buried here */
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

	return(0);
}

static void
stratusMono(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, c, o, 1 - v);
}

static void
stratusMultiLFO(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * We don't need a shim for the MULTI option but I want one to add 
	 * changes for LFO sync
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 16, v);
	bristolMidiSendMsg(global.controlfd, synth->sid, 5, 1, v);
}

/* Mono Alternate osc A/B */
static void
stratusAlternate(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, c, o, 1 - v);
}

static void
trilogyWaveform(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
printf("trilogy wave %i/%i %i\n", c, o, v);
	 * 16' to 8'
	 */
	if (o == 0) {
		bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 0,
			C_RANGE_MIN_1 - v);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 1, v);
	} else
		bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 12, v);
}

/*
 */
static void
stratusWaveform(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * Mix Ramp to Square on both oscillators
	if (synth->mem.param[13] == 0)
	 */
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 5, C_RANGE_MIN_1-v);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 6, v);
	}
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 5, C_RANGE_MIN_1 - v);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 6, v);
}

static int
stratusMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
stratusMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;
	int bank = synth->bank;
	int location = synth->location;

	event.value = 1.0;
	event.type = BRISTOL_FLOAT;

	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (synth->dispatch[MEM_START].other2)
	{
		synth->dispatch[MEM_START].other2 = 0;
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
			if (synth->dispatch[MEM_START].other1 != -1)
			{
				synth->dispatch[MEM_START].other2 = 1;

				if (synth->dispatch[MEM_START].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[MEM_START].other1 + MEM_START - 1, &event);
			}
			synth->dispatch[MEM_START].other1 = o;

			if (synth->flags & BANK_SELECT) {
				if ((synth->bank * 10 + o) >= 100)
				{
					synth->location = o;
					synth->flags &= ~BANK_SELECT;

					if (loadMemory(synth, synth->resources->name, 0,
						synth->bank * 10 + synth->location, synth->mem.active,
							0, BRISTOL_STAT) < 0)
						displayText(synth, "FREE",
							synth->bank * 10 + synth->location, DISPLAY_DEV);
					else
						displayText(synth, "PROG",
							synth->bank * 10 + synth->location, DISPLAY_DEV);
				} else {
					synth->bank = synth->bank * 10 + o;
					displayText(synth, "BANK",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
				}
			} else {
				if (synth->bank < 1)
					synth->bank = 1;
				synth->location = o;
				if (loadMemory(synth, synth->resources->name, 0,
					synth->bank * 10 + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FREE",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
				else
					displayText(synth, "PROG",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
			}
			break;
		case 1:
			if (synth->bank < 1)
				synth->bank = 1;
			if (synth->location == 0)
				synth->location = 1;
			if (loadMemory(synth, synth->resources->name, 0, synth->bank * 10 + synth->location,
				synth->mem.active, 0, BRISTOL_FORCE) < 0)
				displayText(synth, "FREE",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
			else
				displayText(synth, "PROG",
					synth->bank * 10 + synth->location, DISPLAY_DEV);

			if (brightonDoubleClick(dc))
				stratusLoadMemory(synth, global.controlfd, synth->midichannel,
					0, 0, 0);
			synth->flags &= ~BANK_SELECT;
			break;
		case 2:
			if (synth->bank < 1)
				synth->bank = 1;
			if (synth->location == 0)
				synth->location = 1;
			if (brightonDoubleClick(dc)) {
				saveMemory(synth, synth->resources->name, 0,
					synth->bank * 10 + synth->location, 0);
				displayText(synth, "PROG",
					synth->bank * 10 + synth->location, DISPLAY_DEV);
			}
			synth->flags &= ~BANK_SELECT;
			break;
		case 3:
			if (synth->flags & BANK_SELECT) {
				synth->flags &= ~BANK_SELECT;
				if (loadMemory(synth, synth->resources->name, 0,
					synth->bank * 10 + synth->location, synth->mem.active,
						0, BRISTOL_STAT) < 0)
					displayText(synth, "FREE",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
				else
					displayText(synth, "PROG",
						synth->bank * 10 + synth->location, DISPLAY_DEV);
			} else {
				synth->bank = 0;
				displayText(synth, "BANK", synth->bank, DISPLAY_DEV);
				synth->flags |= BANK_SELECT;
			}
			break;
		case 4:
			if (--location < 1) {
				location = 8;
				if (--bank < 1)
					bank = 88;
			}
			while (loadMemory(synth, synth->resources->name, 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_STAT) < 0)
			{
				if (--location < 1) {
					location = 8;
					if (--bank < 1)
						bank = 88;
				}
				if ((bank * 10 + location)
					== (synth->bank * 10 + synth->location))
					break;
			}
			displayText(synth, "PROG", bank * 10 + location, DISPLAY_DEV);
			synth->bank = bank;
			synth->location = location;
			loadMemory(synth, synth->resources->name, 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_FORCE);
//			stratusLoadMemory(synth, global.controlfd, synth->midichannel,
//				0, 0, 0);
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			break;
		case 5:
			if (++location > 8) {
				location = 1;
				if (++bank > 88)
					bank = 1;
			}
			while (loadMemory(synth, synth->resources->name, 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_STAT) < 0)
			{
				if (++location > 8) {
					location = 1;
					if (++bank > 88)
						bank = 1;
				}
				if ((bank * 10 + location)
					== (synth->bank * 10 + synth->location))
					break;
			}
			displayText(synth, "PROG", bank * 10 + location, DISPLAY_DEV);
			synth->bank = bank;
			synth->location = location;
			loadMemory(synth, synth->resources->name, 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_FORCE);
//			stratusLoadMemory(synth, global.controlfd, synth->midichannel,
//				0, 0, 0);
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			break;
		case 6:
			/* Find the next free mem */
			if (++location > 8) {
				location = 1;
				if (++bank > 88)
					bank = 1;
			}

			while (loadMemory(synth, synth->resources->name, 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_STAT) >= 0)
			{
				if (++location > 8) {
					location = 1;
					if (++bank > 88)
						bank = 1;
				}
				if ((bank * 10 + location)
					== (synth->bank * 10 + synth->location))
					break;
			}

			if (loadMemory(synth, synth->resources->name, 0, bank * 10 + location,
				synth->mem.active, 0, BRISTOL_STAT) >= 0)
				displayText(synth, "PROG",
					bank * 10 + location, DISPLAY_DEV);
			else
				displayText(synth, "FREE",
					bank * 10 + location, DISPLAY_DEV);

			synth->bank = bank;
			synth->location = location;
			brightonParamChange(synth->win, 0,
				MEM_START - 1 + synth->location, &event);
			break;
	}
/*	printf("	stratusMemory(B: %i L %i: %i)\n", */
/*		synth->bank, synth->location, o); */
}

static void
stratusFilter(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * We need a shim since changing the filter type means we have to push
	 * the parameters again.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, v);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 0,
		(int) (synth->mem.param[5] * C_RANGE_MIN_1));
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1,
		(int) (synth->mem.param[6] * C_RANGE_MIN_1));
}

static void
stratusEnvelope(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * Send touch to envelope and reverse to emulator to decide who controls
	 * response.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 5, v);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 27, v);
}

static void
trilogyStringHarmonics(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 0, C_RANGE_MIN_1 - v);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 2, v>>1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 3, v);
}

static void
stratusMod(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid2, c, 5, v);
	bristolMidiSendMsg(global.controlfd, synth->sid2, c, 6, v);
}

static void
stratusOscHarmonics(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, c, 0, C_RANGE_MIN_1 - v);
	bristolMidiSendMsg(global.controlfd, synth->sid, c, 2, v>>1);
	bristolMidiSendMsg(global.controlfd, synth->sid, c, 3, v);
}

static void
stratusOrganEnv(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/* Clicky or not */
	if (v == 0) {
		bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 0, 40);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 1, 4096);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 2, 16383);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 3, 10);
	} else {
		bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 0, 20);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 1, 2000);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 2, 2000);
		bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 3, 10);
	}
}

static int
stratusMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY_DEV);
			return(0);
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 16)
		{
			synth->midichannel = 15;
			displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY_DEV);
			return(0);
		}
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

/*	printf("P: going to display: %x, %x\n", synth, synth->win); */
	displayText(synth, "CHAN", synth->midichannel + 1, DISPLAY_DEV);

	return(0);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
int
stratusCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (stratusApp.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	/* We need to keep a hole for the shadowed opts */
	if ((index < ACTIVE_DEVS - 4) && (index >= STRATUS_DEVS))
		return(0);

	synth->mem.param[index] = value;

	if ((!global.libtest) || (index >= ACTIVE_DEVS))
	{
		if (synth->dispatch[index].other1 < 0) {
			synth->dispatch[index].routine(synth,
				global.controlfd,
				synth->sid,
				synth->dispatch[index].controller,
				synth->dispatch[index].operator,
				sendvalue);
			synth->dispatch[index].routine(synth,
				global.controlfd,
				synth->sid2,
				synth->dispatch[index].controller,
				synth->dispatch[index].operator,
				sendvalue);
		} else
			synth->dispatch[index].routine(synth,
				global.controlfd,
				synth->dispatch[index].other1, /* The SID is buried here */
				synth->dispatch[index].controller,
				synth->dispatch[index].operator,
				sendvalue);
	} else
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd,
			synth->dispatch[index].other1, /* The SID is buried here */
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);

	return(0);
}

static int shade_id;

void
panelSwitch(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	brightonEvent event;

	/* 
	 * If the sendvalue is zero, then withdraw the opts window, draw the
	 * slider window, and vice versa.
	 */
	if (value == 0)
	{
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(synth->win, 3, -1, &event);
		event.intvalue = 1;
		brightonParamChange(synth->win, 0, -1, &event);

		if (strstr(synth->resources->name, "stratus") != 0)
			shade_id = brightonPut(synth->win,
				"bitmaps/blueprints/stratusshade.xpm",
				0, 0, synth->win->width, synth->win->height);
		else if (strstr(synth->resources->name, "trilogy") != 0)
			shade_id = brightonPut(synth->win,
				"bitmaps/blueprints/trilogyshade.xpm",
				0, 0, synth->win->width, synth->win->height);
	} else {
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(synth->win, 0, -1, &event);
		event.intvalue = 1;
		brightonParamChange(synth->win, 3, -1, &event);

		brightonRemove(synth->win, shade_id);
	}
}

/*
 * We are going to start two synths here, the organ and string sections will
 * be one emulator with as many voices as the engine can handle since they are
 * going to be lightweight and were divider circuits anyway. The synth section
 * is then separate with a limited number of voices.
 */
int
stratusInit(brightonWindow *win)
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

	if ((initmem = synth->location) == 0)
		initmem = 11;

	synth->win = win;

	printf("Initialise the strilogy link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;

	if (synth->voices == BRISTOL_VOICECOUNT)
		synth->voices = 6;

	synth->dispatch = (dispatcher *)
		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher));
	dispatch = synth->dispatch;

	synth->synthtype = BRISTOL_TRILOGY;
	synth->second = brightonmalloc(sizeof(guiSynth));
	bcopy(synth, ((guiSynth *) synth->second), sizeof(guiSynth));
	((guiSynth *) synth->second)->mem.param =
		(float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	((guiSynth *) synth->second)->mem.count = DEVICE_COUNT;
	((guiSynth *) synth->second)->mem.active = ACTIVE_DEVS;
	((guiSynth *) synth->second)->dispatch = synth->dispatch;

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets.
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
	{
		int voices = synth->voices;

		printf("Initialise the strilogy synth circuits\n");
		bcopy(&global, &manual, sizeof(guimain));
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

		/*
		 * We have to tweak the voicecount as we use the same synth definition
		 * to request the second layer
		 */
		manual.flags |= BRISTOL_CONN_FORCE|BRIGHTON_NOENGINE;
		manual.host = global.host;
		manual.port = global.port;

		printf("Initialise the strilogy organ divider circuits\n");
		synth->synthtype = BRISTOL_TRILOGY_ODC;
		synth->voices = BRISTOL_VOICECOUNT;
		if ((synth->sid2 = initConnection(&manual, synth)) < 0)
			return(-1);

		global.manualfd = manual.controlfd;
		global.manual = &manual;
		manual.manual = &global;
		/*
		 * Having played with the synth type to get the correct emulator we now
		 * have to put it back to the actual model so that the GUI will load
		 * the correct profiles.
		 */
		if (strcmp("stratus", synth->resources->name) == 0)
			synth->synthtype = BRISTOL_STRATUS;
		else
			synth->synthtype = BRISTOL_TRILOGY;
		synth->voices = voices;
	}

	for (i = 0; i < DEVICE_COUNT; i++) {
		synth->dispatch[i].routine = stratusMidiSendMsg;

		dispatch[i].controller = 126;
		dispatch[i].operator = 101;
		dispatch[i].other1 = synth->sid;
	}

	dispatch[0].controller = 0;
	dispatch[0].operator = 3;
	dispatch[0].other1 = synth->sid2;
	dispatch[1].controller = 0;
	dispatch[1].operator = 2;
	dispatch[1].other1 = synth->sid2;
	dispatch[2].controller = 0;
	dispatch[2].operator = 1;
	dispatch[2].other1 = synth->sid2;
	dispatch[3].controller = 0;
	dispatch[3].operator = 0;
	dispatch[3].other1 = synth->sid2;

	/* Filter env, c, e */
	dispatch[4].controller = 126;
	dispatch[4].operator = 10;
	dispatch[5].controller = 3;
	dispatch[5].operator = 0;
	dispatch[6].controller = 3;
	dispatch[6].operator = 1;
	dispatch[7].controller = 126; /* Pedal/Manual filter */
	dispatch[7].operator = 26;

	dispatch[8].controller = 126;
	dispatch[8].operator = 22;
	dispatch[9].controller = 126;
	dispatch[9].operator = 23;
	dispatch[10].controller = 126;
	dispatch[10].operator = 24;
	dispatch[11].controller = 126;
	dispatch[11].operator = 25;

	/* VCO 1 - tune, sync-2, octave */
	dispatch[12].controller = 0;
	dispatch[12].operator = 9;
	dispatch[13].controller = 126;
	dispatch[13].operator = 11; /* Sync */
	dispatch[14].controller = 0;
	dispatch[14].operator = 8;
	/* VCO 2 - tune, sync-2, octave */
	dispatch[15].controller = 126; /* This mixes LFO into osc-2 */
	dispatch[15].operator = 12;
	dispatch[16].controller = 1;
	dispatch[16].operator = 8;
	dispatch[17].controller = 1;
	dispatch[17].operator = 9;

	/* LFO Routing, multi, shape */
	dispatch[18].controller = 126;
	dispatch[18].operator = 15;
	dispatch[19].controller = 126;
	dispatch[19].operator = 16;
	dispatch[19].routine = (synthRoutine) stratusMultiLFO;
	dispatch[20].controller = 126;
	dispatch[20].operator = 17;

	/* Gains */
	dispatch[21].controller = 126;
	dispatch[21].operator = 9;
	dispatch[22].controller = 126;
	dispatch[22].operator = 5;
	dispatch[22].other1 = synth->sid2;
	dispatch[23].controller = 126;
	dispatch[23].operator = 8;
	dispatch[23].other1 = synth->sid2;

	/* Trilogy string section */
	dispatch[24].controller = 2;
	dispatch[24].operator = 0;
	dispatch[24].other1 = synth->sid2;
	dispatch[24].routine = (synthRoutine) trilogyWaveform;
	dispatch[25].controller = 2;
	dispatch[25].operator = 1;
	dispatch[25].other1 = synth->sid2;
	dispatch[25].routine = (synthRoutine) trilogyWaveform;
	/* Env */
	dispatch[26].controller = 3;
	dispatch[26].operator = 0;
	dispatch[26].other1 = synth->sid2;
	dispatch[27].controller = 3;
	dispatch[27].operator = 3;
	dispatch[27].other1 = synth->sid2;

	/* Waveform */
	dispatch[28].routine = (synthRoutine) stratusWaveform;
	dispatch[29].controller = 126;
	dispatch[29].operator = 13;
	dispatch[29].routine = (synthRoutine) stratusAlternate;
	dispatch[30].controller = 126;
	dispatch[30].operator = 14;
	dispatch[30].routine = (synthRoutine) stratusMono;

	/*
	 * LFO May need to be shimmed - if the LFO were not the same index in the
	 * two different synths then this would not have worked.
	 */
	dispatch[31].controller = 5;
	dispatch[31].operator = 0;
	dispatch[31].other1 = -1;
	dispatch[32].controller = 126;
	dispatch[32].operator = 18;
	dispatch[32].other1 = -1;
	dispatch[33].controller = 126;
	dispatch[33].operator = 19;
	dispatch[33].other1 = -1;
	dispatch[34].controller = 126;
	dispatch[34].operator = 20;
	dispatch[34].other1 = -1;

	/* Organ options */
	dispatch[47].controller = 126;
	dispatch[47].operator = 1;
	dispatch[47].other1 = -1;
	dispatch[48].controller = 126; /* Organ panning */
	dispatch[48].operator = 3;
	dispatch[48].other1 = synth->sid2;
	dispatch[49].controller = 0; /* Waveform */
	dispatch[49].operator = 4;
	dispatch[49].other1 = synth->sid2;
	dispatch[50].controller = 126; /* Space */
	dispatch[50].operator = 4;
	dispatch[50].other1 = synth->sid2;
	dispatch[51].controller = 0; /* Modulation */
	dispatch[51].operator = 6;
	dispatch[51].other1 = synth->sid2;
	dispatch[51].routine = (synthRoutine) stratusMod;
	/* Synth harmonics */
	dispatch[52].controller = 0;
	dispatch[52].routine = (synthRoutine) stratusOscHarmonics;
	/* Organ env */
	dispatch[53].routine = (synthRoutine) stratusOrganEnv;
	dispatch[54].controller = 126; /* ODC Tuning */
	dispatch[54].operator = 2;
	dispatch[54].other1 = synth->sid2;
	dispatch[55].controller = 126; /* Synth Pan */
	dispatch[55].operator = 21;
	dispatch[56].controller = 126; /* Synth Tuning? */
	dispatch[56].operator = 2;
	dispatch[57].controller = 1;
	dispatch[57].routine = (synthRoutine) stratusOscHarmonics;
	dispatch[58].controller = 2; /* Env touch */
	dispatch[58].operator = 5;
	dispatch[58].routine = (synthRoutine) stratusEnvelope;
	dispatch[59].controller = 3; /* Filter Type */
	dispatch[59].operator = 4;
	dispatch[59].routine = (synthRoutine) stratusFilter;
	dispatch[60].controller = 3; /* Filter Tracking */
	dispatch[60].operator = 3;
	/* String options */
	dispatch[61].controller = 126; /* String panning */
	dispatch[61].operator = 6;
	dispatch[61].other1 = synth->sid2;
	dispatch[62].routine = (synthRoutine) trilogyStringHarmonics;
	dispatch[63].controller = 126; /* String spacialisation */
	dispatch[63].operator = 7;
	dispatch[63].other1 = synth->sid2;
	dispatch[64].controller = 2; /* String mod */
	dispatch[64].operator = 6;
	dispatch[64].other1 = synth->sid2;
	dispatch[64].routine = (synthRoutine) stratusMod;
	dispatch[65].controller = 2; /* String Waveform */
	dispatch[65].operator = 4;
	dispatch[65].other1 = synth->sid2;

	/* Synth env */
	dispatch[66].controller = 2;
	dispatch[66].operator = 0;
	dispatch[67].controller = 2;
	dispatch[67].operator = 1;
	dispatch[68].controller = 2;
	dispatch[68].operator = 2;
	dispatch[69].controller = 2;
	dispatch[69].operator = 3;

	/* Memory and MIDI */
	dispatch[MEM_START + 0].operator = 1;
	dispatch[MEM_START + 1].operator = 2;
	dispatch[MEM_START + 2].operator = 3;
	dispatch[MEM_START + 3].operator = 4;
	dispatch[MEM_START + 4].operator = 5;
	dispatch[MEM_START + 5].operator = 6;
	dispatch[MEM_START + 6].operator = 7;
	dispatch[MEM_START + 7].operator = 8;

	dispatch[MEM_START + 8].controller = 1;
	dispatch[MEM_START + 9].controller = 2;
	dispatch[MEM_START + 10].controller = 3;
	dispatch[MEM_START + 11].controller = 4;
	dispatch[MEM_START + 12].controller = 5;
	dispatch[MEM_START + 13].controller = 6;

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
		dispatch[MEM_START + 13].routine = (synthRoutine) stratusMemory;

	dispatch[DEVICE_COUNT - 3].routine = dispatch[DEVICE_COUNT - 2].routine =
		(synthRoutine) stratusMidi;
	dispatch[DEVICE_COUNT - 3].controller = 1;
	dispatch[DEVICE_COUNT - 2].controller = 2;

	dispatch[MEM_START].other1 = -1;

	/* Organ oscillator */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 1, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 4, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 5, 0);
	/* Pulse signal level and width */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 7, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 8, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 9, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 11, 16383);

	/* ODC grooming envelope */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 0, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 1, 100);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 2, 6000);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 3, 500);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 4, 16383);
	/* Velocity tracking envelope. Default is off */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 5, 0);

	/* String oscillator */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 0, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 1, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 3, 16383);
	/* Load it up on a sawtooth although this is now a parameter */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 5, 0);
	/* Pulse signal level and width */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 7, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 8, 12);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 9, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 11, 16383);
	/* String grooming ASR envelope */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 1, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 5, 1);

	/* Filter mod, tracking and type */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);

	/* Synth envelope */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 4, 16383);

	/*
	 * Envelope is not touch sensitive, this is moved into the gain stage
	 * since the same filter is used for the filter and amp
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 5, 0);
	
	/* Osc harmonics - should add control */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 1, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, 8192);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
int
stratusConfigure(brightonWindow *win)
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

	synth->flags |= OPERATIONAL;
	synth->keypanel = 1;
	synth->keypanel2 = -1;
	synth->transpose = 24;

	synth->bank = initmem / 10;
	synth->location = initmem % 10;

	loadMemory(synth, synth->resources->name, 0, initmem, synth->mem.active, 0,
		BRISTOL_FORCE);
	stratusLoadMemory(synth, global.controlfd, synth->midichannel, 0,
		initmem, 0);

	event.value = 1.0;
	brightonParamChange(synth->win, synth->panel,
		MEM_START + synth->location - 1, &event);

	if (strstr(synth->resources->name, "stratus") != NULL)
		shade_id = brightonPut(win, "bitmaps/blueprints/stratusshade.xpm",
			0, 0, win->width, win->height);
	else if (strstr(synth->resources->name, "trilogy") != NULL)
		shade_id = brightonPut(win, "bitmaps/blueprints/trilogyshade.xpm",
			0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
//	brightonParamChange(synth->win, 0, -1, &event);
	configureGlobals(synth);

	dc = brightonGetDCTimer(win->dcTimeout);

	return(0);
}

