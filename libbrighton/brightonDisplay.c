
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
 * This is intended to be a cheap looking LCD display panel.
 */
#include "brightoninternals.h"
#include "brightonkeymappings.h"

/*
 * Use of this variable means every display must have the same coloration
 */

int
destroyDisplay(brightonDevice *dev)
{
	printf("destroyDisplay()\n");

	if (dev->image)
		brightonFreeBitmap(dev->bwin, dev->image);

	dev->image = NULL;

	return(0);
}

static void
renderDot(brightonDevice *dev, int xoff, int yoff, int onoff, int dsize)
{
	int xcount = dsize, ycount = dsize, color;

	if (onoff == 1) {
		color = dev->lastvalue;
	} else {
		color = dev->value2;
	}

	while (ycount--)
	{
		while (xcount--)
		{
			if (yoff + xoff + xcount >= dev->width * dev->height)
				return;
			dev->image->pixels[yoff + xoff + xcount] = color;
		}
		xcount = dsize;
		yoff += dev->width;
	}
}

static int
renderChar(brightonDevice *dev, char ch, int xoffset, int dsize, int dspace)
{
	int yoff = dev->width, boff, xoff = xoffset;
	unsigned char bitmask = 0x80, btarget;

	if ((ch = key[(int) ch].map) == -1)
		ch = 0;

	if ((((key[(int) ch].width + 1) * dsize + dspace) + xoff) >= dev->width)
		return(key[(int) ch].width + dsize);

	btarget = bitmask >> key[(int) ch].width; 

	/*
	 * We now attempt to render the character - we have the space we need.
	 *
	 * If any given bit is set in the character mapping then we paint in a 
	 * dot of size dsize.
	 */
	for (boff = 0; boff < KEY_HEIGHT; boff++)
	{
		while ((bitmask & btarget) == 0)
		{
			if (key[(int) ch].bitmap[boff] & bitmask)
			{
				renderDot(dev, xoff, yoff, BIT_ON, dsize);
			} else {
				renderDot(dev, xoff, yoff, BIT_OFF, dsize);
			}
			xoff += dsize + dspace;
			bitmask >>= 1;
		}
		renderDot(dev, xoff, yoff, BIT_OFF, dsize);
		yoff += (dev->width * (dsize + dspace));
		xoff = xoffset;
		bitmask = 0x80;
	}
	return((key[(int) ch].width + 1) * (dspace + dsize));
}

static void
renderText(brightonDevice *dev, char *text)
{
	int dotsize = 1, dotspace = 1, xoffset = 1;
	char ch;

	if ((text == (char *) 0) || (text == (char *) -1))
		return;

	if (dev->height < 7) {
		dotspace = 0;
	} else if (dev->height < 12) {
		dotspace = 0;
	} else if (dev->height >= 17) {
		dotsize = (dev->height - 6) / 5;
	}

	while ((ch = *text++) != '\0')
	{
		if ((xoffset += renderChar(dev, ch, xoffset, dotsize, dotspace))
			>= dev->width)
			return;
	}
	/*
	 * Clear out any remaining text by rendering spaces.
	 */
	while ((xoffset += renderChar(dev, ' ', xoffset, dotsize, dotspace))
		< dev->width)
	{
		;
	}
}

static int
displaydisplay(brightonDevice *dev)
{
	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(-1);

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_WITHDRAWN)
		return(-1);

	renderText(dev, &dev->text[0]);
/*	renderText(dev, "test"); */
	/*
	 * We should render any text we desire into the bmap, and then have it
	 * painted
	 */
	brightonRender(dev->bwin, dev->image,
		/*dev->bwin->app->resources[dev->panel].canvas, */
		/*dev->x, dev->y, */
		dev->bwin->dlayer,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height, 0);

	/*
	 * We can consider the alpha layer here.
	 */
	if (dev->image2 != 0)
	{
		brightonAlphaLayer(dev->bwin, dev->image2,
			dev->bwin->dlayer,
			dev->x + dev->bwin->app->resources[dev->panel].sx,
			dev->y + dev->bwin->app->resources[dev->panel].sy,
			dev->width, dev->height,
			dev->position);
	}

	brightonFinalRender(dev->bwin,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height);

	return(0);
}

brightonBitmap *first;

static int
configure(brightonDevice *dev, brightonEvent *event)
{
/*	printf("configureDisplay(%i)\n", event->command); */

	if (event->command == -1)
		return(-1);

	if (event->command == BRIGHTON_RESIZE)
	{
		int i;

		dev->originx = event->x;
		dev->originy = event->y;

		dev->x = event->x;
		dev->y = event->y;
		dev->width = event->w;
		dev->height = event->h;

		brightonfree(dev->image->pixels);
		dev->image->width = dev->width;
		dev->image->height = dev->height;
		dev->image->pixels =
			(int *) brightonmalloc(dev->width * dev->height * sizeof(int));
		for (i = 0; i < dev->width * dev->height; i++)
			dev->image->pixels[i] = dev->value2;
		/*
		 * We should consider altering the locations structure, so that
		 * event dispatching is correct.
		brightonLocation(dev->bwin, dev->index,
			dev->x, dev->y, dev->width, dev->height);
		 */

		displaydisplay(dev);

		return(0);
	}

	if (event->command == BRIGHTON_KEYRELEASE)
	{
	}

	if (event->command == BRIGHTON_KEYPRESS)
	{
		brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

		panel->callback(dev->bwin, dev->panel, dev->index, event->key);
	}

	if (event->command == BRIGHTON_MOTION)
	{
	}

	if (event->command == BRIGHTON_PARAMCHANGE)
	{
		char *string;

		if (event->type != BRIGHTON_MEM)
			return(-1);

		if ((string = (char *) event->m) == 0)
			return(-1);

		if (strlen(string) > 64)
			string[64] = '\0';

		sprintf(&dev->text[0], "%s", string);

		displaydisplay(dev);

		return(0);
	}

	return(0);
}

int *
createDisplay(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
/* printf("createDisplay()\n"); */

	dev->destroy = destroyDisplay;
	dev->configure = configure;
	dev->bwin = bwin;

	if (dev->image)
		brightonFreeBitmap(bwin, dev->image);

	if (bwin->app->resources[dev->panel].devlocn[dev->index].image != 0)
		dev->image =
			bwin->app->resources[dev->panel].devlocn[dev->index].image;
	else
		dev->image = brightonReadImage(bwin, "bitmaps/digits/display.xpm");

	if (dev->image == 0) {
		printf("Cannot resolve the bitmap library location\n");
		_exit(0);
	}
	dev->value2 = dev->image->pixels[0];

	/*
	 * We should take a peek at the second image resource. A display uses no
	 * bitmaps, the second may be used as an alpha layer.
	 */
	if (bwin->app->resources[dev->panel].devlocn[dev->index].image2 != 0)
		dev->image2 = 
			bwin->app->resources[dev->panel].devlocn[dev->index].image2;

	initkeys();
	/*
	 * These will force an update when we first display ourselves.
	 */
	dev->value = 0;
	dev->lastvalue = -1;
	dev->lastposition = -1;

	/*
	 * Need to bury this one in the device itself, not globally. It breaks when
	 * we have multiple windows.
	 */
	if (dev->lastvalue == -1)
		dev->lastvalue = brightonGetGC(dev->bwin, 0, 0, 0);

	return(0);
}

