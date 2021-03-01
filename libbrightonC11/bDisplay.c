
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

#include "brightonC11internals.h"

bdisplay *displays;

bdisplay *
bFindDisplayByName(bdisplay *dlist, char *name)
{
	if (dlist == 0)
		return(0);

	if (strcmp(dlist->name, name) == 0)
		return(dlist);

	return(bFindDisplayByName(dlist->next, name));
}

bdisplay *
bFindDisplay(bdisplay *dlist, bdisplay *display)
{
	if (dlist == 0)
		return(0);

	if (dlist == display)
		return(display);

	return(bFindDisplay(dlist->next, display));
}

bdisplay *
BOpenDisplay(brightonDisplay *bd, char *displayname)
{
	bdisplay *display;

	display = (bdisplay *) brightonX11malloc(sizeof(bdisplay));

	display->display = (void *) 0xdeadbeef;
	display->flags |= _BRIGHTON_WINDOW;
	display->uses = 1;

	/*
	 * Link in the new display
	 */
	display->next = displays;
	if (displays)
		displays->last = display;
	displays = display;

	sprintf(display->name, "%s", displayname);

	bd->width = display->width;
	bd->height = display->height;
	bd->depth = display->depth;

	return(display);
}

int
BCloseDisplay(bdisplay *display)
{
	bdisplay *d;

	/*
	 * Find the display
	 */
	if ((d = bFindDisplay(displays, display)) == 0)
		return(0);

	/*
	 * Unlink it.
	 */
	if (d->next)
		d->next->last = d->last;
	if (d->last)
		d->last->next = d->next;
	else
		displays = d->next;

	brightonX11free(d);

	return(0);
}

