/* FIX VERTICAL LINKS */

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
 * The toplayer is used as an overlay, initially targetted for the patch cable
 * implementation started with the ARP2600. It may be generalised such that 
 * multiple toplayers can be defined with some kind of stacking order.
 *
 * Toplayers should be a panel option? No, keep it at the global level since
 * we may want patching between panels.
 */

#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "brightoninternals.h"

extern int brightonInitBitmap(brightonBitmap *, int);

int
brightonSRotate(brightonWindow *bwin, brightonBitmap *src, brightonBitmap *dst,
int a, int b, int c, int d)
{
	float x1, y1, x2, y2;
	float xstep, ystep, srcx;
	float length, height, shearx;
	int offset, cdown, p;

	/*
	 * This will not be a simple rotation. The bitmap needs to be stretched 
	 * first, and then we are going to find the lowest * x/y (of a/b or c/d)
	 * and rotate the bitmap up or down from there. The co-ordinates are not
	 * the central point for rotation, we will have to fix that too.
	 */
	if (c < a)
	{
		/*
		 * Let c/d become the starting point
		 */
		x1 = c; y1 = d; x2 = a; y2 = b;
	} else {
		x1 = a; y1 = b; x2 = c; y2 = d;
	}

	/*
	 * Find out what length we should have
	 */
	length = x2 - x1;
	height = y2 - y1;

	if (length < 0)
		length = -length;

	/*
	 * Go stepping through the virtual src bitmap, taking the pixal value from
	 * the true source and placing it on the dest, rotated.
	 *
	 * Going to change this to transform rather than rotate, we first stretch
	 * the bitmap to the desired length and then shear it to the desired target
	 * elevation.
	 */
	if (((height >= 9) && (length >= height))
		|| ((height < 0) && (length > -height)))
	{
		shearx = ((float) src->width - 10) / (length - 10);

		for (ystep = 0; ystep < src->height; ystep++)
		{
			cdown = 5;

			for (xstep = 0; xstep < length; xstep++)
			{
				if (xstep >= length - 5)
				{
					srcx = src->width - (cdown--);
					offset = (int) (ystep * src->width + srcx);
				} else if (xstep < 5) {
					srcx = xstep;
					offset = (int) (ystep * src->width + srcx);
				} else {
					srcx = 5 + (xstep - 5) * shearx;
					offset = (int) (ystep * src->width + srcx);
				}

				if (isblue(offset, bwin->display->palette, src->pixels))
					continue;

				if ((p = ((int) ((y1 + ystep
					+ ((int) ((height * xstep)/length)))
					* dst->width + x1 + xstep)))
					> dst->width * dst->height)
					continue;

				dst->pixels[p] = src->pixels[offset];
			}
		}
	} else {
		int oy;
		int h = 1;

		cdown = 6;

		if (height < 0)
		{
			/*
			 * We should take the lowest y, ie, probably swap the coord
			 */
			h = y2;
			y2 = y1;
			y1 = h;
			h = x2;
			x2 = x1;
			x1 = h;
			h = -1;

			height = -height;
		}

		shearx = ((float) src->height - 10) / (height - 10);

		/*
		 * We need to stretch and slip rather than shear
		 */
		for (ystep = 0; ystep < height; ystep++)
		{
			if (ystep >= height - 5)
				--cdown;

			for (xstep = 0; xstep < src->width; xstep++)
			{
				if (ystep >= height - 5)
				{
					offset = (src->height - cdown) * src->width + xstep;
					oy = length - 1;
				} else if (ystep < 5) {
					offset = (int) ystep * src->width + xstep;
					oy = 0;
				} else {
					offset = (5 + (int) ((ystep - 5) * shearx)) * src->width
						+ xstep;
					oy = length * (ystep - 5) / (height - 10);
				}

				oy *= h;

				if (isblue(offset, bwin->display->palette, src->pixels))
					continue;

				if ((p = ((int) ((y1 + ystep) * dst->width
					+ x1 + xstep + oy))) >= dst->width * dst->height)
					continue;

				dst->pixels[p] = src->pixels[offset];
			}
		}
	}
	return(0);
}

/*
 * This will place a bitmap into the transparency layer at fixed co-ordinates,
 * for now probably full window stretching.
 */
int
brightonPut(brightonWindow *bwin, char *image, int x, int y, int w, int h)
{
	int i;

	/*
	 * Find free layer item
	 */
	for (i = 0; i < BRIGHTON_ITEM_COUNT; i++)
		if (bwin->items[i].id == 0)
			break;

	if (i == BRIGHTON_ITEM_COUNT)
	{
		printf("No spare layer items\n");
		/*
		 * Consider returning zero?
		 */
		return(0);
	}

	bwin->items[i].id = 1;
	bwin->items[i].x = x;
	bwin->items[i].y = y;
	bwin->items[i].w = w;
	bwin->items[i].h = h;
	bwin->items[i].scale = bwin->width;

	if (bwin->items[i].image != NULL)
		brightonFreeBitmap(bwin, bwin->items[i].image);

	/*
	 * If an image is specified then we should try to read it, otherwise we
	 * we should just put a default red patch cable in. The use of ReadImage
	 * ensures we can share bitmaps if they already exist.
	 */
	if ((image == NULL) || 
		((bwin->items[i].image = brightonReadImage(bwin, image))
			== (brightonBitmap *) NULL))
	{
		/*
		 * If image is NULL then we could open a default bitmap however that
		 * would make less sense than just not painting anything.
		 */
		printf("Failed to open any transparency bitmap\n");
		bwin->items[i].id = 0;
		return(0);
	}

	brightonStretch(bwin, bwin->items[i].image, bwin->tlayer, x, y, w, h, 0);

	brightonFinalRender(bwin, x, y, w, h);

	bwin->items[i].flags = BRIGHTON_LAYER_PUT;
	if ((w == bwin->width) && (h == bwin->height))
		bwin->items[i].flags |= BRIGHTON_LAYER_ALL;

	return(i);
}

/*
 * For now this will just place the specified bitmap on the top layer
 */
int
brightonPlace(brightonWindow *bwin, char *image, int x, int y, int w, int h)
{
	int i, a = x, b = y, c = w, d = h;

	/*
	 * Find free layer item
	 */
	for (i = 0; i < BRIGHTON_ITEM_COUNT; i++)
		if (bwin->items[i].id == 0)
			break;

	if (i == BRIGHTON_ITEM_COUNT)
	{
		printf("No spare layer items\n");
		/*
		 * Consider returning zero?
		 */
		return(0);
	}

	bwin->items[i].id = 1;
	bwin->items[i].x = x;
	bwin->items[i].y = y;
	bwin->items[i].w = w;
	bwin->items[i].h = h;
	bwin->items[i].scale = bwin->width;

	if (bwin->items[i].image != NULL)
		brightonFreeBitmap(bwin, bwin->items[i].image);

	/*
	 * If an image is specified then we should try to read it, otherwise we
	 * we should just put a default red patch cable in.
	 */
	if ((image == NULL) || 
		((bwin->items[i].image = brightonReadImage(bwin, image))
			== (brightonBitmap *) NULL))
	{
		/*
		 * If image is NULL then we could open a default bitmap however that
		 * would make less sense than just not painting anything.
		 */
		printf("Failed to open any transparency bitmap\n");
		bwin->items[i].id = 0;
		return(0);
	}

	/*
	 * We cannot use a stretch algorithm as it does not look very good. We
	 * should find the lowest coordinates and do a stretch/rotate operation
	 * from the source onto the dest. This is not really a function that 
	 * should be in here, but we will put it here for now.
	 */
	if (y == h) {
		/*
		 * Just stretch it to fit the given area. We may need to check for a
		 * similar case with vertical?
		 */
		brightonRender(bwin, bwin->items[i].image, bwin->tlayer, x, y, w,
			bwin->items[i].image->height, 0);
	} else
		brightonSRotate(bwin, bwin->items[i].image, bwin->tlayer, x, y, w, h);

	if (c < a) {
		a = c;
		c = x;
	}
	c += 10;
	if (d < b) {
		d = b;
		b = h;
	}
	d += bwin->items[i].image->height;

	brightonFinalRender(bwin, a, b, c - a + 6, d - b);

	bwin->items[i].flags = BRIGHTON_LAYER_PLACE;

	return(i);
}

/*
 * Removal is not really possible, we have to clean up the item in our
 * list and then repaint the whole lot.
 */
int
brightonRemove(brightonWindow *bwin, int id)
{
	int a, b, c, d, i;

	if ((id < 0) || (id >= BRIGHTON_ITEM_COUNT))
	{
		for (i = 0; i < BRIGHTON_ITEM_COUNT; i++)
			bwin->items[i].id = 0;

		brightonInitBitmap(bwin->tlayer, -1);
		brightonFinalRender(bwin, 0, 0, bwin->width, bwin->height);

		return(0);
	}

	if (bwin->items[id].id <= 0)
		return(0);

	bwin->items[id].id = 0;

	a = bwin->items[id].x;
	b = bwin->items[id].y;
	c = bwin->items[id].w;
	d = bwin->items[id].h;

	brightonInitBitmap(bwin->tlayer, -1);

	for (i = 0; i < BRIGHTON_ITEM_COUNT; i++)
	{
		if (bwin->items[i].id > 0)
			brightonSRotate(bwin, bwin->items[i].image, bwin->tlayer,
				bwin->items[i].x, 
				bwin->items[i].y, 
				bwin->items[i].w, 
				bwin->items[i].h);
	}

	if (c < a) {
		a = c;
		c = bwin->items[id].x;
	}
	c += 10;
	if (d < b) {
		d = b;
		b = bwin->items[id].h;
	}
	d += bwin->items[id].image->height;

/*	brightonFinalRender(bwin, 0, 0, bwin->width, bwin->height); */
	brightonFinalRender(bwin, a, b, c - a + 6, d - b);

	return(0);
}

static char tImage[1024];

/*
 * This is called for world changes
 */
int
brightonRePlace(brightonWindow *bwin)
{
	int id, flags;
	float x = 0, y = 0, w = 0, h = 0, scale = 1.0;

	for (id = 0; id < BRIGHTON_ITEM_COUNT; id++)
	{
		if (bwin->items[id].id > 0)
		{
			flags = bwin->items[id].flags;
			if (flags & BRIGHTON_LAYER_ALL) {
				x = y = 0;
				w = bwin->width;
				h = bwin->height;
			} else {
				scale = ((float) bwin->width) / ((float) bwin->items[id].scale);
				w = ((float) bwin->items[id].w) * scale;
				h = ((float) bwin->items[id].h) * scale;
				x = ((float) bwin->items[id].x) * scale;
				y = ((float) bwin->items[id].y) * scale;
			}

			sprintf(tImage, "%s", bwin->items[id].image->name);

			/* Remove this, it will result in the rest being repainted */

			if (flags & BRIGHTON_LAYER_PLACE)
			{
				brightonRemove(bwin, id);
				brightonPlace(bwin, tImage, x, y, w, h);
			} else {
				bwin->items[id].id = 0;
				brightonPut(bwin, tImage, x, y, w, h);
			}
		}
	}

	return(0);
}

