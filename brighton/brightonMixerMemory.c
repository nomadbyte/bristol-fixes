
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
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bristol.h"
#include "bristolmidi.h"
#include "brightonMixer.h"
#include "brightonMixerMemory.h"

#include "brighton.h"
#include "brightonMini.h"

static mixerMem *new = NULL, *prev = NULL;
static char songDir[32] = "default";

static DIR *memlist = NULL;
static struct dirent *entry;

extern guimain global;
extern void displayPanel(guiSynth *, char *, int, int, int);

/*
 * Most of this is actually borrowed from brightonroutines.c, but since the
 * mixer operations are a little central to the whole application have chosen
 * to make it bespoke
 */
int saveMixerMemory(guiSynth *synth, char *name)
{
	char path[256];
	int fd;

	sprintf(path, "%s/memory/%s/%s/%s.mem",
		getBristolCache("midicontrollermap"), "mixer", songDir, name);
	sprintf(synth->mem.algo, "mixer");
	if (name == NULL)
		sprintf(synth->mem.name, "no name");
	else
		sprintf(synth->mem.name, "%s", name);

	if ((fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0660)) < 0)
		return(-1);

	if (write(fd, &synth->mem, sizeof(struct Memory) - sizeof(float *)) < 0)
		printf("write failed 1\n");
	if (write(fd, synth->mem.param, sizeof(mixerMem)) < 0)
		printf("write failed 1\n");

	close(fd);

	return(0);
}

static int
doLoadMixerMemory(guiSynth *synth)
{
	brightonEvent event;
	int channel, param;

	event.type = BRIGHTON_FLOAT;

	/*
	 * Now we have the parameters loaded we need to start sending off events
	 * to the GUI to draw the mixer, that will notify the engine and set the
	 * parameters here again (as an undesirable sideffect);
	 *
	 * Going to start with the steroe bus section as it is (comparatively) easy.
	 */
	for (channel = 0; channel < 4; channel++)
	{
		event.value = new->vbus[channel].clear.vol;
		brightonParamChange(synth->win, BUS_PANEL,
			240 + channel * 3, &event);

		event.value = new->vbus[channel].clear.left;
		brightonParamChange(synth->win, BUS_PANEL,
			240 + channel * 3 + 1, &event);

		event.value = new->vbus[channel].clear.right;
		brightonParamChange(synth->win, BUS_PANEL,
			240 + channel * 3 + 2, &event);
	}

	/*
	 * Effects bussing section
	 */
	for (channel = 0; channel < 8; channel++)
	{
		/*
		 * Six events for the continuous controllers
		 */
		for (param = 0; param < 6; param++)
		{
			event.value = new->bus[channel].b.opaque[param];
			brightonParamChange(synth->win, BUS_PANEL,
				channel * 30 + param, &event);
		}

		/*
		 * One event for the FX select
		new->bus[channel].b.clear.algorithm;
		 */
		event.value = 1.01;
		if (new->bus[channel].b.clear.algorithm < 0)
		{
			brightonParamChange(synth->win, BUS_PANEL,
				channel * 30 + 6, &event);
			event.value = 0.0;
			brightonParamChange(synth->win, BUS_PANEL,
				channel * 30 + 6, &event);
		} else {
			if (new->bus[channel].b.clear.algorithm !=
				prev->bus[channel].b.clear.algorithm)
				brightonParamChange(synth->win, BUS_PANEL,
					channel * 30 + 6 + new->bus[channel].b.clear.algorithm,
						&event);
		}

		/*
		 * Sixteen events for the output busses.
		 */
		for (param = 0; param < 16; param++)
		{
			/*
			if (new->bus[channel].b.clear.outputSelect[param] <= 0)
				event.value = 0.0;
			else
				event.value = 1.01;
			*/
			event.value = new->bus[channel].b.clear.outputSelect[param];
			brightonParamChange(synth->win, BUS_PANEL,
				channel * 30 + 14 + param, &event);
		}
	}

	/*
	 * And the channels themselves.
 	 */
	for (channel = 0; channel < new->chancount; channel++)
	{
		/*
		 * One event for the input selection. There are several logical states:
		 * 1. New track has selection - call it.
		 * 2. New Track has no selection - clear the previous one.
	 	 */
		if (new->chan[channel].inputSelect >= 0)
		{
			/*
			 * Only apply if the parameter has changed
			 */
			if (new->chan[channel].inputSelect
				!= prev->chan[channel].inputSelect)
			{
				event.value = 1.0;
				brightonParamChange(synth->win, channel + 4,
					new->chan[channel].inputSelect, &event);
			}
		} else {
			event.value = 1.0;
			brightonParamChange(synth->win, channel + 4, 0, &event);
			event.value = 0.0;
			brightonParamChange(synth->win, channel + 4, 0, &event);
		}

		/*
		 * Presend bus selections
		 */
		for (param = 0; param < 4; param++)
		{
			event.value = new->chan[channel].preSend[param];
			brightonParamChange(synth->win, channel + 4,
				16 + param, &event);
		}

		/*
		 * Dynamics algorithm selection
		 */
		if (new->chan[channel].dynamics >= 0)
		{
			if (new->chan[channel].dynamics != prev->chan[channel].dynamics)
			{
				event.value = 1.01;
				brightonParamChange(synth->win, channel + 4,
					20 + new->chan[channel].dynamics, &event);
			}
		} else {
			event.value = 1.01;
			brightonParamChange(synth->win, channel + 4,
				20, &event);
			event.value = 0.0;
			brightonParamChange(synth->win, channel + 4,
				20, &event);
		}

		/*
		 * Filter algorithm selection
		 */
		if (new->chan[channel].filter >= 0)
		{
			if (new->chan[channel].filter != prev->chan[channel].filter)
			{
				event.value = 1.01;
				brightonParamChange(synth->win, channel + 4,
					23 + new->chan[channel].filter, &event);
			}
		} else {
			event.value = 1.01;
			brightonParamChange(synth->win, channel + 4,
				23, &event);
			event.value = 0.0;
			brightonParamChange(synth->win, channel + 4,
				23, &event);
		}

		/*
		 * Postsend bus selections
		 */
		for (param = 0; param < 4; param++)
		{
			event.value = new->chan[channel].postSend[param];
			brightonParamChange(synth->win, channel + 4,
				27 + param, &event);
		}

		/*
		 * FX algorithm selection
		 */
		if (new->chan[channel].fxAlgo >= 0)
		{
			if (new->chan[channel].fxAlgo != prev->chan[channel].fxAlgo)
			{
				event.value = 1.01;
				brightonParamChange(synth->win, channel + 4,
					31 + new->chan[channel].fxAlgo, &event);
			}
		} else {
			event.value = 1.01;
			brightonParamChange(synth->win, channel + 4,
				31, &event);
			event.value = 0.0;
			brightonParamChange(synth->win, channel + 4,
				31, &event);
		}

		/*
		 * Continuous controllers
		 */
		for (param = 0; param < 17; param++)
		{
			event.value = new->chan[channel].p.opaque[param];
			brightonParamChange(synth->win, channel + 4,
				38 + param, &event);
		}

		event.value = new->chan[channel].mute;
		brightonParamChange(synth->win, channel + 4, 55, &event);
		event.value = new->chan[channel].solo;
		brightonParamChange(synth->win, channel + 4, 56, &event);
		event.value = new->chan[channel].boost;
		brightonParamChange(synth->win, channel + 4, 57, &event);

		/*
		 * Output select
		 */
		for (param = 0; param < 16; param++)
		{
			event.value = new->chan[channel].outputSelect[param];
			brightonParamChange(synth->win, channel + 4,
				58 + param, &event);
		}

		/*
		 * vBus selection
		 */
		if (new->chan[channel].stereoBus >= 0)
		{
			if (new->chan[channel].stereoBus != prev->chan[channel].stereoBus)
			{
				event.value = 1.01;
				brightonParamChange(synth->win, channel + 4,
					74 + new->chan[channel].stereoBus, &event);
			}
		} else {
			event.value = 1.01;
			brightonParamChange(synth->win, channel + 4,
				74, &event);
			event.value = 0.0;
			brightonParamChange(synth->win, channel + 4,
				74, &event);
		}

		event.value = new->chan[channel].p.clear.vol;
		brightonParamChange(synth->win, channel + 4, 78, &event);

		displayPanel(synth, new->chan[channel].scratch, 0, channel + 4, 79);
		sprintf(((mixerMem *) synth->mem.param)->chan[channel].scratch, "%s", new->chan[channel].scratch);
	}

	return(0);
}

/*
 * Saving the memory is reasonably trivial. Reading it back is a little more
 * work since we have to call the parameter routines to set the values.
 */
int loadMixerMemory(guiSynth *synth, char *name, int param)
{
	char path[256];
	int fd;

	if (param == 1)
	{
		sprintf(songDir, "%s", name);
		return(0);
	}
	if (param == 2)
	{
		if (strlen(name) != 0)
		{
			sprintf(path, "%s/memory/%s/%s", getBristolCache("midicontrollermap"), "mixer", name);
			mkdir(path, 0755);
		}
		return(0);
	}

	if (strcmp(name, "revert") == 0)
	{
		mixerMem *t;

		bcopy(prev, new, sizeof(mixerMem));
		t = prev;
		prev = (mixerMem *) synth->mem.param;
		synth->mem.param = (float *) t;

		doLoadMixerMemory(synth);

		return(0);
	}

	sprintf(path, "%s/memory/%s/%s/%s.mem",
		getBristolCache("midicontrollermap"), "mixer", songDir, name);

	if ((fd = open(path, O_RDONLY, 0770)) < 0)
	{
		sprintf(path, "%s/memory/%s/%s/%s.mem",
			global.home, "mixer", songDir, name);
		if ((fd = open(path, O_RDONLY, 0770)) < 0)
			return(-1);
	}

	if (read(fd, &synth->mem.algo[0], 32) < 0)
		printf("read failed\n");
	if (read(fd, &synth->mem.name[0], 32) < 0)
		printf("read failed\n");

	if (read(fd, new, 2 * sizeof(int)) < 0)
		printf("read failed\n");

	if (read(fd, new, sizeof(mixerMem)) < 0)
		printf("read failed\n");

	bcopy(synth->mem.param, prev, sizeof(mixerMem));

	doLoadMixerMemory(synth);

	close(fd);

	return(0);
}

/*
 * Get a memory structure, null it out, put in a few details and give it back
 */
void *
initMixerMemory(int count)
{
	int i;
	mixerMem *m;

	if ((m = (mixerMem *) malloc(sizeof(mixerMem))) == NULL)
		return(0);
	if ((new = (mixerMem *) malloc(sizeof(mixerMem))) == NULL)
		return(0);
	if ((prev = (mixerMem *) malloc(sizeof(mixerMem))) == NULL)
		return(0);

	bzero(m, sizeof(mixerMem));

	sprintf(&m->name[0], "no name");
	m->version = MIXER_VERSION;
	m->chancount = count;

	for (i = 0; i < MAX_CHAN_COUNT; i++)
	{
		m->chan[i].inputSelect = -1;
		m->chan[i].dynamics = -1;
		m->chan[i].filter = -1;
		m->chan[i].fxAlgo = -1;
		m->chan[i].stereoBus = -1;
		sprintf(&m->chan[i].scratch[0], "Trk: %i", i + 1);
	}
	for (i = 0; i < MAX_VBUS_COUNT; i++)
		m->bus[i].b.clear.algorithm = -1;

	bcopy(m, prev, sizeof(mixerMem));

	return(m);
}

char *
getMixerMemory(mixerMem *m, int op, int param)
{
	char path[256], *dotptr;

	switch(op)
	{
		case MM_GETLIST:
			if (memlist == 0)
			{
				/*
				 * See if we want to list songs or memories in a song dir
				 */
				if (param == 1)
					sprintf(path, "%s/memory/%s",
						global.home, "mixer");
				else
					sprintf(path, "%s/memory/%s/%s",
						global.home, "mixer", songDir);

				if ((memlist = opendir(path)) == NULL)
					return(0);
			}

			if ((entry = readdir(memlist)) == 0)
			{
				closedir(memlist);
				memlist = NULL;
				return(0);
			}

			while (entry->d_name[0] == '.')
			{
				if ((entry = readdir(memlist)) == 0)
				{
					closedir(memlist);
					memlist = NULL;
					return(0);
				}
			}

			if ((dotptr = index(entry->d_name, '.')) != NULL)
				*dotptr = '\0';

			/*
			 * Call a set of routines that will open the directory and then
			 * return its contents until finnished.
			 */
			return(entry->d_name);
		default:
			if (param == 79) {
				return(&m->chan[op - 4].scratch[0]);
			}
	}

	return(0);
}

int
setMixerMemory(mixerMem *m, int op, int param, float *value, char *text)
{
	int channel = 0, offset = 0;

	switch(op)
	{
		case VBUS_PANEL:
			/*
			 * We expect parameters from 0 to 12, they will translate to
			 * The bus:v/l/r
			 */
			channel = param / 3;
			offset = param % 3;

			m->vbus[channel].opaque[offset] = *value;

			break;
		case BUS_PANEL:
			/*
			 * but parameters are opaque but I want to separate them out 
			 * here since the functions will be unique in the engine.
			 */
			channel = param / 30;
			offset = param % 30;

			if (offset < 6)
				/*
				 * Continuous controllers
				 */
				m->bus[channel].b.opaque[offset] = *value;
			else if (offset < 14) {
				/*
				 * FX Selector, single value
				 */
				if (*value == 0)
					m->bus[channel].b.clear.algorithm = -1;
				else
					m->bus[channel].b.clear.algorithm = offset - 6;
			} else if (offset < 30) {
				/*
				 * Output channel selection
				 */
				m->bus[channel].b.clear.outputSelect[offset - 14] = *value;
			}

			break;
		default:
			channel = op - 4;
			offset = param;

			if (offset < 16) {
				/*
				 * Input channel selection radio buttons
				 */
				if (*value == 0) {
					m->chan[channel].inputSelect = -1;
				} else {
					m->chan[channel].inputSelect = offset;
				}
			} else if (offset < 20) {
				/*
				 * Presend bus selection.
				 */
				m->chan[channel].preSend[offset - 16] = *value;
			} else if (offset < 23) {
				/*
				 * Dynamics also radio button selection.
				 */
				if (*value == 0) {
					m->chan[channel].dynamics = -1;
				} else {
					m->chan[channel].dynamics = offset - 20;
				}
			} else if (offset < 27) {
				/*
				 * Filter algo radio button selection.
				 */
				if (*value == 0) {
					m->chan[channel].filter = -1;
				} else {
					m->chan[channel].filter = offset - 23;
				}
			} else if (offset < 31) {
				/*
				 * Postsend bus selection.
				 */
				m->chan[channel].postSend[offset - 27] = *value;
			} else if (offset < 38) {
				/*
				 * Effect radio button selection.
				 */
				if (*value == 0) {
					m->chan[channel].fxAlgo = -1;
				} else {
					m->chan[channel].fxAlgo = offset - 31;
				}
			} else if (offset < 55) {
				/*
				 * The continuous controllers less fader
				 */
				m->chan[channel].p.opaque[offset - 38] = *value;
			} else if (offset == 55) {
				m->chan[channel].mute = *value;
			} else if (offset == 56) {
				m->chan[channel].solo = *value;
			} else if (offset == 57) {
				m->chan[channel].boost = *value;
			} else if (offset < 74) {
				m->chan[channel].outputSelect[offset - 58] = *value;
			} else if (offset < 78) {
				/*
				 * vBus radio buttons
				 */
				if (*value == 0) {
					m->chan[channel].stereoBus = -1;
				} else {
					m->chan[channel].stereoBus = offset - 74;
				}
			} else if (offset == 78) {
				m->chan[channel].p.clear.vol = *value;
			} else if (offset == 79) {
				if (text != NULL)
					sprintf(m->chan[channel].scratch, "%s", text);
			}
			break;
	}
	return(0);
}


/*
 * loadMemory, saveMemory, initMemory, get/setMemory?
 */
/* wow. the return type of this varies depending on op.  no.  -DR */
int
mixerMemory(guiSynth *synth, int op, int subop, int param, void *value)
{
	switch(op) {
		case MM_SAVE:
			return(saveMixerMemory(synth, value));
		case MM_LOAD:
			return(loadMixerMemory(synth, value, param));
		case MM_INIT:
			/*
			 * This was orginally a rather dumb typecast
			return((int) initMixerMemory(param));
			 * Need to review all of this code as the mixer gets rolled out
			 */
			return(0);
		case MM_SET:
			return(setMixerMemory((mixerMem *) synth->mem.param,
				subop, param, value, NULL));
		/*
		 * This will be moved into a separate call.
		case MM_GET:
			return(getMixerMemory((mixerMem *) synth->mem.param,
				subop, param, value));
		 */
	}
	return(0);
}

