
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
 * This will be a linear potmeter. Takes a bitmap and locates it according to
 * input from the mouse/keyboard. Previous positions need to be unmapped, since
 * there is total repositioning of the image bitmap.
 */

#include "brightoninternals.h"

int
destroyScale(brightonDevice *dev)
{
	printf("destroyScale()\n");

	if (dev->image)
		brightonFreeBitmap(dev->bwin, dev->image);
	dev->image = NULL;

	return(0);
}

static int
displayscale(brightonDevice *dev)
{
	brightonIResource *panel;
	float displayvalue;

	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(0);

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_WITHDRAWN)
		return(0);

	panel = &dev->bwin->app->resources[dev->panel];

	/*
	 * Build up a smooth position for the pot. We may need to adjust this based
	 * on the to/from values.
	 */

	if (dev->value == dev->lastvalue)
		return(0);

	displayvalue = dev->value;

/*
	brightonFinalRender(dev->bwin,
		dev->x + dev->bwin->app->resources[dev->panel].sx,
		dev->y + dev->lastposition + dev->bwin->app->resources[dev->panel].sy,
		dev->width, dev->height / 4);
*/

	/*
	 * This seems odd, the vertical flag refers to the scrolled item, not the
	 * direction of movement. Perhaps that should change as the direction of
	 * movement is counterintuitive
	 */
	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_VERTICAL)
	{
		if (dev->lastposition >= 0)
		{
			brightonDevUndraw(dev->bwin, dev->bwin->dlayer,
				dev->x + ((int) dev->lastposition)
					+ dev->bwin->app->resources[dev->panel].sx - 5,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width / 8 + 5, dev->height);
			brightonRenderShadow(dev, 1);
		}

		dev->position = displayvalue * (dev->width - dev->width / 4);

		/*
		 * Only draw fixed number of steps. Panned. Just draw it.....
		 */
		brightonStretch(dev->bwin, dev->image,
			dev->bwin->dlayer,
			(int)(dev->x + dev->position
				+ dev->bwin->app->resources[dev->panel].sx),
			dev->y + dev->bwin->app->resources[dev->panel].sy,
			dev->width / 8, dev->height,
			1);

		brightonRenderShadow(dev, 0);

		/*
		 * And request the panel to put this onto the respective image.
		 */
		if (dev->position > dev->lastposition)
		{
			/*
			 * We have to render from the last position up to the current one
			 * adding in bits for shade.
			 */
			brightonFinalRender(dev->bwin,
				dev->x + dev->lastposition
					+ dev->bwin->app->resources[dev->panel].sx - 7,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->position - dev->lastposition + (dev->width >> 2) + 5,
				dev->height + 9);
		} else {
			brightonFinalRender(dev->bwin,
				dev->x + dev->position
					+ dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->lastposition - dev->position + (dev->width >> 2),
				dev->height + 7);
		}
	} else {
		if (dev->lastposition >= 0)
		{
			brightonDevUndraw(dev->bwin, dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + ((int) dev->lastposition)
					+ dev->bwin->app->resources[dev->panel].sy - 1,
				dev->width, dev->height / 4 + 1);
			brightonRenderShadow(dev, 1);
		}

		dev->position = displayvalue * (dev->height - dev->height / 4);

		/*
		 * Drawbars are only ever rendered vertically.
		 */
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_HSCALE)
		{
			brightonDevUndraw(dev->bwin, dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height);

			brightonStretch(dev->bwin, dev->image2,
				dev->bwin->dlayer,
				(int) (dev->x + dev->bwin->app->resources[dev->panel].sx),
				(int) (dev->y + dev->bwin->app->resources[dev->panel].sy),
				dev->width, (int) dev->position, 0);

			brightonFinalRender(dev->bwin,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, (int) dev->height);
		}

		/*
		 * Only draw fixed number of steps. Panned. Just draw it.....
		 */
		brightonStretch(dev->bwin, dev->image,
			dev->bwin->dlayer,
			dev->x + dev->bwin->app->resources[dev->panel].sx,
			(int)(dev->y + dev->position
				+ dev->bwin->app->resources[dev->panel].sy),
			dev->width, dev->height / 4,
			0);

		brightonRenderShadow(dev, 0);

		/*
		 * And request the panel to put this onto the respective image.
		 */
		if (dev->position > dev->lastposition)
		{
			brightonFinalRender(dev->bwin,
				dev->x + dev->bwin->app->resources[dev->panel].sx - 1,
				dev->y + dev->lastposition
					+ dev->bwin->app->resources[dev->panel].sy - 1,
				dev->width * 2 + 1,
				dev->position - dev->lastposition + (dev->height >> 2)
					+ (dev->width >> 1) + 1);
		} else {
			brightonFinalRender(dev->bwin,
				dev->x + dev->bwin->app->resources[dev->panel].sx - 1,
				dev->y + dev->position
					+ dev->bwin->app->resources[dev->panel].sy,
				dev->width * 2 + 1,
				dev->lastposition - dev->position + (dev->height >> 2) + 1
					+ (dev->width >> 1));
		}
	}

	dev->lastvalue = dev->value;
	dev->lastposition = dev->position;

	return(0);
}

static void
considercallback(brightonDevice *dev)
{
	brightonIResource *panel = &dev->bwin->app->resources[dev->panel];
	float callvalue;

	if (dev->bwin->flags & BRIGHTON_NO_DRAW)
		return;

	if (dev->value > 1.0)
		dev->value = 1.0;
	else if (dev->value < 0)
		dev->value = 0.0;

	/*
	 * Due to the co-ordinate system, if we do NOT have the reverse flags
	 * then we need to reverse the value.
	 */
	if ((dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_REVERSE) == 0)
		callvalue = 1.0 - dev->value;
	else
		callvalue = dev->value;

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].to != 1.0)
	{
		callvalue =
			(callvalue
			* dev->bwin->app->resources[dev->panel].devlocn[dev->index].to);

		if ((callvalue - ((int) callvalue)) > 0.5)
			callvalue = ((float) ((int) callvalue) + 1);
		else
			callvalue = ((float) ((int) callvalue));
	}

	if (dev->lastvalue != dev->value)
	{
		if (panel->devlocn[dev->index].callback)
		{
			panel->devlocn[dev->index].callback(dev->bwin, dev->panel,
				dev->index, callvalue);
		} else {
			if (panel->callback)
				panel->callback(dev->bwin, dev->panel, dev->index, callvalue);
		}
	}
}

static int cx, cy;
static float sval;

static int
configure(brightonDevice *dev, brightonEvent *event)
{
/*	printf("configureScale(%i, %f)\n", event->command, event->value); */

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
		 * Highlights need to be rendered dynamically in displayscale().
		 */

		dev->lastvalue = -1;
		displayscale(dev);

		return(0);
	}

	if (event->command == BRIGHTON_KEYRELEASE)
	{
		/*
		 * This is a little bit 'happens they work'. We should fix these key
		 * mappings for keycodes.
		 */
		switch(event->key) {
			default:
				break;
			case 37:
			case 66:
			case 109:
			case 65508:
			case 65507:
				dev->flags &= ~BRIGHTON_CONTROLKEY;
				cx = cy = sval = -1;
				break;
			case 50:
			case 62:
			case 65505:
				dev->flags &= ~BRIGHTON_SHIFTKEY;
				break;
		}
	}

	if (event->command == BRIGHTON_BUTTONPRESS)
	{
		/*
		 * This is hard coded, it calls back to the GUI. This is incorrect as
		 * the callback dispatcher should be requested by the GUI.
		 *
		 * Perhaps the MIDI code should actually be in the same library? Why
		 * does the GUI need to know about this?
		 */
		if (event->key == BRIGHTON_BUTTON2)
			brightonRegisterController(dev);

		cx = event->x;
		cy = event->y;

		return(0);
	}

	if (event->command == BRIGHTON_BUTTONRELEASE)
	{
		cx = cy = sval = -1;
		dev->flags &= ~BRIGHTON_CONTROLKEY;

		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_CENTER)
		{
			dev->value = 0.5;

			considercallback(dev);

			displayscale(dev);
		}

		return(0);
	}

	if (event->command == BRIGHTON_KEYPRESS)
	{
		switch(event->key) {
			default:
				break;
			case 37:
			case 66:
			case 109:
			case 65508:
			case 65507:
				cx = event->x;
				cy = event->y;
				sval = dev->value;
				dev->flags |= BRIGHTON_CONTROLKEY;
				break;
			case 50:
			case 62:
			case 65505:
				dev->flags |= BRIGHTON_SHIFTKEY;
				break;
			case 0x6a:
			case 0xff54:
				if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags & BRIGHTON_VERTICAL)
				{
					if (dev->flags & BRIGHTON_SHIFTKEY)
						dev->value -= ((float) 256) / 16384;
					else
						dev->value -= ((float) 1) / 16384;
				} else {
					if (dev->flags & BRIGHTON_SHIFTKEY)
						dev->value += ((float) 256) / 16384;
					else
						dev->value += ((float) 1) / 16384;
				}
				break;
			case 0x6b:
			case 0xff52:
				/*if (dev->flags & BRIGHTON_VERTICAL) */
				if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags & BRIGHTON_VERTICAL)
				{
					if (dev->flags & BRIGHTON_SHIFTKEY)
						dev->value += ((float) 256) / 16384;
					else
						dev->value += ((float) 1) / 16384;
				} else {
					if (dev->flags & BRIGHTON_SHIFTKEY)
						dev->value -= ((float) 256) / 16384;
					else
						dev->value -= ((float) 1) / 16384;
				}
				break;
			case 0xff51:
				if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags & BRIGHTON_VERTICAL)
				{
					if (dev->flags & BRIGHTON_SHIFTKEY)
						dev->value -= ((float) 2048) / 16384;
					else
						dev->value -= ((float) 32) / 16384;
				} else {
					if (dev->flags & BRIGHTON_SHIFTKEY)
						dev->value += ((float) 2048) / 16384;
					else
						dev->value += ((float) 32) / 16384;
				}
				break;
			case 0xff53:
				/*if (dev->flags & BRIGHTON_VERTICAL) */
				if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags & BRIGHTON_VERTICAL)
				{
					if (dev->flags & BRIGHTON_SHIFTKEY)
						dev->value += ((float) 2048) / 16384;
					else
						dev->value += ((float) 32) / 16384;
				} else {
					if (dev->flags & BRIGHTON_SHIFTKEY)
						dev->value -= ((float) 2048) / 16384;
					else
						dev->value -= ((float) 32) / 16384;
				}
				break;
		}

		considercallback(dev);

		displayscale(dev);
	}

	if (event->command == BRIGHTON_MOTION)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_VERTICAL)
	 	{
			/*
			 * Need to add in an optional central dead spot. This should be a
			 * small fraction of either side of the pot. It will be where
			 * position deviates from mouse location.
			if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
				& BRIGHTON_NOTCH)
			{
			} else
			 */
			if (dev->flags & BRIGHTON_CONTROLKEY)
			{
				float deltax;

				if (cx == -1)
				{
					sval = dev->value;
					cx = event->x;
					cy = event->y;
				}

				deltax = ((float) (event->x - cx)) / 16383.0f;

				dev->value = sval + deltax;
			} else if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags & BRIGHTON_NOTCH)
			{
				/*
				 * When we are passing zero we should hold for a bit.
				 */
				dev->value = (((float) (event->x + 5 - dev->x - (dev->width / 8)))
					/ (dev->width - dev->width / 4));

				if (dev->value > 0.6)
					dev->value -= 0.1;
				else if (dev->value < 0.4)
					dev->value += 0.1;
				else
					dev->value = 0.5;
			} else {
				/* Fanning scaler controls */
				if ((event->y - dev->y) > dev->height) {
					dev->value = (((float)
						(cx - dev->x - (dev->width / 8)))
							/ (dev->width - dev->width / 4));
					dev->value += (event->x - cx)
						/ (16.0 * (1.0 + event->y - dev->y - dev->height/2));
				} else if ((event->y - dev->y) < -dev->height) {
					dev->value = (((float)
						(cx - dev->x - (dev->width / 8)))
							/ (dev->width - dev->width / 4));
					dev->value -= (event->x - cx)
						/ (16.0 * (1.0 + event->y - dev->y - dev->height/2));
				} else {
					cx = event->x;

					dev->value = (((float)
						(event->x + 5 - dev->x - (dev->width / 8)))
							/ (dev->width - dev->width / 4));
				}
			}
		} else {
			if (dev->flags & BRIGHTON_CONTROLKEY)
			{
				float deltax;

				if (cx == -1)
				{
					sval = dev->value;
					cx = event->x;
					cy = event->y;
				}

				deltax = ((float) (event->y - cy)) / 16383.0f;

				dev->value = sval + deltax;
			} else if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags & BRIGHTON_NOTCH)
			{
				dev->value = (((float) (event->y - dev->y - (dev->height / 8)))
					/ (dev->height - dev->height / 4));

				if (dev->value > 0.6)
					dev->value -= 0.1;
				else if (dev->value < 0.4)
					dev->value += 0.1;
				else
					dev->value = 0.5;
			} else {
				/* Fanning scaler controls */
//		} else if (dev->bwin->flags & BRIGHTON_ROTARY_UD) {
				if ((event->x - dev->x) > dev->width) {
					dev->value = (((float)
						(cy - dev->y - (dev->height / 8)))
							/ (dev->height - dev->height / 4));
					dev->value += (event->y - cy)
						/ (16.0 * (1.0 + event->x - dev->x - dev->width/2));
				} else if ((event->x - dev->x) < -dev->width) {
					dev->value = (((float)
						(cy - dev->y - (dev->height / 8)))
							/ (dev->height - dev->height / 4));
					dev->value -= (event->y - cy)
						/ (16.0 * (1.0 + event->x - dev->x - dev->width/2));
				} else {
					cy = event->y;

					dev->value = (((float)
						(event->y - dev->y - (dev->height / 8)))
							/ (dev->height - dev->height / 4));
				}
			}
		}

/*printf("scale motion %i %i, %i %i, %i %i: %f\n", */
/*event->x, event->y, dev->x, dev->y, dev->width, dev->height, dev->value); */

		/*
		 * We now need to consider rounding this to the resolution of this
		 * device. If the max value is not 1.0 then we need to put fixed steps
		 * into our new device value.
		 */
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].to != 1.0)
		{
			dev->value = (float) ((int)
				(dev->value
				* dev->bwin->app->resources[dev->panel].devlocn[dev->index].to))
				/ dev->bwin->app->resources[dev->panel].devlocn[dev->index].to;
		}

		considercallback(dev);

		displayscale(dev);

		return(0);
	}

	if (event->command == BRIGHTON_PARAMCHANGE)
	{
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_REVERSE)
			dev->value = event->value
				/ dev->bwin->app->resources[dev->panel].devlocn[dev->index].to;
		else {
			if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].to
				!= 1.0)
			{
				dev->value =
					(dev->bwin->app->resources[dev->panel].devlocn[dev->index].to - event->value)
				/ dev->bwin->app->resources[dev->panel].devlocn[dev->index].to;
			} else {
				dev->value = (1.0 - event->value) /
					dev->bwin->app->resources[dev->panel].devlocn[dev->index].to;
			}
		}

		considercallback(dev);

		displayscale(dev);

		return(0);
	}
	return(0);
}

int *
createScale(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
/*	printf("createScale(%s)\n", bitmap); */

	dev->destroy = destroyScale;
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
		else
			dev->image = brightonReadImage(bwin, "bitmaps/knobs/slider1.xpm");
	} else {
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		dev->image = brightonReadImage(bwin, bitmap);
	}

	if (bwin->app->resources[dev->panel].devlocn[dev->index].flags &
		BRIGHTON_HSCALE)
	{
		if (dev->image2)
			brightonFreeBitmap(bwin, dev->image2);
		dev->image2 = brightonReadImage(bwin, "bitmaps/knobs/extend.xpm");
	}

	/*
	 * These will force an update when we first display ourselves.
	 */
	if (bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_CENTER)
		dev->value = 0.5;
	else
		dev->value = 0;

	dev->value = 0.500001;

	dev->lastvalue = -1;
	dev->lastposition = 0;

	return(0);
}

