
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

#include "brightoninternals.h"
#include "brightonX11.h"

extern void initSqrtTab();

brightonDisplay *
brightonOpenDisplay(char *displayname)
{
	brightonDisplay *display;

	/*
	 * This should be moved to a more generic init() procedure, it is not 
	 * required for each display connection, but at the same time we will
	 * probably only connect to one anyway.
	 */
	initSqrtTab();

	display = (brightonDisplay *) brightonmalloc(sizeof(brightonDisplay));

	if (displayname == NULL)
		if ((displayname = getenv("DISPLAY")) == NULL)
			displayname = BRIGHTON_DEFAULT_DISPLAY;

	sprintf(&display->name[0], "%s", displayname);

	if ((display->display = (void *)
		BOpenDisplay(display, displayname)) == NULL)
	{
		printf("brightonOpenDisplay(): cannot open %s\n", displayname);
		brightonfree(display);
		return(0);
	}

	return(display);
}

brightonDisplay *
brightonFindDisplay(brightonDisplay *dlist, brightonDisplay *display)
{
	if (dlist == 0)
		return(0);

	if (dlist == display)
		return(display);

	return(brightonFindDisplay(dlist->next, display));
}

int
brightonCloseDisplay(brightonDisplay *display)
{
	BCloseDisplay(display->display);

	brightonfree(display);

	return(0);
}

