
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
 * Render a brightonBitmap into an area.
 */
#include "brightoninternals.h"

int
destroyKbd(brightonDevice *dev)
{
	printf("destroyKbd()\n");

	if (dev->image)
		brightonFreeBitmap(dev->bwin, dev->image);

	dev->image = NULL;

	return(0);
}

static int
displayKbd(brightonDevice *dev)
{
	brightonIResource *panel;

/*printf("displayKbd\n"); */
	/*
	 * We should render any text we desire into the bmap, and then have it
	 * painted
	 */
	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(0);

	panel = &dev->bwin->app->resources[dev->panel];

	brightonDevUndraw(dev->bwin, dev->bwin->dlayer,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height);

	/*
	 * Only draw fixed number of steps.
	 */
	brightonStretch(dev->bwin, dev->image,
		dev->bwin->dlayer,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height,
		dev->position);

	brightonFinalRender(dev->bwin,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height);

	return(0);
}

static int
configure(brightonDevice *dev, brightonEvent *event)
{
	/*printf("configureKbd(%i)\n", event->command); */

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

		displayKbd(dev);

		return(0);
	}

	if (event->command == BRIGHTON_PARAMCHANGE)
	{
		/*
		 * This will be used to configure different background images */ if ((event->type == BRIGHTON_MEM) && (event->m != 0))
		{
			if (dev->image)
				brightonFreeBitmap(dev->bwin, dev->image);

			dev->image = brightonReadImage(dev->bwin, event->m);

			displayKbd(dev);
		}
	}

	return(0);
}

int *
createKbd(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
	printf("createKbd(%s)\n", bitmap);

	dev->destroy = destroyKbd;
	dev->configure = configure;
	dev->bwin = bwin;

	if (bitmap != 0)
	{
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		dev->image = brightonReadImage(bwin, bitmap);
	}

	/*
	 * These will force an update when we first display ourselves.
	 */
	dev->value = 0;
	dev->lastvalue = -1;
	dev->lastposition = -1;

	return(0);
}

