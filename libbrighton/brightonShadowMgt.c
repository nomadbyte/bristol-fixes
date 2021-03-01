
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "brightoninternals.h"

#define S_D 6
#define S_D2 20
#define S_D3 23
#define S_D4 8
/*#define S_DEPTH * 4 / 5 */
/*#define S_DEPTH2 / 2 */

/*
 * For now we only have cheap shading - puts a fixed shape of reduced intensity
 * pixels into the shadow layer. nc - 17/04/02.
 * Extended intensity gradients for smoother shadow. nc - 03/06/06.
 */
int
brightonRenderShadow(brightonDevice *dev, int flags)
{
	register int x, y, pf, py, po = 0, ys, xs, wr = 0, sfact = 0;
	register brightonWindow *bwin = dev->bwin;
	register brightonPalette *palette = bwin->display->palette;

/*	printf("brightonRenderShadow(%x, %i, %i, %i, %i, %i, %i)\n", dev, */
/*		dev->x, dev->y, dev->width, dev->height, */
/*		dev->bwin->app->resources[dev->panel].sx, */
/*		dev->bwin->app->resources[dev->panel].sy); */

	if ((bwin->app->resources[dev->panel].flags & BRIGHTON_WITHDRAWN)
		|| (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_NOSHADOW)
		|| (dev->device == -1))
		return(0);

	if (dev->device == -1)
		return(0);

	/*
	 * If it is a rotary:
	 */
	if (dev->device == 0)
	{
		int th, tw, xo = 0, salt;
		float yo, cy = 0, dy2 = dev->height / 4;

		ys = dev->y + dev->bwin->app->resources[dev->panel].sy;
		xs = dev->x + dev->bwin->app->resources[dev->panel].sx;

/*		tw = dev->width * 4/3; */
/*		th = dev->height * 4/3; */

		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_HALFSHADOW)
		{
			tw = dev->width * 7 / 8;
			th = dev->height * 7 / 8;
		} else {
			tw = dev->width;
			th = dev->height;
		}

		th = tw = sqrtf(th * th / 2) + 1;

		/*
		 * Rotary. This needs to be changed. I want to take an angle of 45 
		 * degrees from the pot, full width, and tailor the shadow at both
		 * ends. At the moment this only tails downwards.
		 */
		yo = th;

		for (x = 0; x < tw; x++)
		{
			for (y = 0; y < th - dy2; y++)
			{
				if (y < cy)
					sfact = 1;
				else if ((sfact = y - cy) < 2)
					sfact = 1;
				else if (sfact >= S_D4)
					sfact = S_D4 - 1;

				/*
				 * As y reaches the end of the shadow we need to tail it off.
				 *	S_D4 - (th - y - 1)
				 * Salt graduates the tip
				 */
				if ((salt = S_D4 - (th - y + 1)) < 2)
					salt = 1;
				else if (salt > S_D4)
					salt = S_D4 - 1;

				if (salt > sfact)
					sfact = salt;

				if ((x < 2) && (y < 2))
					sfact = S_D4 - 1;
				if ((x < 2) && (y < 3))
					sfact = S_D4 - 2;
				else if (((x == tw) || (x == tw - 1)) && (y == 0))
					sfact = S_D4 - 1;

				pf = (ys + y + yo + 1) * bwin->slayer->width
					+ xs + x + 2 + xo;

				bwin->slayer->pixels[pf]
					= brightonGetGC(bwin,
						palette[bwin->canvas->pixels[pf]].red *sfact/S_D4,
						palette[bwin->canvas->pixels[pf]].green *sfact/S_D4,
						palette[bwin->canvas->pixels[pf]].blue *sfact/S_D4);

				if (x != tw - 1)
				{
					bwin->slayer->pixels[pf + 1]
						= brightonGetGC(bwin,
						palette[bwin->canvas->pixels[pf + 1]].red *sfact/S_D4,
						palette[bwin->canvas->pixels[pf + 1]].green *sfact/S_D4,
						palette[bwin->canvas->pixels[pf + 1]].blue *sfact/S_D4);
				}
				xo++;
			}

			yo--;
			xo = 0;

			/*
			 * This triangulates the shadow.
			 */
			if (x < tw / 2)
				cy+=2;
			else if (x > tw / 2)
				cy-=2;

			/*
			 * This shortens the outer edges
			 */
			if (x > tw * 5 / 8)
				dy2 += 1.0;
			else if ((dy2 -= 1.0) < 0)
				dy2 = 0;
		}
	} else if (dev->device == 2) {
		int height = dev->height;
		int width = dev->width;

		if ((width < 5) || (height < 5))
			return(0);

		/*
		 * Buttons. These give their real size and shape but want light 
		 * shading as they are not really supposed to have a high profile.
		 */

		/*
		 * for a brightonScale we have to cater for vertical and horizontal 
		 * movements.
		 */
		ys = dev->y + dev->bwin->app->resources[dev->panel].sy;
		xs = dev->x + dev->bwin->app->resources[dev->panel].sx;

		for (y = ys; y < ys + height; y++)
		{
			py = y * bwin->slayer->width;

			sfact = S_D;

/*			for (x = xs; x < xs + width + po; x++) */
			for (x = xs + width + po; x != xs + 4; x--)
			{
				if (--sfact < 0)
					sfact = 0;

				pf = py + x;
				/*
				 * Take whatever is in the canvas area, reduce its brightness,
				 * put that into the shadow area.
				 */
				if (flags)
					bwin->slayer->pixels[pf] = -1;
				else
					bwin->slayer->pixels[pf]
						= brightonGetGC(bwin,
							palette[bwin->canvas->pixels[pf]].red *sfact/S_D,
							palette[bwin->canvas->pixels[pf]].green *sfact/S_D,
							palette[bwin->canvas->pixels[pf]].blue *sfact/S_D);
			}
			if (po < width / 6)
				po++;
		}

		for (; y < ys + height + width / 5; y++)
		{
			py = y * bwin->slayer->width;

			sfact = S_D;

			for (x = xs + width + po - wr; x != xs; x--)
			{
				if (--sfact < 0)
					sfact = 0;

				if (y == (ys + height + width / 5) - 3)
					sfact = S_D - 3;
				else if (y == (ys + height + width / 5) - 2)
					sfact = S_D - 2;

				if (y == (ys + height + width / 5) - 1)
					sfact = S_D - 1;
				else if (x == (xs + width + po - wr))
					sfact = S_D - 1;
				else if (x == (xs + width + po - wr) - 1)
					sfact = S_D - 2;

				pf = py + x + wr;
				/*
				 * Take whatever is in the canvas area, reduce its brightness,
				 * put that into the shadow area.
				 */
				if (flags)
					bwin->slayer->pixels[pf] = -1;
				else
					bwin->slayer->pixels[pf]
						= brightonGetGC(bwin,
							palette[bwin->canvas->pixels[pf]].red *sfact/S_D,
							palette[bwin->canvas->pixels[pf]].green *sfact/S_D,
							palette[bwin->canvas->pixels[pf]].blue *sfact/S_D);
			}
			wr++;
		}
	} else {
		int height = dev->height;
		int width = dev->width;
		int div = 4;

		if ((width < 5) || (height < 5))
			return(0);

		/*
		 * for a brightonScale we have to cater for vertical and horizontal 
		 * movements.
		 */
		if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
			& BRIGHTON_VERTICAL)
		{
			height = dev->width * 2 / 3;
			width = dev->height * 2 / 3;
			xs = dev->x + dev->bwin->app->resources[dev->panel].sx
				+ (int) dev->position;
			ys = dev->y + dev->bwin->app->resources[dev->panel].sy;
		} else {
			if (dev->bwin->app->resources[dev->panel].devlocn[dev->index].flags
				& BRIGHTON_HALFSHADOW)
			{
				ys = dev->y + dev->bwin->app->resources[dev->panel].sy
					+ dev->height / 16
					+ (int) dev->position;
				div = 8;
				xs = dev->x + dev->bwin->app->resources[dev->panel].sx;
			} else {
				ys = dev->y + dev->bwin->app->resources[dev->panel].sy
					+ (int) dev->position;
				xs = dev->x + dev->bwin->app->resources[dev->panel].sx;
			}
		}

		for (y = ys; y < ys + height / div - 1; y++)
		{
			py = y * bwin->slayer->width;

			sfact = S_D;

/*			for (x = xs; x < xs + width + po; x++) */
			for (x = xs + width + po; x != xs + 4; x--)
			{
				if (--sfact < 0)
					sfact = 0;

				pf = py + x;
				/*
				 * Take whatever is in the canvas area, reduce its brightness,
				 * put that into the shadow area.
				 */
				if (flags)
					bwin->slayer->pixels[pf] = -1;
				else
					bwin->slayer->pixels[pf]
						= brightonGetGC(bwin,
							palette[bwin->canvas->pixels[pf]].red *sfact/S_D,
							palette[bwin->canvas->pixels[pf]].green *sfact/S_D,
							palette[bwin->canvas->pixels[pf]].blue *sfact/S_D);
			}
			if (po < width / 2)
				po++;
		}

		for (; y < ys + height / div + width / 2; y++)
		{
			py = y * bwin->slayer->width;

			sfact = S_D;

/*			for (x = xs; x < xs + width + po - wr; x++) */
			for (x = xs + width + po - wr; x != xs; x--)
			{
				if (--sfact < 0)
					sfact = 0;

				if (y == (ys + height / div + width / 2) - 4)
					sfact = S_D - 4;
				else if (y == (ys + height / div + width / 2) - 3)
					sfact = S_D - 3;
				else if (y == (ys + height / div + width / 2) - 2)
					sfact = S_D - 2;

				if (y == (ys + height / div + width / 2) - 1)
					sfact = S_D - 1;
				else if (x == (xs + width + po - wr))
					sfact = S_D - 1;
				else if (x == (xs + width + po - wr) - 1)
					sfact = S_D - 2;

				pf = py + x + wr;
				/*
				 * Take whatever is in the canvas area, reduce its brightness,
				 * put that into the shadow area.
				 */
				if (flags)
					bwin->slayer->pixels[pf] = -1;
				else
					bwin->slayer->pixels[pf]
						= brightonGetGC(bwin,
							palette[bwin->canvas->pixels[pf]].red *sfact/S_D,
							palette[bwin->canvas->pixels[pf]].green *sfact/S_D,
							palette[bwin->canvas->pixels[pf]].blue *sfact/S_D);
			}
			wr++;
		}

	}
	return(0);
}

