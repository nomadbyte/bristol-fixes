
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

#include "math.h"
#include "brightoninternals.h"

static float winwidth, winheight;

int
destroyRibbon(brightonDevice *dev)
{
	printf("destroyRibbon()\n");

	if (dev->image)
		brightonFreeBitmap(dev->bwin, dev->image);
	dev->image = NULL;

	return(0);
}

static int
displayRibben(brightonDevice *dev)
{
	brightonIResource *panel;

	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(0);

	panel = &dev->bwin->app->resources[dev->panel];

	dev->lastvalue = dev->value;
	dev->lastposition = dev->position;

	return(0);
}

static void considercallback(brightonDevice *dev)
{
	brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

	if (dev->lastvalue != dev->value)
	{
		if (panel->devlocn[dev->index].callback)
		{
			panel->devlocn[dev->index].callback(dev->bwin, dev->panel,
				dev->index, dev->value);
		} else {
			if (panel->callback)
				panel->callback(dev->bwin, dev->panel, dev->index, dev->value);
		}
	}

	dev->lastvalue = dev->value;
	dev->lastposition = dev->position;
}

static float rbckeymap[40] = {
	0.0f,
	0.0f,
	2.0f,
	2.0f,
	4.0f,
	4.0f,
	5.0f,
	5.0f,
	7.0f,
	7.0f,
	9.0f,
	9.0f,
	11.0f,
	11.0f,
	12.0f,
	12.0f,
	14.0f,
	14.0f,
	16.0f,
	16.0f,
	0.0f,
	1.0f,
	1.0f,
	3.0f,
	3.0f,
	4.0f,
	5.0f,
	6.0f,
	6.0f,
	8.0f,
	8.0f,
	10.0f,
	10.0f,
	11.0f,
	12.0f,
	13.0f,
	13.0f,
	15.0f,
	15.0f,
	16.0f,
};

static float
getRibbonKey(int x, int y, int w, int h)
{
	int v;

	if ((x <= 0) || (x >= w) || (y <= 0) || (y >= h))
		return(-1.0f);

	if (((v = x * 20 / w) < 0) || (v >= 20))
		return(-1.0f);

	if ((y << 1) < h) {
		/* Black key */
		return(rbckeymap[v + 20]);
	} else {
		/* White key */
		return(rbckeymap[v]);
	}

	return(-1.0f);
}

static int
configure(brightonDevice *dev, brightonEvent *event)
{
//printf("configureRibbon(%i)\n", event->command);

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

//printf("Ribben resize %i %i, %i %i, %i %i: %f\n",
//event->x, event->y, dev->x, dev->y, dev->width, dev->height, dev->value);

		dev->lastvalue = -1;
		displayRibben(dev);

		return(0);
	}

	if (event->command == BRIGHTON_BUTTONPRESS)
	{
		if (dev->width == 0)
			return(0);

		dev->value = getRibbonKey(event->x, event->y, dev->width, dev->height);

//printf("Ribben press %i %i, %i %i, %i %i: %f\n",
//event->x, event->y, dev->x, dev->y, dev->width, dev->height, dev->value);

		considercallback(dev);

		return(0);
	}

	if (event->command == BRIGHTON_BUTTONRELEASE)
	{
//printf("Ribben release %i %i, %i %i, %i %i: %f\n",
//event->x, event->y, dev->x, dev->y, dev->width, dev->height, dev->value);

		dev->value = -1;

		considercallback(dev);

		return(0);
	}

	if (event->command == BRIGHTON_MOTION)
	{
//printf("Ribben motion %i %i, %i %i, %i %i: %f\n",
//event->x, event->y, dev->x, dev->y, dev->width, dev->height, dev->value);

		dev->value = getRibbonKey(event->x, event->y, dev->width, dev->height);

		considercallback(dev);

		return(0);
	}
	return(0);
}

int *
createRibbon(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
//printf("createRibbon(%s): %i\n", bitmap, index);

	winwidth = bwin->display->width / 2;
	winheight = bwin->display->height / 2;

	dev->destroy = destroyRibbon;
	dev->configure = configure;
	dev->index = index;

	dev->bwin = bwin;

	if (bitmap == 0)
	{
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);

		/*
		 * Open the default bitmap
		 */
		if (bwin->app->resources[dev->panel].devlocn[dev->index].image != 0)
			dev->image =
				bwin->app->resources[dev->panel].devlocn[dev->index].image;
		if (dev->image == 0)
			dev->image = brightonReadImage(bwin, "bitmaps/images/pointer.xpm");
	} else {
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		dev->image = brightonReadImage(bwin, bitmap);
		if (dev->image == 0)
			dev->image = brightonReadImage(bwin, "bitmaps/images/pointer.xpm");
	}

	dev->lastvalue = -1;
	dev->lastposition = 0;

	return(0);
}

