
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

/*
 * Slow timer events.
 *
 * Devices can register for slow timers, cancel requests, and a call has to be
 * made to clock the slow timer. Per default this is from brighton.c on the
 * active sense timeout, default every 1000ms.
 */
#include "brightoninternals.h"

static struct {
	brightonWindow *win;
	int panel;
	int index;
} brightonSTL[1024];

static void
brightonScanTimerList()
{
	int i;
	brightonEvent event;

//printf("Brighton ST Clock scan\n");

	for (i = BRIGHTON_ST_FIRST; i < 1024; i++)
	{
		if (brightonSTL[i].win != 0)
		{
//printf("matched on %i/%i\n", brightonSTL[i].panel, brightonSTL[i].index);

			event.command = BRIGHTON_SLOW_TIMER;
			event.value = 0;
			brightonParamChange(brightonSTL[i].win, brightonSTL[i].panel,
				brightonSTL[i].index, &event);
		}
	}
}

static int
brightonSTRegister(brightonWindow *bwin, brightonDevice *dev)
{
	int i, free = 0;

	/*
	 * We should scan our list to see if this is a duplicate then insert it
	 * at the first free location
	 */
	for (i = BRIGHTON_ST_FIRST; i < 1024; i++)
	{
		if ((brightonSTL[i].win == NULL) && (free == 0))
			free = i;

		if ((brightonSTL[i].win == bwin)
			&& (brightonSTL[i].panel == dev->panel)
			&& (brightonSTL[i].index == dev->index))
			return(i);
	}

	if (free == 0)
		return(-1);

	brightonSTL[free].win = bwin;
	brightonSTL[free].panel = dev->panel;
	brightonSTL[free].index = dev->index;

//printf("Register slow timer %i: %x/%i/%i\n", free, (size_t) bwin, brightonSTL[free].panel, brightonSTL[free].index);

	return(-1);
}

static int
brightonSTDegister(brightonWindow *bwin, brightonDevice *dev, int id)
{
	int i;

	if ((brightonSTL[id].win == bwin)
		&& (brightonSTL[id].panel == dev->panel)
		&& (brightonSTL[id].index == dev->index))
	{
//printf("Deregister slow timer %i: %i/%i\n", id, brightonSTL[id].panel, brightonSTL[id].index);
		brightonSTL[id].win = NULL;
		return(id);
	}

	/*
	 * If we get here then the deregister failed on the index. Scan the list
	 * to see if it is still a known timer
	 */
	for (i = BRIGHTON_ST_FIRST; i < 1024; i++)
	{
		if ((brightonSTL[i].win == bwin)
			&& (brightonSTL[i].panel == dev->panel)
			&& (brightonSTL[i].index == dev->index))
		{
//printf("Deregister slow timer %i: %i/%i\n", i, brightonSTL[i].panel, brightonSTL[i].index);
			brightonSTL[i].win = NULL;
			return(i);
		}
	}

//printf("Failed to deregister slow timer %i/%i\n", brightonSTL[id].panel, brightonSTL[id].index);

	return(-1);
}

int
brightonSlowTimer(brightonWindow *bwin, brightonDevice *dev, int command)
{
//printf("brightonSlowTimer(%i)\n", command);

	if (command < 0)
		return(command);

	switch (command) {
		case BRIGHTON_ST_CLOCK:
			brightonScanTimerList();
			break;
		case BRIGHTON_ST_REQ:
			return(brightonSTRegister(bwin, dev));
		case BRIGHTON_ST_CANCEL:
		default:
			return(brightonSTDegister(bwin, dev, command));
	}

	return(0);
}

