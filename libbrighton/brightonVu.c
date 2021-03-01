
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
 * This will be a VU meter set, 16 LED sets for different tracks.
 *
 * We are probably going to cheat on the implementation: The backdrop will be
 * an array of LEDs. We will produce a "blueprint" bitmap that will mask in/out
 * the backdrop.
 *
 * This was written with internal brighton bitmaps, but to improve efficiency
 * of these rather slow access routines (XDrawPixmap, etc), am going to build
 * another extrapolated access to the X11 PixMap structures. These will use
 * native sizes rather than scaling the results.
 */

#include "brightoninternals.h"

int
destroyVu(brightonDevice *dev)
{
	printf("destroyVu()\n");

	if (dev->image)
		brightonFreeBitmap(dev->bwin, dev->image);
	if (dev->image2)
		brightonFreeBitmap(dev->bwin, dev->image2);

	dev->image = NULL;
	dev->image2 = NULL;

	return(0);
}

static int
displayvu(brightonDevice *dev)
{
	brightonIResource *panel;

/*	printf("displayvu()\n"); */

	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(0);

	panel = &dev->bwin->app->resources[dev->panel];

	/*
	 * Only draw fixed number of steps.
	 */
	brightonStretch(dev->bwin, dev->image2,
		dev->bwin->dlayer,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		(int)(dev->y + dev->bwin->app->resources[dev->panel].sy),
		dev->width, dev->height, 0);

	brightonFinalRender(dev->bwin,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width * 2,
		(dev->height >> 2) + 1 + (dev->width >> 1));

	return(0);
}

static void
pbm(brightonDevice *dev, register int o, register int c)
{
	register int *pixels2 = dev->image2->pixels;
/*
	dev->image2->pixels[o + 0] = dev->image->pixels[o + 0];
	dev->image2->pixels[o + 1] = dev->image->pixels[o + 1];
	dev->image2->pixels[o + 2] = dev->image->pixels[o + 2];
	dev->image2->pixels[o + 3] = dev->image->pixels[o + 3];
	dev->image2->pixels[o + 4] = dev->image->pixels[o + 4];
	dev->image2->pixels[o + 5] = dev->image->pixels[o + 5];
	dev->image2->pixels[o + 6] = dev->image->pixels[o + 6];
	dev->image2->pixels[o + 7] = dev->image->pixels[o + 7];
	dev->image2->pixels[o + 8] = dev->image->pixels[o + 8];
	dev->image2->pixels[o + 9] = dev->image->pixels[o + 9];
*/
	pixels2[o + 0] = c;
	pixels2[o + 1] = c;
	pixels2[o + 2] = c;
	pixels2[o + 3] = c;
	pixels2[o + 4] = c;
	pixels2[o + 5] = c;
	pixels2[o + 6] = c;
	pixels2[o + 7] = c;
	pixels2[o + 8] = c;
	pixels2[o + 9] = c;
}

static void
fbm(brightonDevice *dev)
{
	register int i, j;
	register int *pixels2 = dev->image2->pixels;

	/* 
	 * Param indicates the values on the points. Just deal with one for now
	 *
	 * Bitmap image 2 is all black. We are going to cut a few holes in it.
	 */
	for (i = 63; i > 0; --i)
	{
		j = i * 160;

		pixels2[j + 0] = dev->image->pixels[j + 0];
		pixels2[j + 1] = dev->image->pixels[j + 1];
		pixels2[j + 2] = dev->image->pixels[j + 2];
		pixels2[j + 3] = dev->image->pixels[j + 3];
		pixels2[j + 4] = dev->image->pixels[j + 4];
		pixels2[j + 5] = dev->image->pixels[j + 5];
		pixels2[j + 6] = dev->image->pixels[j + 6];
		pixels2[j + 7] = dev->image->pixels[j + 7];
		pixels2[j + 8] = dev->image->pixels[j + 8];
		pixels2[j + 9] = dev->image->pixels[j + 9];

		pixels2[j + 20] = dev->image->pixels[j + 0];
		pixels2[j + 21] = dev->image->pixels[j + 1];
		pixels2[j + 22] = dev->image->pixels[j + 2];
		pixels2[j + 23] = dev->image->pixels[j + 3];
		pixels2[j + 24] = dev->image->pixels[j + 4];
		pixels2[j + 25] = dev->image->pixels[j + 5];
		pixels2[j + 26] = dev->image->pixels[j + 6];
		pixels2[j + 27] = dev->image->pixels[j + 7];
		pixels2[j + 28] = dev->image->pixels[j + 8];
		pixels2[j + 29] = dev->image->pixels[j + 9];

		pixels2[j + 40] = dev->image->pixels[j + 0];
		pixels2[j + 41] = dev->image->pixels[j + 1];
		pixels2[j + 42] = dev->image->pixels[j + 2];
		pixels2[j + 43] = dev->image->pixels[j + 3];
		pixels2[j + 44] = dev->image->pixels[j + 4];
		pixels2[j + 45] = dev->image->pixels[j + 5];
		pixels2[j + 46] = dev->image->pixels[j + 6];
		pixels2[j + 47] = dev->image->pixels[j + 7];
		pixels2[j + 48] = dev->image->pixels[j + 8];
		pixels2[j + 49] = dev->image->pixels[j + 9];

		pixels2[j + 90] = dev->image->pixels[j + 0];
		pixels2[j + 91] = dev->image->pixels[j + 1];
		pixels2[j + 92] = dev->image->pixels[j + 2];
		pixels2[j + 93] = dev->image->pixels[j + 3];
		pixels2[j + 94] = dev->image->pixels[j + 4];
		pixels2[j + 95] = dev->image->pixels[j + 5];
		pixels2[j + 96] = dev->image->pixels[j + 6];
		pixels2[j + 97] = dev->image->pixels[j + 7];
		pixels2[j + 98] = dev->image->pixels[j + 8];
		pixels2[j + 99] = dev->image->pixels[j + 9];
	}
}

static void
cbm(brightonDevice *dev)
{
	int i, j, c;

	/* 
	 * Param indicates the values on the points. Just deal with one for now
	 *
	 * Bitmap image 2 is all black. We are going to cut a few holes in it.
	 */
	for (i = 64 - dev->value--; i > 0; i--)
	{
		j = i * 160;
		c = dev->image2->colormap[0];

/*		pbm(dev, j, c); */
		pbm(dev, j + 20, c);
/*		pbm(dev, j + 20, c); */
/*		pbm(dev, j + 40, c); */
/*		pbm(dev, j + 90, c); */
	}
}

static int
configure(brightonDevice *dev, brightonEvent *event)
{
	printf("configureVu()\n");

	if (event->command == -1)
		return(-1);

	if (event->command == BRIGHTON_RESIZE)
	{
		dev->originx = event->x;
		dev->originy = event->y;

		dev->x = event->x;
		dev->y = event->y;
		dev->width = event->w;
		dev->height = event->h;
printf("vuMeter width = %i, height = %i\n", dev->width, dev->height);
		/*
		 * We should now delete our existing bitmap and create a new one for
		 * the size of this window.
		 */

		/*
		 * We should now rework our parent understanding of our window, since
		 * it will have altered. NOT NECESSARY FOR SCALE.
		brightonPanelLocation(dev->bwin,
			dev->panel, dev->index, dev->x, dev->y, dev->width, dev->height);
		 */

		displayvu(dev);

		return(0);
	}

	if (event->command == BRIGHTON_KEYRELEASE)
	{
	}

	if (event->command == BRIGHTON_BUTTONRELEASE)
	{
	}

	if (event->command == BRIGHTON_KEYPRESS)
	{
	}

	if (event->command == BRIGHTON_MOTION)
	{
	}

	if ((event->command == BRIGHTON_PARAMCHANGE) ||
		(event->command == BRIGHTON_KEYPRESS))
	{
		cbm(dev);
		cbm(dev);
		cbm(dev);
		cbm(dev);

		if (dev->value <= 1)
		{
			dev->value = 64;
			fbm(dev);
		}

		displayvu(dev);

		return(0);
	}
	return(0);
}

int *
createVu(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
	printf("createVu(%s)\n", bitmap);

	dev->destroy = destroyVu;
	dev->configure = configure;
	dev->index = index;

	dev->bwin = bwin;

	if (bitmap == 0)
	{
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		if (dev->image2)
			brightonFreeBitmap(bwin, dev->image2);
		/*
		 * Open the default bitmap
		 */
		if (bwin->app->resources[dev->panel].devlocn[dev->index].image != 0)
			dev->image =
				bwin->app->resources[dev->panel].devlocn[dev->index].image;

		if (bwin->app->resources[dev->panel].devlocn[dev->index].image2 != 0)
			dev->image2 =
				bwin->app->resources[dev->panel].devlocn[dev->index].image2;
	} else {
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		dev->image = brightonReadImage(bwin, bitmap);

		if (bwin->app->resources[dev->panel].devlocn[dev->index].image2 != 0)
			dev->image2 =
				bwin->app->resources[dev->panel].devlocn[dev->index].image2;
	}

	/*
	 * These will force an update when we first display ourselves.
	 */
	dev->value = 64;

	fbm(dev);

	return(0);
}

