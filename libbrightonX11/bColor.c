
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

#include "brightonX11internals.h"

int
BFreeColor(brightonDisplay *display, brightonPalette *color)
{
	bdisplay *bd = (bdisplay *) display->display;

	if (~display->flags & _BRIGHTON_WINDOW)
	{
		XFreeColors(bd->display, bd->cm,
			(unsigned long *) &(color->pixel), 1, 0);
	 
		if (color->gc != NULL)
			XFreeGC(bd->display, color->gc);
	}

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
	bdisplay *bd = (bdisplay *) display->display;
	XColor *screen, exact;

//	printf("alloc color by name %s\n", name);

	screen = (XColor *) brightonX11malloc(sizeof(XColor));

	if (~display->flags & _BRIGHTON_WINDOW)
		XAllocNamedColor(bd->display, bd->cm, name, screen, &exact);

	color->red = exact.red;
	color->green = exact.green;
	color->blue = exact.blue;

	color->color = screen;
	color->pixel = screen->pixel;

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
	bdisplay *bd = (bdisplay *) display->display;
	unsigned long valuemask = GCForeground;
	XGCValues values;
	XColor *screen = 0;

	if (color->color == 0)
	{
		screen = (XColor *) brightonX11malloc(sizeof(XColor));
		screen->red = r;
		screen->green = g;
		screen->blue = b;
		screen->flags = DoRed|DoGreen|DoBlue;

		if (~display->flags & _BRIGHTON_WINDOW)
			XAllocColor(bd->display, bd->cm, screen);

		color->color = screen;
		color->pixel = screen->pixel;
	} else
		screen = color->color;

	values.foreground = screen->pixel;

	if (~display->flags & _BRIGHTON_WINDOW)
		color->gc = XCreateGC(bd->display,
			(Window) ((brightonWindow *) display->bwin)->win,
				valuemask, &values);

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
	bdisplay *bd = (bdisplay *) display->display;

	/*
	 * Greyscale tests.
	 *	1 is average,
	 *	2 is min
	 *	3 is max
	switch ((Window) ((brightonWindow *) display->bwin)->grayscale) {
		case 1:
			r = (r + g + b) / 3;
			g = b = r;
			break;
		case 2:
			r = (r + g + b) / 6;
			g = b = r;
			break;
		case 3:
			r = (r + g + b) / 12;
			g = b = r;
			break;
		case 4:
		    if (r < g) {
   				r = r < b? r:b;
			} else {
				r = g < b? g:b;
			}
			g = b = r;
			break;
		case 5:
		    if (r > g) {
   				r = r > b? r:b;
			} else {
				r = g > b? g:b;
			}
			g = b = r;
			break;
	}
	 */

	if (color->pixel <= 0)
	{
		XColor *screen;

		screen = (XColor *) brightonX11malloc(sizeof(XColor));
		screen->red = r;
		screen->green = g;
		screen->blue = b;
		screen->flags = DoRed|DoGreen|DoBlue;

		if (~display->flags & _BRIGHTON_WINDOW)
			XAllocColor(bd->display, bd->cm, screen);

		color->color = screen;
		color->pixel = screen->pixel;
	}

	color->flags |= B_ALLOCATED;

	xcolorcount++;

	return(0);
}

brightonPalette *
BInitColorMap(brightonDisplay *display)
{
	bdisplay *bd = (bdisplay *) display->display;

	if (display->depth == 1)
	{
		printf("cannot support monochrome monitors....\n");
		return(0);
	}

	if (display->flags & _BRIGHTON_WINDOW)
		return(display->palette);

	bd->cm = DefaultColormap(bd->display, bd->screen_num);

	if (!XMatchVisualInfo(bd->display, bd->screen_num, bd->depth,
		PseudoColor, &bd->dvi))
	{
		if (!XMatchVisualInfo(bd->display, bd->screen_num,
			bd->depth, DirectColor, &bd->dvi))
		{
			if (!XMatchVisualInfo(bd->display, bd->screen_num,
				bd->depth, TrueColor, &bd->dvi))
			{
				if (!XMatchVisualInfo(bd->display, bd->screen_num,
					bd->depth, DirectColor, &bd->dvi))
				{
					/*
					 * No Psuedos or Directs. We could consider greyscale....
					 * This is probably superfluous these days.
					 */
					printf("Prefer not to have greyscale graphics.\n");
					bd->flags |= BRIGHTON_GREYSCALE;
					return(display->palette);
				}
			} else {
				printf("Using TrueColor display\n");
			}
		} else {
			printf("Using DirectColor display\n");
		}
	} else {
		printf("Using PseudoColor display\n");
	}

/*
	printf("masks are %x %x %x\n",
		bd->dvi.red_mask, bd->dvi.red_mask, bd->dvi.red_mask);
*/

	return(display->palette);
}

