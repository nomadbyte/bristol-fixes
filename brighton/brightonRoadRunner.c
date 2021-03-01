
/*

Check out

	parameter to effects
	gain of tri wave
	bit error in triwave
	touch response

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

static int roadrunnerInit();
static int roadrunnerConfigure();
static int roadrunnerCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;
static int dc, shade_id;

#include "brightonKeys.h"

#define OPTS_PANEL 0
#define MOD_PANEL 4
#define KEY_PANEL 2
#define MEM_PANEL 3

#define MODS_COUNT 4
#define OPTS_COUNT 22
#define MEM_COUNT 14

#define MODS_START 0
#define OPTS_START MODS_COUNT
#define MEM_START (OPTS_COUNT + OPTS_START)

/* We use '+2' so that bass and treble get saved with memory */
#define ACTIVE_DEVS (MODS_COUNT + OPTS_COUNT + 2)
#define DEVICE_COUNT (ACTIVE_DEVS + MEM_COUNT - 2)

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
 * This example is for a roadrunnerBristol type synth interface.
 */

#define R1 400

#define C1 15
#define C2 (C1 + 114)
#define C3 (C2 + 114)
#define C4 (C3 + 114)

#define W1 100
#define L1 600

static brightonLocations modspanel[MODS_COUNT] = {
	{"Tune", 1, C1, R1, W1, L1, 0, 1, 0, 0, 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOSHADOW|BRIGHTON_NOTCH},
	{"LFO Frequency", 1, C2, R1, W1, L1, 0, 1, 0, 0, 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOSHADOW},
	{"Vibrato", 1, C3, R1, W1, L1, 0, 1, 0, 0, 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOSHADOW},
	{"Volume", 1, C4, R1, W1, L1, 0, 1, 0, 0, 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_NOSHADOW},
};

#define S1 200

#define oW1 35
#define oL1 500

#define oC1 16
#define oC2 66
#define oC3 116

#define oC4 172
#define oC5 220

#define oC6 270

#define oC7 735
#define oC8 782
#define oC9 835
#define oC10 884
#define oC11 933

#define oC12 592
#define oC13 642
#define oC14 692

#define oC15 500
#define oC16 549

#define WFS 308
#define oC50 (WFS + 0)
#define oC51 (WFS + 28)
#define oC52 (WFS + 69)
#define oC53 (WFS + 97)
#define oC54 (WFS + 138)
#define oC55 (WFS + 166)

#define oC56 (WFS + 202)

#define oR1 140
#define oR2 300
#define oR3 500
#define oR4 700

static brightonLocations options[OPTS_COUNT] = {
	/* Osc parameters */
	{"Detune", 0, oC1, oR1, oW1, oL1, 0, 1, 0, 0, 0, BRIGHTON_REVERSE|BRIGHTON_NOTCH},
	{"PulseWidth", 0, oC2, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"PulseWidthMod", 0, oC3, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	/* waveforms */
	{"Bass 1", 2, oC50, 100, 27, 600, 0, 1, 0, "bitmaps/buttons/rockersmoothBB.xpm",
		"bitmaps/buttons/rockersmoothBBd.xpm", 0},
	{"Bass 2", 2, oC51, 100, 27, 600, 0, 1, 0, "bitmaps/buttons/rockersmoothBB.xpm",
		"bitmaps/buttons/rockersmoothBBd.xpm", 0},
	{"Mid 1", 2, oC52, 100, 27, 600, 0, 1, 0, "bitmaps/buttons/rockersmoothBBL.xpm",
		"bitmaps/buttons/rockersmoothBBLd.xpm", 0},
	{"Mid 2", 2, oC53, 100, 27, 600, 0, 1, 0, "bitmaps/buttons/rockersmoothBBL.xpm",
		"bitmaps/buttons/rockersmoothBBLd.xpm", 0},
	{"Treble 1", 2, oC54, 100, 27, 600, 0, 1, 0, "bitmaps/buttons/rockersmoothBC.xpm",
		"bitmaps/buttons/rockersmoothBCd.xpm", 0},
	{"Treble 2", 2, oC55, 100, 27, 600, 0, 1, 0, "bitmaps/buttons/rockersmoothBC.xpm",
		"bitmaps/buttons/rockersmoothBCd.xpm", BRIGHTON_NOSHADOW},

	/* The remaining envelope parameters */
	{"Decay", 0, oC4, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"Release", 0, oC5, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"TouchSense", 2, oC6, 100, 27, 600, 0, 1, 0, "bitmaps/buttons/rockersmoothBW.xpm",
		"bitmaps/buttons/rockersmoothBWd.xpm", 0},

	/* Tremelo */
	{"Tremelo", 0, oC16, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},

	/* Chorus */
	{"Choris Wet", 0, oC7, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"ChorusD1", 0, oC8, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"ChorusD2", 0, oC9, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"ChorusSpeed", 0, oC10, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},

	/* Reverb */
	{"Reberb Wet", 0, oC11, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"Reverb Cross", 0, oC12, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"Reverb Feed", 0, oC13, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},
	{"Reverg Length", 0, oC14, oR1, oW1, oL1, 0, 1, 0, 0, 0, 0},

	/* Single/Multi LFO */
	{"LFO Multi", 2, oC56, 100, 27, 600, 0, 1, 0, "bitmaps/buttons/rockersmoothBGR.xpm",
		"bitmaps/buttons/rockersmoothBGRd.xpm", 0},
};

#define mR1 100
#define mR2 600
#define mR3 820

#define mC0 (120)
#define mC1 (mC0 +  129)
#define mC2 (mC1 +  129)
#define mC3 (mC2 +  129)
#define mC4 (mC3 +  129)
#define mC4s (mC4 + 129)

#define mC5 50
#define mC6 550
#define mC7 650
#define mC8 780
#define mC9 900

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
	{"Bass", 0, mC2 -  70, mR2, 260, 260, 0, 1, 0, 0, 0, 0},
	{"Treble", 0, mC4 - 100, mR2, 260, 260, 0, 1, 0, 0, 0, 0},

	/* memories */
	{"Mem-0", 2, mC0, mR1, 125, 400, 0, 1, 0, "bitmaps/buttons/rockersmoothBB.xpm",
		"bitmaps/buttons/rockersmoothBBd.xpm", 0}, /* -127 65 */
	{"Mem-1", 2, mC1, mR1, 125, 400, 0, 1, 0, "bitmaps/buttons/rockersmoothBG.xpm",
		"bitmaps/buttons/rockersmoothBGd.xpm", 0},
	{"Mem-2", 2, mC2, mR1, 125, 400, 0, 1, 0, "bitmaps/buttons/rockersmoothBBL.xpm",
		"bitmaps/buttons/rockersmoothBBLd.xpm", 0}, /* 190 31 -3 */
	{"Mem-3", 2, mC3, mR1, 125, 400, 0, 1, 0, "bitmaps/buttons/rockersmoothBGR.xpm",
		"bitmaps/buttons/rockersmoothBGRd.xpm", 0},
	{"Mem-4", 2, mC4, mR1, 125, 400, 0, 1, 0, "bitmaps/buttons/rockersmoothBC.xpm",
		"bitmaps/buttons/rockersmoothBCd.xpm", 0},
	{"Mem-5", 2, mC4s, mR1, 125, 400, 0, 1, 0, "bitmaps/buttons/rockersmoothBW.xpm",
		"bitmaps/buttons/rockersmoothBWd.xpm", 0},

	{"", 1, 0, 0, 50, 50, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN},
	{"", 1, 0, 0, 50, 50, 0, 1, 0, "bitmaps/knobs/sliderpointL.xpm", 0,
		BRIGHTON_VERTICAL|BRIGHTON_REVERSE|BRIGHTON_WITHDRAWN},

	/* mem U/D, midi U/D, Load + Save */
	{"", 2, mC0 + 35, mR3, S4, S6, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, mC0 + 35, mR2, S4, S6, 0, 1, 0, "bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
	{"", 2, mC8 + 7, mR3, S4, S6, 0, 1, 0, "bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_NOSHADOW},
	{"", 2, mC8 + 7, mR2, S4, S6, 0, 1, 0, "bitmaps/buttons/pressoffo.xpm", 
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW},
};

int singleclick = 0;

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

	if (index < 2)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 2 - index, 3,
			(int) (value * C_RANGE_MIN_1));
		return(0);
	}

	if (index == 13)
	{
		if (brightonDoubleClick(dc)) {
			synth->location = synth->dispatch[MEM_START].other1 - 2;
			saveMemory(synth, "roadrunner", 0, synth->bank + synth->location,0);
			singleclick = 0;
		} else {
			singleclick = 1;
		}
		return(0);
	}

	if (index < 8)
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

		synth->location = index - 2;

		/*
		 * If the last button pressed was the save button (just once) then save
		 * to the new location.
		 */
		if (singleclick) {
			saveMemory(synth, "roadrunner", 0, synth->bank + synth->location,0);
			singleclick = 0;
			synth->dispatch[MEM_START].other1 = index;
			return(0);
		}

		loadMemory(synth, "roadrunner", 0, synth->bank + synth->location,
			synth->mem.active, 0, BRISTOL_NOCALLS|BRISTOL_FORCE);

		/*
		 * We have loaded the memory but cannot call the devices as they have
		 * various panels.
		 */
		for (i = 0; i < MODS_COUNT; i++)
		{
			event.value = synth->mem.param[i];
			brightonParamChange(synth->win, MOD_PANEL, i, &event);
		}

		for (; i < MEM_START; i++)
		{
			event.value = synth->mem.param[i];
			brightonParamChange(synth->win, OPTS_PANEL, i - OPTS_START, &event);
		}

		event.value = synth->mem.param[i];
		brightonParamChange(synth->win, MEM_PANEL, i - MEM_START, &event);
		i++;
		event.value = synth->mem.param[i];
		brightonParamChange(synth->win, MEM_PANEL, i - MEM_START, &event);

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

	singleclick = 0;

	return(0);
}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp roadrunnerApp = {
	"roadrunner",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/leather.xpm",
	0,
	roadrunnerInit,
	roadrunnerConfigure, /* 3 callbacks */
	midiCallback,
	destroySynth,
	{-1, 0, 2, 2, 5, 520, 0, 0},
	660, 138, 0, 0,
	7, /* panel count */
	{
		{
			"Options panel",
			"bitmaps/blueprints/roadrunneropts.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0x020,
			0,
			0,
			roadrunnerCallback,
			/*30, 50, 940, 365, */
			15, 9, 970, 300,
			OPTS_COUNT,
			options
		},
		{
			"RoadRunner",
			0,
			"bitmaps/textures/leather.xpm", /* flags */
			0,
			0,
			0,
			0,
			15, 9, 970, 300,
			0,
			0
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/kbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			204, 305, 790, 555,
			KEY_COUNT,
			keysprofile
		},
		{
			"Memory Panel",
			0,
			"bitmaps/textures/metal5.xpm",
			0,
			0,
			0,
			roadrunnerCallback,
			15, 300, 189, 560,
			MEM_COUNT,
			mem
		},
		{
			"Front Panel Mods",
			"bitmaps/blueprints/roadrunnermods.xpm",
			0,
			0,
			0,
			0,
			roadrunnerCallback,
			15, 860, 970, 140,
			MODS_COUNT,
			modspanel,
		},
		{
			"Metal Side",
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
			"Metal Side",
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
		brightonParamChange(id->win, 0, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 1, -1, &event);

		shade_id = brightonPut(id->win,
			"bitmaps/blueprints/roadrunnershade.xpm", 0, 0, id->win->width,
				id->win->height);
	} else {
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 1, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 0, -1, &event);

		brightonRemove(id->win, shade_id);
	}
}

static int
roadrunnerMidiNull(void *synth, int fd, int chan, int c, int o, int v)
{
/*	printf("%i, %i, %i\n", c, o, v); */
	return(0);
}

static int
roadrunnerMidiDetune(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
roadrunnerMidiWF(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	int cont, square = 0, tri = 0, ramp = 0;

	/*
	 * Find out which pair this is, B/M/T, if they are both up then select
	 * a triangular wave, otherwise they represent their respective waves.
	 */
	switch (c) {
		default:
		case OPTS_START + 3:
		case OPTS_START + 4:
			/* Bass oscillator */
			cont = 30;
			switch ((int) synth->mem.param[OPTS_START + 3]) {
				case 0:
					if (synth->mem.param[OPTS_START + 4] == 0)
						/*
						 * Tri on, rest off
						 */
						tri = 1;
					else
						/*
						 * Tri off, ramp off, square on
						 */
						square = 1;
					break;
				default:
					if (synth->mem.param[OPTS_START + 4] == 0)
						/*
						 * ramp on, rest off
						 */
						ramp = 1;
					else {
						/*
						 * Tri off, ramp on, square on
						 */
						ramp = 1;
						square = 1;
					}
					break;
			}
			break;
		case OPTS_START + 5:
		case OPTS_START + 6:
			/* Oscillator */
			cont = 10;
			switch ((int) synth->mem.param[OPTS_START + 5]) {
				case 0:
					if (synth->mem.param[OPTS_START + 6] != 0)
						/*
						 * Tri off, ramp off, square on
						 */
						square = 1;
					break;
				default:
					if (synth->mem.param[OPTS_START + 6] == 0.0)
						/*
						 * ramp on, rest off
						 */
						ramp = 1;
					else {
						/*
						 * Tri off, ramp on, square on
						 */
						ramp = 1;
						square = 1;
					}
					break;
			}
			break;
		case OPTS_START + 7:
		case OPTS_START + 8:
			/* Treble oscillator */
			cont = 20;
			switch ((int) synth->mem.param[OPTS_START + 7]) {
				case 0:
					if (synth->mem.param[OPTS_START + 8] == 0)
						/*
						 * Tri on, rest off
						 */
						tri = 1;
					else
						/*
						 * Tri off, ramp off, square on
						 */
						square = 1;
					break;
				default:
					if (synth->mem.param[OPTS_START + 8] == 0)
						/*
						 * ramp on, rest off
						 */
						ramp = 1;
					else {
						/*
						 * Tri off, ramp on, square on
						 */
						ramp = 1;
						square = 1;
					}
					break;
			}
			break;
	}

	bristolMidiSendMsg(global.controlfd, synth->sid, 126, cont + 0, ramp);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, cont + 1, square);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, cont + 2, tri);

	return(0);
}

static int
roadrunnerMidiPW(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, v / 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, v * 3 / 4);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 0, v);
	return(0);
}

static int
roadrunnerMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
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
roadrunnerCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (roadrunnerApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	switch (panel) {
		case OPTS_PANEL:
			index+=OPTS_START;
			break;
		case MOD_PANEL:
			break;
		case MEM_PANEL:
			if (index == 12) {
				panelSwitch(synth, 0, 0, 0, 0, value);
				return(0);
			} else if (index > 1) {
				memCallback(win, panel, index, value);
				return(0);
			} else
				index+=MEM_START;
	}

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

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
roadrunnerInit(brightonWindow* win)
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

	printf("Initialise the roadrunner link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].routine = roadrunnerMidiNull;

	/* Front panel - tune, lfo, vibrato, volume */
	synth->dispatch[MODS_START + 0].controller = 126; /* Tuning */
	synth->dispatch[MODS_START + 0].operator = 1;
	synth->dispatch[MODS_START + 1].controller = 5; /* LFO frequency */
	synth->dispatch[MODS_START + 1].operator = 0;
	synth->dispatch[MODS_START + 2].controller = 126; /* vibrato */
	synth->dispatch[MODS_START + 2].operator = 7;
	synth->dispatch[MODS_START + 3].controller = 3; /* gain */
	synth->dispatch[MODS_START + 3].operator = 4;

	synth->dispatch[MODS_START + 0].routine
	 	= synth->dispatch[MODS_START + 1].routine
	 	= synth->dispatch[MODS_START + 2].routine
	 	= synth->dispatch[MODS_START + 3].routine
			= roadrunnerMidiSendMsg;

	/* EQ controls from memory panel - not in memories */
	synth->dispatch[MEM_START + 0].controller = 2; /* Bass gain */
	synth->dispatch[MEM_START + 0].operator = 3;
	synth->dispatch[MEM_START + 1].controller = 1; /* Treble gain */
	synth->dispatch[MEM_START + 1].operator = 3;

	synth->dispatch[MEM_START + 0].routine
	 	= synth->dispatch[MEM_START + 1].routine
			= roadrunnerMidiSendMsg;

	/*
	 * The rest are from the options panel that is normally hidden.
	 */
	synth->dispatch[OPTS_START + 0].routine
		= (synthRoutine) roadrunnerMidiDetune;
	synth->dispatch[OPTS_START + 1].routine
		= (synthRoutine) roadrunnerMidiPW;
	synth->dispatch[OPTS_START + 2].controller = 126; /* PWM and LFO */
	synth->dispatch[OPTS_START + 2].operator = 3;

	/* Waveforms */
	synth->dispatch[OPTS_START + 3].controller = OPTS_START + 3;
	synth->dispatch[OPTS_START + 4].controller = OPTS_START + 4;
	synth->dispatch[OPTS_START + 5].controller = OPTS_START + 5;
	synth->dispatch[OPTS_START + 6].controller = OPTS_START + 6;
	synth->dispatch[OPTS_START + 7].controller = OPTS_START + 7;
	synth->dispatch[OPTS_START + 8].controller = OPTS_START + 8;

	synth->dispatch[OPTS_START + 3].routine
		= synth->dispatch[OPTS_START + 4].routine
		= synth->dispatch[OPTS_START + 5].routine
		= synth->dispatch[OPTS_START + 6].routine
		= synth->dispatch[OPTS_START + 7].routine
		= synth->dispatch[OPTS_START + 8].routine
			= (synthRoutine) roadrunnerMidiWF;

	synth->dispatch[OPTS_START + 9].controller = 3; /* Env - 2 parameters */
	synth->dispatch[OPTS_START + 9].operator = 1;
	synth->dispatch[OPTS_START + 10].controller = 3;
	synth->dispatch[OPTS_START + 10].operator = 3;
	synth->dispatch[OPTS_START + 11].controller = 3;
	synth->dispatch[OPTS_START + 11].operator = 5;

	synth->dispatch[OPTS_START + 12].controller = 126; /* Tremelo */
	synth->dispatch[OPTS_START + 12].operator = 8;

/* Almost done effects - a couple may have to be compound parameters */
	synth->dispatch[OPTS_START + 13].controller = 98; /* Effects */
	synth->dispatch[OPTS_START + 13].operator = 0;
	synth->dispatch[OPTS_START + 14].controller = 98; /* Effects */
	synth->dispatch[OPTS_START + 14].operator = 1;
	synth->dispatch[OPTS_START + 15].controller = 98; /* Effects */
	synth->dispatch[OPTS_START + 15].operator = 2;
	synth->dispatch[OPTS_START + 16].controller = 98; /* Effects */
	synth->dispatch[OPTS_START + 16].operator = 3;

	synth->dispatch[OPTS_START + 17].controller = 99; /* Effects */
	synth->dispatch[OPTS_START + 17].operator = 0;
	synth->dispatch[OPTS_START + 18].controller = 99; /* Effects */
	synth->dispatch[OPTS_START + 18].operator = 1;
	synth->dispatch[OPTS_START + 19].controller = 99; /* Effects */
	synth->dispatch[OPTS_START + 19].operator = 2;
	synth->dispatch[OPTS_START + 20].controller = 99; /* Effects */
	synth->dispatch[OPTS_START + 20].operator = 3;
	synth->dispatch[OPTS_START + 21].controller = 126; /* Multi LFO */
	synth->dispatch[OPTS_START + 21].operator = 4;

	synth->dispatch[OPTS_START + 2].routine
		= synth->dispatch[OPTS_START + 9].routine
		= synth->dispatch[OPTS_START + 10].routine
		= synth->dispatch[OPTS_START + 11].routine
		= synth->dispatch[OPTS_START + 12].routine
		= synth->dispatch[OPTS_START + 13].routine
		= synth->dispatch[OPTS_START + 14].routine
		= synth->dispatch[OPTS_START + 15].routine
		= synth->dispatch[OPTS_START + 16].routine
		= synth->dispatch[OPTS_START + 17].routine
		= synth->dispatch[OPTS_START + 19].routine
		= synth->dispatch[OPTS_START + 19].routine
		= synth->dispatch[OPTS_START + 20].routine
		= synth->dispatch[OPTS_START + 21].routine
		= roadrunnerMidiSendMsg;

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
roadrunnerConfigure(brightonWindow *win)
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
		"bitmaps/blueprints/roadrunnershade.xpm", 0, 0, synth->win->width,
			synth->win->height);

	/* Fix the env attack and sustain */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 0, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 0);
	/* Fix the pulsewidths of the first two osc */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, 4096);
	/* detune all oscs just a small amount */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2, 8192);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 8292);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 2, 8092);
	/* LFO to restart on NOTE_ON */
	bristolMidiSendMsg(global.controlfd, synth->sid, 5, 1, 8092);

	/* Configure no split point */
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 5, 127);
	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 6, 0);

	event.command = BRIGHTON_PARAMCHANGE;
	event.type = BRIGHTON_FLOAT;
	event.value = 1;
	panelSwitch(synth, 0, 0, 0, 0, 1);
	brightonParamChange(synth->win, MEM_PANEL, 2, &event);
	panelSwitch(synth, 0, 0, 0, 0, 0);

	loadMemory(synth, "roadrunner", 0, 0, synth->mem.active-6, 0, 0);

	synth->dispatch[MEM_START].other1 = 2;
	synth->dispatch[MEM_START].other2 = 0;

	dc = brightonGetDCTimer(win->dcTimeout);

	return(0);
}

