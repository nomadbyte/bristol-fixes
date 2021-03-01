
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

static int voxM2Init();
static int voxM2Configure();
static int voxM2Callback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

static guimain manual;
static int dc;

#define MOD_PANEL 0
#define OPTS_PANEL 1
#define KEY_PANEL 2
#define VARS_PANEL 4
#define KEY_PANEL2 (KEY_PANEL + 1)

#define FIRST_DEV 0

#define MOD_COUNT 44
#define DUMMY_OFFSET 26
#define OPTS_COUNT 4
#define VARS_COUNT 11

#define MOD_START 0
#define OPTS_START MOD_COUNT
#define MEM_START (MOD_COUNT - 7)

#define ACTIVE_DEVS (MOD_COUNT - 7)
#define DEVICE_COUNT (MOD_COUNT + OPTS_COUNT)
#define DISPLAY_DEV (DEVICE_COUNT - 1)
#define MEM_MGT ACTIVE_DEVS

#define MIDI_MGT (MEM_MGT + 12)


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
 * This example is for a voxM2Bristol type synth interface.
 */

#define R1 230

#define W1 20
#define L1 850
#define L2 300

#define D1 25
#define D2 20

/* 2 + 4+2 + 5+2 */
#define C1 240
#define C2 (C1 + D1)

#define C3 340
#define C4 (C3 + D1)
#define C5 (C4 + D1)
#define C6 (C5 + D1)
#define C7 (C6 + D1 + 20)
#define C8 (C7 + D1)

#define C9 570
#define C10 (C9 + D1)
#define C11 (C10 + D1)
#define C12 (C11 + D1)
#define C13 (C12 + D1)
#define C14 (C13 + D1 + 20)
#define C15 (C14 + D1)

#define C16 835
#define C17 (C16 + D2)
#define C18 (C17 + D2)
#define C19 (C18 + D2)

#define C20 40
#define C21 (C20 + D2)
#define C22 (C21 + D2)
#define C23 (C22 + D2)
#define C24 (C23 + D2)
#define C25 (C24 + D2)
#define C26 (C25 + D2)

static brightonLocations locations[MOD_COUNT] = {
	/* 0 - Drawbars, lower, upper then bass */
	{"Lower 8'", 1, C3, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower 4'", 1, C4, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower 2'", 1, C5, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower IV'", 1, C6, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Flute", 1, C7, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Lower Reed", 1, C8, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper 16'", 1, C9, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper 8'", 1, C10, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper 4'", 1, C11, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper II'", 1, C12, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper III'", 1, C13, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Flute", 1, C14, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Upper Reed", 1, C15, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Bass Flute", 1, C1, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Bass Reed", 1, C2, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},

	/* 15 - Percussive buttons, 4 of them */
	{"Perc-I", 2, C16, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", 0},
	{"Perc-II", 2, C17, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", 0},
	{"Perc-L", 2, C18, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", 0},
	{"Perc-S", 2, C19, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", 0},

	/* 19 - Dummies, 12 in total for opts and vars */
	{"", 2, C16, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C17, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C18, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C19, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C19, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C16, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C17, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C18, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C19, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C19, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C16, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C17, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", BRIGHTON_WITHDRAWN},
	/*
	 * Dummies, 6. Note that the first set, above, are used to save the Opts
	 * controls for Bass register/sustain and vibrato so to maintain the
	 * memories between releases they should remain reserved, the rest are
	 * used as placeholders for the VARS panel. The following six remain free
	 * however they may become reverb depth and level.
	 * 31 - 36
	 */
	{"", 2, C18, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C19, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C18, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C19, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C18, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},
	{"", 2, C19, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", BRIGHTON_WITHDRAWN},

	/* Memory buttons, 7 buttons for 6 locations and save */
	{"", 2, C20, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", 0},
	{"", 2, C21, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", 0},
	{"", 2, C22, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", 0},
	{"", 2, C23, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", 0},
	{"", 2, C24, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", 0},
	{"", 2, C25, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushoutw.xpm",
		"bitmaps/buttons/pushinw.xpm", 0},
	{"", 2, C26, R1, W1, L2, 0, 1, 0, "bitmaps/buttons/pushout.xpm",
		"bitmaps/buttons/pushin.xpm", BRIGHTON_CHECKBUTTON},
};

static brightonLocations options[OPTS_COUNT] = {
	{"", 2, 420, 300, 100, 300, 0, 1, 0, "bitmaps/buttons/rockersmoothBW.xpm",
		"bitmaps/buttons/rockersmoothBWd.xpm", 0},
	{"", 0, 600, 300, 250, 250, 0, 1, 0, 0, 0, 0},
	{"", 2, 850, 300, 100, 300, 0, 1, 0, "bitmaps/buttons/rockersmoothBW.xpm",
		"bitmaps/buttons/rockersmoothBWd.xpm", 0},
	{"", 2, 100, 300, 170, 300, 0, 1, 0, "bitmaps/buttons/rockersmoothBWR.xpm",
		"bitmaps/buttons/rockersmoothBWRd.xpm", 0},
};

static brightonLocations variables[VARS_COUNT] = {
	{"", 0, 35, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW},
	{"", 0, 110, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW},
	{"", 0, 185, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW},
	{"", 2, 270, 280, 25, 400, 0, 1, 0, "bitmaps/buttons/rockersmoothBW.xpm",
		"bitmaps/buttons/rockersmoothBWd.xpm", 0},
	{"", 0, 335, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW},
	{"", 0, 410, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW},
	{"", 0, 485, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW},
	{"", 0, 560, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW},
	/* Reverb params - 3 */
	{"", 0, 635, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 710, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
	{"", 0, 775, 280, 100, 400, 0, 1, 0, 0, 0, BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN},
};

static void
memFix(brightonWindow* win, guiSynth *synth)
{
	brightonEvent event;
	int i;

	event.command = BRIGHTON_PARAMCHANGE;
	event.type = BRIGHTON_FLOAT;

	for (i = 0; i < 19; i++)
	{
		event.value = synth->mem.param[i];
		brightonParamChange(synth->win, 0, i, &event);
	}

	/* MODS */
	event.value = synth->mem.param[19 + 0];
	brightonParamChange(synth->win, OPTS_PANEL, 0, &event);
	event.value = synth->mem.param[19 + 1];
	brightonParamChange(synth->win, OPTS_PANEL, 1, &event);
	event.value = synth->mem.param[19 + 2];
	brightonParamChange(synth->win, OPTS_PANEL, 2, &event);
	/* VARS */
	event.value = synth->mem.param[23 + 0];
	brightonParamChange(synth->win, VARS_PANEL, 0, &event);
	event.value = synth->mem.param[23 + 1];
	brightonParamChange(synth->win, VARS_PANEL, 1, &event);
	event.value = synth->mem.param[23 + 2];
	brightonParamChange(synth->win, VARS_PANEL, 2, &event);
	event.value = synth->mem.param[23 + 3];
	brightonParamChange(synth->win, VARS_PANEL, 3, &event);
	event.value = synth->mem.param[23 + 4];
	brightonParamChange(synth->win, VARS_PANEL, 4, &event);
	event.value = synth->mem.param[23 + 5];
	brightonParamChange(synth->win, VARS_PANEL, 5, &event);
	event.value = synth->mem.param[23 + 6];
	brightonParamChange(synth->win, VARS_PANEL, 6, &event);
	event.value = synth->mem.param[23 + 7];
	brightonParamChange(synth->win, VARS_PANEL, 7, &event);
}

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

	/* Is this the save key, double clicked? */
	if (index == MOD_COUNT - 1)
	{
		if (brightonDoubleClick(dc)) {
			synth->location = synth->dispatch[MEM_START].other1 - MEM_START;
			saveMemory(synth, "voxM2", 0, synth->bank + synth->location, 0);
		}
		return(0);
	}

	/*
	 * Is this one of the presets?
	 *
	 * We should consider making these require a doubleclick also.
	 */
	if (index >= MEM_START)
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

		synth->location = index - MEM_START;

		loadMemory(synth, "voxM2", 0, synth->bank + synth->location,
			synth->mem.active, 0, BRISTOL_NOCALLS);

		memFix(win, synth);

		synth->dispatch[MEM_START].other1 = index;
	}

	return(0);
}

int
voxloadMemory(guiSynth *synth, char *algo, char *name, int location,
int active, int skip, int flags)
{
	loadMemory(synth, "voxM2", 0, location,
		synth->mem.active, 0, BRISTOL_NOCALLS);

	memFix(synth->win, synth);

	return(0);
}

int
voxPedalCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 */
	if (value > 0)
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYON, 0, index + 24);
	else
		bristolMidiSendMsg(global.controlfd, synth->midichannel,
			BRISTOL_EVENT_KEYOFF, 0, index + 24);

	return(0);
}

int
voxkeyCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int chan = 0, transp = 0;

	if (global.libtest)
		return(0);

	if (panel == KEY_PANEL2)
	{
		transp = 12;
		chan = 1;
	}

/*printf("keycallback(%x, %i, %i, %f): %i %i\n", synth, panel, index, value, */
/*		synth->transpose, global.controlfd); */

	/*
	 * Want to send a note event, on or off, for this index + transpose.
	 */
	if (value)
		bristolMidiSendMsg(global.controlfd, synth->midichannel + chan,
			BRISTOL_EVENT_KEYON, 0, index + synth->transpose + transp);
	else
		bristolMidiSendMsg(global.controlfd, synth->midichannel + chan,
			BRISTOL_EVENT_KEYOFF, 0, index + synth->transpose + transp);

	return(0);
}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp voxM2App = {
	"voxM2",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/leather.xpm",
	0,
	voxM2Init,
	voxM2Configure, /* 3 callbacks */
	midiCallback,
	destroySynth,
	{-1, 0, 2, 2, 5, 520, 0, 0},
	520, 310, 0, 0,
	9, /* panel count */
	{
		{
			"Drawbars", /* and percussive selections */
			"bitmaps/blueprints/voxm2.xpm",
			0,
			BRIGHTON_STRETCH,
			0,
			0,
			voxM2Callback,
			19, 184, 970, 190,
			MOD_COUNT,
			locations
		},
		{
			"Mods", /* Bass sustain, vibrato on/off */
			"bitmaps/blueprints/voxM2.xpm",
			"bitmaps/textures/leather.xpm",
			0,
			0,
			0,
			voxM2Callback,
			19, 386, 194, 210,
			OPTS_COUNT,
			options
		},
		{
			"Keyboard",
			0,
			"bitmaps/keys/vkbg.xpm",
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			voxkeyCallback,
			55, 612, 710, 210,
			VKEY_COUNT,
			vkeys
		},
		{
			"Keyboard",
			0,
			"bitmaps/keys/vkbg.xpm",
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			voxkeyCallback,
			224, 386, 730, 210,
			VKEY_COUNT,
			vkeys
		},
		{
			"Variables",
			"bitmaps/blueprints/voxM2vars.xpm",
			"bitmaps/textures/leather.xpm",
			0, /* flags */
			0,
			0,
			voxM2Callback,
			16, 24, 970, 160,
			VARS_COUNT,
			variables
		},
		{
			"Top Panel",
			0,
			"bitmaps/textures/orangeleather.xpm",
			0, /* flags */
			0,
			0,
			0,
			16, 24, 970, 160,
			0,
			0
		},
		{
			"Pedalboard",
			0,
			"bitmaps/keys/vkbg.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			voxPedalCallback,
			250, 850, 500, 150,
			12, /* KEY_COUNT_PEDAL, */
			pedalBoard
		},
		{
			/* Underneath keyboards */
			"Bottom Panel",
			0,
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH|BRIGHTON_REVERSE, /* flags */
			0,
			0,
			0,
			16, 386, 966, 440,
			0,
			0
		},
		{
			"Pedalboard",
			0,
			"bitmaps/keys/vkbg.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			0,
			0, 850, 1000, 150,
			0,
			0
		},
	},
};

static int shade_id = 0;

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
		brightonRemove(id->win, shade_id);

		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 5, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 4, -1, &event);

		shade_id = brightonPut(id->win, "bitmaps/blueprints/voxshade.xpm", 0, 0,
				id->win->width, id->win->height);
	} else {
		brightonRemove(id->win, shade_id);

		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 4, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 5, -1, &event);

		shade_id = brightonPut(id->win, "bitmaps/blueprints/voxshade.xpm", 0, 0,
				id->win->width, id->win->height);
	}
}

static int
midiCallback(brightonWindow *win, int controller, int value, float n)
{
	guiSynth *synth = findSynth(global.synths, win);

/*	printf("midi callback: %x, %i\n", controller, value); */

	switch(controller)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", controller, value);
			synth->location = value;
			loadMemory(synth, synth->resources->name, 0, synth->bank
				+ synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_NOCALLS);

			memFix(win, synth);

			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value * 10;
			break;
	}
	return(0);
}

static int
voxM2MidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
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
voxM2Callback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (voxM2App.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	if (panel == OPTS_PANEL)
		index += 19;

	if (index == 22)
	{
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
		return(0);
	}

	if (panel == VARS_PANEL)
		index += 23;

	synth->mem.param[index] = value;

	if (index >= ACTIVE_DEVS) {
		memCallback(win, panel, index, value);
		return(0);
	}

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
voxM2Drawbar(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int sval;

/* printf("voxM2Drawbar(%x, %i, %i, %i, %i, %i)\n",
synth, fd, chan, cont, op, value); */

	/*
	 * We have to take care of dual/triple manual stuff here. Also need to 
	 * block the upper manual 4' and II (which we are going to use for perc)
	 * if they are selected on controllers 15 and 16.
	 */
	sval = cont * 16 + value;

	if ((op == 1) && (cont == 2) && (synth->mem.param[15] != 0))
	{
		sval = cont * 16 + 8;
		bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0, sval);
		return;
	}

	if ((op == 1) && (cont == 6) && (synth->mem.param[16] != 0))
	{
		sval = cont * 16 + 8;
		bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0, sval);
		return;
	}

	switch (op) {
		case 0:
			bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, sval);
			break;
		case 1:
			bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0, sval);
			break;
	}
}

static void
voxM2Vibrato(guiSynth *id, int fd, int chan, int cont, int op, int value)
{
	bristolMidiSendMsg(global.controlfd, chan, cont, op, value);
	bristolMidiSendMsg(global.controlfd, chan + 1, cont, op, value);
}

static void
voxM2Volume(guiSynth *id, int fd, int chan, int cont, int op, int value)
{
	bristolMidiSendMsg(global.controlfd, chan,   0, 1, value);
	bristolMidiSendMsg(global.controlfd, chan+1, 0, 1, value);
	bristolMidiSendMsg(global.controlfd, chan+1, 2, 1, value);
}

static void
voxM2Option(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int actual;

	/*
	 * We deal with two values representing vibrato speed and depth, then with
	 * percussive decays.
	 */
	switch (cont) {
		case 24:
			/* Vibra Speed */
			bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, value);
			bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 0, value);
			return;
		case 25:
			/* Vibra Depth */
			bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, value);
			bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 1, value);
			return;
		case 26:
			/* Vibra Chorus */
			bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, value);
			bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 2, value);
			return;
		case 27:
			/*
			 * The next 4 are percusive controls. We have to see if they are
			 * active and get their values before we send the requests.
			 *
			 * We have short/long/soft/loud
			 *
			 * Here - if L is not selected then short - set value.
			 */
			actual = synth->mem.param[27] * C_RANGE_MIN_1 / 2;
			if (synth->mem.param[17] == 0)
				bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 1, actual);
			return;
		case 28:
			/*
			 * Here - if L is selected then short - set value.
			 */
			actual = synth->mem.param[28] * C_RANGE_MIN_1 / 2;
			if (synth->mem.param[17] != 0)
				bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 1, actual);
			return;
		case 29:
			/*
			 * Here - if S is selected then soft - set value.
			 */
			actual = synth->mem.param[29] * C_RANGE_MIN_1;
			if (synth->mem.param[18] != 0)
				bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 4, actual);
			return;
		case 30:
			/*
			 * Here - if S is not selected then loud - set value.
			 */
			actual = synth->mem.param[30] * C_RANGE_MIN_1;
			if (synth->mem.param[18] == 0)
				bristolMidiSendMsg(global.controlfd, synth->sid2, 3, 4, actual);
			return;
	}
}
/*
 * Called when a wave selector button
 */
static void
voxM2Waves(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int tval;

/* printf("waves %i %i %i\n", cont, op, value); */
	if (value != 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid2, cont, op, value);
		/* We just set the percussive option, must also send full on drawbar */
		if (op == 2)
			bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0, 2 * 16 + 8);
		else
			bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0, 6 * 16 + 8);
		return;
	}

	/*
	 * Ok, so we are turning the percussive off, that means we have to change
	 * back to the drawbar current setting and disable percussive.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid2, cont, op, value);

	if (op == 2)
		tval = 2 * 16 + synth->mem.param[8];
	else
		tval = 6 * 16 + synth->mem.param[9];
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 0, tval);
}

static void
voxM2Bass(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	switch (op) {
		case 0:
			bristolMidiSendMsg(global.controlfd, synth->sid, 2, 0,
				value == 0? 16:24);
			break;
		case 4:
		case 5:
			bristolMidiSendMsg(global.controlfd, synth->sid, 2, 0,
				op * 16 + value);
			break;
	}
}

/*
 * This is called when the percussive wave selection buttons are pressed.
 * It needs to set wave gains and request perc options.
 */
static void
voxM2Perc(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int v;

/* printf("perc %i %i %i\n", cont, op, value); */
	switch (op) {
		default:
			if (value == 0)
				v = synth->mem.param[27] * C_RANGE_MIN_1 / 4;
			else
				v = synth->mem.param[28] * C_RANGE_MIN_1 / 4;
			break;
		case 4:
			if (value == 0)
				v = synth->mem.param[30] * C_RANGE_MIN_1;
			else
				v = synth->mem.param[29] * C_RANGE_MIN_1;
			break;
	}
	bristolMidiSendMsg(global.controlfd, synth->sid2, cont, op, v);
}

static void
voxM2Reverb(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	bristolMidiSendMsg(global.controlfd, synth->sid2, cont, op, value);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
voxM2Init(brightonWindow* win)
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

	printf("Initialise the voxM2 link to bristol: %p\n", synth->win);

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
		if (synth->midichannel > 14)
			synth->midichannel--;
		bcopy(&global, &manual, sizeof(guimain));
		synth->synthtype = BRISTOL_VOX;
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

		/* Crank up another VOX for the upper manual */
		manual.flags |= BRISTOL_CONN_FORCE|BRIGHTON_NOENGINE;
		manual.port = global.port;
		manual.host = global.host;
		synth->synthtype = BRISTOL_VOXM2;
		synth->midichannel++;

		if ((synth->sid2 = initConnection(&manual, synth)) < 0)
			return(-1);

		global.manualfd = manual.controlfd;
		global.manual = &manual;
		manual.manual = &global;

		synth->midichannel--;
	}

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = voxM2MidiSendMsg;

	/* Lower manual basic VOX oscillator */
	synth->dispatch[MOD_START + 0].operator = 0;
	synth->dispatch[MOD_START + 0].controller = 0;
	synth->dispatch[MOD_START + 1].operator = 0;
	synth->dispatch[MOD_START + 1].controller = 1;
	synth->dispatch[MOD_START + 2].operator = 0;
	synth->dispatch[MOD_START + 2].controller = 2;
	synth->dispatch[MOD_START + 3].operator = 0;
	synth->dispatch[MOD_START + 3].controller = 3;
	synth->dispatch[MOD_START + 4].operator = 0;
	synth->dispatch[MOD_START + 4].controller = 4;
	synth->dispatch[MOD_START + 5].operator = 0;
	synth->dispatch[MOD_START + 5].controller = 5;
	synth->dispatch[MOD_START + 0].routine
		= synth->dispatch[MOD_START + 1].routine
		= synth->dispatch[MOD_START + 2].routine
		= synth->dispatch[MOD_START + 3].routine
		= synth->dispatch[MOD_START + 4].routine
		= synth->dispatch[MOD_START + 5].routine
			= (synthRoutine) voxM2Drawbar;

	/* Upper manual basic VOX oscillator */
	synth->dispatch[MOD_START + 6].operator = 1;
	synth->dispatch[MOD_START + 6].controller = 0;
	synth->dispatch[MOD_START + 7].operator = 1;
	synth->dispatch[MOD_START + 7].controller = 1;
	synth->dispatch[MOD_START + 8].operator = 1;
	synth->dispatch[MOD_START + 8].controller = 2;
	synth->dispatch[MOD_START + 9].operator = 1;
	synth->dispatch[MOD_START + 9].controller = 6;
	synth->dispatch[MOD_START + 10].operator = 1;
	synth->dispatch[MOD_START + 10].controller = 7;
	synth->dispatch[MOD_START + 11].operator = 1;
	synth->dispatch[MOD_START + 11].controller = 4;
	synth->dispatch[MOD_START + 12].operator = 1;
	synth->dispatch[MOD_START + 12].controller = 5;
	synth->dispatch[MOD_START + 6].routine
		= synth->dispatch[MOD_START + 7].routine
		= synth->dispatch[MOD_START + 8].routine
		= synth->dispatch[MOD_START + 9].routine
		= synth->dispatch[MOD_START + 10].routine
		= synth->dispatch[MOD_START + 11].routine
		= synth->dispatch[MOD_START + 12].routine
			= (synthRoutine) voxM2Drawbar;

	/*
	 * 13 and 14 are the Bass drawbars
	 */
	synth->dispatch[MOD_START + 13].controller = 2;
	synth->dispatch[MOD_START + 13].operator = 4;
	synth->dispatch[MOD_START + 13].routine = (synthRoutine) voxM2Bass;
	synth->dispatch[MOD_START + 14].controller = 2;
	synth->dispatch[MOD_START + 14].operator = 5;
	synth->dispatch[MOD_START + 14].routine = (synthRoutine) voxM2Bass;

	/*
	 * 15 and 16 are the percussive selectors that need to be sent to the
	 * modified vox oscillator. We need a shim since they go to the second
	 * manual.
	 */
	synth->dispatch[MOD_START + 15].controller = 0;
	synth->dispatch[MOD_START + 15].operator = 2;
	synth->dispatch[MOD_START + 16].controller = 0;
	synth->dispatch[MOD_START + 16].operator = 3;
	synth->dispatch[MOD_START + 15].routine
		= synth->dispatch[MOD_START + 16].routine = (synthRoutine) voxM2Waves;

	/*
	 * Percussive harmonics then envelope
	 */
	synth->dispatch[MOD_START + 17].controller = 2;
	synth->dispatch[MOD_START + 17].operator = 1;
	synth->dispatch[MOD_START + 18].controller = 2;
	synth->dispatch[MOD_START + 18].operator = 4;
	synth->dispatch[MOD_START + 17].routine
		= synth->dispatch[MOD_START + 18].routine = (synthRoutine) voxM2Perc;

	/* Bass harmonics */
	synth->dispatch[MOD_START + 19].controller = 2;
	synth->dispatch[MOD_START + 19].operator = 0;
	synth->dispatch[MOD_START + 19].routine = (synthRoutine) voxM2Bass;
	/* Bass sustain (actualy env release). */
	synth->dispatch[MOD_START + 20].controller = 3;
	synth->dispatch[MOD_START + 20].operator = 3;

	/* Vibrato */
	synth->dispatch[MOD_START + 21].controller = 126;
	synth->dispatch[MOD_START + 21].operator = 0;
	synth->dispatch[MOD_START + 21].routine = (synthRoutine) voxM2Vibrato;
	/* Panel switch */
	synth->dispatch[MOD_START + 22].routine = (synthRoutine) panelSwitch;

	/* Volume */
	synth->dispatch[MOD_START + 23].routine = (synthRoutine) voxM2Volume;

	/* Options */
	synth->dispatch[MOD_START + 24].controller = 24;
	synth->dispatch[MOD_START + 24].routine = (synthRoutine) voxM2Option;
	synth->dispatch[MOD_START + 25].controller = 25;
	synth->dispatch[MOD_START + 25].routine = (synthRoutine) voxM2Option;
	synth->dispatch[MOD_START + 26].controller = 26;
	synth->dispatch[MOD_START + 26].routine = (synthRoutine) voxM2Option;
	synth->dispatch[MOD_START + 27].controller = 27;
	synth->dispatch[MOD_START + 27].routine = (synthRoutine) voxM2Option;
	synth->dispatch[MOD_START + 28].controller = 28;
	synth->dispatch[MOD_START + 28].routine = (synthRoutine) voxM2Option;
	synth->dispatch[MOD_START + 29].controller = 29;
	synth->dispatch[MOD_START + 29].routine = (synthRoutine) voxM2Option;
	synth->dispatch[MOD_START + 30].controller = 30;
	synth->dispatch[MOD_START + 30].routine = (synthRoutine) voxM2Option;

	/* Dummies need to be covered */
	synth->dispatch[MOD_START + 31].controller = 99;
	synth->dispatch[MOD_START + 31].operator = 0;
	synth->dispatch[MOD_START + 32].controller = 99;
	synth->dispatch[MOD_START + 32].operator = 1;
	synth->dispatch[MOD_START + 33].controller = 99;
	synth->dispatch[MOD_START + 33].operator = 2;
	synth->dispatch[MOD_START + 31].routine
		= synth->dispatch[MOD_START + 32].routine
		= synth->dispatch[MOD_START + 33].routine = (synthRoutine) voxM2Reverb;

	synth->dispatch[MOD_START + 34].operator = 100;
	synth->dispatch[MOD_START + 34].controller = 100;
	synth->dispatch[MOD_START + 35].operator = 100;
	synth->dispatch[MOD_START + 35].controller = 100;
	synth->dispatch[MOD_START + 36].operator = 100;
	synth->dispatch[MOD_START + 36].controller = 100;
	synth->dispatch[MOD_START + 37].operator = 100;
	synth->dispatch[MOD_START + 37].controller = 100;

	synth->dispatch[MEM_START + 0].routine
		= synth->dispatch[MEM_START + 1].routine
		= synth->dispatch[MEM_START + 2].routine
		= synth->dispatch[MEM_START + 3].routine
		= synth->dispatch[MEM_START + 4].routine
		= synth->dispatch[MEM_START + 5].routine
			= (synthRoutine) memCallback;

	/* Vox Mark-II Oscillator */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 0, 4, 1);
	/* Bass Osc 8' (actually remapped to 4') */
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 0, 40);
	/*
	 * We have to setup the vibra with speed and depth defaults
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, 500);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, 2000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 0, 500);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 1, 2000);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 1, 2, 0);
	/* Gain */
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 1, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 2, 1, 16383);
	/* Perc env parameters */
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 0, 5);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 1, 2048);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 2, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 3, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid2, 2, 4, 16383);
	/* Bass env parameters ADSG, R is in the panel */
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 0, 100);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, 100);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 12000);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
voxM2Configure(brightonWindow *win)
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
	synth->keypanel2 = KEY_PANEL2;
	synth->transpose = 36;
	synth->bank = synth->location - (synth->location % 10);
	synth->location -= synth->bank;
	loadMemory(synth, "voxM2", 0, synth->bank + synth->location,
		synth->mem.active, FIRST_DEV, BRISTOL_NOCALLS);

	memFix(win, synth);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	brightonParamChange(synth->win, KEY_PANEL2, -1, &event);
	brightonParamChange(synth->win, 6, -1, &event);
	brightonParamChange(synth->win, 1, -1, &event);
	event.type = BRIGHTON_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, 1, 3, &event);
	configureGlobals(synth);

	synth->dispatch[MEM_START].other1 = MEM_START + synth->location;
	synth->dispatch[MEM_START].other2 = 1;
	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;
	brightonParamChange(synth->win, 0, synth->location + MEM_START, &event);

	dc = brightonGetDCTimer(win->dcTimeout);

	synth->loadMemory = (loadRoutine) voxloadMemory;

	return(0);
}

