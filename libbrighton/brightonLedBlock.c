
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
 * This will emulate a 7 red LED digit block.
 */

#include <math.h>

#include "brightoninternals.h"

extern int brightonPanelLocation();

int
destroyLedblock(brightonDevice *dev)
{
	printf("destroyLedblock()\n");

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
	if (dev->image5)
		brightonFreeBitmap(dev->bwin, dev->image5);
	if (dev->image6)
		brightonFreeBitmap(dev->bwin, dev->image6);
	if (dev->image7)
		brightonFreeBitmap(dev->bwin, dev->image7);
	if (dev->image8)
		brightonFreeBitmap(dev->bwin, dev->image8);
	if (dev->image9)
		brightonFreeBitmap(dev->bwin, dev->image9);
	if (dev->imagec)
		brightonFreeBitmap(dev->bwin, dev->imagec);
	if (dev->imagee)
		brightonFreeBitmap(dev->bwin, dev->imagee);
	dev->image = NULL;
	dev->image2 = NULL;

	return(0);
}

static void
displayledblock(brightonDevice *dev)
{
	int flags = dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags;
	brightonBitmap *choice;

	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return;

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_WITHDRAWN)
		return;

	switch ((int) dev->value) {
		default:
			choice = dev->imagec;
			break;
		case 0:
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
		case 5:
			choice = dev->image5;
			break;
		case 6:
			choice = dev->image6;
			break;
		case 7:
			choice = dev->image7;
			break;
		case 8:
			choice = dev->image8;
			break;
		case 9:
			choice = dev->image9;
			break;
		case 10:
			choice = dev->imagee;
			break;
	}

	brightonStretch(dev->bwin, choice,
		/*dev->bwin->app->resources[dev->panel].canvas, */
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

static int considercallback(brightonDevice *dev)
{
	brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

	if ((dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_WITHDRAWN)
		|| (dev->bwin->flags & BRIGHTON_NO_DRAW))
		return(0);

	if ((dev->lastvalue != dev->value)
			|| (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON))
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
	return(0);
}

static int
configure(brightonDevice *dev, brightonEvent *event)
{
/* printf("configureLedblock(%i, %f)\n", dev->index, dev->value); */

	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(0);

	if (event->command == -1)
		return(-1);

	if (event->command == BRIGHTON_RESIZE)
	{
		dev->x = event->x;
		dev->y = event->y;
		dev->width = event->w;
		dev->height = event->h;
		/*
		 * We should consider altering the locations structure, so that
		 * event dispatching is correct.
		 */
		if ((dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON) == 0)
			considercallback(dev);

		brightonPanelLocation(dev->bwin,
			dev->panel, dev->index, dev->x, dev->y, dev->width, dev->height);

		dev->lastvalue = -1;
		displayledblock(dev);

		return(0);
	}

	if (event->command == BRIGHTON_LEAVE)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON)
		{
			dev->value = 0;

			displayledblock(dev);
		}
		return(0);
	}

	if (event->command == BRIGHTON_ENTER)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON)
		{
			dev->value = 1;

			displayledblock(dev);
		}
		return(0);
	}

	if (event->command == BRIGHTON_BUTTONRELEASE)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON)
		{
			dev->value = 0;

			displayledblock(dev);

			considercallback(dev);
		}
		return(0);
	}

	if (event->command == BRIGHTON_BUTTONPRESS)
	{
		brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

		if (event->key == BRIGHTON_BUTTON2)
		{
			brightonRegisterController(dev);

			return(0);
		}

		if (dev->value == 0)
			dev->value = panel->devlocn[dev->index].to;
		else
			dev->value = panel->devlocn[dev->index].from;

		if ((dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON) == 0)
			considercallback(dev);

		displayledblock(dev);

		return(0);
	}

	if (event->command == BRIGHTON_KEYPRESS)
	{
		brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

		if (event->key == 0x20)
		{
			if (dev->value == 0)
				dev->value = panel->devlocn[dev->index].to;
			else
				dev->value = panel->devlocn[dev->index].from;

			considercallback(dev);

			displayledblock(dev);

			return(0);
		}
		/*
		 * If this was not a space bar which we use to activate and de-activate
		 * any arbitrary ledblock then it could be that we pressed some key that
		 * can otherwise be interpretted.
		 * This is awkward since here we are in a single ledblock and I would 
		 * like to use keypress to emulate a piano keyboard from the computer.
		 * These events would have to be delivered to the parent, not to the
		 * device, and the parent would then decide to which device the event
		 * should be delivered.
		 */
	}

	if (event->command == BRIGHTON_KEYRELEASE)
	{
		/*
		 * This is just to clear the event and repaint the key, we should not
		 * be bothered with the callback.
		 */
		if (event->key == 0x20)
		{
			if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
				& BRIGHTON_CHECKBUTTON)
			{
				dev->value = 0;

				displayledblock(dev);

/*				considercallback(dev); */
			}
		}
		return(0);
	}

	if (event->command == BRIGHTON_PARAMCHANGE)
	{
		dev->value = event->value;
		dev->lastvalue = -1;

		if ((dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON) == 0)
			considercallback(dev);

		displayledblock(dev);

		return(0);
	}

	return(0);
}

int *
createLedblock(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
/* printf("createLedblock()\n"); */

	dev->destroy = destroyLedblock;
	dev->configure = configure;
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
	if (dev->image5)
		brightonFreeBitmap(bwin, dev->image5);
	if (dev->image6)
		brightonFreeBitmap(bwin, dev->image6);
	if (dev->image7)
		brightonFreeBitmap(bwin, dev->image7);
	if (dev->image8)
		brightonFreeBitmap(bwin, dev->image8);
	if (dev->image9)
		brightonFreeBitmap(bwin, dev->image9);

	if ((dev->image0
		= brightonReadImage(bwin, "bitmaps/digits/redled0.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image1
		= brightonReadImage(bwin, "bitmaps/digits/redled1.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image2
		= brightonReadImage(bwin, "bitmaps/digits/redled2.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image3
		= brightonReadImage(bwin, "bitmaps/digits/redled3.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image4
		= brightonReadImage(bwin, "bitmaps/digits/redled4.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image5
		= brightonReadImage(bwin, "bitmaps/digits/redled5.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image6
		= brightonReadImage(bwin, "bitmaps/digits/redled6.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image7
		= brightonReadImage(bwin, "bitmaps/digits/redled7.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image8
		= brightonReadImage(bwin, "bitmaps/digits/redled8.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->image9
		= brightonReadImage(bwin, "bitmaps/digits/redled9.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->imagec
		= brightonReadImage(bwin, "bitmaps/digits/redledoff.xpm")) == 0)
		printf("could not load redled image\n");
	if ((dev->imagee
		= brightonReadImage(bwin, "bitmaps/digits/redledE.xpm")) == 0)
		printf("could not load redled image\n");

	/*
	 * These will force an update when we first display ourselves.
	 */
	dev->value = 0;
	dev->lastvalue = -1;
	dev->lastposition = -1;

	return(0);
}

