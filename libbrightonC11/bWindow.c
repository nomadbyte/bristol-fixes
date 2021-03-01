
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

char *args[2] = {"bristol gui", (char *) 0};

int
BOpenWindow(brightonDisplay *display, brightonWindow *bwin, char *programme)
{
	bdisplay *bd = (bdisplay *) display->display;

	bd->flags |= _BRIGHTON_WINDOW;
	return(bwin->win = 0xdeadbeef);
}

int
BCloseWindow(brightonDisplay *display, brightonWindow *bwin)
{
	return(0);
}

void
BRaiseWindow(brightonDisplay *display, brightonWindow *bwin)
{
	return;
}

void
BLowerWindow(brightonDisplay *display, brightonWindow *bwin)
{
	return;
}

int
BGetGeometry(brightonDisplay *display, brightonWindow *bwin)
{
	bwin->width = 1024;
	bwin->height = 768;
	bwin->depth = 24;

	return(0);
}

int
BFlush(brightonDisplay *display)
{
	return(0);
}

int
BAutoRepeat(brightonDisplay *display, int onoff)
{
	return(0);
}

