
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

static int junoInit();
static int junoConfigure();
static int junoCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define FIRST_DEV 0
#define DEVICE_COUNT (56 + FIRST_DEV)
#define ACTIVE_DEVS (39 + FIRST_DEV)
#define DISPLAY_DEV (DEVICE_COUNT - 1)
#define MEM_MGT ACTIVE_DEVS
#define MIDI_MGT (MEM_MGT + 12)
#define KEY_TRANSPOSE (FIRST_DEV + 6)
#define KEY_HOLD (FIRST_DEV + 7)
#define CHORUS (FIRST_DEV + 32)

#define KEY_PANEL 1

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
 * This example is for a junoBristol type synth interface.
 */
#define D3 80
#define BR1 300
#define BR2 BR1 + D3
#define BR3 BR2 + D3
#define BR4 BR3 + D3

#define MBR2 BR4
#define MBR1 MBR2 - 100
#define MBR3 MBR2 + 100
#define MBR4 MBR3 + 100

#define R1 200
#define R2 275
#define R3 450
#define R4 650

#define D1 15
#define D2 31

#define C1 10
#define C2 C1 + D2 - 1
#define C3 C2 + D1 + 1
#define C4 C3 + D2 - 7
#define C5 C4 + D2 + 4
#define C6 C5 + D1 + 2
/* Need to insert a few for the arpeg */
#define C6_1 C6 + D2 - 6
#define C6_2 C6_1 + D2 - 4
#define C6_3 C6_2 + D2 - 2
#define C6_4 C6_3 + D2 - 1
#define C7 275
#define C8 C7 + D1
#define C9 C8 + D2 - 9
#define C10 C9 + D2 - 6
#define C11 C10 + D1 + 4
#define C12 C11 + D2 - 12
#define C13 C12 + D1 + D2 + 4 /* text space */
#define C14 C13 + 15
#define C15 C14 + 15
#define C16 C15 + D2 + 3
#define C17 C16 + D1

#define C18 C17 + D2 + 2 /* HPF */
#define C19 C18 + D2 + 5
#define C20 C19 + D1 + 1
#define C21 C20 + D2 - 6
#define C22 C21 + D2 - 10
#define C23 C22 + D1 + 2
#define C24 C23 + D1 + 3

#define C25 C24 + D2 + 5
#define C26 C25 + D1 - 2

#define C27 C26 + D2 - 3
#define C28 C27 + D1 + 4
#define C29 C28 + D1 + 2
#define C30 C29 + D1 + 3

#define C31 C30 + D2
#define C32 C31 + 15
#define C33 C32 + 16

#define C34 C33 + D2 + D1 - 13
#define C35 C34 + D1 + 3
#define C36 C35 + D1 + 3
#define C37 C36 + D1 + 3
#define C38 C37 + D1 + 3

#define W1 14
#define L1 485

#define S2 120

#define BS1 17
#define BS2 100
#define BS3 115
#define BS4 12
#define BS5 160

static brightonLocations locations[DEVICE_COUNT] = {
	/* Power, etc */
	{"On/Off", 2, C1, BR2, 18, BS5, 0, 1, 0,
		"bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_NOSHADOW},
	{"DCO Mod", 1, C2, R2, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack4.xpm", 0, BRIGHTON_HALFSHADOW},
	{"VCF Mod", 1, C3, R2, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack4.xpm", 0, BRIGHTON_HALFSHADOW},
	{"Tuning", 0, C4, R2, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob1.xpm", 0, BRIGHTON_NOTCH},
	{"Glide", 0, C4, R3 + 20, S2, S2, 0, 1, 0, 
		"bitmaps/knobs/knob1.xpm", 0, 0},
	{"LFO Manual", 2, C4 + 3, R4, BS1, BS2 + 40, 0, 1, 0,
		"bitmaps/buttons/touchoff.xpm",
		"bitmaps/buttons/touch.xpm", BRIGHTON_NOSHADOW},
	/* Transpose - 6 */
	{"Transpose", 2, C5 + 1, BR2 + 30, 12, 180, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"Hold", 2, C6 + 2, BR2, BS1, BS5, 0, 1, 0,
		"bitmaps/buttons/touchoy.xpm",
		"bitmaps/buttons/touchy.xpm", BRIGHTON_NOSHADOW},
	{"LFO Rate", 1, C7, R2, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack4.xpm", 0, BRIGHTON_HALFSHADOW},
	{"LFO Delay", 1, C8, R2, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack4.xpm", 0, BRIGHTON_HALFSHADOW},
	{"Man/Auto", 2, C9, BR2 + 70, BS4, BS3, 0, 1, 0,
		"bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	/* DCO - 11 */
	{"DCO-LFO Mod", 1, C10, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"PWM", 1, C11, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"PWM Source", 2, C12 + 1, BR2 + 30, 12, 180, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"Pulse", 2, C13 - 7, BR2, BS1, BS5, 0, 1, 0,
		"bitmaps/buttons/touchow.xpm",
		"bitmaps/buttons/touchw.xpm", BRIGHTON_NOSHADOW},
	{"Ramp", 2, C14 - 3, BR2, BS1, BS5, 0, 1, 0,
		"bitmaps/buttons/touchoy.xpm",
		"bitmaps/buttons/touchy.xpm", BRIGHTON_NOSHADOW},
	{"Square", 2, C15 + 2, BR2, BS1, BS5, 0, 1, 0,
		"bitmaps/buttons/touchoo.xpm",
		"bitmaps/buttons/toucho.xpm", BRIGHTON_NOSHADOW},
	{"SubOsc", 1, C16, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Noise", 1, C17, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	/* hpf - 19 */
	{"HPF", 1, C18, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	/* filter - 20 */
	{"Filter Freq", 1, C19, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Resonance", 1, C20, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Env +/-", 2, C21, BR2 + 70, BS4, BS3, 0, 1, 0,
		"bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"FilterEnvMod", 1, C22, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"FilterLFOMod", 1, C23, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Filter KBD", 1, C24, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	/* VCA - 26  */
	{"Amp Env/Gate", 2, C25 - 10, BR2 + 70, BS4, BS3, 0, 1, 0,
		"bitmaps/buttons/sw1.xpm",
		"bitmaps/buttons/sw2.xpm", BRIGHTON_NOSHADOW},
	{"Amp Level", 1, C26, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	/* adsr - 28 */
	{"Attack", 1, C27, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Decay", 1, C28, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Sustain", 1, C29, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	{"Release", 1, C30, R2, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW},
	/* chorus - 32 */
	{"Chorus 1", 2, C31 - 9, BR2, BS1, BS5, 0, 1, 0,
		"bitmaps/buttons/touchow.xpm",
		"bitmaps/buttons/touchw.xpm", BRIGHTON_NOSHADOW},
	{"Chorus 2", 2, C32 - 4, BR2, BS1, BS5, 0, 2, 0,
		"bitmaps/buttons/touchoy.xpm",
		"bitmaps/buttons/touchy.xpm", BRIGHTON_NOSHADOW},
	{"Chorus 3", 2, C33, BR2, BS1, BS5, 0, 4, 0,
		"bitmaps/buttons/touchoo.xpm",
		"bitmaps/buttons/toucho.xpm", BRIGHTON_NOSHADOW},
	/* Arpeggiator - 35 */
	{"Arpeg On", 2, C6_1, BR2, BS1, BS5, 0, 1, 0,
		"bitmaps/buttons/touchoo.xpm",
		"bitmaps/buttons/toucho.xpm", BRIGHTON_NOSHADOW},
	{"Arpeg UpDown", 2, C6_2, BR2 + 30, 12, 180, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"Arpeg Range", 2, C6_3, BR2 + 30, 12, 180, 0, 2, 0,
		0, 0, BRIGHTON_THREEWAY|BRIGHTON_NOSHADOW},
	{"Arpeg Rate", 1, C6_4, R2, W1, L1, 0, 1, 0,
		"bitmaps/knobs/sliderblack4.xpm", 0, 
		BRIGHTON_HALFSHADOW|BRIGHTON_REVERSE},
/* memory tablet */
	{"", 2, C34, MBR2, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C35, MBR2, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C36, MBR2, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C37, MBR2, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C38, MBR2, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C34, MBR3, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C35, MBR3, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C36, MBR3, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C37, MBR3, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C38, MBR3, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlw.xpm",
		"bitmaps/buttons/touchnlw.xpm", 0},
	{"", 2, C37, MBR1, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnl.xpm",
		"bitmaps/buttons/touchnl.xpm", 0},
	{"", 2, C38, MBR1, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnlo.xpm",
		"bitmaps/buttons/touchnlo.xpm", 0},
	{"", 2, C34, MBR1, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnl.xpm",
		"bitmaps/buttons/touchnl.xpm", 0},
	{"", 2, C35, MBR1, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnl.xpm",
		"bitmaps/buttons/touchnl.xpm", 0},
	{"", 2, C34, MBR4, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnl.xpm",
		"bitmaps/buttons/touchnl.xpm", 0},
	{"", 2, C38, MBR4, BS1, BS2, 0, 1, 0,
		"bitmaps/buttons/touchnl.xpm",
		"bitmaps/buttons/touchnl.xpm", 0},
	{"", 3, C34 - 5, R1, 100, BS2, 0, 1, 0, 0,
		"bitmaps/images/alphadisplay3.xpm", 0},
};

/*
 */

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp junoApp = {
	"juno",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal4.xpm",
	BRIGHTON_STRETCH,
	junoInit,
	junoConfigure, /* 3 callbacks, unused? */
	midiCallback,
	destroySynth,
	{6, 100, 1, 2, 5, 520, 0, 0},
	820, 250, 0, 0,
	5, /* panels */
	{
		{
			"Juno",
			"bitmaps/blueprints/juno.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			junoCallback,
			25, 20, 953, 550,
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
			114, 562, 889, 425,
			KEY_COUNT,
			//keys
			keysprofile2
		},
		{
			"Mods",
			0, //"bitmaps/blueprints/mods.xpm",
			"bitmaps/keys/vkbg.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			modCallback,
			25, 562, 90, 410,
			2,
			mods
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			2, 5, 22, 985,
			0,
			0
		},
		{
			"Wood",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			979, 5, 21, 985,
			0,
			0
		},
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

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
			if (loadMemory(synth, "juno", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY_DEV);
			else
				displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			synth->location = (synth->location % 10) + value * 10;
			if (loadMemory(synth, "juno", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY_DEV);
			else
				displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
	}
	return(0);
}

static int
junoMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
/*printf("%i, %i, %i\n", c, o, v); */
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
junoMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if (synth->flags & MEM_LOADING)
		return;
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	switch (c) {
		default:
		case 0:
			synth->location = synth->location * 10 + o;

			if (synth->location >= 1000)
				synth->location = o;
			if (loadMemory(synth, "juno", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY_DEV);
			else
				displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
		case 1:
			if (loadMemory(synth, "juno", 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayText(synth, "FRE", synth->location, DISPLAY_DEV);
			else
				displayText(synth, "PRG", synth->bank + synth->location,
					DISPLAY_DEV);
			break;
		case 2:
			saveMemory(synth, "juno", 0, synth->bank + synth->location,
				FIRST_DEV);
			displayText(synth, "PRG", synth->bank + synth->location,
				DISPLAY_DEV);
			break;
		case 3:
			while (loadMemory(synth, "juno", 0, --synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location < 0)
					synth->location = 1000;
			}
			displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
		case 4:
			while (loadMemory(synth, "juno", 0, ++synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location > 999)
					synth->location = -1;
			}
			displayText(synth, "PRG", synth->location, DISPLAY_DEV);
			break;
	}
}

static void
junoMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int newchan;

	if ((synth->flags & OPERATIONAL) == 0)
		return;

	if (c == 1) {
		if ((newchan = synth->midichannel - 1) < 0)
		{
			synth->midichannel = 0;
			return;
		}
	} else {
		if ((newchan = synth->midichannel + 1) >= 15)
		{
			synth->midichannel = 15;
			return;
		}
	}

	if (global.libtest == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);
	}

	synth->midichannel = newchan;

	displayText(synth, "MIDI", synth->midichannel + 1, DISPLAY_DEV);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
junoCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	/*printf("junoCallback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (junoApp.resources[0].devlocn[index].to == 1)
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
#define DEBUG
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

static void
junoChorus(guiSynth *synth)
{
	int value = 0;

	if (synth->mem.param[CHORUS])
		value++;
	if (synth->mem.param[CHORUS + 1])
		value+=2;
	if (synth->mem.param[CHORUS + 2])
		value+=4;

	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 0, value);
}

static void
junoSublevel(guiSynth *synth)
{
	/*printf("junoSublevel %f %f\n", synth->mem.param[16], synth->mem.param[17]); */

	if (synth->mem.param[16] == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid,
			1, 6, 0);
		return;
	}

	bristolMidiSendMsg(global.controlfd, synth->sid,
		1, 6, ((int) (synth->mem.param[17] * C_RANGE_MIN_1)));
}

static void
junoTranspose(guiSynth *synth)
{
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	//printf("junoTranspose %f\n", synth->mem.param[KEY_TRANSPOSE]);

	bristolMidiSendMsg(global.controlfd, synth->sid, 127, 0,
		BRISTOL_TRANSPOSE + (int) (synth->mem.param[KEY_TRANSPOSE] * 12) - 12);

/*
	if (synth->mem.param[KEY_TRANSPOSE] == 0)
		synth->transpose = 24;
	else if (synth->mem.param[KEY_TRANSPOSE] == 1)
			synth->transpose = 36;
	else
		synth->transpose = 48;
*/
}

static void
junoHold(guiSynth *synth)
{
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	/*printf("junoHold %x\n", synth); */

	if (synth->mem.param[KEY_HOLD])
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|1);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|0);

}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
junoInit(brightonWindow *win)
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

	printf("Initialise the juno link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].routine = junoMidiSendMsg;

	dispatch[FIRST_DEV].controller = 126;
	dispatch[FIRST_DEV + 0].operator = 17;
	dispatch[FIRST_DEV + 1].controller = 126;
	dispatch[FIRST_DEV + 1].operator = 9;
	dispatch[FIRST_DEV + 2].controller = 126;
	dispatch[FIRST_DEV + 2].operator = 16;
	dispatch[FIRST_DEV + 3].controller = 126;
	dispatch[FIRST_DEV + 3].operator = 1;
	dispatch[FIRST_DEV + 4].controller = 126;
	dispatch[FIRST_DEV + 4].operator = 0;
	dispatch[FIRST_DEV + 5].controller = 126;
	dispatch[FIRST_DEV + 5].operator = 3;

	dispatch[FIRST_DEV + 6].routine = (synthRoutine) junoTranspose;
	dispatch[FIRST_DEV + 7].routine = (synthRoutine) junoHold;

	dispatch[FIRST_DEV + 8].controller = 0;
	dispatch[FIRST_DEV + 8].operator = 2;
	dispatch[FIRST_DEV + 9].controller = 7;
	dispatch[FIRST_DEV + 9].operator = 0;
	dispatch[FIRST_DEV + 10].controller = 126;
	dispatch[FIRST_DEV + 10].operator = 4;
	dispatch[FIRST_DEV + 11].controller = 126;
	dispatch[FIRST_DEV + 11].operator = 5;
	dispatch[FIRST_DEV + 12].controller = 126;
	dispatch[FIRST_DEV + 12].operator = 7;
	dispatch[FIRST_DEV + 13].controller = 126;
	dispatch[FIRST_DEV + 13].operator = 6;

	dispatch[FIRST_DEV + 14].controller = 1;
	dispatch[FIRST_DEV + 14].operator = 7;
	dispatch[FIRST_DEV + 15].controller = 1;
	dispatch[FIRST_DEV + 15].operator = 4;
	dispatch[FIRST_DEV + 16].controller = 1;
	dispatch[FIRST_DEV + 16].operator = 6;
	dispatch[FIRST_DEV + 17].controller = 1;
	dispatch[FIRST_DEV + 17].operator = 6;
	dispatch[FIRST_DEV + 16].routine = dispatch[FIRST_DEV + 17].routine
		= (synthRoutine) junoSublevel;
	dispatch[FIRST_DEV + 18].controller = 6;
	dispatch[FIRST_DEV + 18].operator = 0;

	dispatch[FIRST_DEV + 19].controller = 2;
	dispatch[FIRST_DEV + 19].operator = 0;

	dispatch[FIRST_DEV + 20].controller = 3;
	dispatch[FIRST_DEV + 20].operator = 0;
	dispatch[FIRST_DEV + 21].controller = 3;
	dispatch[FIRST_DEV + 21].operator = 1;
	dispatch[FIRST_DEV + 22].controller = 126;
	dispatch[FIRST_DEV + 22].operator = 11;
	dispatch[FIRST_DEV + 23].controller = 126;
	dispatch[FIRST_DEV + 23].operator = 12;
	dispatch[FIRST_DEV + 24].controller = 126;
	dispatch[FIRST_DEV + 24].operator = 13;
	dispatch[FIRST_DEV + 25].controller = 3;
	dispatch[FIRST_DEV + 25].operator = 3;

	dispatch[FIRST_DEV + 26].controller = 126;
	dispatch[FIRST_DEV + 26].operator = 8;
	dispatch[FIRST_DEV + 27].controller = 126;
	dispatch[FIRST_DEV + 27].operator = 15;

	dispatch[FIRST_DEV + 28].controller = 4;
	dispatch[FIRST_DEV + 28].operator = 0;
	dispatch[FIRST_DEV + 29].controller = 4;
	dispatch[FIRST_DEV + 29].operator = 1;
	dispatch[FIRST_DEV + 30].controller = 4;
	dispatch[FIRST_DEV + 30].operator = 2;
	dispatch[FIRST_DEV + 31].controller = 4;
	dispatch[FIRST_DEV + 31].operator = 3;

	dispatch[FIRST_DEV + 32].controller = 100;
	dispatch[FIRST_DEV + 32].operator = 0;
	dispatch[FIRST_DEV + 33].controller = 100;
	dispatch[FIRST_DEV + 33].operator = 0;
	dispatch[FIRST_DEV + 34].controller = 100;
	dispatch[FIRST_DEV + 34].operator = 0;
	dispatch[CHORUS].routine = dispatch[CHORUS + 1].routine
		= dispatch[CHORUS + 2].routine
		= (synthRoutine) junoChorus;

	dispatch[FIRST_DEV + 35].controller = BRISTOL_ARPEGGIATOR;
	dispatch[FIRST_DEV + 35].operator = BRISTOL_ARPEG_ENABLE;
	/* The next two may need shimming due to damaged values. */
	dispatch[FIRST_DEV + 36].controller = BRISTOL_ARPEGGIATOR;
	dispatch[FIRST_DEV + 36].operator = BRISTOL_ARPEG_DIR;
	dispatch[FIRST_DEV + 37].controller = BRISTOL_ARPEGGIATOR;
	dispatch[FIRST_DEV + 37].operator = BRISTOL_ARPEG_RANGE;
	dispatch[FIRST_DEV + 38].controller = BRISTOL_ARPEGGIATOR;
	dispatch[FIRST_DEV + 38].operator = BRISTOL_ARPEG_RATE;

	/* Memory management */
	dispatch[MEM_MGT].operator = 0;
	dispatch[MEM_MGT + 1].operator = 1;
	dispatch[MEM_MGT + 2].operator = 2;
	dispatch[MEM_MGT + 3].operator = 3;
	dispatch[MEM_MGT + 4].operator = 4;
	dispatch[MEM_MGT + 5].operator = 5;
	dispatch[MEM_MGT + 6].operator = 6;
	dispatch[MEM_MGT + 7].operator = 7;
	dispatch[MEM_MGT + 8].operator = 8;
	dispatch[MEM_MGT + 9].operator = 9;

	dispatch[MEM_MGT + 10].controller = 1;
	dispatch[MEM_MGT + 11].controller = 2;
	dispatch[MEM_MGT + 14].controller = 3;
	dispatch[MEM_MGT + 15].controller = 4;

	dispatch[MEM_MGT].routine = dispatch[MEM_MGT + 1].routine =
		dispatch[MEM_MGT + 2].routine = dispatch[MEM_MGT + 3].routine =
		dispatch[MEM_MGT + 4].routine = dispatch[MEM_MGT + 5].routine =
		dispatch[MEM_MGT + 6].routine = dispatch[MEM_MGT + 7].routine =
		dispatch[MEM_MGT + 8].routine = dispatch[MEM_MGT + 9].routine =
		dispatch[MEM_MGT + 10].routine = dispatch[MEM_MGT + 11].routine =
		dispatch[MEM_MGT + 14].routine = dispatch[MEM_MGT + 15].routine
			= (synthRoutine) junoMemory;

	/* Midi management */
	dispatch[MIDI_MGT].routine = dispatch[MIDI_MGT + 1].routine =
		(synthRoutine) junoMidi;
	dispatch[MIDI_MGT].controller = 2;
	dispatch[MIDI_MGT + 1].controller = 1;

	/* Filter type */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 4);
	/*bristol paramchange 126 17 1 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 17, 1);
	/*after 500 bristol paramchange 7 1 12000 */
	/*after 500 bristol paramchange 7 2 16383 */
	/*after 500 bristol paramchange 7 3 0 */
	/*after 500 bristol paramchange 7 4 16383 */
	/*after 500 bristol paramchange 7 5 0 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 1, 12000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 3, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 4, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 7, 5, 0);

	bristolMidiSendMsg(global.controlfd, synth->sid, BRISTOL_ARPEGGIATOR,
		BRISTOL_ARPEG_TRIGGER, 1);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
junoConfigure(brightonWindow *win)
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
	loadMemory(synth, "juno", 0, synth->location, synth->mem.active,
		FIRST_DEV, 0);

	brightonPut(win,
		"bitmaps/blueprints/junoshade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	return(0);
}

