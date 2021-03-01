
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
 * This will be a simple pushbutton, initially it will function as 2 throw,
 * but may incorporate capabilities to support pure contact switches.
 * radio buttons will probably have to be separate.
 */

#include <math.h>

#include "brightoninternals.h"

extern int brightonPanelLocation();

int
destroyButton(brightonDevice *dev)
{
	printf("destroyButton()\n");

	if (dev->image)
		brightonFreeBitmap(dev->bwin, dev->image);
	if (dev->image2)
		brightonFreeBitmap(dev->bwin, dev->image2);
	dev->image = NULL;
	dev->image2 = NULL;

	return(0);
}

static void
displaybutton(brightonDevice *dev)
{
	int flags = dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags;

	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return;

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_WITHDRAWN)
		return;

	/*
	 * This is not needed except for moving parts - sliders primarily, but 
	 * also touch panels. Rotary and buttons are located and stay that way.
	 */
	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_REDRAW)
		brightonDevUndraw(dev->bwin, dev->bwin->dlayer,
			dev->x + dev->bwin->app->resources[dev->panel].sx,
			dev->y + + dev->bwin->app->resources[dev->panel].sy,
			dev->width, dev->height);

	brightonRenderShadow(dev, 1);

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& (BRIGHTON_THREEWAY|BRIGHTON_FIVEWAY))
	{
		if ((dev->value >= 0) && (dev->value <= 0.5))
		{
			brightonStretch(dev->bwin, dev->imagec,
				dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height, flags);
		} else if ((dev->value > 0.5) && (dev->value <= 1.5)) {
			brightonStretch(dev->bwin, dev->image2,
				dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height, flags);
		} else if ((dev->value > 1.5) && (dev->value <= 2.5)) {
			brightonStretch(dev->bwin, dev->image,
				dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height, flags);
		} else if ((dev->value > 2.5) && (dev->value <= 3.5)) {
			brightonStretch(dev->bwin, dev->image3,
				dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height, flags);
		} else if ((dev->value > 3.5) && (dev->value <= 4.5)) {
			brightonStretch(dev->bwin, dev->image4,
				dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height, flags);
		}
	} else if (dev->image2) {
		if (dev->value)
			brightonStretch(dev->bwin, dev->image2,
				dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height,
				dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags);
		else
			brightonStretch(dev->bwin, dev->image,
				dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height,
				dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags);
	} else {
		if ((~dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON))
		{
			if (dev->value == 0)
				flags = 0;
			else
				flags = BRIGHTON_REVERSE|BRIGHTON_HALF_REVERSE;
	
			flags |= (BRIGHTON_VERTICAL
				& dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags);
		}

		brightonStretch(dev->bwin, dev->image,
			/*dev->bwin->app->resources[dev->panel].canvas, */
			dev->bwin->dlayer,
			dev->x + dev->bwin->app->resources[dev->panel].sx,
			dev->y + dev->bwin->app->resources[dev->panel].sy,
			dev->width, dev->height, flags);
	}

	brightonRenderShadow(dev, 0);

	brightonFinalRender(dev->bwin,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height);

	dev->lastvalue = dev->value;
	dev->lastposition = dev->position;
}

/*
 * This will go into brighton render
 */
static int
renderHighlights(brightonWindow *bwin, brightonDevice *dev)
{
	float d, streak, dx, dy;
	brightonCoord p[8];

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags &
		(BRIGHTON_CHECKBUTTON|BRIGHTON_NOSHADOW|BRIGHTON_WITHDRAWN))
		return(0);

	dx = dev->x - bwin->lightX;
	dy = dev->y - bwin->lightY;

	d = sqrt((double) (dx * dx + dy * dy));

	streak = (dev->width * 2.0 * d / bwin->lightH)
		/ (1 - dev->width * 2.0 / bwin->lightH);

	p[0].x = dev->x;
	p[0].y = dev->y + dev->height;
	p[1].x = dev->x + dev->width;
	p[1].y = dev->y;
	p[2].x = dev->x + dx * streak / d;
	p[2].y = dev->y + dy * streak / d;

/*	XFillPolygon(bwin->display, bwin->background, bwin->cheap_shade, */
/*		(XPoint *) &p, 3, Complex, CoordModeOrigin); */
	return(0);
}

static int considercallback(brightonDevice *dev)
{
	brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

	if (dev->bwin->flags & BRIGHTON_NO_DRAW)
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
/*	printf("configureButton(%i, %f)\n", dev->index, dev->value); */

	/*
	 * We had to do this for some withdrawn panels
	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(0);
	 */

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

		/*
		 * We need to build in some shadow, to prevent the button from looking
		 * like it is hanging in mid air.
		 */
		renderHighlights(dev->bwin, dev);

		dev->lastvalue = -1;
		displaybutton(dev);

		return(0);
	}

	if (event->command == BRIGHTON_LEAVE)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON)
		{
			dev->value = 0;

			displaybutton(dev);
		}
		return(0);
	}

	if (event->command == BRIGHTON_ENTER)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON)
		{
			dev->value = 1;

			displaybutton(dev);
		}
		return(0);
	}

	if (event->command == BRIGHTON_BUTTONRELEASE)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON)
		{
			dev->value = 0;

			displaybutton(dev);

			if ((event->x >= dev->x) && (event->y >= dev->y)
				&& (event->x < (dev->x + dev->width))
				&& (event->y < (dev->y + dev->height)))
				considercallback(dev);
		} else {
			brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

			if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
				& BRIGHTON_NO_TOGGLE)
			{
				if (dev->value == 0)
					dev->value = panel->devlocn[dev->index].to;
				else
					dev->value = panel->devlocn[dev->index].from;

				considercallback(dev);

				displaybutton(dev);
			}
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
/*
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON)
			return(0);
*/
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags &
			BRIGHTON_THREEWAY)
		{
			if (--dev->value < 0)
				dev->value = 2;
		} else if
			(dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags &
			BRIGHTON_FIVEWAY)
		{
			if (--dev->value < 0)
				dev->value = 4;
		} else if (dev->value == 0)
			dev->value = panel->devlocn[dev->index].to;
		else
			dev->value = panel->devlocn[dev->index].from;

		if ((dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CHECKBUTTON) == 0)
			considercallback(dev);

		displaybutton(dev);

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

			displaybutton(dev);

			return(0);
		}
		/*
		 * If this was not a space bar which we use to activate and de-activate
		 * any arbitrary button then it could be that we pressed some key that
		 * can otherwise be interpretted.
		 * This is awkward since here we are in a single button and I would 
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

				displaybutton(dev);

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

		displaybutton(dev);

		return(0);
	}

	return(0);
}

int *
createButton(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
	brightonIResource *panel = &bwin->app->resources[dev->panel];

/*printf("createButton(%s, %x)\n", bitmap, panel->devlocn[dev->index].image); */

	dev->destroy = destroyButton;
	dev->configure = configure;
	dev->bwin = bwin;

	if (bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_THREEWAY)
	{
		char path[256];

		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		if (dev->image2)
			brightonFreeBitmap(bwin, dev->image2);
		if (dev->imagec)
			brightonFreeBitmap(bwin, dev->imagec);

		sprintf(path, "bitmaps/buttons/%s1.xpm", bitmap);
		if ((dev->image = brightonReadImage(bwin, path)) != 0)
		{
			sprintf(path, "bitmaps/buttons/%s2.xpm", bitmap);
			if ((dev->image2 = brightonReadImage(bwin, path)) == 0)
				dev->image2 =
					brightonReadImage(bwin, "bitmaps/buttons/sw4.xpm");
			sprintf(path, "bitmaps/buttons/%s3.xpm", bitmap);
			if ((dev->imagec = brightonReadImage(bwin, path)) == 0)
				dev->image2 =
					brightonReadImage(bwin, "bitmaps/buttons/sw4.xpm");
		} else {
			dev->image = brightonReadImage(bwin, "bitmaps/buttons/sw5.xpm");
			dev->image2 = brightonReadImage(bwin, "bitmaps/buttons/sw4.xpm");
			dev->imagec = brightonReadImage(bwin, "bitmaps/buttons/sw3.xpm");
		}
	} else if (bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_FIVEWAY)
	{
		char path[256];

		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		if (dev->image2)
			brightonFreeBitmap(bwin, dev->image2);
		if (dev->image3)
			brightonFreeBitmap(bwin, dev->image3);
		if (dev->image4)
			brightonFreeBitmap(bwin, dev->image4);
		if (dev->imagec)
			brightonFreeBitmap(bwin, dev->imagec);

		sprintf(path, "bitmaps/buttons/%s1.xpm", bitmap);

		if ((dev->image = brightonReadImage(bwin, path)) != 0)
		{
			sprintf(path, "bitmaps/buttons/%s2.xpm", bitmap);
			if ((dev->image2 = brightonReadImage(bwin, path)) == 0)
				dev->image2 =
					brightonReadImage(bwin, "bitmaps/buttons/sw2.xpm");
			sprintf(path, "bitmaps/buttons/%s3.xpm", bitmap);
			if ((dev->image3 = brightonReadImage(bwin, path)) == 0)
				dev->image3 =
					brightonReadImage(bwin, "bitmaps/buttons/sw3.xpm");
			sprintf(path, "bitmaps/buttons/%s4.xpm", bitmap);
			if ((dev->image4 = brightonReadImage(bwin, path)) == 0)
				dev->image4 =
					brightonReadImage(bwin, "bitmaps/buttons/sw4.xpm");
			sprintf(path, "bitmaps/buttons/%s5.xpm", bitmap);
			if ((dev->imagec = brightonReadImage(bwin, path)) == 0)
				dev->imagec =
					brightonReadImage(bwin, "bitmaps/buttons/sw5.xpm");
		} else {
			dev->image = brightonReadImage(bwin, "bitmaps/buttons/sw1.xpm");
			dev->image2 = brightonReadImage(bwin, "bitmaps/buttons/sw2.xpm");
			dev->image3 = brightonReadImage(bwin, "bitmaps/buttons/sw3.xpm");
			dev->image4 = brightonReadImage(bwin, "bitmaps/buttons/sw4.xpm");
			dev->imagec = brightonReadImage(bwin, "bitmaps/buttons/sw5.xpm");
		}

	} else if (bitmap == NULL) {
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		/*
		 * If we have been passed a specific image name for this device then
		 * use it.
		 */
		if (panel->devlocn[dev->index].image != 0)
			dev->image =
				bwin->app->resources[dev->panel].devlocn[dev->index].image;
		else
			dev->image = brightonReadImage(bwin,
				"bitmaps/buttons/rockerred.xpm");
		if (panel->devlocn[dev->index].image2 != 0)
			dev->image2 =
				bwin->app->resources[dev->panel].devlocn[dev->index].image2;
		else
			dev->image =
				brightonReadImage(bwin, "bitmaps/buttons/rockerred.xpm");
	} else {
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);

		if (panel->devlocn[dev->index].image != 0)
			dev->image =
				bwin->app->resources[dev->panel].devlocn[dev->index].image;
		else
			dev->image = brightonReadImage(bwin, bitmap);

		if (dev->image2)
			brightonFreeBitmap(bwin, dev->image2);

		dev->image2 = brightonReadImage(bwin,
			bwin->template->resources[dev->panel].devlocn[dev->index].image2);

/*
		if (panel->devlocn[dev->index].image2 != 0)
			dev->image2 =
				bwin->app->resources[dev->panel].devlocn[dev->index].image2;
*/
	}

	/*
	 * These will force an update when we first display ourselves.
	 */
	dev->value = 0;
	dev->lastvalue = -1;
	dev->lastposition = -1;

	return(0);
}

