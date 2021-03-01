
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

static int hammondInit();
static int hammondConfigure();
static int hammondCallback(brightonWindow * , int, int, float);

extern guimain global;

#define FIRST_DEV 0

#define OPTS_PANEL 0
#define SLIDER_PANEL 1
#define MOD_PANEL 2
#define VOL_PANEL 3
#define MEM_PANEL 4

#define OPTS_COUNT 27
#define SLIDER_COUNT 9
#define MOD_COUNT 14
#define VOL_COUNT 1
#define MEM_COUNT 18

#define OPTS_START FIRST_DEV
#define SLIDER_START (OPTS_START + OPTS_COUNT)
#define MOD_START (SLIDER_START + SLIDER_COUNT)
#define VOL_START (MOD_START + MOD_COUNT)
#define MEM_START (VOL_START + VOL_COUNT)

#define DEVICE_COUNT (SLIDER_COUNT + MOD_COUNT + MEM_COUNT + VOL_COUNT + OPTS_COUNT + FIRST_DEV)
#define ACTIVE_DEVS (SLIDER_COUNT + MOD_COUNT + VOL_COUNT + OPTS_COUNT + FIRST_DEV)

#define DISPLAY (MEM_COUNT - 1)
#define DISPLAYPANEL 2

#define R1 60
#define D1 100

#define C1 75
#define C2 (C1 + D1)
#define C3 (C2 + D1)
#define C4 (C3 + D1)
#define C5 (C4 + D1)
#define C6 (C5 + D1)
#define C7 (C6 + D1)
#define C8 (C7 + D1)
#define C9 (C8 + D1)

#define W1 70
#define L1 800

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
 * This example is for a hammondBristol type synth interface.
 */
static brightonLocations sliders[SLIDER_COUNT] = {
	{"", 1, C1, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"", 1, C2, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondbrown.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"", 1, C3, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"", 1, C4, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"", 1, C5, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"", 1, C6, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"", 1, C7, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"", 1, C8, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondblack.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE},
	{"", 1, C9, R1, W1, L1, 0, 7, 0,
		"bitmaps/knobs/hammondwhite.xpm", 0, BRIGHTON_REVERSE|BRIGHTON_HSCALE}
};

#define MR1 150
#define MR2 550

#define MD1 80

#define MC1 100
#define MC2 (MC1 + MD1)
#define MC3 (MC2 + MD1)
#define MC4 (MC3 + MD1)
#define MC5 (MC4 + MD1)

#define MC6 (500 + MC1)
#define MC7 (500 + MC2)
#define MC8 (500 + MC3)
#define MC9 (500 + MC4)
#define MC10 (500 + MC5)

#define S4 35
#define S5 200

static brightonLocations memories[MEM_COUNT] = {
/* Memory tablet */
	{"", 2, MC10, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC6, MR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC7, MR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC8, MR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC9, MR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC10, MR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC6, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC7, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC8, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC9, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 2, MC1 - 50, MR2, S4, S5, 0, 1, 0, /* panel switch */
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", 0},
	/* midi U, D, Load, Save */
	{"", 2, MC2, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC3, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC4, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm", 
		"bitmaps/buttons/pressong.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, MC5, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoffo.xpm", 
		"bitmaps/buttons/pressono.xpm", BRIGHTON_CHECKBUTTON},
	/* mem up down */
	{"", 2, 520, MR1, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	{"", 2, 520, MR2, S4, S5, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", 
		"bitmaps/buttons/presson.xpm", BRIGHTON_CHECKBUTTON},
	/* display */
	{"", 3, MC2, MR1, 275, 250, 0, 1, 0, 0, 0, 0}
};

#define MODD1 200
#define MODD2 300

#define MODC1 50
#define MODC2 (MODC1 + MODD1)
#define MODC3 (MODC2 + MODD1)
#define MODC4 (MODC3 + MODD1)
#define MODC5 (MODC4 + MODD1)

#define MODR1 100
#define MODR2 (MODR1 + MODD2)
#define MODR3 (MODR2 + MODD2)

#define MW1 80
#define MH1 150

static brightonLocations mods[MOD_COUNT] = {
	/* Leslie */
	{"", 2, MODC1, MODR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"", 2, MODC2, MODR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	/* Vibra */
	{"", 0, MODC3 - 30, MODR1, MW1 + 150, MH1 + 150, 0, 2, 0,
		0, 0, 0},
	{"", 2, MODC4, MODR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerred.xpm", 0, 0},
	{"", 2, MODC5, MODR1, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerred.xpm", 0, 0},
	/* Percussives */
	{"", 2, MODC1, MODR3, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	{"", 2, MODC2, MODR3, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	{"", 2, MODC4, MODR2, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	{"", 2, MODC3, MODR3, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	/* diverse */
	{"", 2, MODC1, MODR2, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerblue.xpm", 0, 0},
	{"", 2, MODC2, MODR2, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerblue.xpm", 0, 0},

	{"", 2, MODC5, MODR2, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},

	{"", 2, MODC4, MODR3, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerred.xpm", 0, 0},
	{"", 2, MODC5, MODR3, MW1, MH1, 0, 1, 0,
		"bitmaps/buttons/rockerred.xpm", 0, 0}
};

static brightonLocations volumes[VOL_COUNT] = {
	{"", 0, 0, 0, 1000, 800, 0, 1, 0, 0, 0, 0},
};

#define OD1 140
#define OD2 70
#define OD3 70

#define OC1 150
#define OC2 (OC1 + OD1 + 50)
#define OC3 (OC2 + OD1)
#define OC4 (OC3 + OD1)
#define OC5 (OC4 + OD1)
#define OC6 (OC5 + OD1)

#define OR1 20
#define OR2 (OR1 + OD2)
#define OR3 (OR2 + OD2)
#define OR4 (OR3 + OD2)

#define OR5 (OR4 + 150)
#define OR6 (OR5 + 200)
#define OR7 (OR6 + 200)

#define OS1 25
#define OS2 60
#define OS3 100

static brightonLocations opts[OPTS_COUNT] = {
	{"", 2, OC1, OR1, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, OC1, OR2, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, OC1, OR3, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, OC1, OR4, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 0, OC2, OR1 + 75, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC3, OR1, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC4, OR1, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC5, OR1, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC6, OR1, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC3, OR1 + 150, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC4, OR1 + 150, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC5, OR1 + 150, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC6, OR1 + 150, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC4, OR5, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC5, OR5, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC6, OR5, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC3, OR6, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC4, OR6, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC5, OR6, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC6, OR6, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 2, OC1, OR7, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm",
		"bitmaps/buttons/presson.xpm", 0},
	{"", 2, OC1, OR7 + OD2, OS1, OS2, 0, 1, 0,
		"bitmaps/buttons/pressoffg.xpm",
		"bitmaps/buttons/pressong.xpm", 0},
	{"", 0, OC2, OR7, OS3, OS3, 0, 7, 0, 0, 0, 0},
	{"", 0, OC3, OR7, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC4, OR7, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC5, OR7, OS3, OS3, 0, 1, 0, 0, 0, 0},
	{"", 0, OC6, OR7, OS3, OS3, 0, 1, 0, 0, 0, 0},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp hammondApp = {
	"hammond",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/wood2.xpm",
	0, /* or BRIGHTON_STRETCH, default is tesselate */
	hammondInit,
	hammondConfigure, /* 3 callbacks, unused? */
	0,
	destroySynth,
	{32, 0, 2, 2, 5, 520, 0, 0},
	808, 259, 0, 0,
	5, /* four panels */
	{
		{
			"Options",
			"bitmaps/blueprints/hammondopts.xpm",
			"bitmaps/textures/metal5.xpm",
			0x020, /* flags - 0x020 withdraws */
			0,
			0,
			hammondCallback,
			/*900, 700, 70, 250, */
			20, 50, 460, 900,
			OPTS_COUNT,
			opts
		},
		{
			"Harmonics",
			"bitmaps/blueprints/hammond.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			hammondCallback,
			20, 50, 460, 900,
			SLIDER_COUNT,
			sliders
		},
		{
			"Modulations",
			"bitmaps/blueprints/hammondmods.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			hammondCallback,
			500, 50, 480, 600,
			MOD_COUNT,
			mods
		},
		{
			"Volume",
			"bitmaps/blueprints/hammondvol.xpm",
			0,
			0, /* flags */
			0,
			0,
			hammondCallback,
			900, 700, 70, 250,
			VOL_COUNT,
			volumes
		},
		{
			"Memories",
			"bitmaps/blueprints/hammondmem.xpm",
			"bitmaps/textures/metal6.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			hammondCallback,
			500, 700, 375, 250,
			MEM_COUNT,
			memories
		}
	}
};

static void
hammondMemory(guiSynth *synth, int fd, int chan, int c, int o, int v)
{
/*	printf("hammondMemory(%i %i %i %i %i)\n", fd, chan, c, o, v); */

	/*
	 * radio button exception
	 */
	if (synth->dispatch[MEM_START].other2)
	{
		synth->dispatch[MEM_START].other2 = 0;
		return;
	}

	switch (c) {
		default:
		case 0:
			if (synth->dispatch[MEM_START].other2 == 0)
			{
				brightonEvent event;

				if (synth->dispatch[MEM_START].other1 != o)
					event.value = 0;
				else
					event.value = 1;

				synth->dispatch[MEM_START].other2 = 1;

				brightonParamChange(synth->win, MEM_PANEL,
					synth->dispatch[MEM_START].other1,
					&event);
			}
			synth->dispatch[MEM_START].other1 = o;

			synth->location = synth->location * 10 + o;

			if (synth->location > 1000)
				synth->location = o;
/*printf("location is now %i\n", synth->location); */
			if (loadMemory(synth, "hammond", 0, synth->location,
				synth->mem.active, FIRST_DEV, BRISTOL_STAT) < 0)
				displayPanelText(synth, "FRE", synth->location, MEM_PANEL,
					DISPLAY);
			else
				displayPanelText(synth, "PRG", synth->location, MEM_PANEL,
					DISPLAY);
			break;
		case 1:
			synth->flags |= MEM_LOADING;
			if (loadMemory(synth, "hammond", 0, synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
				displayPanelText(synth, "FRE", synth->location, MEM_PANEL,
					DISPLAY);
			else
				displayPanelText(synth, "PRG", synth->location, MEM_PANEL,
					DISPLAY);
			synth->flags &= ~MEM_LOADING;
/*			synth->location = 0; */
			break;
		case 2:
			saveMemory(synth, "hammond", 0, synth->location, FIRST_DEV);
			displayPanelText(synth, "PRG", synth->location, MEM_PANEL, DISPLAY);
/*			synth->location = 0; */
			break;
		case 3:
			while (loadMemory(synth, "hammond", 0, ++synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location > 999)
					synth->location = -1;
			}
			displayPanelText(synth, "PRG", synth->location, MEM_PANEL, DISPLAY);
			break;
		case 4:
			while (loadMemory(synth, "hammond", 0, --synth->location,
				synth->mem.active, FIRST_DEV, 0) < 0)
			{
				if (synth->location < 0)
					synth->location = 1000;
			}
			displayPanelText(synth, "PRG", synth->location, MEM_PANEL, DISPLAY);
			break;
	}
}

static void
hammondMidi(guiSynth *synth, int fd, int chan, int c, int o, int v)
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
		bristolMidiSendMsg(global.controlfd, synth->sid,
			127, 0, BRISTOL_MIDICHANNEL|newchan);

	synth->midichannel = newchan;

	displayPanelText(synth, "MIDI", synth->midichannel + 1, MEM_PANEL, DISPLAY);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
hammondCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

	if (synth == 0)
		return(0);

	if ((synth->flags & OPERATIONAL) == 0)
		return(0);

	switch (panel) {
		case SLIDER_PANEL:
			index += SLIDER_START;
			break;
		case MOD_PANEL:
			index += MOD_START;
			break;
		case VOL_PANEL:
			index += VOL_START;
			break;
		case OPTS_PANEL:
			index += OPTS_START;
			break;
		case MEM_PANEL:
			index += MEM_START;
			break;
	}

/*	printf("hammondCallback(%i, %i, %f): %x\n", panel, index, value, synth); */

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);
	if (synth->dispatch[index].controller >= DEVICE_COUNT)
		return(0);

	if (hammondApp.resources[0].devlocn[index].to == 1.0)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	synth->mem.param[index] = value;

/*printf("index is now %i %i %i\n", index, DEVICE_COUNT, ACTIVE_DEVS); */

/*	if ((!global.libtest) || (index >= ACTIVE_DEVS)) */
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#define DEBUG
#ifdef DEBUG
	printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
		global.controlfd, synth->sid,
		synth->dispatch[index].controller,
		synth->dispatch[index].operator,
		sendvalue);
#endif

	return(0);
}

static int
hammondNull()
{
	return(0);
}

static void
doSlider(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	int slidervalue;

	/*printf("doSlider(%x, %i, %i, %i, %i, %i)\n", */
	/*	synth, fd, chan, cont, op, value); */

	slidervalue = cont * 8 + value;

	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 2,
		slidervalue);
}

static void
doOverdrive(guiSynth *synth)
{
	if (synth->mem.param[MOD_START + MOD_COUNT - 1] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 100, 5, 0);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 100, 5,
			(int) (synth->mem.param[OPTS_START + 9] * C_RANGE_MIN_1));
}

static void
doReverb(guiSynth *synth)
{
	int revlevel = 0;

	/*printf("doReverb\n"); */

	if (synth->mem.param[MOD_START + 9] != 0)
		revlevel++;
	if (synth->mem.param[MOD_START + 10] != 0)
		revlevel+=2;

	switch (revlevel) {
		default:
		case 0:
			bristolMidiSendMsg(global.controlfd, synth->sid, 100, 4, 0);
			break;
		case 1:
			bristolMidiSendMsg(global.controlfd, synth->sid, 100, 4,
				(int) (synth->mem.param[OPTS_START + 24] * C_RANGE_MIN_1));
			break;
		case 2:
			bristolMidiSendMsg(global.controlfd, synth->sid, 100, 4,
				(int) (synth->mem.param[OPTS_START + 25] * C_RANGE_MIN_1));
			break;
		case 3:
			bristolMidiSendMsg(global.controlfd, synth->sid, 100, 4,
				(int) (synth->mem.param[OPTS_START + 26] * C_RANGE_MIN_1));
			break;
	}
}

static void
doBright(guiSynth *synth)
{
	if (synth->mem.param[MOD_START + 12] == 0)
	{
		/*
		 * Once to the hammond manager
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 1, 0);
		/*
		 * And to the sine oscillator
		 */
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 0);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 8);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 16);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 24);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 32);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 40);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 48);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 56);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 64);
	} else {
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 1,
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 0 +
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 8 +
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 16 +
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 24 +
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 32 +
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 40 +
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 48 +
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 56 +
			(int) (synth->mem.param[OPTS_START + 22]));
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 0, 64 +
			(int) (synth->mem.param[OPTS_START + 22]));
	}
}

static void
doClick(guiSynth *synth)
{
	if (synth->mem.param[MOD_START + 11] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 6, 0);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 6,
			(int) (synth->mem.param[OPTS_START + 23] * C_RANGE_MIN_1));
}

static void
doCompress(guiSynth *synth)
{
	if (synth->mem.param[OPTS_START + 21] == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 2, 0);
	} else {
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 2, 1);
	}
}

static void
doPreacher(guiSynth *synth)
{
	if (synth->mem.param[OPTS_START + 20] == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 3, 0);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 7, 0);
	} else {
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 3, 1);
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 7, 1);
	}
}

static void
doGrooming(guiSynth *synth)
{
	if (synth->mem.param[MOD_START + 7] != 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0,
			(int) (synth->mem.param[OPTS_START + 18] * C_RANGE_MIN_1));
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0,
			(int) (synth->mem.param[OPTS_START + 19] * C_RANGE_MIN_1));
}

static void
doPerc(guiSynth *synth)
{
	if (synth->mem.param[MOD_START + 5] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 24);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 25);

	if (synth->mem.param[MOD_START + 6] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 32);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 0, 3, 33);

	if (synth->mem.param[MOD_START + 8] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 1,
			(int) (synth->mem.param[OPTS_START + 16] * C_RANGE_MIN_1));
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 4, 1,
			(int) (synth->mem.param[OPTS_START + 17] * C_RANGE_MIN_1));
}

static void
doVibra(guiSynth *synth)
{
	if (synth->mem.param[MOD_START + 3] == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 126, 0, 0);
		return;
	}

	bristolMidiSendMsg(global.controlfd, synth->sid, 126, 0, 1);

	if (synth->mem.param[MOD_START + 4] == 0)
		bristolMidiSendMsg(global.controlfd, synth->sid, 6, 2, 0);
	else
		bristolMidiSendMsg(global.controlfd, synth->sid, 6, 2, 1);

	switch ((int) synth->mem.param[MOD_START + 2])
	{
		case 0:
		default:
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0,
				(int) ((1 - synth->mem.param[OPTS_START + 13])
					* C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1,
				(int) ((1 - synth->mem.param[OPTS_START + 13])
					* C_RANGE_MIN_1));
			break;
		case 1:
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0,
				(int) ((1 - synth->mem.param[OPTS_START + 14])
					* C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1,
				(int) ((1 - synth->mem.param[OPTS_START + 14])
					* C_RANGE_MIN_1));
			break;
		case 2:
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 0,
				(int) ((1 - synth->mem.param[OPTS_START + 15])
					* C_RANGE_MIN_1));
			bristolMidiSendMsg(global.controlfd, synth->sid, 6, 1,
				(int) ((1 - synth->mem.param[OPTS_START + 14])
					* C_RANGE_MIN_1));
			break;
	}
}

static void
doVolume(guiSynth *synth)
{
	bristolMidiSendMsg(global.controlfd, synth->sid, 0, 4,
		(int) (synth->mem.param[VOL_START] * C_RANGE_MIN_1));
}

static void
doLeslieSpeed(guiSynth *synth)
{
	int speed, depth, phase;

	if (synth->mem.param[MOD_START] == 0)
	{
		bristolMidiSendMsg(global.controlfd, synth->sid, 100, 7, 0);
		return;
	}
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 7,
		(int) (synth->dispatch[OPTS_START + 1].other1));

	if (synth->mem.param[MOD_START + 1] != 0)
	{
		speed = synth->mem.param[OPTS_START + 6] * C_RANGE_MIN_1;
		depth = synth->mem.param[OPTS_START + 7] * C_RANGE_MIN_1;
		phase = synth->mem.param[OPTS_START + 8] * C_RANGE_MIN_1;
	} else {
		speed = synth->mem.param[OPTS_START + 10] * C_RANGE_MIN_1;
		depth = synth->mem.param[OPTS_START + 11] * C_RANGE_MIN_1;
		phase = synth->mem.param[OPTS_START + 12] * C_RANGE_MIN_1;
	}
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 0, speed);
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 3, depth);
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 2, phase);
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 6,
		(int) (synth->mem.param[OPTS_START + 4] * C_RANGE_MIN_1));
	bristolMidiSendMsg(global.controlfd, synth->sid, 100, 1,
		(int) (synth->mem.param[OPTS_START + 5] * C_RANGE_MIN_1));
}

static void
hammondPanelSwitch(guiSynth *id, int fd, int chan, int cont, int op, int value)
{
	brightonEvent event;

/*	printf("hammondPanelSwitch(%x, %i, %i, %i, %i, %i)\n", */
/*		id, fd, chan, cont, op, value); */

	/* 
	 * If the sendvalue is zero, then withdraw the opts window, draw the
	 * slider window, and vice versa.
	 */
	if (value == 0)
	{
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, OPTS_PANEL, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, SLIDER_PANEL, -1, &event);
	} else {
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, SLIDER_PANEL, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, OPTS_PANEL, -1, &event);
	}
}

static void
hammondOption(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	brightonEvent event;

	/*printf("hammondOption(%x, %i, %i, %i, %i, %i)\n", */
	/*	synth, fd, chan, cont, op, value); */

	switch (cont) {
		case OPTS_START:
			/*
			 * Rotation type. Send 100.7 becomes op;
			 */
			if ((synth->flags & MEM_LOADING) == 0)
			{
				if (synth->dispatch[OPTS_START].other2)
				{
					synth->dispatch[OPTS_START].other2 = 0;
					return;
				}
				synth->dispatch[OPTS_START].other2 = 1;
				if (synth->dispatch[OPTS_START].other1 >= 0)
				{
					event.command = BRIGHTON_PARAMCHANGE;
					if (synth->dispatch[OPTS_START].other1 != (op - 1))
						event.value = 0;
					else
						event.value = 1;
					brightonParamChange(synth->win,
						OPTS_PANEL, synth->dispatch[OPTS_START].other1, &event);
				}
			}

			if (synth->mem.param[OPTS_START + op - 1] != 0)
			{
				synth->dispatch[OPTS_START].other1 = op - 1;
				synth->dispatch[OPTS_START + 1].other1 = op;

				if (synth->mem.param[MOD_START] == 0)
					bristolMidiSendMsg(global.controlfd, chan, 100, 7, 0);
				else {
					bristolMidiSendMsg(global.controlfd, chan, 100, 7, op);
					bristolMidiSendMsg(global.controlfd, chan, 100, 1,
						(int) (synth->mem.param[OPTS_START + 5]
							* C_RANGE_MIN_1));
				}
			}
			break;
		case OPTS_START + 3:
			/*
			 * Rotor break. Send 100.7 = 4 off, 100.7 = 5 on.
			 */
			if (value == 0)
				bristolMidiSendMsg(global.controlfd, chan, 100, 7, 4);
			else
				bristolMidiSendMsg(global.controlfd, chan, 100, 7, 5);
			break;
		case OPTS_START + 4:
		case OPTS_START + 5:
		case OPTS_START + 6:
		case OPTS_START + 7:
		case OPTS_START + 8:
		case OPTS_START + 10:
		case OPTS_START + 11:
		case OPTS_START + 12:
			doLeslieSpeed(synth);
			break;
		case OPTS_START + 9:
			/* overdrive */
			doOverdrive(synth);
			break;
		case OPTS_START + 13:
		case OPTS_START + 14:
		case OPTS_START + 15:
			doVibra(synth);
			break;
		case OPTS_START + 16:
		case OPTS_START + 17:
			doPerc(synth);
			break;
		case OPTS_START + 18:
		case OPTS_START + 19:
			doGrooming(synth);
			break;
		case OPTS_START + 20:
			doPreacher(synth);
			break;
		case OPTS_START + 21:
			doCompress(synth);
			break;
		case OPTS_START + 22:
			doBright(synth);
			break;
		case OPTS_START + 23:
			doClick(synth);
			break;
		case OPTS_START + 24:
		case OPTS_START + 25:
		case OPTS_START + 26:
			doReverb(synth);
			break;
		default:
			break;
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
hammondInit(brightonWindow *win)
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

	printf("Initialise the hammond link to bristol: %p\n", synth->win);

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
		synth->dispatch[i].routine = hammondNull; /*hammondMidiSendMsg; */

/*
	dispatch[FIRST_DEV + 0].controller = 10;
	dispatch[FIRST_DEV + 0].operator = 0;
	dispatch[FIRST_DEV + 1].controller = 9;
	dispatch[FIRST_DEV + 1].operator = 0;
	dispatch[FIRST_DEV + 2].controller = 11;
	dispatch[FIRST_DEV + 2].operator = 1;
*/

	dispatch[OPTS_START].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START].controller = OPTS_START;
	dispatch[OPTS_START].operator = 1;
	dispatch[OPTS_START + 1].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 1].controller = OPTS_START;
	dispatch[OPTS_START + 1].operator = 2;
	dispatch[OPTS_START + 2].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 2].controller = OPTS_START;
	dispatch[OPTS_START + 2].operator = 3;
	dispatch[OPTS_START].other1 = -1;
	dispatch[OPTS_START].other2 = 0;
	dispatch[OPTS_START + 3].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 3].controller = OPTS_START + 3;

	dispatch[OPTS_START + 4].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 4].controller = OPTS_START + 4;
	dispatch[OPTS_START + 5].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 5].controller = OPTS_START + 5;
	dispatch[OPTS_START + 6].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 6].controller = OPTS_START + 6;
	dispatch[OPTS_START + 7].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 7].controller = OPTS_START + 7;
	dispatch[OPTS_START + 8].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 8].controller = OPTS_START + 8;
	dispatch[OPTS_START + 9].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 9].controller = OPTS_START + 9;
	dispatch[OPTS_START + 10].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 10].controller = OPTS_START + 10;
	dispatch[OPTS_START + 11].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 11].controller = OPTS_START + 11;
	dispatch[OPTS_START + 12].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 12].controller = OPTS_START + 12;

	/* vibra */
	dispatch[OPTS_START + 13].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 13].controller = OPTS_START + 13;
	dispatch[OPTS_START + 14].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 14].controller = OPTS_START + 14;
	dispatch[OPTS_START + 15].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 15].controller = OPTS_START + 15;

	dispatch[OPTS_START + 16].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 16].controller = OPTS_START + 16;
	dispatch[OPTS_START + 17].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 17].controller = OPTS_START + 17;
	dispatch[OPTS_START + 18].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 18].controller = OPTS_START + 18;
	dispatch[OPTS_START + 19].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 19].controller = OPTS_START + 19;

	/* preacher */
	dispatch[OPTS_START + 20].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 20].controller = OPTS_START + 20;
	dispatch[OPTS_START + 21].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 21].controller = OPTS_START + 21;
	dispatch[OPTS_START + 22].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 22].controller = OPTS_START + 22;
	dispatch[OPTS_START + 23].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 23].controller = OPTS_START + 23;

	/* reverb */
	dispatch[OPTS_START + 24].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 24].controller = OPTS_START + 24;
	dispatch[OPTS_START + 25].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 25].controller = OPTS_START + 25;
	dispatch[OPTS_START + 26].routine = (synthRoutine) hammondOption;
	dispatch[OPTS_START + 26].controller = OPTS_START + 26;

	/* Memory/Midi buttons */
	dispatch[MEM_START + 10].routine = (synthRoutine) hammondPanelSwitch;
	dispatch[MEM_START + 10].controller = MEM_COUNT;
	dispatch[MEM_START + 10].operator = 0;

	/*
	 * These are for the memory radio buttons
	 */
	dispatch[MEM_START].other1 = 0;
	dispatch[MEM_START].other2 = 0;

	dispatch[MEM_START].operator = 0;
	dispatch[MEM_START + 1].operator = 1;
	dispatch[MEM_START + 2].operator = 2;
	dispatch[MEM_START + 3].operator = 3;
	dispatch[MEM_START + 4].operator = 4;
	dispatch[MEM_START + 5].operator = 5;
	dispatch[MEM_START + 6].operator = 6;
	dispatch[MEM_START + 7].operator = 7;
	dispatch[MEM_START + 8].operator = 8;
	dispatch[MEM_START + 9].operator = 9;

	dispatch[MEM_START].routine = dispatch[MEM_START + 1].routine =
	dispatch[MEM_START + 2].routine = dispatch[MEM_START + 3].routine =
	dispatch[MEM_START + 4].routine = dispatch[MEM_START + 5].routine =
	dispatch[MEM_START + 6].routine = dispatch[MEM_START + 7].routine =
	dispatch[MEM_START + 8].routine = dispatch[MEM_START + 9].routine =
		(synthRoutine) hammondMemory;
	/*
	 * Mem load and save
	 */
	dispatch[MEM_START + 13].controller = 1;
	dispatch[MEM_START + 14].controller = 2;
	dispatch[MEM_START + 15].controller = 3;
	dispatch[MEM_START + 16].controller = 4;
	dispatch[MEM_START + 13].routine = dispatch[MEM_START + 14].routine
		= dispatch[MEM_START + 15].routine = dispatch[MEM_START + 16].routine
		= (synthRoutine) hammondMemory;

	/*
	 * Midi up/down
	 */
	dispatch[MEM_START + 11].controller = 2;
	dispatch[MEM_START + 12].controller = 1;
	dispatch[MEM_START + 11].routine = dispatch[MEM_START + 12].routine =
		(synthRoutine) hammondMidi;

	/*
	 * Then we do the mods, starting with leslie
	 */
	dispatch[MOD_START].routine = (synthRoutine) doLeslieSpeed;
	dispatch[MOD_START + 1].routine = (synthRoutine) doLeslieSpeed;

	dispatch[MOD_START + 2].routine = (synthRoutine) doVibra;
	dispatch[MOD_START + 3].routine = (synthRoutine) doVibra;
	dispatch[MOD_START + 4].routine = (synthRoutine) doVibra;
	/* Perc */
	dispatch[MOD_START + 5].routine = (synthRoutine) doPerc;
	dispatch[MOD_START + 6].routine = (synthRoutine) doPerc;
	dispatch[MOD_START + 8].routine = (synthRoutine) doPerc;
	/* grooming */
	dispatch[MOD_START + 7].routine = (synthRoutine) doGrooming;
	/* reverb */
	dispatch[MOD_START + 9].routine = (synthRoutine) doReverb;
	dispatch[MOD_START + 10].routine = (synthRoutine) doReverb;
	dispatch[MOD_START + 11].routine = (synthRoutine) doClick;
	dispatch[MOD_START + 12].routine = (synthRoutine) doBright;
	dispatch[MOD_START + 13].routine = (synthRoutine) doOverdrive;

	dispatch[VOL_START].routine = (synthRoutine) doVolume;

	dispatch[SLIDER_START].routine =
	dispatch[SLIDER_START + 1].routine =
	dispatch[SLIDER_START + 2].routine =
	dispatch[SLIDER_START + 3].routine =
	dispatch[SLIDER_START + 4].routine =
	dispatch[SLIDER_START + 5].routine =
	dispatch[SLIDER_START + 6].routine =
	dispatch[SLIDER_START + 7].routine =
	dispatch[SLIDER_START + 8].routine = (synthRoutine) doSlider;

	dispatch[SLIDER_START].controller = 0;
	dispatch[SLIDER_START + 1].controller = 1;
	dispatch[SLIDER_START + 2].controller = 2;
	dispatch[SLIDER_START + 3].controller = 3;
	dispatch[SLIDER_START + 4].controller = 4;
	dispatch[SLIDER_START + 5].controller = 5;
	dispatch[SLIDER_START + 6].controller = 6;
	dispatch[SLIDER_START + 7].controller = 7;
	dispatch[SLIDER_START + 8].controller = 8;

	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 0, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 1, 3);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 2, 16383);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 3, 10);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 4, 13000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 1, 5, 0);

	bristolMidiSendMsg(global.controlfd, synth->sid, 12, 7, 1);
	bristolMidiSendMsg(global.controlfd, synth->sid, 10, 0, 4);

	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 0, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 1, 10);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 2, 13000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 3, 20);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 4, 13000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 3, 5, 0);

	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 0, 2);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 1, 1200);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 2, 0);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 3, 20);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 4, 13000);
	bristolMidiSendMsg(global.controlfd, synth->sid, 4, 5, 0);

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
hammondConfigure(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);

	if (synth == 0)
	{
		printf("problems going operational\n");
		return(-1);
	}

	synth->keypanel = synth->keypanel2 = -1;

	if (synth->flags & OPERATIONAL)
		return(0);

	synth->flags |= OPERATIONAL;
	loadMemory(synth, "hammond", 0, synth->location, synth->mem.active,
		FIRST_DEV, 0);
	configureGlobals(synth);

	return(0);
}

