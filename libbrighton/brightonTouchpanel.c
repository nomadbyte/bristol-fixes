
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

static float winwidth, winheight;
static float sx = -1, sy = -1;

int
destroyTouch(brightonDevice *dev)
{
	printf("destroyTouch()\n");

	if (dev->image)
		brightonFreeBitmap(dev->bwin, dev->image);
	dev->image = NULL;

	return(0);
}

static int
displaytouch(brightonDevice *dev)
{
	brightonIResource *panel;
	float dv, dv2;

	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(0);

	panel = &dev->bwin->app->resources[dev->panel];

	if (dev->value < 0)
	{
		if ((dev->lastposition >= 0) && (dev->lastposition2 >= 0))
		{
			brightonDevUndraw(dev->bwin, dev->bwin->dlayer,

				dev->x + ((int) dev->lastposition2)
					+ dev->bwin->app->resources[dev->panel].sx,
				dev->y + ((int) dev->lastposition)
					+ dev->bwin->app->resources[dev->panel].sy,
				dev->image->width, dev->image->height);

			brightonFinalRender(dev->bwin,
				dev->x + dev->lastposition2
					+ dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->lastposition
					+ dev->bwin->app->resources[dev->panel].sy,
				dev->image->width,
				dev->image->height);

			return(0);
		}
	}

	/*
	 * Build up a smooth position for the pot. We may need to adjust this based
	 * on the to/from values.
	 */
	if ((dev->value == dev->lastvalue) && (dev->value2 == dev->lastvalue2))
		return(0);

	dv = dev->value;
	dv2 =  1 - dev->value2;

	if ((dev->lastposition >= 0) && (dev->lastposition2 >= 0))
		brightonDevUndraw(dev->bwin, dev->bwin->dlayer,
			dev->x + ((int) dev->lastposition2)
				+ dev->bwin->app->resources[dev->panel].sx,
			dev->y + ((int) dev->lastposition)
				+ dev->bwin->app->resources[dev->panel].sy,
			dev->image->width, dev->image->height);

	brightonFinalRender(dev->bwin,
		dev->x + dev->lastposition2
			+ dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->lastposition
			+ dev->bwin->app->resources[dev->panel].sy,
		dev->image->width,
		dev->image->height);

	dev->position = dv * (dev->height - dev->image->height);
	dev->position2 = dv2 * (dev->width - dev->image->width);

	/*
	 * Only draw fixed number of steps.
	 */
	brightonRender(dev->bwin, dev->image,
		dev->bwin->dlayer,
		(int) (dev->x + dev->position2
			+ dev->bwin->app->resources[dev->panel].sx),
		(int) (dev->y + dev->position
			+ dev->bwin->app->resources[dev->panel].sy),
		dev->image->width, dev->image->height,
		dev->position);

	brightonFinalRender(dev->bwin,
		dev->x + dev->position2
			+ dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->position
			+ dev->bwin->app->resources[dev->panel].sy,
		dev->image->width,
		dev->image->height);

	dev->lastvalue = dev->value;
	dev->lastposition = dev->position;
	dev->lastvalue2 = dev->value2;
	dev->lastposition2 = dev->position2;

	return(0);
}

static void considercallback(brightonDevice *dev)
{
	brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

	if ((dev->value < 0) || (dev->value2 < 0))
		return;

	/*
	 * Due to the co-ordinate system, if we do NOT have the reverse flags
	 * then we need to reverse the value.
	if ((dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_REVERSE) == 0)
		dev->value = 1.0 - dev->value;
	 */

	if ((dev->lastvalue != dev->value) || (dev->lastvalue2 != dev->value2))
	{
		if (panel->devlocn[dev->index].callback)
		{
			panel->devlocn[dev->index].callback(dev->bwin, dev->panel,
				dev->index, 1 - dev->value);
			panel->devlocn[dev->index].callback(dev->bwin, dev->panel,
				dev->index + 1, 1 - dev->value2);
		} else {
			if (panel->callback)
				panel->callback(dev->bwin, dev->panel, dev->index,
					1 - dev->value);
			if (panel->callback)
				panel->callback(dev->bwin, dev->panel, dev->index + 1,
					1 - dev->value2);
		}
	}
}

static int
configure(brightonDevice *dev, brightonEvent *event)
{
/*printf("configureTouch(%i)\n", event->command); */

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

		/*
		 * We should now rework our parent understanding of our window, since
		 * it will have altered. NOT NECESSARY FOR SCALE.
		brightonPanelLocation(dev->bwin,
			dev->panel, dev->index, dev->x, dev->y, dev->width, dev->height);

		considercallback(dev);
		 */

		/*
		 * Highlights need to be rendered dynamically in displaytouch().
		renderHighlights(dev->bwin, dev);
		 */

		dev->lastvalue = -1;
		displaytouch(dev);

		return(0);
	}

	if (event->command == BRIGHTON_BUTTONRELEASE)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CENTER)
		{
			dev->value = 0.5;
			dev->value2 = 0.5;

			considercallback(dev);

			displaytouch(dev);
		} else {
			if ((dev->flags & BRIGHTON_WIDE) == 0)
				dev->value = -1;

			displaytouch(dev);
		}

		sx = sy = -1;

		return(0);
	}

	if (event->command == BRIGHTON_MOTION)
	{
		if (dev->flags & BRIGHTON_WIDE)
		{
			if (sx == -1)
			{
				sx = event->x;
				sy = event->y;
			}
/*printf("aks motion %i %i, %i %i, %i %i: %f\n", */
/*event->x, event->y, dev->x, dev->y, winwidth, winheight, dev->value); */

			if ((dev->value = ((float) event->y - sy) / winheight + 0.5) < 0)
				dev->value = 0;
			else if (dev->value > 1.0)
				dev->value = 1.0;

			/*
			 * Our motion started from sx, we should look for a value
			 * such as
			 *	sx + event->x / winwidth
			 */
			if ((dev->value2 = ((float) event->x - sx) / winwidth + 0.5) < 0)
				dev->value2 = 0;
			else if (dev->value2 > 1.0)
				dev->value2 = 1.0;

			/*
			 * Need to to reverse value due to co-ordinate space
			 */
			dev->value2 = 1.0 - dev->value2;
		} else {
			if ((dev->value = (((float) (event->y - dev->y)) / dev->height))
				< 0)
				dev->value = 0;
			else if (dev->value > 1.0)
				dev->value = 1.0;
			if ((dev->value2 = (((float) (event->x - dev->x)) / dev->width))
				< 0)
				dev->value2 = 0;
			else if (dev->value2 > 1.0)
				dev->value2 = 1.0;
			dev->value2 = 1.0 - dev->value2;
		}

/*printf("touch motion %i %i, %i %i, %i %i: %f\n", */
/*event->x, event->y, dev->x, dev->y, dev->width, dev->height, dev->value); */

		considercallback(dev);

		displaytouch(dev);

		return(0);
	}
	return(0);
}

int *
createTouch(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
	/*printf("createTouch(%s)\n", bitmap); */

	winwidth = bwin->display->width / 2;
	winheight = bwin->display->height / 2;

	dev->destroy = destroyTouch;
	dev->configure = configure;
	dev->index = index;

	dev->bwin = bwin;

	/*
	 * Notch has been reused, should give it another name here
	 */
	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_WIDE)
		dev->flags |= BRIGHTON_WIDE;

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

	/*
	 * These will force an update when we first display ourselves.
	 */
	if (bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_CENTER)
	{
		dev->value = 0.5;
		dev->value2 = 0.5;
	} else
		dev->value = 0;

	if (dev->flags & BRIGHTON_WIDE)
	{
		dev->value = 0.5;
		dev->value2 = 0.5;
	}
	else
		dev->value = -1;

	dev->lastvalue = -1;
	dev->lastposition = 0;
	dev->lastvalue2 = -1;
	dev->lastposition2 = 0;

	return(0);
}

