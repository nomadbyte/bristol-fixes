
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

static int initmem;

static int obxInit();
static int obxConfigure();
static int obxCallback(brightonWindow *, int, int, float);
/*static int keyCallback(void *, int, int, float); */
static int obxModCallback(brightonWindow *, int, int, float);
static int obxMidiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"
#include "brightoninternals.h"

#define FIRST_DEV 0
#define DEVICE_COUNT 65
#define ACTIVE_DEVS 43
#define OBX_DEVS 46 /* Have to account for the manual options. */
#define MEM_START (OBX_DEVS - 1)
#define RADIOSET_1 MEM_START
#define RADIOSET_2 (MEM_START + 1)
#define RADIOSET_3 (MEM_START + 2)
#define DISPLAY_DEV (DEVICE_COUNT - 1)

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
 * This example is for a obxBristol type synth interface.
 */
#define S1 110
#define S2 17
#define S3 70

#define BO 10
#define B2 6

#define R1 200
#define R2 430
#define R3 600
#define R3b 600
#define R4 780

#define C1 50
#define C2 66
#define C3 100

#define C4 175

#define C5 242
#define C6 300
#define C7 354

#define C8 415
#define C9 475
#define C10 535

#define C11 590
#define C12 650
#define C13 710

#define C11b 750
#define C12b 800
#define C13b 850

#define C14 775
#define C15 825
#define C16 875
#define C17 925

/* memories */
#define MD 23
#define C20 (C8 + MD + MD/2)
#define C21 (C20 + MD)
#define C22 (C21 + MD)
#define C23 (C22 + MD)
#define C24 (C23 + MD)
#define C25 (C24 + MD)
#define C26 (C25 + MD)
#define C27 (C26 + MD)

#define C28 (C27 + MD + MD/2)
#define C29 (C28 + MD)
#define C30 (C29 + MD)
#define C31 (C30 + MD)
#define C32 (C31 + MD)
#define C33 (C32 + MD)
#define C34 (C33 + MD)
#define C35 (C34 + MD)

#define C36 (C35 + MD + MD/2)

static brightonLocations locations[DEVICE_COUNT] = {
/* 0 - control */
	{"Glide", 0, C4, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Unison", 2, C4 + B2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc2-Detune", 0, C4, R3b, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
/* 3 - mods */
	{"LFO-Freq", 0, C5, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-Sine", 2, C5 + B2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"LFO-Square", 2, C5 + B2, R3, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"LFO-S/H", 2, C5 + B2, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"LFO-FM-Depth", 0, C6, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-FM-Osc1", 2, C6 + B2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"LFO-FM-Osc2", 2, C6 + B2, R3, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"LFO-FM-Filt", 2, C6 + B2, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},

	{"LFO-PWM-Depth", 0, C7, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"LFO-PWM-Osc1", 2, C7 + B2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"LFO-PWM-Osc2", 2, C7 + B2, R3, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* 14 - oscillators: */
	{"Osc1-Transpose", 0, C8, R1, S1, S1, 0, 5, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Osc-PW", 0, C9, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Osc2-Tune", 0, C10, R1, S1, S1, 0, 47, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Osc1-Saw", 2, C8 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc1-PW-CLI!", 2, C8 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc-CrossMod", 2, C9 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc-Sync", 2, C9 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc1-Saw", 2, C10 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"Osc1-Pulse", 2, C10 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* 23 - filter */
	{"VCF-Cutoff", 0, C11, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Resonance", 0, C12, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-ModLevel", 0, C13, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"MixOsc1", 2, C11 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"VCF-KBD", 2, C11 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"MixOsc2-Half", 2, C12 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"MixOsc2-Full", 2, C12 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"MixNoise-Half", 2, C13 - 8, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"MixNoise-Full", 2, C13 + 17, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* 32 - envelope */
	{"VCF-Attack", 0, C14, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Decay", 0, C15, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Sustain", 0, C16, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCF-Release", 0, C17, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	{"VCA-Attack", 0, C14, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCA-Decay", 0, C15, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCA-Sustain", 0, C16, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"VCA-Release", 0, C17, R2, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},

	/* According to the original, these are not saved. Volume will be, and */
	/* reset should be.. */
	{"MasterVolume", 0, C2 + BO, R1, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"Reset", 2, C3 + BO, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/* Total 42 parameters. */
	{"Auto", 2, C1, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C2 + BO * 3/2, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
/*	{"", 2, C3 + BO, R2, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", */
/*		"bitmaps/buttons/presson.xpm", 0}, */
	{"", 0, C2 + BO, R3b, S1, S1, 0, 1, 0, "bitmaps/knobs/knob5.xpm", 
		"bitmaps/knobs/alpharotary.xpm", BRIGHTON_NOTCH},
/*46 */
	/* Programmer */
	{"", 2, C8, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, C20, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C21, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C22, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C23, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C24, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C25, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C26, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C27, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C28, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C29, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C30, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C31, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C32, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C33, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C34, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},
	{"", 2, C35, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_RADIOBUTTON},

	{"", 2, C36, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},

	{"", 2, C17 - 10, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, C17 + MD, R4, S2, S3, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
};

#define OBX_MODCOUNT 5
static brightonLocations obxMods[OBX_MODCOUNT] = {
	{"", 1, 385, 200, 100, 468, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, BRIGHTON_CENTER},
	{"", 1, 520, 200, 100, 468, 0, 1, 0,
		"bitmaps/knobs/sliderblack.xpm", 0, 0},

	{"", 2, 180, 800, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 450, 800, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, 690, 800, 130, 140, 0, 1, 0, "bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0}
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp obxApp = {
	"obx",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal6.xpm",
	BRIGHTON_STRETCH, /*flags */
	obxInit,
	obxConfigure, /* 3 callbacks, unused? */
	obxMidiCallback,
	destroySynth,
	{5, 100, 2, 2, 5, 520, 0, 0},
	770, 350, 0, 0,
	6, /* Panel count */
	{
		{
			"OBX",
			"bitmaps/blueprints/obx.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* flags */
			0,
			0,
			obxCallback,
			14, 130, 970, 520,
			DEVICE_COUNT,
			locations
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			155, 690, 854, 290,
			KEY_COUNT,
			keys
		},
		{
			"Mods",
			"bitmaps/blueprints/obxmod.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0,
			0,
			0,
			obxModCallback, /* Needs to be obxModCallback */
			17, 690, 136, 290,
			OBX_MODCOUNT,
			obxMods
		},
		{
			"Logo",
			"bitmaps/blueprints/obxlogo.xpm",
			0,
			BRIGHTON_STRETCH, /*flags */
			0,
			0,
			0,
			15, 0, 970, 130,
			0,
			0
		},
		{
			"Edge",
			0,
			"bitmaps/images/rhodes.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 15, 1000,
			0,
			0
		},
		{
			"Edge",
			0,
			"bitmaps/images/rhodes.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /*flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
			0,
			0
		},
	}
};

static int
obxKeyLoadMemory(guiSynth *synth, char *algo, char *name, int location,
	int active, int skip, int flags)
{
	brightonEvent event;

	synth->flags |= MEM_LOADING;

	loadMemory(synth, "obx", 0, location, synth->mem.active, FIRST_DEV, 0);

	/*
	 * We now need to force the PW of the two oscillators?
	 */
	bristolMidiSendMsg(synth->sid, synth->midichannel,
		0, 0, (int) synth->mem.param[15]);
	bristolMidiSendMsg(synth->sid, synth->midichannel,
		1, 0, (int) synth->mem.param[ACTIVE_DEVS-1]);

	event.value = 0;
	brightonParamChange(synth->win, 0, ACTIVE_DEVS-1, &event);

	synth->flags &= ~MEM_LOADING;
	return(0);
}

static void
obxLoadMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	synth->flags |= MEM_LOADING;

	loadMemory(synth, "obx", 0, synth->bank * 10 + synth->location,
		synth->mem.active, FIRST_DEV, 0);

	/*
	 * We now need to force the PW of the two oscillators?
	 */
	bristolMidiSendMsg(fd, chan, 0, 0, (int) synth->mem.param[15]);
	bristolMidiSendMsg(fd, chan, 1, 0, (int) synth->mem.param[ACTIVE_DEVS-1]);

	event.value = 0;
	brightonParamChange(synth->win, 0, ACTIVE_DEVS-1, &event);

	synth->flags &= ~MEM_LOADING;
}

static int
obxMidiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", controller, value);

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
/*			loadMemory(synth, "obx", 0, synth->bank + synth->location, */
/*				synth->mem.active, FIRST_DEV, 0); */
			obxLoadMemory(synth, global.controlfd, synth->midichannel, 0, 0, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int narrow = 0;
static int osc2 = 0;

static int
obxModCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/* printf("obxModCallback(%x, %i, %i, %f)\n", synth, panel, index, value); */

	/*
	 * If this is controller 0 it is the frequency control, otherwise a 
	 * generic controller 1.
	 */
	switch (index) {
		case 0:
			if (narrow != 0)
				sendvalue = C_RANGE_MIN_1 / 2 +
					(int) (((value - 0.5) / 3) * C_RANGE_MIN_1);
			else
				sendvalue = (int) ((value * C_RANGE_MIN_1));
			if (osc2 == 0)
				bristolMidiSendMsg(global.controlfd, synth->sid, 0, 11,
					sendvalue);
			bristolMidiSendMsg(global.controlfd, synth->sid, 1, 11, sendvalue);
			break;
		case 1:
			/*
			 * Not sure why this should work, and it probably doesn't. It is
			 * the 'mod' controller but appears to affect Osc-1 tranpose
			 */
			if (narrow != 0)
				/*sendvalue = (int) ((value * C_RANGE_MIN_1)) >> 3; */
				sendvalue = C_RANGE_MIN_1 / 2 +
					(int) (((value - 0.5) / 3) * C_RANGE_MIN_1);
			else
				sendvalue = (int) ((value * C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->midichannel,
				2, 0, sendvalue);
			break;
		case 2: /* Narrow */
			if (value != 0.0)
				narrow = 4;
			else
				narrow = 0;
			break;
		case 3: /* Osc2 */
			/*
			 * This switch is a bit of a pain, it controls whether Osc-2 is
			 * affected by the pitchbend. This means we have to use another
			 * modulation method for case 0 above since the existing method
			 * is global. This is painful as it means we need another param
			 * on the prophetdco.....
			 * May remap the button for single/multi LFO?
			bristolMidiSendMsg(global.controlfd, synth->sid,
				126, 22, (int) value);
			 */
			osc2 = value;
			break;
		case 4: /* Transpose */
			if (value != 0.0)
				synth->transpose = 48;
			else
				synth->transpose = 36;
			break;
	}
	return(0);
}

#define KEY_HOLD (FIRST_DEV + 43)

static void
obxHold(guiSynth *synth)
{
	if ((synth->flags & OPERATIONAL) == 0)
		return;

	/*printf("obxHold %x\n", synth); */

	if (synth->mem.param[KEY_HOLD])
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|1);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_HOLD|0);
}

static int dreset = 0;
static int d1 = 0;
static int d2 = 0;

/*
 * If controller #41 has value zero then we configure the release rates, else
 * we use the shortest release rate.
 */
static void
obxDecay(void *synth, int fd, int chan, int c, int o, int v)
{
	if (c == 41)
	{
		if (v == 0)
		{
			/*
			 * Set values to their current settings
			 */
			dreset = 0;
			bristolMidiSendMsg(fd, chan, 3, 3, d1);
			bristolMidiSendMsg(fd, chan, 5, 3, d2);
		} else {
			/*
			 * Set values to minimum release time
			 */
			dreset = 1;
			bristolMidiSendMsg(fd, chan, 3, 3, 20);
			bristolMidiSendMsg(fd, chan, 5, 3, 20);
		}
	} else {
		if (dreset == 0)
			bristolMidiSendMsg(fd, chan, c, o, v);

		if (c == 3)
			d1 = v;
		else
			d2 = v;
	}
}

static int pwSelect = 0;
/*
 * We need to see if for any given oscillator none of the wave buttons are
 * selected. If this is the case then select triangle. Else disable tri and
 * configure the selected values.
 */
static void
obxWaveform(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * See if this is the PW parameter.
	 */
	if (c == 15)
	{
		if (synth->flags & MEM_LOADING)
			return;
		synth->mem.param[15] = v;
		if (pwSelect == 0) {
			bristolMidiSendMsg(fd, chan, 0, 0, v);
		} else {
			synth->mem.param[ACTIVE_DEVS - 1] = v;
			bristolMidiSendMsg(fd, chan, 1, 0, v);
		}
		return;
	}

	/*
	 * See if we need to set PW selector
	 */
	if (o == 6)
	{
		/*
		 * If the sqr is being selected, then PW will modify this oscillator,
		 * and if going off then it will cotrol the other
		 */
		if (v != 0)
			pwSelect = c;
		else
			pwSelect = 1 - c;
	}

	/*
	 * If the value is '1' we can just set the desired wave and disable tri.
	 */
	if (v != 0)
	{
		bristolMidiSendMsg(fd, chan, c, o, v);
		bristolMidiSendMsg(fd, chan, c, 5, 0);
	} else {
		bristolMidiSendMsg(fd, chan, c, o, v);
		if (c == 0)
		{
			if ((synth->mem.param[17] == 0) &&
				(synth->mem.param[18] == 0))
				bristolMidiSendMsg(fd, chan, c, 5, 1);
		} else {
			if ((synth->mem.param[21] == 0) &&
				(synth->mem.param[22] == 0))
				bristolMidiSendMsg(fd, chan, c, 5, 1);
		}
	}
}

static int
obxMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

static void
multiTune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

	if (synth->flags & MEM_LOADING)
		return;

	/*
	 * Configures all the tune settings to zero (ie, 0.5).
	 * We have master tune, id 44 and Osc-2 detune, id 2.
	 */
	event.type = BRIGHTON_FLOAT;
	event.value = 0.5;
	brightonParamChange(synth->win, synth->panel, FIRST_DEV + 2,
		&event);
	brightonParamChange(synth->win, synth->panel, FIRST_DEV + 44,
		&event);
}

/*
 * The OB-X does not have a lot of memories, just 8 banks of 8 memories. This
 * is a bit sparse for the emulater but it keeps the interface easy. Will be
 * two sets of buttons, one for the bank, one for the index, then load and 
 * save. Load and Save will be intermittent and the banks radio buttons.
 */
static void
obxMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	brightonEvent event;

/*printf("obxMemory(%i, %i, %i, %i)\n", chan, c, o, v); */

	switch (c) {
		case 16:
			/* Load */
			/*
			synth->flags |= MEM_LOADING;
			loadMemory(synth, "obx", 0, synth->bank * 10 + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			 * We now need to force the PW of the two oscillators?
			bristolMidiSendMsg(fd, chan, 0, 0,
				(int) synth->mem.param[15]);
			bristolMidiSendMsg(fd, chan, 1, 0,
				(int) synth->mem.param[ACTIVE_DEVS-1]);

			event.value = 0;
			brightonParamChange(synth->win, 0, ACTIVE_DEVS-1, &event);

			synth->flags &= ~MEM_LOADING;
			 */
			obxLoadMemory(synth, fd, chan, c, o, v);
			break;
		case 17:
			/* Save */
			saveMemory(synth, "obx", 0, synth->bank * 10 + synth->location, 0);
			break;
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			if (synth->dispatch[RADIOSET_1].other2)
			{
				synth->dispatch[RADIOSET_1].other2 = 0;
				return;
			}

			if (synth->dispatch[RADIOSET_1].other1 != -1)
			{
				synth->dispatch[RADIOSET_1].other2 = 1;

				if (synth->dispatch[RADIOSET_1].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_1].other1, &event);
			}
			synth->dispatch[RADIOSET_1].other1 = o;

			synth->bank = c + 1;

			break;
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			if (synth->dispatch[RADIOSET_2].other2)
			{
				synth->dispatch[RADIOSET_2].other2 = 0;
				return;
			}

			if (synth->dispatch[RADIOSET_2].other1 != -1)
			{
				synth->dispatch[RADIOSET_2].other2 = 1;

				if (synth->dispatch[RADIOSET_2].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				brightonParamChange(synth->win, synth->panel,
					synth->dispatch[RADIOSET_2].other1, &event);
			}
			synth->dispatch[RADIOSET_2].other1 = o;

			synth->location = c - 7;

			break;
	}

/*printf("	obxMemory(B: %i, L: %i, %i, %i)\n", */
/*synth->bank, synth->location, synth->bank * 8 + synth->location, o); */
}

static void
obxMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
		if ((newchan = synth->midichannel + 1) >= 16)
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
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
obxCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/* printf("obxCallback(%i, %f): %x\n", index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (obxApp.resources[0].devlocn[index].to == 1.0)
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

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
obxInit(brightonWindow *win)
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

	printf("Initialise the obx link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].routine = obxMidiSendMsg;

	/* Glide and Unison, done */
	dispatch[FIRST_DEV].controller = 126;
	dispatch[FIRST_DEV].operator = 0;
	dispatch[FIRST_DEV + 1].controller = 126;
	dispatch[FIRST_DEV + 1].operator = 1;
	dispatch[FIRST_DEV + 2].controller = 1;
	dispatch[FIRST_DEV + 2].operator = 10;

	/* LFO - operator 2 parameters - Done */
	dispatch[FIRST_DEV + 3].controller = 2;
	dispatch[FIRST_DEV + 3].operator = 0;
	dispatch[FIRST_DEV + 4].controller = 126;
	dispatch[FIRST_DEV + 4].operator = 19;
	dispatch[FIRST_DEV + 5].controller = 126;
	dispatch[FIRST_DEV + 5].operator = 20;
	/* S/H - this should be a routing switch. */
	dispatch[FIRST_DEV + 6].controller = 126;
	dispatch[FIRST_DEV + 6].operator = 21;

	/* Depth Osc/Osc/Filt - routing control/switches */
	dispatch[FIRST_DEV + 7].controller = 126;
	dispatch[FIRST_DEV + 7].operator = 4;
	dispatch[FIRST_DEV + 8].controller = 126;
	dispatch[FIRST_DEV + 8].operator = 5;
	dispatch[FIRST_DEV + 9].controller = 126;
	dispatch[FIRST_DEV + 9].operator = 6;
	dispatch[FIRST_DEV + 10].controller = 126;
	dispatch[FIRST_DEV + 10].operator = 7;

	/* PWM Osc/Osc - routing control/switches */
	dispatch[FIRST_DEV + 11].controller = 126;
	dispatch[FIRST_DEV + 11].operator = 8;
	dispatch[FIRST_DEV + 12].controller = 126;
	dispatch[FIRST_DEV + 12].operator = 9;
	dispatch[FIRST_DEV + 13].controller = 126;
	dispatch[FIRST_DEV + 13].operator = 10;

	/* Osc - Some routing switches. */
	dispatch[FIRST_DEV + 14].controller = 0;
	dispatch[FIRST_DEV + 14].operator = 1;
	/* PWM - needs to adjust both oscillators, so need a dispatcher. */
	dispatch[FIRST_DEV + 15].controller = 15;
	dispatch[FIRST_DEV + 15].operator = 1;
	dispatch[FIRST_DEV + 16].controller = 1;
	dispatch[FIRST_DEV + 16].operator = 9;
	/* Saw/Square. May need dispatch for tri when both off */
	dispatch[FIRST_DEV + 17].controller = 0;
	dispatch[FIRST_DEV + 17].operator = 4;
	dispatch[FIRST_DEV + 18].controller = 0;
	dispatch[FIRST_DEV + 18].operator = 6;
	/* Crossmod and sync - routing switches */
	dispatch[FIRST_DEV + 19].controller = 126;
	dispatch[FIRST_DEV + 19].operator = 11;
	dispatch[FIRST_DEV + 20].controller = 1;
	dispatch[FIRST_DEV + 20].operator = 7;
	/* Saw/Square. May need dispatch for tri when both off */
	dispatch[FIRST_DEV + 21].controller = 1;
	dispatch[FIRST_DEV + 21].operator = 4;
	dispatch[FIRST_DEV + 22].controller = 1;
	dispatch[FIRST_DEV + 22].operator = 6;
	/* Osc waveforms need a dispatcher to enable tri with both buttons off. */
	dispatch[FIRST_DEV + 15].routine =
		dispatch[FIRST_DEV + 17].routine =
		dispatch[FIRST_DEV + 18].routine =
		dispatch[FIRST_DEV + 21].routine =
		dispatch[FIRST_DEV + 22].routine =
			(synthRoutine) obxWaveform;

	/* Filter - some routing switches */
	dispatch[FIRST_DEV + 23].controller = 4;
	dispatch[FIRST_DEV + 23].operator = 0;
	dispatch[FIRST_DEV + 24].controller = 4;
	dispatch[FIRST_DEV + 24].operator = 1;
	dispatch[FIRST_DEV + 25].controller = 4;
	dispatch[FIRST_DEV + 25].operator = 2;
	/* Routing switches. Need to check on KBD tracking though */
	dispatch[FIRST_DEV + 26].controller = 126;
	dispatch[FIRST_DEV + 26].operator = 12;
	dispatch[FIRST_DEV + 27].controller = 4;
	dispatch[FIRST_DEV + 27].operator = 3;
	dispatch[FIRST_DEV + 28].controller = 126;
	dispatch[FIRST_DEV + 28].operator = 14;
	dispatch[FIRST_DEV + 29].controller = 126;
	dispatch[FIRST_DEV + 29].operator = 15;
	dispatch[FIRST_DEV + 30].controller = 126;
	dispatch[FIRST_DEV + 30].operator = 16;
	dispatch[FIRST_DEV + 31].controller = 126;
	dispatch[FIRST_DEV + 31].operator = 17;

	/* Envelopes */
	dispatch[FIRST_DEV + 32].controller = 3;
	dispatch[FIRST_DEV + 32].operator = 0;
	dispatch[FIRST_DEV + 33].controller = 3;
	dispatch[FIRST_DEV + 33].operator = 1;
	dispatch[FIRST_DEV + 34].controller = 3;
	dispatch[FIRST_DEV + 34].operator = 2;
	dispatch[FIRST_DEV + 35].controller = 3;
	dispatch[FIRST_DEV + 35].operator = 3;
	dispatch[FIRST_DEV + 36].controller = 5;
	dispatch[FIRST_DEV + 36].operator = 0;
	dispatch[FIRST_DEV + 37].controller = 5;
	dispatch[FIRST_DEV + 37].operator = 1;
	dispatch[FIRST_DEV + 38].controller = 5;
	dispatch[FIRST_DEV + 38].operator = 2;
	dispatch[FIRST_DEV + 39].controller = 5;
	dispatch[FIRST_DEV + 39].operator = 3;
	/* We need a separate dispatcher for the decay, since the reset button */
	/* will affect the value. */
	dispatch[FIRST_DEV + 35].routine =
		dispatch[FIRST_DEV + 39].routine =
			(synthRoutine) obxDecay;

	/* Master volume - gain of envelope? */
	dispatch[FIRST_DEV + 40].controller = 5;
	dispatch[FIRST_DEV + 40].operator = 4;
	/* Reset of 'release' DONE */
	dispatch[FIRST_DEV + 41].controller = 41;
	dispatch[FIRST_DEV + 41].operator = 0;
	dispatch[FIRST_DEV + 41].routine =
		(synthRoutine) obxDecay;
	/* Autotune */
	dispatch[FIRST_DEV + 42].routine = (synthRoutine) multiTune;
	/* Hold - DONE (stole from Juno.....) */
	dispatch[FIRST_DEV + 43].routine = (synthRoutine) obxHold;
	/* Master tune - Done. */
	dispatch[FIRST_DEV + 44].controller = 126;
	dispatch[FIRST_DEV + 44].operator = 2;

/* Memories. */
	dispatch[MEM_START + 0].controller = 16;
	dispatch[MEM_START + 1].controller = 0;
	dispatch[MEM_START + 2].controller = 1;
	dispatch[MEM_START + 3].controller = 2;
	dispatch[MEM_START + 4].controller = 3;
	dispatch[MEM_START + 5].controller = 4;
	dispatch[MEM_START + 6].controller = 5;
	dispatch[MEM_START + 7].controller = 6;
	dispatch[MEM_START + 8].controller = 7;
	dispatch[MEM_START + 9].controller = 8;
	dispatch[MEM_START + 10].controller = 9;
	dispatch[MEM_START + 11].controller = 10;
	dispatch[MEM_START + 12].controller = 11;
	dispatch[MEM_START + 13].controller = 12;
	dispatch[MEM_START + 14].controller = 13;
	dispatch[MEM_START + 15].controller = 14;
	dispatch[MEM_START + 16].controller = 15;
	dispatch[MEM_START + 17].controller = 17;

	dispatch[MEM_START + 0].operator = MEM_START + 0;
	dispatch[MEM_START + 1].operator = MEM_START + 1;
	dispatch[MEM_START + 2].operator = MEM_START + 2;
	dispatch[MEM_START + 3].operator = MEM_START + 3;
	dispatch[MEM_START + 4].operator = MEM_START + 4;
	dispatch[MEM_START + 5].operator = MEM_START + 5;
	dispatch[MEM_START + 6].operator = MEM_START + 6;
	dispatch[MEM_START + 7].operator = MEM_START + 7;
	dispatch[MEM_START + 8].operator = MEM_START + 8;
	dispatch[MEM_START + 9].operator = MEM_START + 9;
	dispatch[MEM_START + 10].operator = MEM_START + 10;
	dispatch[MEM_START + 11].operator = MEM_START + 11;
	dispatch[MEM_START + 12].operator = MEM_START + 12;
	dispatch[MEM_START + 13].operator = MEM_START + 13;
	dispatch[MEM_START + 14].operator = MEM_START + 14;
	dispatch[MEM_START + 15].operator = MEM_START + 15;
	dispatch[MEM_START + 16].operator = MEM_START + 16;
	dispatch[MEM_START + 17].operator = MEM_START + 17;

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
		dispatch[MEM_START + 17].routine =
			(synthRoutine) obxMemory;

	dispatch[FIRST_DEV + 63].routine = dispatch[FIRST_DEV + 64].routine =
		(synthRoutine) obxMidi;
	dispatch[FIRST_DEV + 63].controller = 1;
	dispatch[FIRST_DEV + 64].controller = 2;

	dispatch[RADIOSET_1].other1 = -1;
	dispatch[RADIOSET_2].other1 = -1;

	/* Tune/Gain osc-1/2 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, C_RANGE_MIN_1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 3, C_RANGE_MIN_1);
	/* Gain on filter contour */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, C_RANGE_MIN_1);
	/* Select alt filter and pole remix */
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 4);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 7, 4096);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
obxConfigure(brightonWindow *win)
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

	synth->bank = 0;
	/* Don't load a memory, send a bank, location and load request. */
	event.intvalue = 1;
	brightonParamChange(synth->win, synth->panel,
		MEM_START + 1, &event);
	brightonParamChange(synth->win, synth->panel,
		MEM_START + 9, &event);

	multiTune(synth, 0,0,0,0,0);

/*	loadMemory(synth, "obx", 0, 0, synth->mem.active, FIRST_DEV, 0); */
	obxLoadMemory(synth, global.controlfd, synth->midichannel, 0, initmem, 0);

	brightonPut(win,
		"bitmaps/blueprints/obxshade.xpm", 0, 0, win->width, win->height);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	synth->loadMemory = (loadRoutine) obxKeyLoadMemory;

	return(0);
}

