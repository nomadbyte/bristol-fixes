
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

#include <stdlib.h>
#include <string.h>

#include "brightoninternals.h"

extern brightonBitmap *xpmread(brightonWindow *, char *);

char *brightonhome = 0;

brightonBitmap *
brightonCreateBitmap(brightonWindow *bwin, int width, int height)
{
	brightonBitmap *bitmap;

	bitmap = brightonmalloc((size_t) sizeof(struct BrightonBitmap));

	bitmap->width = width;
	bitmap->height = height;
	bitmap->pixels = (int *) brightonmalloc((2 + width) * (2 + height)
		* sizeof(int));
	bitmap->ncolors = 0;
	bitmap->ctabsize = BRIGHTON_QR_COLORS;
	bitmap->colormap = (int *) brightonmalloc(BRIGHTON_QR_COLORS * sizeof(int));
	bitmap->uses = 1;

	/*
	 * Link it in to our window.
	 */
	if (bwin->bitmaps != NULL)
		bwin->bitmaps->last = bitmap;
	bitmap->next = bwin->bitmaps;
	bwin->bitmaps = bitmap;

	return(bitmap);
}

brightonBitmap *
brightonCreateNamedBitmap(brightonWindow *bwin, int width, int height, char *name)
{
	brightonBitmap *bitmap;

	bitmap = brightonCreateBitmap(bwin, bwin->width, bwin->height);

	bitmap->name = brightonmalloc(strlen(name) + 1);
	sprintf(bitmap->name, "%s", name);

	return(bitmap);
}

brightonBitmap *
brightonReadImage(brightonWindow *bwin, char *filename)
{
	char *extension, file[256];
	brightonBitmap *bitmap = bwin->bitmaps;

	/* printf("brightonReadImage(%x, %s)\n", bwin, filename); */

	if (filename == 0)
		return(0);

	if (filename[0] == '/')
		sprintf(file, "%s", filename);
	else {
		if (brightonhome == 0)
	 		brightonhome = getenv("BRIGHTON");

		sprintf(file, "%s/%s", brightonhome, filename);
	}

	/*
	 * See if we already have this image loaded on this screen. We cannot have
	 * global bitmap assignments since if we are working on different screens
	 * then we might not have the full set of GCs to render this image.
	 */
	while (bitmap != 0)
	{
		if ((bitmap->name != 0) && (strcmp(file, bitmap->name) == 0))
		{
			bitmap->uses++;
			/* printf("ReUse Bitmap: %s %i\n", bitmap->name, bitmap->uses); */
			return(bitmap);
		}
		bitmap = bitmap->next;
	}
	/*
	 * See if we have this already loaded. If so, pass back a handle, otherwise
	 * determine the type from the name and pass the brightonBitmap back to 
	 * the caller.
	 */
	if ((extension = rindex(file, (int) '.')) == 0)
		return(0);

	if (strcmp(".xpm", extension) == 0)
		return(xpmread(bwin, file));

	/*
	 * Add in any other handlers for gif/bmp, etc.
	 */
	return(0);
}

brightonBitmap *
brightonFreeBitmap(brightonWindow *bwin, brightonBitmap *bitmap)
{
	brightonBitmap *bitmaplist = bwin->bitmaps, *next = NULL;

	if (bitmap == 0)
		return(0);

	/* printf("brightonFreeBitmap(%x)\n", bitmap->name); */

	while (bitmaplist != 0)
	{
		if (bitmap == bitmaplist)
		{
			int i;

			if (--bitmap->uses <= 0)
			{
				/*printf("freeing %x (%s)\n", bitmaplist, bitmaplist->name); */
				/*
				 * Unlink the bitmap
				 */
				if (bitmaplist->next)
					bitmaplist->next->last = bitmaplist->last;
				if (bitmaplist->last)
					bitmaplist->last->next = bitmaplist->next;
				else
					bwin->bitmaps = bitmaplist->next;

				if (bitmaplist->colormap)
					for (i = 0; i < bitmaplist->ncolors;i++)
					{
						brightonFreeGC(bwin, bitmaplist->colormap[i]);
					}

				if (bitmaplist->colormap)
					brightonfree(bitmaplist->colormap);
				if (bitmaplist->pixels)
					brightonfree(bitmaplist->pixels);
				if (bitmaplist->name)
					brightonfree(bitmaplist->name);

				next = bitmaplist->next;

				brightonfree(bitmaplist);
			}
#ifdef DEBUG
			else {
				printf("bitmap %s has uses %i\n", bitmaplist->name,
					bitmaplist->uses);
			}
#endif
			return(next);
		}
		bitmaplist = bitmaplist->next;
	}

	return(0);
}

