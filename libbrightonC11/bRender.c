
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

/*
 * At the moment (14/3/02) this code uses XDrawPoint directly onto the screen.
 * It could be accelerated by using an XImage structure. In the mean time this
 * will be accelerated using XDrawPoints() rather than the singular version.
 */
int
BCopyArea(brightonDisplay *display, int x, int y, int w, int h, int dx, int dy)
{
	return(0);
}

int
BResizeWindow(brightonDisplay *display, brightonWindow *bwin,
int width, int height)
{
	return(0);
}

int
BDrawArea(brightonDisplay *display, brightonBitmap *bitmap,
int sx, int sy, int sw, int sh, int destx, int desty)
{
	return(0);
}

