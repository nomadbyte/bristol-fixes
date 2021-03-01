
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
 * This will be a rotary potmeter. Takes a bitmap and rotates it according to
 * input from the mouse/keyboard. We need a few different parameters, and a
 * hefty include file. Where possible will try and keep X11 requests in a
 * separate set of files.
 */

#include <math.h>

#include "brightoninternals.h"
/*#include "brightonX11.h" */

extern void brightonPanelLocation(brightonWindow *, int, int, int, int, int, int);

int
destroyRotary(brightonDevice *dev)
{
	printf("destroyRotary()\n");

	if (dev->image)
		brightonFreeBitmap(dev->bwin, dev->image);
	dev->image = NULL;

	return(0);
}

static int
displayrotary(brightonDevice *dev)
{
	if (dev->bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		return(0);

	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_WITHDRAWN)
		return(0);

	/*
	 * Build up a smooth position for the pot. We may need to adjust this based
	 * on the to/from values.
	 */
	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
		& BRIGHTON_STEPPED)
	{
		dev->position = dev->value < 0.5?
			1 * M_PI * (1 - 2 * dev->value) / 3:
			M_PI * (7 - dev->value * 2) / 3;
	} else
		dev->position = dev->value < 0.5?
			7 * M_PI * (1 - 2 * dev->value) / 8:
			M_PI * (23 - dev->value * 14) / 8;

	/*
	 * Only draw fixed number of steps.
	 */
	if ((int) (dev->value * 360 / M_PI) != (int) (dev->lastvalue * 360 / M_PI))
	{
		brightonIResource *panel;

		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_REDRAW)
		{
			brightonDevUndraw(dev->bwin, dev->bwin->dlayer,
				dev->x + dev->bwin->app->resources[dev->panel].sx,
				dev->y + dev->bwin->app->resources[dev->panel].sy,
				dev->width, dev->height);
		}

		panel = &dev->bwin->app->resources[dev->panel];

		/*
		 * Rotate my image onto the parents canvas. For rotaries that have blue
		 * bitmaps its needs to be made sure that the background is clear 
		 * before this rotation is done. This means we have to copy the image
		 * background into place first.
		 */
		brightonRender(dev->bwin, dev->bwin->dlayer, dev->bwin->dlayer,
			dev->x + dev->bwin->app->resources[dev->panel].sx,
			dev->y + dev->bwin->app->resources[dev->panel].sy,
			dev->width, dev->height, 0);
		brightonRotate(dev->bwin, dev->image,
			dev->bwin->dlayer,
			dev->x + dev->bwin->app->resources[dev->panel].sx,
			dev->y + dev->bwin->app->resources[dev->panel].sy,
			dev->width, dev->height,
			dev->position);

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

		/*
		 * And request the panel to put this onto the respective image.
		 */
		brightonFinalRender(dev->bwin,
			dev->x + dev->bwin->app->resources[dev->panel].sx,
			dev->y + dev->bwin->app->resources[dev->panel].sy,
			dev->width, dev->height);
	}

	dev->lastvalue = dev->value;
	dev->lastposition = dev->position;

	return(0);
}

/*
 * This will go into brighton render
 */
static void
renderHighlights(brightonWindow *bwin, brightonDevice *dev)
{
	float d, ho2, streak;
	float ox, oy, dx, dy;

	if (dev->shadow.coords == 0)
		dev->shadow.coords = brightonmalloc(7 * sizeof(brightonCoord));
	dev->shadow.ccount = 7;
	dev->shadow.flags = BRIGHTON_STATIC;

	ho2 = dev->width / 2;
	ox = dev->x + ho2;
	oy = dev->y + ho2;

	/*
	 * We are going to render the shadow directly onto the background bitmap.
	 * We have X and Y for the source of the shadow, plus its height and 
	 * intensity. For now we will take a default relief, and highlight the
	 * background accordingly. This should be a 3D transform.....
	 *
	 * This can all be done with fractional distances, since we are going to
	 * be dealing with a number of similar triangles.
	 */
	dx = ox - bwin->lightX;
	dy = oy - bwin->lightY;
	d = sqrt((double) (dx * dx + dy * dy));

	dev->shadow.coords[0].x = ox + dy * ho2 / d;
	dev->shadow.coords[0].y = oy - dx * ho2 / d;
	dev->shadow.coords[1].x = ox - dy * ho2 / d;
	dev->shadow.coords[1].y = oy + dx * ho2 / d;

	streak = (dev->width * 1.6 * d / bwin->lightH)
		/ (1 - dev->width * 1.6 / bwin->lightH);

	dev->shadow.coords[2].x = dev->shadow.coords[1].x
		+ streak * dx / d + dy * ho2 * 0.4 / d;
	dev->shadow.coords[2].y = dev->shadow.coords[1].y
		+ streak * dy / d - dx * ho2 * 0.4 / d;

	dev->shadow.coords[6].x = dev->shadow.coords[0].x
		+ streak * dx / d - dy * ho2 * 0.4 / d;
	dev->shadow.coords[6].y = dev->shadow.coords[0].y
		+ streak * dy / d + dx * ho2 * 0.4 / d;

	streak = (dev->width * 2.07 * d / bwin->lightH)
		/ (1 - dev->width * 2.07 / bwin->lightH);

	dev->shadow.coords[3].x = ox + (streak * dx / d) - dy * ho2 * 0.5 / d;
	dev->shadow.coords[3].y = oy + (streak * dy / d) + dx * ho2 * 0.5 / d;
	dev->shadow.coords[5].x = ox + (streak * dx / d) + dy * ho2 * 0.5 / d;
	dev->shadow.coords[5].y = oy + (streak * dy / d) - dx * ho2 * 0.5 / d;

	streak = (dev->width * 2.2 * d / bwin->lightH)
		/ (1 - dev->width * 2.2 / bwin->lightH);

	dev->shadow.coords[4].x = ox + (streak * dx / d);
	dev->shadow.coords[4].y = oy + (streak * dy / d);

	/*
	printf("renderHighlights(%i) %i,%i-%i,%i-%i,%i-%i,%i-%i,%i-%i,%i-%i,%i\n",
		dev->index, dev->shadow.coords[0].x,dev->shadow.coords[0].y,
		dev->shadow.coords[1].x,dev->shadow.coords[1].y,
		dev->shadow.coords[2].x,dev->shadow.coords[2].y,
		dev->shadow.coords[3].x,dev->shadow.coords[3].y,
		dev->shadow.coords[4].x,dev->shadow.coords[4].y,
		dev->shadow.coords[5].x,dev->shadow.coords[5].y,
		dev->shadow.coords[6].x,dev->shadow.coords[6].y);
	 */

	dev->shadow.ccount = 7;
	/*
	 * rather than fill the polygon, we need to generate this shape, and lower
	 * the shading of pixels that are within this area.
	 */
}

static void
considercallback(brightonDevice *dev)
{
	brightonIResource *panel = &dev->bwin->app->resources[dev->panel];

	if (dev->bwin->flags & BRIGHTON_NO_DRAW)
		return;

	if (dev->value < 0)
		dev->value = 0;

	/*
	 * We now need to consider rounding this to the resolution of this
	 * device. If the max value is not 1.0 then we need to put fixed steps
	 * into our new device value.
	 */
	if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].to != 1.0)
	{
		dev->value =
			(dev->value
			* dev->bwin->app->resources[dev->panel].devlocn[dev->index].to);

		if ((dev->value - ((int) dev->value)) > 0.5)
			dev->value = ((float) ((int) dev->value) + 1)
				/ dev->bwin->app->resources[dev->panel].devlocn[dev->index].to;
		else
			dev->value = ((float) ((int) dev->value))
				/ dev->bwin->app->resources[dev->panel].devlocn[dev->index].to;
	}

	/*
	 * I am not sure this is desired functionality. Yes, it reduces the total
	 * number of events sent, but there are issues with multifunction panels
	 * and memory loading.
	if (dev->lastvalue != dev->value)
	 */
	{
		if (panel->devlocn[dev->index].callback)
		{
			panel->devlocn[dev->index].callback(dev->bwin,
				dev->panel, dev->index,
				dev->value * panel->devlocn[dev->index].to);
		} else if (panel->callback) {
			panel->callback(dev->bwin, dev->panel, dev->index,
				dev->value * panel->devlocn[dev->index].to);
		}
	}
}

static int cx, cy;
static float sval;

static int
configure(brightonDevice *dev, brightonEvent *event)
{
/*printf("configureRotary(%i)\n", event->command); */

	if (event->command == -1)
		return(-1);

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

		return(0);
	}

	if (event->command == BRIGHTON_BUTTONRELEASE)
	{
		cx = cy = sval = -1;
		dev->flags &= ~BRIGHTON_CONTROLKEY;
		return(0);
	}

	if (event->command == BRIGHTON_RESIZE)
	{
		dev->originx = event->x;
		dev->originy = event->y;

		if (event->w < event->h)
		{
			/*
			 * If width is less than height, then we need to configure
			 * some offsets. Also, we only want even number of pixel areas.
			 */
			dev->x = event->x;
			dev->y = event->y;
			dev->width = event->w & ~0x01;
			dev->height = event->w & ~0x01;
		} else if (event->w > event->h) {
			dev->x = event->x;
			dev->y = event->y;
			dev->width = event->h & ~0x01;
			dev->height = event->h & ~0x01;
		} else {
			dev->x = event->x;
			dev->y = event->y;
			dev->width = event->w & ~0x01;
			dev->height = event->h & ~0x01;
		}

		/*
		 * We should now rework our parent understanding of our window, since
		 * it will have altered.
		 */
		brightonPanelLocation(dev->bwin,
			dev->panel, dev->index, dev->x, dev->y, dev->width, dev->height);

		considercallback(dev);

		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_WITHDRAWN)
			return(0);

		/*
		 * We need to build in some shadow, to prevent the rotary from looking
		 * like it is hanging in mid air.
		 */
		brightonRenderShadow(dev, 0);

		dev->lastvalue = -1;
		displayrotary(dev);

		return(0);
	}

	if (event->command == BRIGHTON_KEYRELEASE)
	{
		switch(event->key) {
			default:
				break;
			case 37:
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

	if (event->command == BRIGHTON_KEYPRESS)
	{
		switch(event->key) {
			default:
				break;
			case 37:
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
			case 0x6b:
			case 0xff52:
				/*
				 * UP arrow - we need to put some bristol keysyms in here. For
				 * portability we should not include the X11 headers but map
				 * our own translations.
				 */
				if (dev->flags & BRIGHTON_SHIFTKEY)
				{
					if ((dev->value += ((float) 256) / 16384) > 1.0)
						dev->value = 1.0;
				} else {
					if ((dev->value += ((float) 1) / 16384) > 1.0)
						dev->value = 1.0;
				}
				break;
			case 0x6a:
			case 0xff54:
				/*
				 * Down arrow
				 * We should have this consider stepped rotary motion where a
				 * single up or down would give a whole step rather than a
				 * micromovement
				 */
				if (dev->flags & BRIGHTON_SHIFTKEY)
				{
					if ((dev->value -= ((float) 256) / 16384) < 0)
						dev->value = 0;
				} else {
					if ((dev->value -= ((float) 1) / 16384) < 0)
						dev->value = 0;
				}
				break;
			case 0xff51:
				/* Left arrow */
				if (dev->flags & BRIGHTON_SHIFTKEY)
				{
					if ((dev->value -= ((float) 2048) / 16384) < 0)
						dev->value = 0;
				} else {
					if ((dev->value -= ((float) 32) / 16384) < 0)
						dev->value = 0;
				}
				break;
			case 0xff53:
				/* Right arrow */
				if (dev->flags & BRIGHTON_SHIFTKEY)
				{
					if ((dev->value += ((float) 2048) / 16384) > 1.0)
						dev->value = 1.0;
				} else {
					if ((dev->value += ((float) 32) / 16384) > 1.0)
						dev->value = 1.0;
				}
				break;
		}

		considercallback(dev);

		displayrotary(dev);
	}

	/*
	 * Adding a fine adjustment control, and adding a notch into the motion
	 * control under option
	 */
	if (event->command == BRIGHTON_MOTION)
	{
		if (dev->flags & BRIGHTON_CONTROLKEY)
		{
			float deltax;

			/*
			 * We should have this consider stepped rotary motion where a single
			 * up or down would give a whole step rather than a micromovement
			 */
			if (cx == -1)
			{
				sval = dev->value;
				cx = event->x;
				cy = event->y;
			}

			deltax = ((float) (event->x - cx)) / 16383.0f;

			dev->value = sval + deltax;

/*printf("Controlled motion from (%i, %i) to (%i, %i): %f\n", */
/*cx, cy, event->x, event->y, deltax); */
		} else if (dev->bwin->flags & BRIGHTON_ROTARY_UD) {
			if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
				& BRIGHTON_NOTCH)
			{
				dev->value = 1.0f - ((float) (event->y)) * 1.1f
					/ (dev->bwin->height);

				/* If in range 0.45/0.55 - interpret 0.5 */
				if (dev->value > 0.55)
					dev->value -= 0.05;
				else if (dev->value < 0.45)
					dev->value += 0.05;
				else
					dev->value = 0.5;

				if (dev->value < 0.0f)
					dev->value = 0.0f;
				else if (dev->value > 1.0f)
					dev->value = 1.0f;
			} else {
				if ((dev->value = 1.0f - ((float) event->y)
					/ (dev->bwin->height)) < 0)
					dev->value = 0.0f;
				else if (dev->value > 1.0f)
					dev->value = 1.0f;
			}
			//printf("%f %i %i, %i %i\n", dev->value,
				//event->x, event->y, event->ox, event->oy);
		} else {
			double angle, diffx, diffy;

			diffx = event->x - (dev->width / 2 + dev->x);
			diffy = event->y - (dev->height / 2 + dev->y);
			angle = atan(diffy / diffx);

			/*
			 * Adjust this so that we get counterclock rotating angles from
			 * mid top.
			 */
			if (((diffx < 0) && (diffy < 0)) || ((diffx < 0) && (diffy >= 0)))
				angle = M_PI / 2 - angle;
			else
				angle = 3 * M_PI / 2 - angle;

			/*
			 * To correct to clock rotating angles, with stop points.
			 */
			if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
				& BRIGHTON_STEPPED)
			{
				/*
				 * Stepped controllers have limited motion, not from 145 to 
				 * -145 but from 60 to -60
				 *
				 * The '7' is 2M_PI + M_PI/3
				 */
				if (angle < M_PI)
					dev->value = (M_PI / 3 - angle) / 2;
				else
					dev->value = (7 * M_PI / 3 - angle) / 2;
			} else {
				/*
				 * If the angle is negative then we are PM on clockface and
				 * we have motion up to 7/8 of M_PI.
				 *
				 * If the angle is positive then we are AM on clockface and
				 * we have motion from 7/8.
				 *
				 * The '23' is 2M_PI + 7M_PI/8
				 */
				if (angle < M_PI)
					dev->value = (7 * M_PI / 8 - angle) * 4 / (M_PI * 7);
				else
					dev->value = (23 * M_PI / 8 - angle) * 4 / (M_PI * 7);
			}

			if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
				& BRIGHTON_NOTCH)
			{
				if (dev->value > 0.55)
					dev->value -= 0.05;
				else if (dev->value < 0.45)
					dev->value += 0.05;
				else
					dev->value = 0.5;
			}
		}

		if (dev->value < 0)
			dev->value = 0;
		else if (dev->value > 1.0)
			dev->value = 1.0;

		considercallback(dev);

		displayrotary(dev);

		return(0);
	}

	if (event->command == BRIGHTON_PARAMCHANGE)
	{
		dev->value = event->value
			/ dev->bwin->app->resources[dev->panel].devlocn[dev->index].to;

		considercallback(dev);

		displayrotary(dev);

		return(0);
	}

	return(0);
}

int *
createRotary(brightonWindow *bwin, brightonDevice *dev, int index, char *bitmap)
{
	/*printf("createRotary(%s)\n", bitmap); */

	dev->destroy = destroyRotary;
	dev->configure = configure;
	dev->bwin = bwin;
	dev->index = index;

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
			dev->image = brightonReadImage(bwin, "bitmaps/knobs/knob.xpm");
	} else {
		if (dev->image)
			brightonFreeBitmap(bwin, dev->image);
		dev->image = brightonReadImage(bwin, bitmap);
	}

	/*
	 * We should take a peek at the second image resource. A rotary only uses
	 * a single bitmap, the second may be used as an alpha layer.
	 */
	if (bwin->app->resources[dev->panel].devlocn[dev->index].image2 != 0)
		dev->image2 = 
			bwin->app->resources[dev->panel].devlocn[dev->index].image2;

	/*
	 * These will force an update when we first display ourselves.
	 */
	dev->value = 0;
	dev->lastvalue = -1;
	dev->lastposition = -1;

	return(0);
}

