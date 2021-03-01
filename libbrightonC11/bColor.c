
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
int brighton_cli_dummy = 10;

int
BFreeColor(brightonDisplay *display, brightonPalette *color)
{
	color->flags &= ~B_ALLOCATED;

	if (color->color) {
		brightonX11free(color->color);
		color->color = 0;
	}

	color->color = 0;
	color->gc = 0;

	return(0);
}


int
BAllocColorByName(brightonDisplay *display, brightonPalette *color, char *name)
{
	color->red = brighton_cli_dummy;
	color->green = brighton_cli_dummy;
	color->blue = brighton_cli_dummy;

	color->pixel = 0;

	color->flags |= B_ALLOCATED;

	return(0);
}

int xcolorcount;

/*
 * Allocation of graphics contexts is only required when using pixmap rendering.
 * With the XImage rendering (0.10.7 and later) the library uses just the
 * pixel values.
 */
int
BAllocGC(brightonDisplay *display, brightonPalette *color,
unsigned short r, unsigned short g, unsigned short b)
{
	color->flags |= B_ALLOCATED;

	xcolorcount++;

	return(0);
}

/*
 * This is used by the XImage interface which does not need a GC. The GC were
 * required by the XDrawPoints() interface for pixmap manipulation which is slow
 * and resource intensive.
 */
int
BAllocColor(brightonDisplay *display, brightonPalette *color,
unsigned short r, unsigned short g, unsigned short b)
{
	if (color->pixel <= 0)
	{
		color->color = &brighton_cli_dummy;
		color->pixel = brighton_cli_dummy;
	}

	color->flags |= B_ALLOCATED;

	xcolorcount++;

	return(0);
}

brightonPalette *
BInitColorMap(brightonDisplay *display)
{
	if (display->depth == 1)
	{
		printf("cannot support monochrome monitors....\n");
		return(0);
	}

	return(display->palette);
}

