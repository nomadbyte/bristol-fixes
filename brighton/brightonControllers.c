
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "brightoninternals.h"
#include "brightonMini.h"
#include "brightonKeys.h"
#include "brightondev.h"
#include "bristolmidi.h"
#include "bristolmessages.h"

/*
 * These should also go into the window structure.....
 */
static int confflag = 0;
static brightonDevice *confdev[BRIGHTON_GANG_COUNT];
static float confval[BRIGHTON_GANG_COUNT];

extern guimain global;

extern void printBrightonHelp(int);
extern void printBrightonReadme();
extern void brightonSetCLIcode(char *);
extern void brightonGetCLIcodes(int);

//static int width = 0, height = 0;

/*
 * kbdmap now burried in the brightonWindow structure.
 *
 * This is an array of MIDI note numbers indexed by ASCII keyboard key number
 * and works for top row of qwerty only: azerty, qwertz, dvorak keyboards, etc,
 * will need their own mapping (text configuration) files.
 *
 * Oops, in 0.20.4 this became qwerty key to button index in the GUI piano
 * keyboard starting from zero.
 */
#define KM_KEY 0
#define KM_CHAN 1

/*
 * This is for note on/off events, it keeps a map to supress keyrepeat events.
 *
 * It does not always work since the events are on/off sequentially so it has
 * been extended such that window enter/leave call XAutoRepeatOff/On().
 * A better solution would be for devices to supply their repeat functionality
 * and have it set per device. For example buttons probably don't want this
 * but continuous controllers do.
 *
 * This does not need to be in the window structure as we only have a single
 * computer keyboard.
 */
static int kbdstate[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

char *pheader = "#\n\
# This the the MIDI controller profile, it keeps controller remappings for\n\
# converting one controller ID to another, and then defines which controllers\n\
# are tracked by which GUI devices. It may be edited manually, in which case\n\
# changes are maintained but the GUI can also alter the controller tracking\n\
# by typing <Control><Middle Mouse Button>, the moving the desired control.\n\
# This file is saved whenever a GUI memory is saved.\n\
#\n\
# The file contains Controller Mapping (one controller to another, for example\n\
# breath controller maps to footpedal, etc), Key Mappings for QWERTY to MIDI\n\
# note events, and Continuous Controller mappings that allow a control surface\n\
# to drive the GUI.\n\
#\n\
# Remap format is \"CM: MidiCC   MidiCC\"\n\
# Keyboard map format is \"KM: ASCII   MIDI_note [MIDI_chan]\"\n\
# Control format is \"CC: MidiCC   panel/index [value]\"\n\
#\n\
# The values are integers from 0 to 16384, the fine resolution controller\n\
# value for the throw of this controller. If in doubt use the value 16383,\n\
# it is only relevant for ganging controllers.\n\
#\n\n";

/*
 * This will parse for a key from the ASCII keyboard and map it to a MIDI note
 * allowing the keyboard to then play the synth.
 */
static void
brightonMapKeyboard(brightonWindow *bwin, brightonApp *app,
int channel, char *param)
{
	int from, to, chan;

	from = param[0];

	if ((param = index(param, ' ')) == NULL)
		return;

	to = atoi(param);

	if ((param = index(++param, ' ')) == NULL)
	{
/*		printf("Keymap %c to %i on def channel 0\n", from, to); */
		bwin->kbdmap[from][KM_KEY] = to;
		bwin->kbdmap[from][KM_CHAN] = 0;
		return;
	}

	chan = atoi(++param);

	bwin->kbdmap[from][KM_KEY] = to;
	bwin->kbdmap[from][KM_CHAN] = chan;

/*	printf("Keymap %c to %i on channel %i\n", from, to, chan); */
}

static void
brightonMapControl(brightonWindow *bwin, brightonApp *app,
int channel, char *param)
{
	int from, to;

	from = atoi(param);

	if ((param = index(param, ' ')) == NULL)
	{
		if ((param = index(param, 9)) == NULL)
		{
/*			printf("failed to get target\n"); */
			return;
		}
	}
	to = atoi(param);

	if ((from < 0) || (from > 127)
		|| (to < 0) || (to > 127))
		return;

	bwin->midimap[from] = to;

	printf("Mapped %i to %i\n", from, to);
}

static void
brightonSetNRPControl(brightonWindow *bwin, brightonApp *app,
int channel, char *param)
{
	int cc, panel, ind, i;
	char *pp;
	brightonIResource *p;
	float value = 0.2;

	cc = atoi(param);

	if ((pp = index(param, ' ')) == NULL)
	{
		if ((pp = index(param, 9)) == NULL)
		{
/*			printf("failed to get panel\n"); */
			return;
		}
	}
	panel = atoi(pp);

	if ((pp = index(param, '/')) == NULL)
	{
/*		printf("failed to get index\n"); */
		return;
	}
	ind = atoi(++pp);

	if ((index(pp, ' ')) == NULL)
	{
		if ((pp = index(pp, 9)) == NULL)
		{
			//printf("failed to get value\n");
			value = 1.0;
		} else {
			printf("got number: %s\n", pp);
			value = ((float) atoi(pp)) / 16384.0;
		}
	} else {
		pp = index(pp, ' ');
		value = ((float) atoi(pp)) / 16384.0;
	}

	/*
	 * We need some error checking
	 */
	if ((cc < 0) || (cc > bwin->nrpcount)
		|| (panel < 0) || (panel >= app->nresources)
		|| (ind < 0) || (ind >= app->resources[panel].ndevices))
		return;
	/*
	 * We now have what looks to be a valid tuple cc/panel/index. Find
	 * the configuration code for that device and plug it into our
	 * dispatcher.
	 */

	p = (brightonIResource *) &bwin->app->resources[panel];

	for (i = 0; i < bwin->nrpcount; i++)
	{
		if (bwin->nrpcontrol[i].device == NULL)
		{
			bwin->nrpcontrol[i].device
				= ((struct brightonDevice *) p->devlocn[ind].dev);
			bwin->nrpcontrol[i].nrp = cc;
			break;
		}
	}
}

static void
brightonSetControl(brightonWindow *bwin, brightonApp *app,
int channel, char *param)
{
	int cc, panel, ind, i;
	char *pp;
	brightonIResource *p;
	float value = 0.2, maxvalue = 0.0;

	cc = atoi(param);

	if ((pp = index(param, ' ')) == NULL)
	{
		if ((pp = index(param, 9)) == NULL)
		{
/*			printf("failed to get panel\n"); */
			return;
		}
	}
	panel = atoi(pp);

	if ((pp = index(param, '/')) == NULL)
	{
/*		printf("failed to get index\n"); */
		return;
	}
	ind = atoi(++pp);

	if ((pp = index(param, '/')) == NULL)
	{
/*		printf("failed to get index\n"); */
		return;
	}
	ind = atoi(++pp);

	if ((index(pp, ' ')) == NULL)
	{
		if ((pp = index(pp, 9)) == NULL)
		{
			printf("failed to get value\n");
			value = 1.0;
		} else {
			printf("got number: %s\n", pp);
			value = ((float) atoi(pp)) / 16384.0;
		}
	} else {
		pp = index(pp, ' ');
		value = ((float) atoi(pp)) / 16384.0;
	}

/*printf("%i %i of max %i %i\n", panel, ind, app->nresources, */
/*app->resources[panel].ndevices); */

	/*
	 * We need some error checking
	 */
	if ((cc < 0) || (cc > 127)
		|| (panel < 0) || (panel >= app->nresources)
		|| (ind < 0) || (ind >= app->resources[panel].ndevices))
		return;
	/*
	 * We now have what looks to be a valid tuple cc/panel/index. Find
	 * the configuration code for that device and plug it into our
	 * dispatcher.
	 */
	p = (brightonIResource *) &bwin->app->resources[panel];
	/*
	 * This should not be indexed by the controller rather the device. That
	 * way we can gang controls. Since the only data we have is the cc and
	 * a device, we should limit the number of calls to perhaps 8?
	 */
	for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
	{
		if (bwin->midicontrol[cc][i] == NULL)
		{
			bwin->midicontrol[cc][i]
				= ((struct brightonDevice *) p->devlocn[ind].dev);
			bwin->midicontrolval[cc][i] = value;
/*				= ((brightonDevice *) p->devlocn[ind].dev)->value; */
			break;
		}
	}

	if (i == BRIGHTON_GANG_COUNT)
	{
		bwin->midicontrol[cc][(i = BRIGHTON_GANG_COUNT - 1)]
			= ((struct brightonDevice *) p->devlocn[ind].dev);
		bwin->midicontrolval[cc][i] = value;
	}

	/*
	 * Build our scaling value
	 */
	for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
	{
		if (bwin->midicontrolval[cc][i] > maxvalue)
			maxvalue = bwin->midicontrolval[cc][i];
	}

	if (maxvalue == 0.0)
		bwin->midicontrolscaler[cc] = 1.0;
	else
		bwin->midicontrolscaler[cc] = 1.0 / maxvalue;

/*
	for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
		if (bwin->midicontrol[cc][i] != NULL)
			printf("MIDI CC %i->%i/%i@%x (index %i: %f scaled by %f)\n",
				cc,
				((brightonDevice *) bwin->midicontrol[cc][i])->panel,
				((brightonDevice *) bwin->midicontrol[cc][i])->index,
				(size_t) bwin->midicontrol[cc][i],
				i,
				bwin->midicontrolval[cc][i],
				bwin->midicontrolscaler[cc]);
*/
}

/*
 * Parse configuration file. Read the panel and index numbers for the devices
 * and find the actual device configure code for the controllers.
 *
 * This will also read the midi controller value mapping file.
 */
void
brightonReadConfiguration(brightonWindow *bwin, brightonApp *app,
int channel, char *synth, char *filename)
{
	FILE *fd;
	char path[1024];
	char param[256];
	int kmnotdone = 1, i;

	if (filename == NULL)
		printf("Read Configuration: %s (no filename)\n", synth);
	else
		printf("Read Configuration: %s %s\n", synth, filename);

	/*
	 * The build the value mapping table
	 */
	bristolMidiValueMappingTable(bwin->valuemap, bwin->midimap, synth);

	/* 
	 * We are going to look for a cached file. If we cannot find it then we
	 * will look for a factory version
	 */
	if (filename == NULL)
		sprintf(path, "%s/memory/profiles/%s", getBristolCache(synth), synth);
	else
		sprintf(path, "%s", filename);

	if ((fd = fopen(path, "r")) == NULL)
	{
		/*
		 * If we could not read an explicit profile then return.
		 */
		if (filename != NULL)
			return;

		sprintf(path, "%s/memory/profiles/%s", getenv("BRISTOL"), synth);

		if ((fd = fopen(path, "r")) == NULL)
		{
			/*
			 * So, no profile. Build a default mapping.
			 */
			for (i = 0; i < 256; i++) {
				bwin->kbdmap[i][KM_KEY] = -1;
				bwin->kbdmap[i][KM_CHAN] = 0;
			}

			/*
			 * This maps the top and bottom rows such that the bottom two rows
			 * are from the first key on the GUI keyboard and the top row the
			 * upper octaves of the keyboards.
			 */
			bwin->kbdmap['\\'][KM_KEY] = 0;
			bwin->kbdmap['a'][KM_KEY] = 1;
			bwin->kbdmap['z'][KM_KEY] = 2;
			bwin->kbdmap['s'][KM_KEY] = 3;
			bwin->kbdmap['x'][KM_KEY] = 4;
			bwin->kbdmap['c'][KM_KEY] = 5;
			bwin->kbdmap['f'][KM_KEY] = 6;
			bwin->kbdmap['v'][KM_KEY] = 7;
			bwin->kbdmap['g'][KM_KEY] = 8;
			bwin->kbdmap['b'][KM_KEY] = 9;
			bwin->kbdmap['h'][KM_KEY] = 10;
			bwin->kbdmap['n'][KM_KEY] = 11;
			bwin->kbdmap['m'][KM_KEY] = 12;
			bwin->kbdmap['k'][KM_KEY] = 13;
			bwin->kbdmap[','][KM_KEY] = 14;
			bwin->kbdmap['l'][KM_KEY] = 15;
			bwin->kbdmap['.'][KM_KEY] = 16;
			bwin->kbdmap['/'][KM_KEY] = 17;
			bwin->kbdmap['\''][KM_KEY] = 18;
			bwin->kbdmap['q'][KM_KEY] = 24;
			bwin->kbdmap['2'][KM_KEY] = 25;
			bwin->kbdmap['w'][KM_KEY] = 26;
			bwin->kbdmap['3'][KM_KEY] = 27;
			bwin->kbdmap['e'][KM_KEY] = 28;
			bwin->kbdmap['r'][KM_KEY] = 29;
			bwin->kbdmap['5'][KM_KEY] = 30;
			bwin->kbdmap['t'][KM_KEY] = 31;
			bwin->kbdmap['6'][KM_KEY] = 32;
			bwin->kbdmap['y'][KM_KEY] = 33;
			bwin->kbdmap['7'][KM_KEY] = 34;
			bwin->kbdmap['u'][KM_KEY] = 35;
			bwin->kbdmap['i'][KM_KEY] = 36;
			bwin->kbdmap['9'][KM_KEY] = 37;
			bwin->kbdmap['o'][KM_KEY] = 38;
			bwin->kbdmap['0'][KM_KEY] = 39;
			bwin->kbdmap['p'][KM_KEY] = 40;
			bwin->kbdmap['['][KM_KEY] = 41;
			bwin->kbdmap['='][KM_KEY] = 42;
			bwin->kbdmap[']'][KM_KEY] = 43;

			return;
		}
	}

	/*
	 * So we got a file.
	 */
	while (fgets(param, 256, fd) != NULL)
	{
		if (param[0] == '#')
			continue;

/*		printf("read line %s", param); */

		if (strncmp(param, "CC", 2) == 0)
			brightonSetControl(bwin, app, channel, &param[4]);

		if (strncmp(param, "CLI", 3) == 0)
			brightonSetCLIcode(&param[5]);

		if (strncmp(param, "NRP", 3) == 0)
			brightonSetNRPControl(bwin, app, channel, &param[5]);

		if (strncmp(param, "CM", 2) == 0)
			brightonMapControl(bwin, app, channel, &param[4]);

		if (strncmp(param, "KM", 2) == 0)
		{
			/*
			 * If this is the first keymap in this file then clear out the
			 * existing table. This has the benefit that profiles that did not
			 * contain keymaps will inherit the existing ones however if they
			 * do contain maps then previous maps will be cleared out when new
			 * ones are defined. We only do this once and only if we find a
			 * KM mapping entry.
			 */
			if (kmnotdone)
			{
				for (i = 0; i < 256; i++) {
					bwin->kbdmap[i][KM_KEY] = -1;
					bwin->kbdmap[i][KM_CHAN] = 0;
				}
				kmnotdone = 0;
			}
			brightonMapKeyboard(bwin, app, channel, &param[4]);
		}
	}

	fclose(fd);

	if (kmnotdone)
	{
		for (i = 0; i < 256; i++) {
			bwin->kbdmap[i][KM_KEY] = -1;
			bwin->kbdmap[i][KM_CHAN] = 0;
		}

		/*
		 * This maps the top and bottom rows such that the bottom two rows
		 * are from the first key on the GUI keyboard and the top row the
		 * upper octaves of the keyboards.
		 */
		bwin->kbdmap['\\'][KM_KEY] = 0;
		bwin->kbdmap['a'][KM_KEY] = 1;
		bwin->kbdmap['z'][KM_KEY] = 2;
		bwin->kbdmap['s'][KM_KEY] = 3;
		bwin->kbdmap['x'][KM_KEY] = 4;
		bwin->kbdmap['c'][KM_KEY] = 5;
		bwin->kbdmap['f'][KM_KEY] = 6;
		bwin->kbdmap['v'][KM_KEY] = 7;
		bwin->kbdmap['g'][KM_KEY] = 8;
		bwin->kbdmap['b'][KM_KEY] = 9;
		bwin->kbdmap['h'][KM_KEY] = 10;
		bwin->kbdmap['n'][KM_KEY] = 11;
		bwin->kbdmap['m'][KM_KEY] = 12;
		bwin->kbdmap['k'][KM_KEY] = 13;
		bwin->kbdmap[','][KM_KEY] = 14;
		bwin->kbdmap['l'][KM_KEY] = 15;
		bwin->kbdmap['.'][KM_KEY] = 16;
		bwin->kbdmap['/'][KM_KEY] = 17;
		bwin->kbdmap['\''][KM_KEY] = 18;
		bwin->kbdmap['q'][KM_KEY] = 24;
		bwin->kbdmap['2'][KM_KEY] = 25;
		bwin->kbdmap['w'][KM_KEY] = 26;
		bwin->kbdmap['3'][KM_KEY] = 27;
		bwin->kbdmap['e'][KM_KEY] = 28;
		bwin->kbdmap['r'][KM_KEY] = 29;
		bwin->kbdmap['5'][KM_KEY] = 30;
		bwin->kbdmap['t'][KM_KEY] = 31;
		bwin->kbdmap['6'][KM_KEY] = 32;
		bwin->kbdmap['y'][KM_KEY] = 33;
		bwin->kbdmap['7'][KM_KEY] = 34;
		bwin->kbdmap['u'][KM_KEY] = 35;
		bwin->kbdmap['i'][KM_KEY] = 36;
		bwin->kbdmap['9'][KM_KEY] = 37;
		bwin->kbdmap['o'][KM_KEY] = 38;
		bwin->kbdmap['0'][KM_KEY] = 39;
		bwin->kbdmap['p'][KM_KEY] = 40;
		bwin->kbdmap['['][KM_KEY] = 41;
		bwin->kbdmap['='][KM_KEY] = 42;
		bwin->kbdmap[']'][KM_KEY] = 43;
	}
}

/*
 * Save a configuration file. This should take the panel and index numbers for
 * each of the devices in the table, for a specific MIDI channel, and write 
 * them out to a file with the controller ID.
 */
void
brightonWriteConfiguration(brightonWindow *bwin, char *synth, int channel,
char *filename)
{
	int j, i, fd, null;
	char path[256];
	char param[256];

	if (global.synths->flags & REQ_MIDI_DEBUG)
		printf("Write CC Configuration file: %s\n", synth);

	if (filename == NULL)
		sprintf(path, "%s/memory/profiles/%s", getBristolCache(synth), synth);
	else
		sprintf(path, "%s", filename);

	if ((fd = open(path, O_WRONLY|O_TRUNC|O_CREAT, 0644)) < 0)
	{
		/*
		 * If we were requested to write a specific file but couldn't (implies
		 * permissions or path?) then return.
		 */
		if (filename != NULL)
			return;

		sprintf(path, "%s/memory/profiles/%s", getenv("BRISTOL"), synth);
		/*
		 * We are unlikey to have write permissions on the factory set, however
		 * with no alternative we will have a go
		 */
		if ((fd = open(path, O_WRONLY|O_TRUNC|O_CREAT, 0644)) < 0)
			return;
	}

	/*
	 * This could be cleaned up a bit
	 */
	null = write(fd, pheader, strlen(pheader));

	for (i = 0; i < 128; i++)
	{
		if (bwin->midimap[i] == i)
			continue;

		sprintf(param, "CM: %i	   %i\n", i, bwin->midimap[i]);
		if (global.synths->flags & REQ_MIDI_DEBUG)
			printf("%s", param);
		null = write(fd, param, strlen(param));
	}

	for (i = 0; i < 128; i++)
	{
		for (j = 0; j < BRIGHTON_GANG_COUNT; j++)
		{
			if (bwin->midicontrol[i][j] != 0)
			{
				sprintf(param, "CC: %i	   %i/%i %i\n",
					i,
					((brightonDevice *) bwin->midicontrol[i][j])->panel,
					((brightonDevice *) bwin->midicontrol[i][j])->index,
					(int) (bwin->midicontrolval[i][j] * 16384));
				if (global.synths->flags & REQ_MIDI_DEBUG)
					printf("%s", param);
				null = write(fd, param, strlen(param));
			}
		}
	}

	for (i = 0; i < bwin->nrpcount; i++)
	{
		if (bwin->nrpcontrol[i].device != NULL)
		{
			sprintf(param, "NRP: %i	   %i/%i\n",
				bwin->nrpcontrol[i].nrp,
				((brightonDevice *) bwin->nrpcontrol[i].device)->panel,
				((brightonDevice *) bwin->nrpcontrol[i].device)->index);
			if (global.synths->flags & REQ_MIDI_DEBUG)
				printf("%s", param);
			null = write(fd, param, strlen(param));
		}
	}

	brightonGetCLIcodes(fd);

	for (i = 0; i < 256; i++)
	{
		if (bwin->kbdmap[i][KM_KEY] >= 0)
		{
			sprintf(param, "KM: %c %i %i\n",
				i, bwin->kbdmap[i][KM_KEY], bwin->kbdmap[i][KM_CHAN]);
			null = write(fd, param, strlen(param));
		}
	}

	close(fd);
}

extern char *gplwarranty;
extern char *gplconditions;

void
brightonControlShiftKeyInput(brightonWindow *cid, int asckey, int on, int flags)
{
	guiSynth *synth = findSynth(global.synths, cid);

	if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_2)
		printf("control shift key handler\n");
}

static void
brightonFillRatios(brightonWindow *win)
{
	float wfact, hfact;

	wfact = ((float) win->display->width) * 0.95
		/ ((float) win->template->width);
	hfact = ((float) win->display->height) * 0.95
		/ ((float) win->template->height);

	if (hfact > wfact) {
		win->maxw = win->display->width * 0.95;
		win->minw = win->template->width;
		win->minh = win->template->height;
		win->maxh = win->template->height * wfact;
	} else {
		win->maxh = win->display->height * 0.95;
		win->minw = win->template->width;
		win->minh = win->template->height;
		win->maxw = win->template->width * hfact;
	}
}

void
brightonShiftKeyInput(brightonWindow *cid, int asckey, int on, int flags)
{
	guiSynth *synth = findSynth(global.synths, cid);

	if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_2)
		printf("shift key handler\n");

	switch (asckey) {
		case '=': /* size up */
		{
			int nw, nh;

			if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_3)
				printf("Increase window size\n");

			if ((nw = synth->win->width * 1.1)
				> (synth->win->display->width * 0.9))
				return;
			if ((nh = synth->win->height * 1.1)
				> (synth->win->display->height * 0.9))
				return;

			synth->win->template->width = nw;
			synth->win->template->height = nh;
			brightonFillRatios(synth->win);

			brightonRequestResize(synth->win,
				synth->win->minw,
				synth->win->minh);
			break;
		}
		case '-': /* size down */
		{
			int nw, nh;

			if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_3)
				printf("Decrease window size\n");

			if ((nw = synth->win->width * 0.9)
				< (synth->win->display->width * 0.1))
				return;
			if ((nh = synth->win->height * 0.9)
				< (synth->win->display->height * 0.1))
				return;

			synth->win->template->width = nw;
			synth->win->template->height = nh;
			brightonFillRatios(synth->win);

			brightonRequestResize(synth->win,
				synth->win->minw,
				synth->win->minh);
			break;
		}
		/*
		 * Put some stuff in to return to normal size, full screen, etc
		 */
		case 65293: /* size switch - this is native return key */
			if ((synth->win->width >= (synth->win->display->width * 0.9))
				|| (synth->win->height >= (synth->win->display->height * 0.9)))
			{
			/* Go native size */
				if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_3)
					printf("t: %i %i\nw: %i %i\nd: %i %i\n",
						synth->win->template->width,
						synth->win->template->height,
						synth->win->width,
						synth->win->height,
						synth->win->display->width,
						synth->win->display->height);

				if ((synth->win->minw == 0) || (synth->win->maxw == 0))
					brightonFillRatios(synth->win);

				brightonRequestResize(synth->win,
					synth->win->minw,
					synth->win->minh);

				if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_1)
					printf("go %i %i\n",
						synth->win->minw,
						synth->win->minh);
			} else {
			/* Go full screen */
				if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_3)
					printf("t: %i %i\nw: %i %i\nd: %i %i\n",
						synth->win->minw,
						synth->win->minh,
						synth->win->width,
						synth->win->height,
						synth->win->display->width,
						synth->win->display->height);

				if ((synth->win->minw == 0) || (synth->win->maxw == 0))
					brightonFillRatios(synth->win);

				synth->win->display->flags |= BRIGHTON_ANTIALIAS_5;

				brightonRequestResize(synth->win,
					synth->win->maxw,
					synth->win->maxh);

				if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_1)
					printf("go %i %i\n",
						synth->win->maxw,
						synth->win->maxh);
			}
			break;
	}
}

/*
 * This is called with key press and the control pressed
 */
void
brightonControlKeyInput(brightonWindow *cid, int asckey, int on)
{
	guiSynth *synth = findSynth(global.synths, cid);

	if ((synth->flags & REQ_DEBUG_MASK) >= REQ_DEBUG_2)
		printf("control key event %i/%i\n", asckey, on);

	/*
	 * So, we are going to be looking for diverse key settings to do some stuff.
	 *
	 * ^S: save
	 * ^L: (re)load
	 * ^R: restore previous = ^L?
	 * ^H: help
	 * ^?: help
	 *
	 * ^W: show warranty
	 * ^C: show GLP copying conditions
	 *
	 * plus: BME Axxe B3 Juno Odyssey Poly6 monopoly Pro10 Pro52 Pro5 RR Solina
	 *	voxM2
	 * mult: Bass CS80 granular OBXa(s) OBX(s) Poly800(s) pro1 Sid/2(s) Sonic6
	 *	Stratus
	 * locn: 2600 DX(s) Explorer MemoryMoog Mini MS20 mg1 rBass Rhodes(s) SAKs
	 *	vox
	 * odds: bitone(+100) Jupiter (+mw)
	 */
	switch (asckey) {
		case '+': /* mem up */
		case '=': /* mem up */
//brightonRequestResize(synth->win, synth->win->width *= 1.1, 
//synth->win->height*=1.1);
			if ((synth->flags & REQ_DEBUG_MASK) > REQ_DEBUG_2)
				printf("mem up\n");
			if (synth->loadMemory != NULL)
				synth->loadMemory(synth, synth->resources->name, 0,
					synth->cmem + 1, synth->mem.active, 0, 0);
			else
				loadMemory(synth, synth->resources->name, 0,
					synth->cmem + 1, synth->mem.active, 0, 0);
			break;
		case '-': /* mem down */
		case '_': /* mem down */
//brightonRequestResize(synth->win, synth->win->width *= 0.9, 
//synth->win->height*=0.9);
			if ((synth->flags & REQ_DEBUG_MASK) > REQ_DEBUG_2)
				printf("mem down\n");
			if (synth->loadMemory != NULL)
				synth->loadMemory(synth, synth->resources->name, 0,
					synth->cmem - 1 < 0? 0: synth->cmem - 1,
					synth->mem.active, 0, 0);
			else
				loadMemory(synth, synth->resources->name, 0,
					synth->cmem - 1 < 0? 0: synth->cmem - 1,
					synth->mem.active, 0, 0);
			break;
		case 'h': /* help */
		case '/': /* help */
			printBrightonHelp(synth->synthtype);
			break;
		case 'r': /* readme */
			printBrightonReadme();
			break;
		case 'k':
			printf("Keyboard shortcuts\n");
			printf("  <Ctrl> 's' - save settings to current memory\n");
			printf("  <Ctrl> 'l' - (re)load current memory\n");
			printf("  <Ctrl> 'u' - (re)load current memory (undo)\n");
			printf("  <Ctrl> '-' - load memory down\n");
			printf("  <Ctrl> '+' - load memory up\n");
			printf("  <Ctrl> 'x' - exchange current with previous memory\n");
			printf("  <Ctrl> 'g' - GNU GPL copying conditions\n");
			printf("  <Ctrl> 'h' - print emulator information\n");
			printf("  <Ctrl> 'r' - print readme information\n");
			printf("  <Ctrl> 'p' - screendump to /tmp/<synth>.xpm\n");
			printf("  <Ctrl> 't' - toggle opacity\n");
			printf("  <Ctrl> 'o' - decrease opacity of patch layer\n");
			printf("  <Ctrl> 'O' - increase opacity of patch layer\n");
			printf("  <Ctrl> 'w' - GNU GPL warranty\n");
			printf("  UpArrow    - controller motion up (shift key accelerator)\n");
			printf("  DownArrow  - controller motion down (shift key accelerator)\n");
			printf("  RightArrow - controller motion up (shift key accelerator)\n");
			printf("  LeftArrow  - controller motion down (shift key accelerator)\n");
			break;
		case 's': /* Save */
			if ((synth->flags & REQ_DEBUG_MASK) > REQ_DEBUG_2)
				printf("excepted save memory\n");
			if (synth->saveMemory != NULL)
				synth->saveMemory(synth, synth->resources->name, 0,
					synth->cmem, 0);
			else
				saveMemory(synth, synth->resources->name, 0,
					synth->cmem, 0);
			break;
		case 'l': /* load */
		case 'u': /* load */
			if ((synth->flags & REQ_DEBUG_MASK) > REQ_DEBUG_2)
				printf("excepted Control load memory: %s\n",
					synth->resources->name);
			if (synth->loadMemory != NULL)
				synth->loadMemory(synth, synth->resources->name, 0,
					synth->cmem, synth->mem.active, 0, 0);
			else
				loadMemory(synth, synth->resources->name, 0,
					synth->cmem, synth->mem.active, 0, 0);
			break;
		case 'x': /* Toggle */
		{
			int cmem;

			if ((synth->flags & REQ_DEBUG_MASK) > REQ_DEBUG_2)
				printf("excepted Control switch memory: %s\n",
					synth->resources->name);

			cmem = synth->cmem;

			if (synth->loadMemory != NULL)
				synth->loadMemory(synth, synth->resources->name, 0,
					synth->lmem, synth->mem.active, 0, 0);
			else
				loadMemory(synth, synth->resources->name, 0,
					synth->lmem, synth->mem.active, 0, 0);
			synth->lmem = cmem;
			break;
		}
		case 'w': /* Warranty */
			printf("%s", gplwarranty);
			break;
		case 'g': /* Conditions */
			printf("%s", gplconditions);
			break;
	}
}

/*
 * We now want a set of keyboard mappings that can be defined, perhaps also
 * one per synth and probably also in the same controller mappings file.
 *
 * Due to the may X11 does the key mapping then we will get multiple key events
 * for presses - the key repeat is interpretted as KeyOff/KeyOn, and they KBD
 * will be monophonic as a newly pressed key will replace the previously held
 * one. For best results we would need to disable key repeat on entering the
 * window. FFS.
 */
void
brightonKeyInput(brightonWindow *cid, int asckey, int on)
{
	guiSynth *synth = findSynth(global.synths, cid);
	brightonEvent event;

	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;

	if ((asckey < 0) || (asckey > 255))
		return;

	if ((cid->kbdmap[asckey][KM_KEY] < 0)
		|| (cid->kbdmap[asckey][KM_KEY] > 127))
		return;

	/*
	 * In release -121 this just sent the MIDI note on/off to start the voices
	 * but by popular demand (my sole vote) this should preferably cause the
	 * GUI keyboard to track the qwerty. This means, preferably, the API should
	 * send the keycode to the synth? That is not easy, we could better tack
	 * it in here.
	 *
	 * We need some generic call back to the synth (the right synth) with the
	 * key number and midi channel. Hm, that would work but still would not
	 * change the graphics as that needs a call to the GUI.
	 */
	if (on) {
		/* Filter out key repeat. */
		if ((kbdstate[cid->kbdmap[asckey][KM_KEY]]
			& (1 << cid->kbdmap[asckey][KM_CHAN])) == 0)
		{
			/*
			 * We have some logic required here. Firstly, if the keypanel is
			 * denoted as -1 then there isn't one (hammond module, ARP2600, 
			 * synthi) so use native MIDI events. If the midi channel is
			 * zero this is the first keypanel. Otherwise the second.
			 *
			 * This is all slightly damaged (0.20.3) since calls directly to
			 * the midi interface did not use transpose and those to the GUI
			 * did. That will be changed, tranpose will be an actual call to
			 * bristol, dropped here, but will have to change most of the
			 * profile files that give me the qwerty mappings. I want to change
			 * those anyway to mimic some other well known qwerty mappings.
			 */
			if (synth->keypanel < 0)
				bristolMidiSendMsg(global.controlfd,
					synth->midichannel + cid->kbdmap[asckey][KM_CHAN],
					BRISTOL_EVENT_KEYON, 0, cid->kbdmap[asckey][KM_KEY]);
			else {
				if ((cid->kbdmap[asckey][KM_CHAN] == 0)
					|| (synth->keypanel2 < 0))
					brightonParamChange(synth->win, synth->keypanel,
						cid->kbdmap[asckey][KM_KEY], &event);
				else {
					if (synth->keypanel2 > 0)
						brightonParamChange(synth->win, synth->keypanel2,
							cid->kbdmap[asckey][KM_KEY], &event);
				}
			}
			kbdstate[cid->kbdmap[asckey][KM_KEY]] |= 1 << cid->kbdmap[asckey][KM_CHAN];
		}
	} else {
		event.value = 0.0;

		if (synth->keypanel < 0)
			bristolMidiSendMsg(global.controlfd,
				synth->midichannel + cid->kbdmap[asckey][KM_CHAN],
				BRISTOL_EVENT_KEYOFF, 0, cid->kbdmap[asckey][KM_KEY]);
		else {
			if ((cid->kbdmap[asckey][KM_CHAN] == 0)
				|| (synth->keypanel2 < 0))
				brightonParamChange(synth->win, synth->keypanel,
					cid->kbdmap[asckey][KM_KEY], &event);
			else {
				if (synth->keypanel2 > 0)
					brightonParamChange(synth->win, synth->keypanel2,
						cid->kbdmap[asckey][KM_KEY], &event);
			}
		}
		kbdstate[cid->kbdmap[asckey][KM_KEY]] &= ~(1 << cid->kbdmap[asckey][KM_CHAN]);
	}
}

/*
 * Since this is going to have a graphical response to a MIDI event we need
 * to flag that the library should be idle, then forward the event to have the
 * keyboard mapping change, then re-enable the interface. If this is not done
 * then events may 'double strike', once in the engine and again here in the
 * GUI. Eventually we want to have this link optional so as not to waste CPU
 * cycles for live work.
 */
void
brightonMidiNoteEvent(guimain *global, bristolMidiMsg *msg)
{
	brightonEvent event;
	int flag;

	if (global->home == NULL)
		return;

	event.type = BRIGHTON_FLOAT;
	event.value = 1.0;

	if (global->synths->flags & REQ_MIDI_DEBUG)
		printf("brightonMidiNoteEvent(%i, %i) %i\n",
			msg->command, msg->params.key.key, msg->channel);

	/*
	 * This tracking can only work for the first synth on the list. That is
	 * currently not an issue since the list is probably only one entry. That
	 * will have to change when we integrate GUI menuing to start more
	 * emulations.
	 *
	 * Anyway, NO_KEYTRACK can stay as a global parameter, after that we will
	 * have to scan for MIDI channel matching.
	if ((global->synths == NULL) || (global->synths->flags & NO_KEYTRACK))
	 */
	if (global->synths->flags & NO_KEYTRACK)
		return;

	flag = global->libtest;

	global->libtest = 1;
	if (global->manual != 0)
		global->manual->libtest = 1;

	if (msg->command == MIDI_NOTE_ON) {
		/*
		if ((msg->params.key.key < global->synths->lowkey)
			|| (msg->params.key.key > global->synths->highkey))
			return;
		*/
			
		if (msg->params.key.velocity == 0)
			event.value = 0.0;
		else
			event.value = ((float) msg->params.key.velocity) / 127;

		if (msg->channel == global->synths->midichannel)
			brightonParamChange(global->synths->win, global->synths->keypanel,
				msg->params.key.key - global->synths->transpose, &event);
		else if ((global->synths->keypanel2 >= 0)
			&& (msg->channel == global->synths->midichannel + 1))
			brightonParamChange(global->synths->win, global->synths->keypanel2,
				msg->params.key.key - global->synths->transpose, &event);
	} else {
		/*
		if ((msg->params.key.key < global->synths->lowkey)
			|| (msg->params.key.key > global->synths->highkey))
			return;
		*/
			
		event.value = 0.0;
		if (msg->channel == global->synths->midichannel)
			brightonParamChange(global->synths->win, global->synths->keypanel,
				msg->params.key.key - global->synths->transpose, &event);
		else if ((global->synths->keypanel2 >= 0)
			&& (msg->channel == global->synths->midichannel + 1))
			brightonParamChange(global->synths->win, global->synths->keypanel2,
				msg->params.key.key - global->synths->transpose, &event);
	}

	global->libtest = flag;
	if (global->manual != 0)
		global->manual->libtest = flag;

	return;
}

void
brightonChangeParam(guiSynth *synth, int panel, int ind, float value)
{
	brightonEvent event;
	brightonIResource *p;

	event.type = BRIGHTON_FLOAT;
	event.type = BRIGHTON_PARAMCHANGE;
	event.value = value;

	if (panel < 0) return;
	if (panel >= synth->win->app->nresources) return;
	if (ind < 0) return;
	if (ind >= synth->win->app->resources[panel].ndevices) return;

	p = (brightonIResource *) &synth->win->app->resources[panel];

	if ((p->devlocn[ind].type == 2) &&
		(~p->devlocn[ind].flags & BRIGHTON_THREEWAY))
	{
		if (value < ((brightonDevice *)
			synth->win->app->resources[panel].devlocn[ind].dev)->value)
			event.value = 0.0f;
		else
			event.value = 1.0f;
	}

	if (event.value < 0)
		event.value = 0.0f;
	if (event.value > p->devlocn[ind].to)
		event.value = p->devlocn[ind].to;

	/*
	 * This might look odd, but if the 'to' is not 1.0 then the
	 * interface expects 'N' integral steps so we have to round
	 * value here. We perhaps should use 'truncf()'.
	if (p->devlocn[ind].to != 1.0)
		event.value = (float)
			((int) (event.value * p->devlocn[ind].to));
	 */

	/*
	 * See if we need to reverse the value already, and to scale
	 * it to a native controller range. There are other cases, such
	 * as "from > to" without the REVERSE flag, etc. Will address
	 * them on a case by case basis.
	if (p->devlocn[ind].flags & BRIGHTON_REVERSE)
		event.value = 1.0 - event.value;
printf("\r%f %f %f", value, event.value, p->devlocn[ind].to);
	 */

	brightonParamChange(synth->win, panel, ind, &event);
	/*
printf(" %f\n\r", 
((brightonDevice *) synth->win->app->resources[panel].devlocn[ind].dev)->value);
*/
}

/*
 * Common dispatch routine for controller to GUI events. Needs to find the
 * right synth, not trivial - may have multiple on this channel, then needs to
 * call parsing and mapping for each.
 */
void
brightonMidiInput(bristolMidiMsg *msg, guimain *global)
{
	int i, j;
	float maxvalue = 0.0;
	guiSynth *synth;

	if (global->synths == 0)
		return;

	if (global->synths->flags & REQ_MIDI_DEBUG)
		printf("brightonMidiInput: %x %i: from %i, cfg %i\n",
		msg->command,
		msg->channel,
		msg->params.bristol.from,
		global->controlfd);

	if (msg->command == MIDI_SYSTEM)
	{
		int memHold = global->synths->cmem;

		if (global->synths->flags & REQ_MIDI_DEBUG)
			printf("brightonMidiInput sysex\n");
		/*
		printf("brightonMidiInput sysex: %i %i, %i\n",
			msg->command,
			msg->channel,
			global->synths->midichannel);
		 * We need to see if anybody is waiting for a message
		 */
		if (msg->params.bristol.msgType >= 8) {
			switch (msg->params.bristolt2.operation)
			{
				case BRISTOL_MT2_WRITE:
					/*
					 * The brighton load/save routines do not really handle
					 * alternative locations, they all rotate around the cache.
					 *
					 * To support Jack here, and LADI later, then just save the
					 * LADI memory file and copy it to wherever it was asked
					 * to be put.
					 */
					printf("bsm save request to \"%s\"\n",
						msg->params.bristolt2.data);

					global->synths->cmem = global->synths->ladimem;
					brightonControlKeyInput(global->synths->win, 's', 0);

					bristolMemoryExport(global->synths->ladimem,
						msg->params.bristolt2.data,
						global->synths->resources->name);

					global->synths->cmem = memHold;
					break;
				case BRISTOL_MT2_READ:
					printf("bsm load request from \"%s\"\n",
						msg->params.bristolt2.data);

					bristolMemoryImport(global->synths->ladimem,
						msg->params.bristolt2.data,
						global->synths->resources->name);

					global->synths->cmem = global->synths->ladimem;
					brightonControlKeyInput(global->synths->win, 'l', 0);

					global->synths->cmem = memHold;
					break;
			}
		} else if (msg->params.bristol.operator == BRISTOL_LADI) {
			if (msg->params.bristol.controller == BRISTOL_LADI_SAVE_REQ) {
				printf("LADI save state request\n");
				global->synths->cmem = global->synths->ladimem;

				brightonControlKeyInput(global->synths->win, 's', 0);

				global->synths->cmem = memHold;
			}

			if (msg->params.bristol.controller == BRISTOL_LADI_LOAD_REQ) {
				printf("LADI load state request\n");
				global->synths->cmem = global->synths->ladimem;

				brightonControlKeyInput(global->synths->win, 'l', 0);

				global->synths->cmem = memHold;
			}
		}

		bristolMidiPost(msg);
		return;
	}

	if (global == NULL)
		return;

#warning that we need to scan the synths to match the MIDI channel
	if ((synth = global->synths) == NULL)
		return;

	if (~synth->flags & OPERATIONAL)
		return;

	if (global->synths->flags & REQ_DEBUG_3)
		printf("not sysex: %x\n", msg->command);

	/*
	 * We should consider what to do with channel changes, but we are not
	 * responsible for those. This is what the controller sent although we
	 * could copy the values across from one table to the other?
	 *
	 * We should also consider an interface to allow for note on/off events,
	 * it would need to register with panel ID and transpose.
	 * We should also consider an interface to allow for pitch wheel
	 * reregistration.
	 */
	if (((msg->command & MIDI_COMMAND_MASK)	== MIDI_NOTE_ON)
		|| ((msg->command & MIDI_COMMAND_MASK)  == MIDI_NOTE_OFF))
	{
		if (global->synths->flags & REQ_DEBUG_3)
			printf("note event: %x\n", msg->command);
		brightonMidiNoteEvent(global, msg);
		return;
	}

	if (msg->command != MIDI_CONTROL)
	{
		if (msg->command == MIDI_PROGRAM)
		{
			/*
			printf("need a callback for this: prg %i\n",
				msg->params.program.p_id);
			 * So who should have the callback. It is really a window function
			 * however it only has callbacks for X Events, not MIDI events.
			 *
			 * This callback template is not used, it requires a window, then
			 * a couple of ints and a float. Should we use controller and
			 * value?
			 */
			if ((msg->channel == synth->midichannel)
				&& (synth->win->template->callback != 0))
                synth->win->template->callback(synth->win,
					msg->command, msg->params.program.p_id, 0);
		}
		return;
	}

	/*
	 * At this point there are only control messages left. I want to have a
	 * mapping of the controller value as well as the controller mapping and
	 * the value should be translated first.
	bristolMidiToGM2(synth->win, msg);
	 */
//printf("PRE: %f\n", msg->params.controller.c_val);
	bristolMidiToGM2(synth->win->GM2values, synth->win->midimap,
		synth->win->valuemap, msg);
//printf("POST: %f\n", msg->params.controller.c_val);

	/* 
	 * This should search the synth list, here we just assume one synth per
	 * GUI and that is damaged for dual manual synths. Only affects the GUI.
	 * We should search for the MIDI channel by sid and sid2 - if sid2 then 
	 * the event is potentially for the second manual.
	 */
	if (msg->channel != synth->midichannel)
		return;

/*printf("Midi Ctrl: %i\n", msg->GM2.c_id); */

	if ((msg->params.controller.c_id ==  MIDI_GM_DATAENTRY_F)
		&& (synth->flags & REQ_MIDI_DEBUG))
	{
		/*
		 * The GM2 conversions bury the controller ID in coarse for
		 * the NRP operations.
		 */
		printf("found nrp: %i value %i\n",
			(synth->win->GM2values[MIDI_GM_NRP] << 7)
				+ synth->win->GM2values[MIDI_GM_NRP_F],
			(synth->win->GM2values[MIDI_GM_DATAENTRY] << 7)
				+ synth->win->GM2values[MIDI_GM_DATAENTRY_F]);

		printf("	%i %i %i %i: %i %i/%f (%i)\n",
			synth->win->GM2values[MIDI_GM_NRP],
			synth->win->GM2values[MIDI_GM_NRP_F],
			synth->win->GM2values[MIDI_GM_DATAENTRY],
			synth->win->GM2values[MIDI_GM_DATAENTRY_F],
			msg->GM2.coarse, msg->GM2.intvalue, msg->GM2.value,
			msg->GM2.c_id);
	}

	if (confflag > 0) {
		/*
		 * Skip NRP/RP/DE if we have not allowed NRP
		 */
		if ((msg->params.controller.c_id == MIDI_GM_DATAENTRY) ||
			(msg->params.controller.c_id == MIDI_GM_NRP) ||
			(msg->params.controller.c_id == MIDI_GM_NRP_F) ||
			(msg->params.controller.c_id == MIDI_GM_RP_F))
			return;

		/*
		 * React to MIDI_GM_DATAENTRY_F, find NRP, find value, decide where
		 * to dispatch.
		 */
		if ((msg->GM2.c_id == MIDI_GM_NRP)
			&& (msg->params.controller.c_id == MIDI_GM_DATAENTRY_F))
		{
			confflag = 0;

			if (~synth->flags & GUI_NRP)
			{
				printf("NRP Registration Requests require -gnrp\n");
				return;
			}

			if (synth->flags & REQ_MIDI_DEBUG)
			{
				/*
				 * The GM2 conversions bury the controller ID in coarse for
				 * the NRP operations.
				 */
				printf("register nrp: %i value %i\n",
					(synth->win->GM2values[MIDI_GM_NRP] << 7)
						+ synth->win->GM2values[MIDI_GM_NRP_F],
					(synth->win->GM2values[MIDI_GM_DATAENTRY] << 7)
						+ synth->win->GM2values[MIDI_GM_DATAENTRY_F]);

				printf("	%i %i %i %i: %i %i\n",
					synth->win->GM2values[MIDI_GM_NRP],
					synth->win->GM2values[MIDI_GM_NRP_F],
					synth->win->GM2values[MIDI_GM_DATAENTRY],
					synth->win->GM2values[MIDI_GM_DATAENTRY_F],
					msg->GM2.coarse, msg->GM2.intvalue);
			}
			/*
			 * Now need to decide what to do. Firstly, drop ganging of NRP for
			 * the time being, may build a second table of NRP registrations
			 * where the table contains N entries only. We will insert this
			 * NRP into the first free entry, then later when we are given NRP
			 * messages we will search for them in this table.
			 *
			 * In the longer term we need to rewrite this to get rid of the
			 * internal MIDI references, convert MIDI CC/RP/NRP, etc, into
			 * an internal format in the library and then handle them here in 
			 * a common fashion. Perhaps this table will do that however then
			 * it would need to support ganging.
			 */
			/*
			 * Until we have ganging support, clear any old registrations.
			 */
			for (i = 0; i < synth->win->nrpcount; i++)
			{
				if ((synth->win->nrpcontrol[i].nrp == msg->GM2.coarse)
					&& (synth->win->nrpcontrol[i].device != NULL))
				{
					printf("NRP Controller Registration Cleared: %i@%p\n",
						synth->win->nrpcontrol[i].nrp,
						synth->win->nrpcontrol[i].device);
					synth->win->nrpcontrol[i].nrp = -1;
					synth->win->nrpcontrol[i].device = NULL;
				}
			}

			if (confdev[0] == NULL)
				return;

			for (i = 0; i < synth->win->nrpcount; i++)
			{
				if (synth->win->nrpcontrol[i].device == NULL)
				{
					synth->win->nrpcontrol[i].nrp = msg->GM2.coarse;
					synth->win->nrpcontrol[i].device
						= (struct brightonDevice *) confdev[0];
					printf("NRP Controller Registration Honoured: %i@%p\n",
						synth->win->nrpcontrol[i].nrp,
						synth->win->nrpcontrol[i].device);
					confdev[0] = 0;
					return;
				}
			}
		}

		if (msg->GM2.c_id >= 127)
		{
			/* This should not happen so flag it */
			printf("Current support limited to first 128 RP/NRP\n");
			confflag = 0;
			return;
		}

		/*
		 * Link this CC to the given device, by MIDI channel? This also
		 * needs to be made into a GM-2 controller (perhaps that should
		 * be buried in the MIDI MSG structure - native and GM-2?)
		 */
		for (j = 0; j < BRIGHTON_GANG_COUNT; j++)
		{
			if (confdev[j] == 0)
				continue;

			for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
			{
				if ((brightonDevice *)
					synth->win->midicontrol[msg->GM2.c_id][i]
						== confdev[j])
				{
					printf("Controller Registration Cleared: %i -> %i/%i@%p\n",
						msg->GM2.c_id, confdev[j]->panel,
						confdev[j]->index, confdev[j]);
					synth->win->midicontrol[msg->GM2.c_id][i] = 0;
					break;
				} else {
					if (synth->win->midicontrol[msg->GM2.c_id][i] != 0)
					{
						if (synth->win->midicontrolval[msg->GM2.c_id][i]
							> maxvalue)
							maxvalue = synth->win->midicontrolval[msg->GM2.c_id][i];

						continue;
					}

					synth->win->midicontrol[msg->GM2.c_id][i]
						= (struct brightonDevice *) confdev[j];
					synth->win->midicontrolval[msg->GM2.c_id][i]
						= confval[j];
/*						= ((brightonDevice *) confdev[j])->value; */

					printf("Controller Registration Honoured: %i -> %i/%i@%p %f\n",
						msg->GM2.c_id, confdev[j]->panel,
						confdev[j]->index, confdev[j], confval[j]);
/*					if (synth->win->midicontrolval[msg->GM2.c_id][i] */
/*						> maxvalue) */
/*						maxvalue = synth->win->midicontrolval[msg->GM2.c_id][i]; */
					break;
				}
			}
		}

		confflag = 0;
		for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
		{
			if (synth->win->midicontrolval[msg->GM2.c_id][i]
				> maxvalue)
				maxvalue = synth->win->midicontrolval[msg->GM2.c_id][i];
			confdev[i] = 0;
		}
		/*
		 * We now have a max value, we need to work on a scaler such that 
		 * this gang will scale to max
		 */
		if (maxvalue == 0.0f)
		{
			synth->win->midicontrolscaler[msg->GM2.c_id] = 1.0;
		} else {
			synth->win->midicontrolscaler[msg->GM2.c_id] = 1.0 / maxvalue;
		}
	} else {
		brightonEvent event;

		if ((msg->params.controller.c_id == MIDI_GM_DATAENTRY) ||
			(msg->params.controller.c_id == MIDI_GM_NRP) ||
			(msg->params.controller.c_id == MIDI_GM_NRP_F) ||
			(msg->params.controller.c_id == MIDI_GM_RP) ||
			(msg->params.controller.c_id == MIDI_GM_RP_F))
			return;

		if (msg->GM2.c_id == MIDI_GM_NRP)
		{
			if (~synth->flags & GUI_NRP)
				return;

			if (synth->flags & REQ_MIDI_DEBUG)
				printf("NRP Message %i\n", msg->GM2.coarse);

			for (i = 0; i < synth->win->nrpcount; i++)
			{
				if (synth->win->nrpcontrol[i].nrp == msg->GM2.coarse)
				{
					brightonIResource *p;
					int panel, cc, ind, channel;

					if (synth->flags & REQ_MIDI_DEBUG)
						printf("Found %i@%p\n",
							synth->win->nrpcontrol[i].nrp,
							synth->win->nrpcontrol[i].device);

					panel = ((brightonDevice *)
						synth->win->nrpcontrol[i].device)->panel;

					ind = ((brightonDevice *)
						synth->win->nrpcontrol[i].device)->index;

					cc = msg->GM2.c_id;
					channel = msg->channel;

					event.value = msg->GM2.value;
					event.command = event.type = BRIGHTON_PARAMCHANGE;

					p = (brightonIResource *)
						&synth->win->app->resources[panel];

				/*
				 * See if we need to reverse the value already, and to scale
				 * it to a native controller range. There are other cases, such
				 * as "from > to" without the REVERSE flag, etc. Will address
				 * them on a case by case basis.
				 */
				if (p->devlocn[ind].flags & BRIGHTON_REVERSE)
					event.value = 1.0 - event.value;
				/*
				 * This might look odd, but if the 'to' is not 1.0 then the
				 * interface expects 'N' integral steps so we have to round
				 * value here. We perhaps should use 'truncf()'.
				 */
				if (p->devlocn[ind].to != 1.0)
					event.value = (float)
						((int) (event.value * p->devlocn[ind].to));

				/*
				if (((brightonDevice *)
					synth->win->midicontrol[msg->GM2.c_id][i])->device
					== 1)
					event.value = 1.0 - event.value;
				 */

/*			printf( */
/*				"synth->win->midicontrol[%i][%i]->%x(%x, &event): %f\n", */
/*				msg->GM2.c_id, */
/*				msg->channel, */
/*				synth->win->midicontrol[msg->GM2.c_id]->configure, */
/*				synth->win->midicontrol[msg->GM2.c_id], */
/*				event.value); */

				((brightonDevice *)
					synth->win->nrpcontrol[i].device)->configure(
					synth->win->nrpcontrol[i].device, &event);
				}

			}
			return;
		}

		/*
		 * If this was bank select pass it to the synth
		 */
		if (msg->GM2.c_id == 0)
		{
			if ((msg->channel == synth->midichannel)
				&& (synth->win->template->callback != 0))
                synth->win->template->callback(synth->win,
					MIDI_BANK_SELECT, msg->GM2.intvalue, 0);
			return;
		}

		/*
		 * This next routine should use a parser on the message that will
		 * build a GM-2 value into the msg, this will take care of things like
		 * controllers that have fine and coarse resolution and will suitably
		 * adjust the value to a suitable float, also changing the controller
		 * number if this is the fine adjustment of a coarse control. This 
		 * may already have been done by the library?
		 */
		for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
		{
			if (synth->win->midicontrol[msg->GM2.c_id][i] != 0)
			{
				brightonIResource *p;
				int panel, cc, ind, channel;

				panel = ((brightonDevice *)
					synth->win->midicontrol[msg->GM2.c_id][i])->panel;
				ind = ((brightonDevice *)
					synth->win->midicontrol[msg->GM2.c_id][i])->index;
				cc = msg->GM2.c_id;
				channel = msg->channel;

				/*
				 * This value needs to be scaled according to the limits that
				 * were set by the original registration. We should check for
				 * max values.
				 */
/*				event.value = msg->GM2.value; */
				if ((event.value = msg->GM2.value *
					synth->win->midicontrolval[msg->GM2.c_id][i] *
					synth->win->midicontrolscaler[msg->GM2.c_id])
					> 1.0)
					event.value = 1.0;
				event.command = event.type = BRIGHTON_PARAMCHANGE;

				p = (brightonIResource *)
					&synth->win->app->resources[panel];

				/*
				 * See if we need to reverse the value already, and to scale
				 * it to a native controller range. There are other cases, such
				 * as "from > to" without the REVERSE flag, etc. Will address
				 * them on a case by case basis.
				 */
				if (p->devlocn[ind].flags & BRIGHTON_REVERSE)
					event.value = 1.0 - event.value;
				/*
				 * This might look odd, but if the 'to' is not 1.0 then the
				 * interface expects 'N' integral steps so we have to round
				 * value here. We perhaps should use 'truncf()'.
				 */
				if (p->devlocn[ind].to != 1.0)
					event.value = (float)
						((int) (event.value * p->devlocn[ind].to));

				/*
				if (((brightonDevice *)
					synth->win->midicontrol[msg->GM2.c_id][i])->device
					== 1)
					event.value = 1.0 - event.value;
				 */

/*			printf( */
/*				"synth->win->midicontrol[%i][%i]->%x(%x, &event): %f\n", */
/*				msg->GM2.c_id, */
/*				msg->channel, */
/*				synth->win->midicontrol[msg->GM2.c_id]->configure, */
/*				synth->win->midicontrol[msg->GM2.c_id], */
/*				event.value); */

				((brightonDevice *)
					synth->win->midicontrol[msg->GM2.c_id][i])->configure(
					synth->win->midicontrol[msg->GM2.c_id][i], &event);
			}

		}
	}
}

/*
 * Common dispatch routine for Event selection to controller events.
 */
void
brightonRegisterController(brightonDevice *dev)
{
	int i;

	for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
	{
		if (confdev[i] == dev)
		{
			printf("Controller Registration %i Cleared: ? -> %i/%i@%p\n",
				i, dev->panel, dev->index, dev);

			confdev[i] = 0;
			if (--confflag <= 0)
			{
				confflag = 0;
				for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
					confdev[i] = 0;
			}

			return;
		}
	}

	/*
	 * This should go into a table. Then, when we get the target controller
	 * we change the device value and consider a paramchange message to be
	 * sent to update the GUI (and send the value?).
	confflag = 0;
	confdev = dev;
	 */
	for (i = 0; i < BRIGHTON_GANG_COUNT; i++)
	{
		if (confdev[i] == 0)
		{
			confdev[i] = dev;

			if (dev->device == 1)
				confval[i] = 1.0 - dev->value;
			else
				confval[i] = dev->value;

			if (confval[i] == 0.0)
				confval[i] = 1.0;

			break;
		}
	}
	if (++confflag >= BRIGHTON_GANG_COUNT)
	{
		confflag = BRIGHTON_GANG_COUNT;
		confdev[BRIGHTON_GANG_COUNT - 1] = dev;
	}

	printf("Controller Registration %i Request: ? -> %i/%i@%p %f\n",
		i, dev->panel, dev->index, dev, dev->value);
}

