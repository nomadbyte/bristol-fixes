
/*
 *  Diverse Bristol midi routines.
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

//#define DEBUG

#include "bristol.h"
#include "bristolmidi.h"

extern bristolMidiMain bmidi;

int
bristolMidiFindDev(char *name)
{
	int i;

#ifdef DEBUG
	if (name == (char *) NULL)
		printf("bristolMidiFindDev(NULL)\n");
	else
		printf("bristolMidiFindDev(\"%s\")\n", name);
#endif

	/*
	 * If we have a name, find that name, or return bad handle.
	 * If we do not have a name then return the first free handle we found.
	 */
	for (i = 0; i < BRISTOL_MIDI_DEVCOUNT; i++)
	{
		if (bmidi.dev[i].state == -1)
		{
			if (name == (char *) NULL)
				return(i);
		} else {
			if (name != (char *) NULL)
			{
				if (strcmp(name, bmidi.dev[i].name) == 0)
				{
#ifdef DEBUG
					printf("found dev %s\n", name);
#endif
					return(i);
				}
			}
		}
	}

	return(BRISTOL_MIDI_DEVICE);
}

int
bristolMidiDevSanity(int dev)
{
#ifdef DEBUG
	printf("bristolMidiDevSanity(%i)\n", dev);
#endif

	if ((dev < 0) || (dev > BRISTOL_MIDI_DEVCOUNT))
		return(BRISTOL_MIDI_DEV);
	if (bmidi.dev[dev].state < 0)
		return(BRISTOL_MIDI_DEV);
	if (bmidi.dev[dev].flags <= 0)
		return(BRISTOL_MIDI_DEV);
	if (bmidi.dev[dev].fd <= 0)
		return(BRISTOL_MIDI_DEV);
	return(BRISTOL_MIDI_OK);
}

void
bristolMidiPrintHandle(handle)
{
	printf("	Handle:	 %i\n", bmidi.handle[handle].handle);
	printf("	State:   %i\n", bmidi.handle[handle].state);
	printf("	Channel: %i\n", bmidi.handle[handle].channel);
	printf("	Dev:     %i\n", bmidi.handle[handle].dev);
	printf("	Flags:   %x\n", bmidi.handle[handle].flags);

	if ((bmidi.handle[handle].dev < 0)
		|| (bmidi.handle[handle].dev >= BRISTOL_MIDI_DEVCOUNT))
		return;

	printf("		devfd:  %i\n", bmidi.dev[bmidi.handle[handle].dev].fd);
	printf("		state:  %i\n", bmidi.dev[bmidi.handle[handle].dev].state);
	printf("		hcount: %i\n", bmidi.dev[bmidi.handle[handle].dev].handleCount);
}

int
bristolMidiSanity(handle)
{
#ifdef DEBUG
	printf("bristolMidiSanity(%i)\n", handle);
#endif

	if ((handle < 0) || (handle > BRISTOL_MIDI_HANDLES))
		return(BRISTOL_MIDI_HANDLE);

	/*
	 * So we can at least print it
	bristolMidiPrintHandle(handle);
	 */

	if (bmidi.handle[handle].state < 0)
		return(BRISTOL_MIDI_HANDLE);

	if ((bmidi.handle[handle].handle < 0)
		|| (bmidi.handle[handle].handle >= BRISTOL_MIDI_HANDLES))
		return(BRISTOL_MIDI_HANDLE);

	if ((bmidi.handle[handle].dev < 0)
		|| (bmidi.handle[handle].dev >= BRISTOL_MIDI_DEVCOUNT))
		return(BRISTOL_MIDI_DEVICE);

	if (bmidi.dev[bmidi.handle[handle].dev].state < 0)
		return(BRISTOL_MIDI_DEVICE);

	if (bmidi.dev[bmidi.handle[handle].dev].handleCount < 1)
		return(BRISTOL_MIDI_DEVICE);

	return(BRISTOL_MIDI_OK);
}

int
bristolMidiFindFreeHandle()
{
	int i;

#ifdef DEBUG
	printf("bristolMidiFindFreeHandle()\n");
#endif

	for (i = 0; i < BRISTOL_MIDI_HANDLES; i++)
		if (bmidi.handle[i].state == -1)
			return(i);

	return(BRISTOL_MIDI_HANDLE);
}

int
bristolFreeHandle(int handle)
{
#ifdef DEBUG
	printf("bristolMidiFreeHandle()\n");
#endif

	bmidi.handle[handle].handle = -1;
	bmidi.handle[handle].state = -1;
	bmidi.handle[handle].channel = -1;
	bmidi.handle[handle].dev = -1;
	bmidi.handle[handle].flags = -1;
	bmidi.handle[handle].messagemask = -1;
	bmidi.handle[handle].callback = NULL;

	return(0);
}

int
bristolFreeDevice(int dev)
{
#ifdef DEBUG
	printf("bristolFreeDevice(%i, %i)\n", dev, bmidi.dev[dev].fd);
#endif

	if (bmidi.dev[dev].fd > 0)
		close(bmidi.dev[dev].fd);

	bmidi.dev[dev].lastcommand = -1;
	bmidi.dev[dev].lastcommstate = -1;
	bmidi.dev[dev].lastchan = -1;
	bmidi.dev[dev].fd = -1;
	bmidi.dev[dev].state = -1;
	bmidi.dev[dev].flags = -1;
	bmidi.dev[dev].handleCount = -1;
	bmidi.dev[dev].name[0] = '\0';
	bmidi.dev[dev].bufcount = 0;
	bmidi.dev[dev].bufindex = 0;

	return(0);
}

/*
 * A routine to initalise any of our internal structures.
 */
void
initMidiLib(int flags)
{
	int i;

	if (bmidi.flags & BRISTOL_MIDI_INITTED)
		return;

	bmidi.msgforwarder = NULL;
	bmidi.flags = 0;

	/*
	 * The first handle we open should be a control socket. I prefer not to use
	 * TCP sockets at the moment, but this will be the eventual goal.
	 */

	for (i = 0; i < BRISTOL_MIDI_DEVCOUNT; i++)
		bristolFreeDevice(i);
	for (i = 0; i < BRISTOL_MIDI_HANDLES; i++)
		bristolFreeHandle(i);

	bmidi.flags |= BRISTOL_MIDI_INITTED|(BRISTOL_MIDI_WAIT & flags);
}

