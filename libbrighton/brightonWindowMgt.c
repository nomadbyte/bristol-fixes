
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

#include "brightoninternals.h"
#include "brightonX11.h"

static int id = 0;
brightonWindow *winlist = 0;

extern void clearout();
extern int brightonCloseDisplay(brightonDisplay *);
extern int BGetGeometry(brightonDisplay *, brightonWindow *);

brightonWindow *
brightonCreateWindow(brightonDisplay *display, brightonApp *app,
int cmapsize, int flags, int quality, int gs, int x, int y)
{
	brightonWindow *bwin;

	bwin = brightonmalloc(sizeof(brightonWindow));

	bwin->cmap_size = cmapsize;
	bwin->quality = quality;
	bwin->grayscale = gs;
	bwin->id = ++id;
	display->bwin = bwin;
	((brightonWindow *) display->bwin)->display = (brightonDisplay *) display;

	printf("display is %i by %i pixels (%i, %i)\n",
		display->width, display->height, x, y);

	if (BGetGeometry(display, bwin) < 0)
		printf("cannot get root window geometry\n");
	else
		printf("Window is w %i, h %i, d %i, %i %i %i\n",
			bwin->width, bwin->height, bwin->depth, bwin->x, bwin->y,
			bwin->border);

	if ((display->palette = brightonInitColormap(bwin, bwin->cmap_size))
		== NULL)
		clearout(-1);

	bwin->image = brightonReadImage(bwin, app->image);
	bwin->surface = brightonReadImage(bwin, app->surface);

	if (bwin->image) {
		bwin->width = bwin->image->width;
		bwin->height = bwin->image->height;
	} else {
		bwin->width = app->width;
		bwin->height = app->height;
	}
	bwin->aspect = ((float) bwin->width) / bwin->height;
	if (x > 0)
		bwin->x = x;
	else if (x < 0)
		bwin->x = display->width + x - app->width;
	if (y > 0)
		bwin->y = y;
	else if (y < 0)
		bwin->y = display->height + y - app->height;

	if (app->flags & BRIGHTON_POST_WINDOW)
		bwin->flags |= _BRIGHTON_POST;

	if (BOpenWindow(display, bwin, app->name) == 0)
	{
		brightonfree(bwin);
		bwin = 0;
		clearout(-1);
		return(0);
	}

	bwin->flags |= BRIGHTON_ACTIVE;

	brightonInitDefHandlers(bwin);

	bwin->next = winlist;
	winlist = bwin;

	/*
	 * Force a fake size to ensure the first configure notify is picked up.
	 */
	bwin->width = bwin->height = 10;

	BFlush((brightonDisplay *) display);

	return(bwin);
}

void
brightonDestroyWindow(brightonWindow *bwin)
{
	brightonBitmap *bitmap;

	printf("brightonDestroyWindow()\n");

	BFlush((brightonDisplay *) bwin->display);

	BCloseWindow((brightonDisplay *) bwin->display, bwin);

	if (brightonCloseDisplay((brightonDisplay *) bwin->display))
	{
		bwin->flags = 0;
		brightonfree(bwin);
		clearout(0);
	}

	brightonFreeBitmap(bwin, bwin->image);
	brightonFreeBitmap(bwin, bwin->surface);
	brightonFreeBitmap(bwin, bwin->canvas);
	brightonFreeBitmap(bwin, bwin->dlayer);
	brightonFreeBitmap(bwin, bwin->slayer);
	brightonFreeBitmap(bwin, bwin->tlayer);
	brightonFreeBitmap(bwin, bwin->render);

	bitmap = bwin->bitmaps;

	while (bitmap) {
		bitmap = brightonFreeBitmap(bwin, bitmap);
	}

	if (bwin->bitmaps)
		printf("Bitmap list is not empty on window exit: %s\n",
			bwin->bitmaps->name);

	bwin->flags = 0;
	brightonfree(bwin);
}

