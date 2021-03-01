
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

static int voxInit();
static int voxConfigure();
static int voxCallback(brightonWindow *, int, int, float);
static int midiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define OPTS_PANEL 0
#define MOD_PANEL 1
#define MEM_PANEL 2
#define KEY_PANEL 3

#define FIRST_DEV 0

#define MOD_COUNT 8
#define OPTS_COUNT 4
#define MEM_COUNT 17

#define OPTS_START 0
#define MOD_START OPTS_COUNT
#define MEM_START (MOD_COUNT + MOD_START)

#define ACTIVE_DEVS (OPTS_COUNT + 7 + FIRST_DEV)
#define DEVICE_COUNT (MOD_COUNT + OPTS_COUNT + MEM_COUNT)
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
 * This example is for a voxBristol type synth interface.
 */

#define R1 0

#define W1 100
#define L1 600

#define C1 40
#define C2 190
#define C3 340
#define C4 490
#define C5 700
#define C6 850

static brightonLocations locations[DEVICE_COUNT] = {
	{"Drawbar 16", 1, C1, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Drawbar 8", 1, C2, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Drawbar 4", 1, C3, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Drawbar IV", 1, C4, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Drawbar Sine", 1, C5, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Drawbar Ramp", 1, C6, R1, W1, L1, 0, 8, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"Opts", 2, 650, 775, 75, 200, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, BRIGHTON_VERTICAL},
	{"Vibrato", 2, 250, 775, 75, 200, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, BRIGHTON_VERTICAL},
};

#define S1 200

static brightonLocations options[OPTS_COUNT] = {
	{"", 0, 500, 100, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", 0, 200, 500, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", 0, 500, 500, S1, S1, 0, 1, 0, 0, 0, 0},
	{"", 2, 800, 500, 75, 200, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, BRIGHTON_VERTICAL},
};

#define mR1 100
#define mR2 300
#define mR3 550
#define mR4 800

#define mC1 100
#define mC2 293
#define mC3 486
#define mC4 679
#define mC5 875

#define mC11 100
#define mC12 255
#define mC13 410
#define mC14 565
#define mC15 720
#define mC16 874

#define S4 80
#define S5 150

brightonLocations mem[MEM_COUNT] = {
	/* memories */
	{"", 2, mC1, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC2, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC3, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC4, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC5, mR3, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC1, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC2, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC3, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC4, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, mC5, mR4, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", 0},
	/* mem U/D, midi U/D, Load + Save */
	{"", 2, mC11, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC12, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC13, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffo.xpm", 
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC14, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC15, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, mC16, mR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	/* display */
	{"", 3, mC1, mR1, 875, 125, 0, 1, 0, 0, 0, 0},
};

/*
 * Should try and make this one as generic as possible, and try to use it as
 * a general memory routine. has Midi u/d, mem u/d, load/save and a display.
 */
int
memCallback(brightonWindow* win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int i, memindex = 0;

/*	printf("memCallback(%i, %i, %f) %i, %s\n", panel, index, value, */
/*		synth->mem.active, synth->resources->name); */

	if (synth->flags & SUPPRESS)
		return(0);

	/*
	 * We need to convert the index into an offset into the mem structure.
	 * To do this we make a simple parse of the panels preceeding this panel.
	 */
	for (i = 0; i < panel; i++)
		memindex += synth->resources->resources[i].ndevices;

/*	printf("memindex is %i\n", memindex); */

	/*
	 * The first ten buttons are exclusive highlighting, we use the first mem
	 * pointer to handle this.
	 */
	if (synth->dispatch[memindex].other2)
	{
		synth->dispatch[memindex].other2 = 0;
		return(0);
	}

	if (index < 10)
	{
		brightonEvent event;

		/*
		 * This is a numeric. We need to force exclusion.
		 */
		if (synth->dispatch[memindex].other1 != -1)
		{
			synth->dispatch[memindex].other2 = 1;

			if (synth->dispatch[memindex].other1 != index)
				event.value = 0;
			else
				event.value = 1;

			brightonParamChange(synth->win, panel,
				synth->dispatch[memindex].other1, &event);
		}
		synth->dispatch[memindex].other1 = index;

		if ((synth->location = synth->location * 10 + index) >= 1000)
			synth->location = index;

		if (loadMemory(synth, synth->resources->name, 0, synth->location,
					synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
			displayPanelText(synth, "FREE", synth->location, panel,
				MEM_COUNT - 1);
		else
			displayPanelText(synth, "PROG", synth->location, panel,
				MEM_COUNT - 1);
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

				bristolMidiSendMsg(global.controlfd, synth->sid,
					127, 0, BRISTOL_MIDICHANNEL|newchan);

				synth->midichannel = newchan;

				displayPanelText(synth, "MIDI", synth->midichannel + 1,
					panel, MEM_COUNT - 1);
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

				bristolMidiSendMsg(global.controlfd, synth->sid,
					127, 0, BRISTOL_MIDICHANNEL|newchan);

				synth->midichannel = newchan;

				displayPanelText(synth, "MIDI", synth->midichannel + 1,
					panel, MEM_COUNT - 1);
				break;
			case 12:
				/*
				 * Save
				 */
				saveMemory(synth, synth->resources->name, 0, synth->location,
					FIRST_DEV);
				displayPanelText(synth, "PROG", synth->location,
					panel, MEM_COUNT - 1);
				break;
			case 13:
			default:
				/*
				 * Load
				 */
				synth->flags |= MEM_LOADING;
				loadMemory(synth, synth->resources->name, 0, synth->location,
					synth->mem.active, FIRST_DEV, 0);
				synth->flags &= ~MEM_LOADING;
				displayPanelText(synth, "PROG", synth->location,
					panel, MEM_COUNT - 1);
					break;
			case 14:
				/*
				 * Mem Down - these are going to be seek routines - find the
				 * next programmed memory.
				 */
				if (--synth->location < 0)
					synth->location = 999;
				synth->flags |= MEM_LOADING;
				while (loadMemory(synth, synth->resources->name, 0,
					synth->location, synth->mem.active, FIRST_DEV, 0) < 0)
				{
					if (--synth->location < 0)
						synth->location = 999;
				}
				synth->flags &= ~MEM_LOADING;
				displayPanelText(synth, "PROG", synth->location,
					panel, MEM_COUNT - 1);
				break;
			case 15:
				/*
				 * Mem Up - seek routine, find next programmed memory.
				 */
				if (++synth->location >= 1000)
					synth->location = 0;
				synth->flags |= MEM_LOADING;
				while (loadMemory(synth, synth->resources->name, 0,
					synth->location, synth->mem.active, FIRST_DEV, 0) < 0)
				{
					if (++synth->location >= 1000)
						synth->location = 0;
				}
				synth->flags &= ~MEM_LOADING;
				displayPanelText(synth, "PROG", synth->location,
					panel, MEM_COUNT - 1);
				break;
		}
	}
	return(0);
}

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp voxApp = {
	"vox",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/leather.xpm",
	0,
	voxInit,
	voxConfigure, /* 3 callbacks */
	midiCallback,
	destroySynth,
	{-1, 0, 2, 2, 5, 520, 0, 1},
	550, 250, 0, 0,
	7, /* panel count */
	{
		{
			"Opts",
			"bitmaps/blueprints/voxopts.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0x020,
			0,
			0,
			voxCallback,
			19, 40, 500, 570,
			OPTS_COUNT,
			options
		},
		{
			"Mods",
			"bitmaps/blueprints/vox.xpm",
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			voxCallback,
			15, 662, 150, 310,
			MOD_COUNT,
			locations
		},
		{
			"Mem",
			"bitmaps/blueprints/genmem.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0x020,
			0,
			0,
			memCallback,
			519, 40, 462, 570,
			MEM_COUNT,
			mem
		},
		{
			"Keyboard",
			0,
			"bitmaps/keys/vkbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			200, 662, 780, 305,
			VKEY_COUNT,
			vkeys
		},
		{
			"Vox",
			0,
			"bitmaps/textures/redleather.xpm",
			0, /* flags */
			0,
			0,
			0,
			/*15, 600, 970, 50, */
			15, 35, 970, 625,
			0,
			0
		},
		{
			"Vox",
			0,
			"bitmaps/textures/orangeleather.xpm",
			0, /* flags */
			0,
			0,
			0,
			19, 40, 962, 570,
			0,
			0
		},
		{
			"backing",
			0,
			"bitmaps/textures/metal5.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			0,
			170, 662, 30, 305,
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
			loadMemory(synth, synth->resources->name, 0, synth->bank + synth->location,
				synth->mem.active, FIRST_DEV, 0);
			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", controller, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static int
voxMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
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
voxCallback(brightonWindow *win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*	printf("voxCallback(%i, %i, %f): %x\n", panel, index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (voxApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	switch (panel) {
		case OPTS_PANEL:
			index += OPTS_START;
			break;
		case MOD_PANEL:
			index += MOD_START;
			break;
		case MEM_PANEL:
			index += MEM_START;
			break;
	}

/* printf("index is now %i\n", index); */
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
voxDrawbar(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int slidervalue;

/*
	printf("voxDrawbar(%x, %i, %i, %i, %i, %i)\n",
		(size_t) synth, fd, chan, cont, op, value);
*/

	slidervalue = cont * 16 + value;

	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0,
		slidervalue);
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
		event.intvalue = 0;
		brightonParamChange(id->win, 2, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 5, -1, &event);
	} else {
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 5, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 0, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 2, -1, &event);
	}
}

static void
voxVolume(guiSynth *id, int fd, int chan, int cont, int op, int value)
{
/*	printf("voxVolume\n"); */
	bristolMidiSendMsg(global.controlfd, chan, 0, 1, value);
}

static void
voxOption(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
/*	printf("voxOption(%i, %i, %i)\n", cont, op, value); */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, cont, value);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
voxInit(brightonWindow* win)
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

	printf("Initialise the vox link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].routine = voxMidiSendMsg;

	/*
	 * First parameter are for the single oscillator. This should pass through
	 * an encaps operator.
	 *
	 * We have 16', 8', 4' pipes, one "IV" composite, flute (sine) and reed
	 * (saw) waves.
	 */

	synth->dispatch[OPTS_START + 0].routine = (synthRoutine) voxVolume;

	synth->dispatch[OPTS_START + 1].routine
		= synth->dispatch[OPTS_START + 2].routine
			= (synthRoutine) voxOption;
	synth->dispatch[OPTS_START + 1].controller = 0;
	synth->dispatch[OPTS_START + 2].controller = 1;

	synth->dispatch[OPTS_START + 3].controller = 0;
	synth->dispatch[OPTS_START + 3].operator = 5;

	synth->dispatch[MOD_START + 0].routine
		= synth->dispatch[MOD_START + 1].routine
		= synth->dispatch[MOD_START + 2].routine
		= synth->dispatch[MOD_START + 3].routine
		= synth->dispatch[MOD_START + 4].routine
		= synth->dispatch[MOD_START + 5].routine
			= (synthRoutine) voxDrawbar;

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

	/*
	 * And one parameter for vibrato. This also needs to be a composite of 
	 * a few parameters? Er, no, we just need to initialise the vibra values to
	 * vibra only, and a speed. This is just an on/off switch for the vibra
	 */
	synth->dispatch[MOD_START + 6].operator = 0;
	synth->dispatch[MOD_START + 6].controller = 126;

	/*
	 * Panel switcher.
	 */
	synth->dispatch[MOD_START + 7].routine = (synthRoutine) panelSwitch;

	/* 
	 * These will be replaced by some opts controllers.
	 */
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, 550);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, 1600);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 0);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
voxConfigure(brightonWindow *win)
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
	synth->keypanel = 3;
	synth->keypanel2 = -1;
	synth->transpose = 36;
	loadMemory(synth, "vox", 0, synth->location, synth->mem.active,
		FIRST_DEV, 0);
	displayPanelText(synth, "PROG", synth->location, MEM_PANEL, MEM_COUNT - 1);

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

