
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

#include "brightoninternals.h"

brightonILocations *
brightonDeviceLocator(register brightonIResource *panel,
register int x, register int y)
{
	register int index;
	register brightonILocations *locn;

	//printf("brightonDeviceLocator(%i, %i)\n", x, y);

	if (((panel->flags & BRIGHTON_ACTIVE) == 0)
		|| (panel->flags & BRIGHTON_WITHDRAWN))
		return(0);

	for (index = 0; index < panel->ndevices; index++)
	{
		locn = &panel->devlocn[index];

//printf("dev location %i %i %i %i\n", locn->ax, locn->ay, locn->aw, locn->ah);
		if ((locn->ax <= x) && ((locn->ax + locn->aw) > x)
			&& (locn->ay <= y) && ((locn->ay + locn->ah) > y))
		{
//printf("found device %i\n", index);
			return(locn);
		}
	}
	return(0);
}

brightonIResource *
brightonPanelLocator(brightonWindow *bwin, int x, int y)
{
	register int index;
	register brightonIResource *locn;

/*	printf("brightonLocator(%i, %i)\n", x, y); */

	for (index = 0; index < bwin->app->nresources; index++)
	{
		locn = &bwin->app->resources[index];

		if (((locn->flags & BRIGHTON_ACTIVE) == 0)
			|| (locn->flags & BRIGHTON_WITHDRAWN))
			continue;

		if ((locn->sx <= x) && ((locn->sx + locn->sw) > x)
			&& (locn->sy <= y) && ((locn->sy + locn->sh) > y))
		{
/*printf("found panel %i\n", index); */
			/*
			 * Found the panel. Now go look for the device
			 */
			return(locn);
		}
	}
	return(0);
}

/*
 * This will be called typically on button press, and attempts to locate
 * which device is under the given x/y co-ord. May have other uses.
 */
brightonILocations *
brightonLocator(brightonWindow *bwin, int x, int y)
{
	register int index;
	register brightonIResource *locn;

/*	printf("brightonLocator(%i, %i)\n", x, y); */

	for (index = 0; index < bwin->app->nresources; index++)
	{
		locn = &bwin->app->resources[index];

		if (((locn->flags & BRIGHTON_ACTIVE) == 0)
			|| (locn->flags & BRIGHTON_WITHDRAWN))
			continue;

		if ((locn->sx <= x) && ((locn->sx + locn->sw) > x)
			&& (locn->sy <= y) && ((locn->sy + locn->sh) > y))
		{
/*printf("found panel %i\n", index); */
			/*
			 * Found the panel. Now go look for the device
			 */
			return(brightonDeviceLocator(locn, x - locn->sx, y - locn->sy));
		}
	}
	return(0);
}

