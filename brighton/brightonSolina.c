
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

static int solinaInit();
static int solinaConfigure();
static int solinaCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;
static int dc, shade_id;

#include "brightonKeys.h"

#define OPTS_PANEL 0
#define MOD_PANEL 1
#define KEY_PANEL 2
#define FX_PANEL 4
#define MEM_PANEL 5

#define OPTS_COUNT 16
#define FX_COUNT 1
#define MOD_COUNT 16
#define MEM_COUNT 14

#define OPTS_START 0
#define FX_START OPTS_COUNT
#define MOD_START (FX_START + FX_COUNT)
#define MEM_START (MOD_COUNT + MOD_START)

#define ACTIVE_DEVS (MOD_COUNT + FX_COUNT + OPTS_COUNT)
#define DEVICE_COUNT (ACTIVE_DEVS + MEM_COUNT)

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
 * This example is for a solinaBristol type synth interface.
 */

#define R1 400

#define W1 75
#define W2 20
#define L1 300
#define L2 250

#define C0 310

#define C1 355
#define C2 380

#define C3 410
#define C4 500
#define C5 590
#define C6 680

#define C7 760
#define C8 785
#define C9 810
#define C10 835

static brightonLocations panelControls[OPTS_COUNT] = {
	{"BassString", 2, C1, R1, W2, L2, 0, 1, 0, "bitmaps/buttons/solinaOff.xpm",
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"BassHorn", 2, C2, R1, W2, L2, 0, 1, 0, "bitmaps/buttons/solinaOff.xpm",
		"bitmaps/buttons/solinaOn.xpm", 0},

	{"BassGain", 1, C3, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	{"Tuning", 1, C4, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, BRIGHTON_VERTICAL|BRIGHTON_NOTCH|BRIGHTON_REVERSE},
	{"Crescendo", 1, C5, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE},
	{"Sustain", 1, C6, R1, W1, L1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, BRIGHTON_VERTICAL|BRIGHTON_REVERSE},

	{"String-16", 2, C7, R1, W2, L2, 0, 1, 0, "bitmaps/buttons/solinaOff.xpm",
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"Horn-16", 2, C9, R1, W2, L2, 0, 1, 0, "bitmaps/buttons/solinaOff.xpm",
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"String-8", 2, C8, R1, W2, L2, 0, 1, 0, "bitmaps/buttons/solinaOff.xpm",
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"Horn-8", 2, C10, R1, W2, L2, 0, 1, 0, "bitmaps/buttons/solinaOff.xpm",
		"bitmaps/buttons/solinaOn.xpm", 0},

	{"MasterVolume", 0, C0, R1 - 100, 400, 400, 0, 1, 0, 0, 0, 0},
	{"", 0, C0, R1 - 100, 400, 400, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C0, R1 - 100, 400, 400, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C0, R1 - 100, 400, 400, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C0, R1 - 100, 400, 400, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
	{"", 0, C0, R1 - 100, 400, 400, 0, 1, 0, 0, 0, BRIGHTON_WITHDRAWN},
};

#define S1 200

#define oW1 20
#define oL1 700

#define oC1 39
#define oC2 103
#define oC3 152
#define oC4 203

#define oC5 280
#define oC6 330

#define oC7 742
#define oC8 790
#define oC9 842
#define oC10 891
#define oC11 942

#define oC12 600
#define oC13 650
#define oC14 700

#define oC15 510
#define oC16 560

#define oR1 100
#define oR2 300
#define oR3 500
#define oR4 700

static brightonLocations options[MOD_COUNT] = {
	{"Chorus-D1", 1, oC7, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, 0},
	{"Chorus-D1", 1, oC8, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, 0},
	{"Chorus-Speed", 1, oC9, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, 0},
	{"On Wet/Dry", 1, oC10, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, 0},
	{"Off Wet/Dry", 1, oC11, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, 0},

	{"Osc Detune", 1, oC2, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm",
		0, BRIGHTON_REVERSE|BRIGHTON_NOTCH},
	{"PulseWidth", 1, oC3, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},

	/* The remaining envelope parameters */
	{"Decay", 1, oC5, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},
	{"Sustain", 1, oC6, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},

	/* These should be PWM and LFO, and LFO to OSC? */
	{"PulseWidthMod", 1, oC4, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},
	{"LFO Rate", 1, oC1, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},

	/* Reverb */
	{"Reverb Feedback", 1, oC12, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},
	{"Reverb Crossover", 1, oC13, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},
	{"Reverb Depth", 1, oC14, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},

	/* Vibrato and tremelo */
	{"Vibraro", 1, oC15, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},
	{"Tremelo", 1, oC16, oR1, oW1, oL1, 0, 1, 0, "bitmaps/knobs/sliderred.xpm", 0, 0},
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

static
brightonLocations mem[MEM_COUNT] = {
	/* memories */
	{"", 2, mC1, mR1, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC2, mR1, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC3, mR1, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC4, mR1, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC5, mR1, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC1, mR2, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC2, mR2, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC3, mR2, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC4, mR2, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	{"", 2, mC5, mR2, S3, S5, 0, 1, 0,
		"bitmaps/buttons/solinaOff.xpm", 
		"bitmaps/buttons/solinaOn.xpm", 0},
	/* mem U/D, midi U/D, Load + Save */
	{"", 2, mC1, mR3, S4, S6, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC2, mR3, S4, S6, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC5, mR3, S4, S6, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC3, mR3, S4, S6, 0, 1, 0,
		"bitmaps/buttons/pressoffo.xpm", 
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
};

static
brightonLocations fx[FX_COUNT] = {
	{"FX on/off", 2, 300, 400, 400, 150, 0, 1, 0, "bitmaps/buttons/solinaOff.xpm",
		"bitmaps/buttons/solinaOn.xpm", 0},
};

/*
 * Should try and make this one as generic as possible, and try to use it as
 * a general memory routine. has Midi u/d, mem u/d, load/save and a display.
 */
static int
memCallback(brightonWindow* win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

/* printf("memCallback(%i, %i, %f) %i, %s\n", panel, index, value, */
/* synth->mem.active, synth->resources->name); */

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

	if (index == 13)
	{
		if (brightonDoubleClick(dc)) {
			synth->location = synth->dispatch[MEM_START].other1;
			saveMemory(synth, "solina", 0, synth->bank + synth->location, 0);
		}
		return(0);
	}

	if (index < 10)
	{
		int i;
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

		synth->location = index;
		loadMemory(synth, "solina", 0, synth->bank + synth->location,
			synth->mem.active, 0, BRISTOL_NOCALLS|BRISTOL_FORCE);

		/*
		 * We have loaded the memory but cannot call the devices as they have
		 * various panels.
		 */
		for (i = 0; i < OPTS_COUNT; i++)
		{
			event.value = synth->mem.param[i];
			brightonParamChange(synth->win, OPTS_PANEL, i, &event);
		}

		event.value = synth->mem.param[i];
		brightonParamChange(synth->win, FX_PANEL, 0, &event);

		for (; i < ACTIVE_DEVS; i++)
		{
			event.value = synth->mem.param[i];
			brightonParamChange(synth->win, MOD_PANEL, i - MOD_START, &event);
		}

		synth->dispatch[MEM_START].other1 = index;
	} else {
		int newchan;

		/*
		 * This is a control button.
		 */
		switch(index) {
			case 10:
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
			case 11:
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
brightonApp solinaApp = {
	"solina",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/metal5.xpm",
	BRIGHTON_STRETCH,
	solinaInit,
	solinaConfigure, /* 3 callbacks */
	midiCallback,
	destroySynth,
	{-1, 0, 2, 2, 5, 520, 0, 0},
	600, 200, 0, 0,
	8, /* panel count */
	{
		{
			"Mods",
			"bitmaps/blueprints/solinapanel.xpm",
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL|BRIGHTON_REVERSE,
			0,
			0,
			solinaCallback,
			15, 390, 970, 180,
			OPTS_COUNT,
			panelControls
		},
		{
			"Opts",
			"bitmaps/blueprints/solinamods.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0x020,
			0,
			0,
			solinaCallback,
			17, 9, 967, 381,
			MOD_COUNT,
			options
		},
		{
			"Keyboard",
			0,
			"bitmaps/keys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			187, 580, 740, 400,
			KEY_COUNT_4OCTAVE,
			keys4octave
		},
		{
			"Solina",
			0,
			"bitmaps/textures/wood6.xpm",
			0, /* flags */
			0,
			0,
			0,
			17, 9, 967, 381,
			0,
			0
		},
		{
			"Effects switch",
			0,
			"bitmaps/textures/metal5.xpm",
			0,
			0,
			0,
			solinaCallback,
			930, 570, 53, 400,
			FX_COUNT,
			fx
		},
		{
			"Memory Panel",
			0,
			"bitmaps/textures/metal5.xpm",
			0,
			0,
			0,
			solinaCallback,
			17, 570, 168, 400,
			MEM_COUNT,
			mem
		},
		{
			"Wood Side",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			0, 0, 15, 1000,
			0,
			0
		},
		{
			"Wood Side",
			0,
			"bitmaps/textures/wood.xpm",
			BRIGHTON_STRETCH|BRIGHTON_VERTICAL, /* flags */
			0,
			0,
			0,
			985, 0, 15, 1000,
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

static void
panelSwitch(guiSynth *id, int fd, int chan, int cont, int op, int value)
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
		brightonParamChange(id->win, 1, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 3, -1, &event);

		shade_id = brightonPut(id->win,
			"bitmaps/blueprints/solinashade.xpm", 0, 0, id->win->width,
				id->win->height);
	} else {
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 3, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 1, -1, &event);

		brightonRemove(id->win, shade_id);
	}
}

static int
solinaMidiNull(void *synth, int fd, int chan, int c, int o, int v)
{
/*	printf("%i, %i, %i\n", c, o, v); */
	return(0);
}

static int
solinaMidiDetune(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	if ((v = 8192 - v) >= 8192)
		v = 8191;
	if (v <= -8191)
		v = -8191;
		
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 8192 + v);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 2, 8192 - v);
	return(0);
}

static int
solinaMidiPW(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, v / 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, v * 3 / 4);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 0, v);
	return(0);
}

static int
solinaMidiFX(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	/*
	 * See if the effect is turned on. If so change the wet dry, otherwise
	 * set wet/dry to 0
	 */
	if (synth->mem.param[FX_START] == 0.0) {
		bristolMidiSendMsg(fd, chan, 98, 3,
			(int) (synth->mem.param[MOD_START + 4] * C_RANGE_MIN_1));
		bristolMidiSendMsg(fd, chan, 99, 3,
			(int) (synth->mem.param[MOD_START + 4] * C_RANGE_MIN_1));
	} else {
		bristolMidiSendMsg(fd, chan, 98, 3,
			(int) (synth->mem.param[MOD_START + 3] * C_RANGE_MIN_1));
		bristolMidiSendMsg(fd, chan, 99, 3,
			(int) (synth->mem.param[MOD_START + 3] * C_RANGE_MIN_1));
	}

	return(0);
}

static int
solinaMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
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
solinaCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (solinaApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	switch (panel) {
		case OPTS_PANEL:
			break;
		case FX_PANEL:
			index+=FX_START;
			break;
		case MOD_PANEL:
			index+=MOD_START;
			break;
		case MEM_PANEL:
			if (index == 12)
				panelSwitch(synth, 0, 0, 0, 0, value);
			else
				memCallback(win, panel, index, value);
			return(0);
	}

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
solinaInit(brightonWindow* win)
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

	printf("Initialise the solina link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].routine = solinaMidiNull;

	/* Bass oscillator */
	synth->dispatch[OPTS_START + 0].controller = 126; /* harmonic mix */
	synth->dispatch[OPTS_START + 0].operator = 14;
	synth->dispatch[OPTS_START + 1].controller = 126;
	synth->dispatch[OPTS_START + 1].operator = 15;
	synth->dispatch[OPTS_START + 2].controller = 2;
	synth->dispatch[OPTS_START + 2].operator = 3;

	synth->dispatch[OPTS_START + 3].controller = 126; /* Global tune */
	synth->dispatch[OPTS_START + 3].operator = 1;
	synth->dispatch[OPTS_START + 4].controller = 3; /* Env - 2 parameters */
	synth->dispatch[OPTS_START + 4].operator = 0;
	synth->dispatch[OPTS_START + 5].controller = 3;
	synth->dispatch[OPTS_START + 5].operator = 3;
	synth->dispatch[OPTS_START + 6].controller = 126; /* Harmonics - 4 params */
	synth->dispatch[OPTS_START + 6].operator = 10;
	synth->dispatch[OPTS_START + 7].controller = 126;
	synth->dispatch[OPTS_START + 7].operator = 11;
	synth->dispatch[OPTS_START + 8].controller = 126;
	synth->dispatch[OPTS_START + 8].operator = 12;
	synth->dispatch[OPTS_START + 9].controller = 126;
	synth->dispatch[OPTS_START + 9].operator = 13;
	synth->dispatch[OPTS_START + 10].controller = 3; /* Volume */
	synth->dispatch[OPTS_START + 10].operator = 4; /* Volume */

	synth->dispatch[OPTS_START + 0].routine
		= synth->dispatch[OPTS_START + 1].routine
		= synth->dispatch[OPTS_START + 2].routine
		= synth->dispatch[OPTS_START + 3].routine
		= synth->dispatch[OPTS_START + 4].routine
		= synth->dispatch[OPTS_START + 5].routine
		= synth->dispatch[OPTS_START + 6].routine
		= synth->dispatch[OPTS_START + 7].routine
		= synth->dispatch[OPTS_START + 8].routine
		= synth->dispatch[OPTS_START + 9].routine
		= synth->dispatch[OPTS_START + 10].routine
		= solinaMidiSendMsg;

	synth->dispatch[FX_START + 0].controller = 98; /* Effects wet/dry */
	synth->dispatch[FX_START + 0].operator = 3;

	synth->dispatch[MOD_START + 0].routine
		= synth->dispatch[MOD_START + 1].routine
		= synth->dispatch[MOD_START + 2].routine
		= synth->dispatch[MOD_START + 3].routine
		= synth->dispatch[MOD_START + 4].routine
		= synth->dispatch[MOD_START + 7].routine
		= synth->dispatch[MOD_START + 8].routine
		= synth->dispatch[MOD_START + 9].routine
		= synth->dispatch[MOD_START + 10].routine
		= synth->dispatch[MOD_START + 11].routine
		= synth->dispatch[MOD_START + 12].routine
		= synth->dispatch[MOD_START + 13].routine
		= synth->dispatch[MOD_START + 14].routine
		= synth->dispatch[MOD_START + 15].routine
			= solinaMidiSendMsg;

	synth->dispatch[MOD_START + 0].controller = 98; /* Effects */
	synth->dispatch[MOD_START + 0].operator = 0;
	synth->dispatch[MOD_START + 1].controller = 98; /* Effects */
	synth->dispatch[MOD_START + 1].operator = 1;
	synth->dispatch[MOD_START + 2].controller = 98; /* Effects */
	synth->dispatch[MOD_START + 2].operator = 2;
	synth->dispatch[MOD_START + 3].controller = 98; /* Effects */
	synth->dispatch[MOD_START + 3].operator = 3;
	synth->dispatch[MOD_START + 4].controller = 98; /* Effects */
	synth->dispatch[MOD_START + 4].operator = 3;

	synth->dispatch[MOD_START + 11].controller = 99; /* Effects */
	synth->dispatch[MOD_START + 11].operator = 0;
	synth->dispatch[MOD_START + 12].controller = 99; /* Effects */
	synth->dispatch[MOD_START + 12].operator = 1;
	synth->dispatch[MOD_START + 13].controller = 99; /* Effects */
	synth->dispatch[MOD_START + 13].operator = 2;
	synth->dispatch[MOD_START + 14].controller = 126; /* Vib */
	synth->dispatch[MOD_START + 14].operator = 7;
	synth->dispatch[MOD_START + 15].controller = 126; /* Trem */
	synth->dispatch[MOD_START + 15].operator = 8;

	synth->dispatch[MOD_START + 5].routine
		= (synthRoutine) solinaMidiDetune;
	synth->dispatch[MOD_START + 6].routine
		= (synthRoutine) solinaMidiPW;

	synth->dispatch[MOD_START + 3].routine
		= synth->dispatch[MOD_START + 4].routine
		= synth->dispatch[FX_START + 0].routine
			= (synthRoutine) solinaMidiFX;

	synth->dispatch[MOD_START + 7].controller = 3; /* Env - 2 parameters */
	synth->dispatch[MOD_START + 7].operator = 1;
	synth->dispatch[MOD_START + 8].controller = 3;
	synth->dispatch[MOD_START + 8].operator = 2;

	synth->dispatch[MOD_START + 9].controller = 126; /* PWM and LFO */
	synth->dispatch[MOD_START + 9].operator = 3;
	synth->dispatch[MOD_START + 10].controller = 5;
	synth->dispatch[MOD_START + 10].operator = 0;

	/* 
	 * These will be replaced by some opts controllers. We need to tie the
	 * envelope parameters for decay, sustain. We need to fix a few parameters
	 * of the oscillators too - transpose, tune and gain.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, 10);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 16382);

	/* Oscillators */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 3, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, 3);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, 1);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
solinaConfigure(brightonWindow *win)
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
	synth->keypanel = 2;
	synth->keypanel2 = -1;
	synth->transpose = 36;
	loadMemory(synth, "solina", 0, synth->location, synth->mem.active, 0, 0);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	configureGlobals(synth);

	shade_id = brightonPut(synth->win,
		"bitmaps/blueprints/solinashade.xpm", 0, 0, synth->win->width,
			synth->win->height);

	/* Fix the pulsewidths of the first two osc */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, 4096);
	/* detune all oscs just a small amount */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 8292);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 2, 8092);

	/* Disable velocity tracking */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 5, 0);
	/* Configure a split point */
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 5, 47);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 6, 35);

	event.command = BRIGHTON_PARAMCHANGE;
	event.type = BRIGHTON_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, MEM_PANEL, 0, &event);

	event.command = BRIGHTON_PARAMCHANGE;
	event.type = BRIGHTON_FLOAT;
	event.value = 0;
	brightonParamChange(synth->win, KEY_PANEL, 0, &event);

	dc = brightonGetDCTimer(win->dcTimeout);

	return(0);
}

