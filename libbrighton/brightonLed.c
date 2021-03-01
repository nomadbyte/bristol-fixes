
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
#include "brightonledstates.h"

int
destroyLed(brightonDevice *dev)
{
	printf("destroyLed()\n");

	if (dev->image0)
		brightonFreeBitmap(dev->bwin, dev->image0);
	if (dev->image1)
		brightonFreeBitmap(dev->bwin, dev->image1);
	if (dev->image2)
		brightonFreeBitmap(dev->bwin, dev->image2);
	if (dev->image3)
		brightonFreeBitmap(dev->bwin, dev->image3);
	if (dev->image4)
		brightonFreeBitmap(dev->bwin, dev->image4);

	dev->image = NULL;

	return(0);
}

static void
displayled(brightonDevice *dev)
{
	int flags = dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags;
	brightonBitmap *choice = NULL;

	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return;

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_WITHDRAWN)
		return;

	if (((int) dev->value) & BRIGHTON_LED_FLASH_FAST)
		choice = dev->image4;
	else if (((int) dev->value) & BRIGHTON_LED_FLASH) {
		if (((int) dev->value) & BRIGHTON_LED_FLASHING) {
			choice = dev->image0;
		 	dev->value = ((int) dev->value) & ~BRIGHTON_LED_FLASHING;
		} else
		 	dev->value = ((int) dev->value) | BRIGHTON_LED_FLASHING;
	} 

	if (choice == NULL)
	{
		switch (((int) dev->value) & BRIGHTON_LED_MASK) {
			default:
				choice = dev->image0;
				break;
			case 1:
				choice = dev->image1;
				break;
			case 2:
				choice = dev->image2;
				break;
			case 3:
				choice = dev->image3;
				break;
			case 4:
				choice = dev->image4;
				break;
		}
	}

	brightonStretch(dev->bwin, choice,
		dev->bwin->dlayer,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height, flags);

	brightonFinalRender(dev->bwin,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height);

	dev->lastvalue = dev->value;
	dev->lastposition = dev->position;
}

static void
considercallback(brightonDevice *dev)
{
	brightonIResource *panel = &dev->bwin->app->resources[dev->panel];
	int callvalue = 0;

	if (((int) dev->value) & BRIGHTON_LED_FLASH_FAST)
		callvalue = 1;

	if (panel->callback)
		panel->callback(dev->bwin, dev->panel, dev->index, callvalue);
}

static int
configure(brightonDevice *dev, brightonEvent *event)
{
	if (event->command == -1)
		return(-1);

//printf("configureLed(%i, %f)\n", event->command, event->value);

	if (event->command == BRIGHTON_SLOW_TIMER)
	{
		/*
		 * Change state of display. If we are still configured to flash then
		 * request another callback
		 */
		displayled(dev);

		return(0);
	}

	if (event->command == BRIGHTON_FAST_TIMER)
	{
//		printf("Fast timer expired\n");

		/*
		 * We are going on or off, we probably have to know that here from our
		 * flag status. In either case we need to set some value and notify a
		 * callback value since these are used for note on and note off events.
		 */
		if (event->value == 0) 
			dev->value = ((int) dev->value) & ~BRIGHTON_LED_FLASH_FAST;
		else
			dev->value = ((int) dev->value) | BRIGHTON_LED_FLASH_FAST;

		considercallback(dev);
		displayled(dev);

		return(0);
	}

	if (event->command == BRIGHTON_RESIZE)
	{
		dev->originx = event->x;
		dev->originy = event->y;

		dev->x = event->x;
		dev->y = event->y;
		dev->width = event->w;
		dev->height = event->h;

		dev->lastvalue = -1;
		displayled(dev);

		return(0);
	}

	if (event->command == BRIGHTON_PARAMCHANGE)
	{
		int iv = (int) event->value;
		/*
		 * If we have a flashing option set then request a callback. Do not
		 * change the color bits, only the 'flash' bit.
		 */
		if (((int) event->value) & BRIGHTON_LED_FLASH)
		{
			brightonSlowTimer(dev->bwin, dev, BRIGHTON_ST_REQ);
			if ((iv & BRIGHTON_LED_MASK) == 0)
				dev->value = ((int) dev->value) | BRIGHTON_LED_FLASH;
			else
				dev->value = event->value;
		} else {
			brightonSlowTimer(dev->bwin, dev, BRIGHTON_ST_CANCEL);
			dev->value = event->value;
		}

		displayled(dev);

		return(0);
	}
	return(0);
}

int *
createLed(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
/*	printf("createLed(%s)\n", bitmap); */

	dev->destroy = destroyLed;
	dev->configure = configure;
	dev->index = index;

	dev->bwin = bwin;

	if (dev->image0)
		brightonFreeBitmap(bwin, dev->image0);
	if (dev->image1)
		brightonFreeBitmap(bwin, dev->image1);
	if (dev->image2)
		brightonFreeBitmap(bwin, dev->image2);
	if (dev->image3)
		brightonFreeBitmap(bwin, dev->image3);
	if (dev->image4)
		brightonFreeBitmap(bwin, dev->image4);

	if ((dev->image0
		= brightonReadImage(bwin, "bitmaps/images/offled.xpm")) == 0)
		printf("could not load offled image\n");
	if ((dev->image1
		= brightonReadImage(bwin, "bitmaps/images/redled.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image2
		= brightonReadImage(bwin, "bitmaps/images/greenled.xpm")) == 0)
		printf("could not load greenled image\n");
	if ((dev->image3
		= brightonReadImage(bwin, "bitmaps/images/yellowled.xpm")) == 0)
		printf("could not load yellowled image\n");
	if ((dev->image4
		= brightonReadImage(bwin, "bitmaps/images/blueled.xpm")) == 0)
		printf("could not load blueled image\n");

	dev->value = 0.500001;

	dev->lastvalue = -1;
	dev->lastposition = 0;

	return(0);
}

