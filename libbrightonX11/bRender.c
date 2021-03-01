
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

#include "brightonX11internals.h"

/*
 * At the moment (14/3/02) this code uses XDrawPoint directly onto the screen.
 * It could be accelerated by using an XImage structure. In the mean time this
 * will be accelerated using XDrawPoints() rather than the singular version.
 */
int
BCopyArea(brightonDisplay *display, int x, int y, int w, int h, int dx, int dy)
{
	bdisplay *bd = display->display;

/*printf("BCopyArea(%x %x, (%i/%i/%i/%i) (%i/%i/%i,%i) %x\n",*/
/*bd, display->image, x, y, w, h, dx, dy, bd->width, bd->height, bd->gc);*/

	if ((display->image == NULL) || (display->flags & _BRIGHTON_WINDOW))
		return(0);

#ifdef BRIGHTON_XIMAGE
	/* 0.10.7 code for XImage accelerators */
	if (display->flags & BRIGHTON_BIMAGE)
	{
//printf("image copy code %i %i, %i %i, %i %i\n", x, y, dx, dy, w, h);
#ifdef BRIGHTON_SHMIMAGE
		XShmPutImage(bd->display,
			(Window) ((brightonWindow *) display->bwin)->win,
			(GC) ((brightonWindow *) display->bwin)->gc,
			(XImage *) display->image,
				x, y, dx, dy, w, h, False);
#else
		XPutImage(bd->display, (Window) ((brightonWindow *) display->bwin)->win,
			(GC) ((brightonWindow *) display->bwin)->gc,
			(XImage *) display->image,
				x, y, dx, dy, w, h);
#endif
	} else
#endif
		XCopyArea(bd->display, (Pixmap) display->image,
			(Window) ((brightonWindow *) display->bwin)->win,
			(GC) ((brightonWindow *) display->bwin)->gc,
				x, y, w, h, dx, dy);

	return(0);
}

int
BResizeWindow(brightonDisplay *display, brightonWindow *bwin,
int width, int height)
{
	bdisplay *bd = display->display;

/*printf("BResizeWindow(%x, %x, %i, %i)\n", display, bwin, width, height);*/

	if (~display->flags & _BRIGHTON_WINDOW)
		XResizeWindow(bd->display, (Window) bwin->win, width, height);

	return(0);
}

#ifdef BRIGHTON_XIMAGE
#ifdef BRIGHTON_SHMIMAGE
static int
BDrawImage(brightonDisplay *display, brightonBitmap *bitmap,
int sx, int sy, int sw, int sh,
int destx, int desty)
{
	register bdisplay *bd = display->display;
/*	register brightonWindow *bwin = (brightonWindow *) display->bwin; */
	register brightonPalette *palette = display->palette;
	register int *pixels = bitmap->pixels;
	register int x, y, pindex;
	register int dx, dy = desty;
	XImage *image;
	struct shmid_ds myshmid;

//printf("BDrawImage(%p, (%i/%i/%i/%i) (%i/%i) (%i/%i) %i\n",
//bitmap, sx, sy, sw, sh, destx, desty, bd->width, bd->height, bd->depth);

	if (display->flags & _BRIGHTON_WINDOW)
		return(0);

	/*
	 * See if we need an image or to resize the image
	 */
	if (display->image == 0)
	{
		char *iData;
		Visual *visual = DefaultVisual(bd->display, bd->screen_num);

		bd->width = ((brightonWindow *) display->bwin)->width;
		bd->height = ((brightonWindow *) display->bwin)->height;

		iData = brightonX11malloc(bd->width * bd->height * sizeof(unsigned));

		/*
		 * The bitmap pad has several interpretations and none of them seem
		 * consistent, however if the last two parameters are not correctly
		 * given then the call fails (seen on 64bit systems). We did have:
		 *
		 *	8 * sizeof(long) - bd->depth, sizeof(long) * bd->width))
		 *
		 * This had been changed to
		 *
		 *	bd->depth >= 24? 32: bd->depth, 0
		 *
		 * However correctly speaking we might prefer
		 *
		 *	bd->depth > 16? 32: bd->depth > 8? 16:8, 0
		 */
		if ((image = (void *)
			XShmCreateImage(
				bd->display,
				visual,
				bd->depth,
				ZPixmap,
				0,
				&bd->shminfo,
				bd->width, bd->height))
			== NULL)
		{
			printf("failed to allocate image: try using option -pixmap\n");
			brightonX11free(iData);
			return(0);
		}
		/* Get the shared memory and check for errors */
		bd->shminfo.shmid = shmget(IPC_PRIVATE,
			image->bytes_per_line * image->height,
			IPC_CREAT | 0777 );

		if(bd->shminfo.shmid < 0)
			return(0);
		/* attach, and check for errrors */
		bd->shminfo.shmaddr = image->data =
			(char *) shmat(bd->shminfo.shmid, 0, 0);
		if (bd->shminfo.shmaddr == (char *) -1)
			return 1;
		/* set as read/write, and attach to the display */
		bd->shminfo.readOnly = False;
		XShmAttach(bd->display, &bd->shminfo);

		/*
		 * This looks odd but now that we have attached it, mark it for
		 * deletion. This will clear up the mess after all the detaches
		 * have occured.
		 */
		shmctl(bd->shminfo.shmid, IPC_STAT, &myshmid);
		shmctl(bd->shminfo.shmid, IPC_RMID, &myshmid);

		display->image = (void *) image;
	} else if ((bd->width != ((brightonWindow *) display->bwin)->width)
			|| (bd->height != ((brightonWindow *) display->bwin)->height))
	{
		char *iData;
		Visual *visual = DefaultVisual(bd->display, bd->screen_num);

		XShmDetach(bd->display, &bd->shminfo);
		XDestroyImage((XImage *) display->image);
		shmdt(bd->shminfo.shmaddr);

		bd->width = ((brightonWindow *) display->bwin)->width;
		bd->height = ((brightonWindow *) display->bwin)->height;

		iData = brightonX11malloc(bd->width * bd->height * sizeof(unsigned));

		if ((image = (void *)
			XShmCreateImage(
				bd->display,
				visual,
				bd->depth,
				ZPixmap,
				0,
				&bd->shminfo,
				bd->width, bd->height))
			== NULL)
		{
			printf("failed to reallocate image\n");
			brightonX11free(iData);
			return(0);
		}
		/* Get the shared memory and check for errors */
		bd->shminfo.shmid = shmget(IPC_PRIVATE,
			image->bytes_per_line * image->height,
			IPC_CREAT | 0777 );
		if(bd->shminfo.shmid < 0)
			return(0);
		/* attach, and check for errrors */
		bd->shminfo.shmaddr = image->data =
			(char *) shmat(bd->shminfo.shmid, 0, 0);
		if (bd->shminfo.shmaddr == (char *) -1)
			return 1;
		/* set as read/write, and attach to the display */
		bd->shminfo.readOnly = False;
		XShmAttach(bd->display, &bd->shminfo);

		/*
		 * This looks odd but now that we have attached it, mark it for
		 * deletion. This will clear up the mess after all the detaches
		 * have occured.
		 */
		shmctl(bd->shminfo.shmid, IPC_STAT, &myshmid);
		shmctl(bd->shminfo.shmid, IPC_RMID, &myshmid);

		display->image = (void *) image;
	}

	/*
	 * We now go through each pixel in the bitmap, check we have its color, and
	 * then render it onto the pixmap. After that we copy the relevant area
	 * of the pixmap over to the screen.
	 *
	 * This is currently the main performance hog since the color cache was 
	 * implemented in the libbrighton color management code. The only real
	 * resolution that I can see is to use on screen bitmap management.
	 */
	for (y = sy; y < (sy + sh); y++)
	{
		if (y >= bitmap->height)
			break;

		dx = destx;

		for (x = sx; x < (sx + sw); x++)
		{
			if (x >= bitmap->width)
				break;

			pindex = y * bitmap->width + x;

			/*
			 * Do not render blue
			 */
			if ((palette[pixels[pindex]].red == 0) 
				&& (palette[pixels[pindex]].green == 0)
				&& (palette[pixels[pindex]].blue == 65535))
			{
				++dx;
				continue;
			}

			if (palette[pixels[pindex]].pixel < 0)
			{
				/*
				 * Allocate a color so we can start painting with them.
				 */
				BAllocColor(display, &palette[pixels[pindex]],
					palette[pixels[pindex]].red,
					palette[pixels[pindex]].green,
					palette[pixels[pindex]].blue);
			}

			XPutPixel((XImage *) display->image,
				dx, dy, palette[pixels[pindex]].pixel);

			++dx;
		}

		++dy;
	}

	BCopyArea(display, destx, desty, sw, sh, destx, desty);

	return(0);
}
#else /* ShmImage code */
static int
BDrawImage(brightonDisplay *display, brightonBitmap *bitmap,
int sx, int sy, int sw, int sh,
int destx, int desty)
{
	register bdisplay *bd = display->display;
/*	register brightonWindow *bwin = (brightonWindow *) display->bwin; */
	register brightonPalette *palette = display->palette;
	register int *pixels = bitmap->pixels;
	register int x, y, pindex;
	register int dx, dy = desty;

//printf("BDrawImage(%x %x, (%i/%i/%i/%i) (%i/%i) (%i/%i) %i\n",
//bwin, bitmap, sx, sy, sw, sh, destx, desty, bd->width, bd->height, bd->depth);
	if (display->flags & _BRIGHTON_WINDOW)
		return(0);

	/*
	 * See if we need an image or to resize the image
	 */
	if (display->image == 0)
	{
		char *iData;
		Visual *visual = DefaultVisual(bd->display, bd->screen_num);

		bd->width = ((brightonWindow *) display->bwin)->width;
		bd->height = ((brightonWindow *) display->bwin)->height;

		iData = brightonX11malloc(bd->width * bd->height * sizeof(unsigned));

		/*
		 * The bitmap pad has several interpretations and none of them seem
		 * consistent, however if the last two parameters are not correctly
		 * given then the call fails (seen on 64bit systems). We did have:
		 *
		 *	8 * sizeof(long) - bd->depth, sizeof(long) * bd->width))
		 *
		 * This had been changed to
		 *
		 *	bd->depth >= 24? 32: bd->depth, 0
		 *
		 * However correctly speaking we might prefer
		 *
		 *	bd->depth > 16? 32: bd->depth > 8? 16:8, 0
		 */
		if ((display->image = (void *)
			XCreateImage(
				bd->display,
				visual,
				bd->depth,
				ZPixmap,
				0,
				iData,
				bd->width, bd->height,
		 		bd->depth > 16? 32:bd->depth > 8? 16:8,
				0))
			== NULL)
		{
			printf("failed to allocate image: try using option -pixmap\n");
			brightonX11free(iData);
			return(0);
		}
	} else if ((bd->width != ((brightonWindow *) display->bwin)->width)
			|| (bd->height != ((brightonWindow *) display->bwin)->height))
	{
		char *iData;
		Visual *visual = DefaultVisual(bd->display, bd->screen_num);

		XDestroyImage((XImage *) display->image);

		bd->width = ((brightonWindow *) display->bwin)->width;
		bd->height = ((brightonWindow *) display->bwin)->height;

		iData = brightonX11malloc(bd->width * bd->height * sizeof(unsigned));

		if ((display->image = (void *)
			XCreateImage(
				bd->display,
				visual,
				bd->depth,
				ZPixmap,
				0,
				iData,
				bd->width, bd->height,
		 		bd->depth > 16? 32:bd->depth > 8? 16:8,
				0))
			== NULL)
		{
			printf("failed to reallocate image\n");
			brightonX11free(iData);
			return(0);
		}
	}

	/*
	 * We now go through each pixel in the bitmap, check we have its color, and
	 * then render it onto the pixmap. After that we copy the relevant area
	 * of the pixmap over to the screen.
	 *
	 * This is currently the main performance hog since the color cache was 
	 * implemented in the libbrighton color management code. The only real
	 * resolution that I can see is to use on screen bitmap management.
	 */
	for (y = sy; y < (sy + sh); y++)
	{
		if (y >= bitmap->height)
			break;

		dx = destx;

		for (x = sx; x < (sx + sw); x++)
		{
			if (x >= bitmap->width)
				break;

			pindex = y * bitmap->width + x;

			/*
			 * Do not render blue
			 */
			if ((palette[pixels[pindex]].red == 0) 
				&& (palette[pixels[pindex]].green == 0)
				&& (palette[pixels[pindex]].blue == 65535))
			{
				++dx;
				continue;
			}

			if (palette[pixels[pindex]].pixel < 0)
			{
				/*
				 * Allocate a color so we can start painting with them.
				 */
				BAllocColor(display, &palette[pixels[pindex]],
					palette[pixels[pindex]].red,
					palette[pixels[pindex]].green,
					palette[pixels[pindex]].blue);
			}

			XPutPixel((XImage *) display->image,
				dx, dy, palette[pixels[pindex]].pixel);

			++dx;
		}

		++dy;
	}

	BCopyArea(display, destx, desty, sw, sh, destx, desty);

	return(0);
}
#endif /* note shmImage code */
#endif /* Image code */

static int
BDrawPixmap(brightonDisplay *display, brightonBitmap *bitmap,
int sx, int sy, int sw, int sh,
int destx, int desty)
{
	bdisplay *bd = display->display;
	brightonWindow *bwin = (brightonWindow *) display->bwin;
	brightonQRender *qrender = bd->qrender;
	brightonPalette *palette = display->palette;
	int *pixels = bitmap->pixels, ncolors = bd->ocount, cindex;
	int missed = 0;
	GC tgc;
	int x, y, pindex;
	int dx, dy = desty;

/*printf("BDrawArea(%x %x, (%i/%i/%i/%i) (%i/%i) (%i/%i) %i\n",*/
/*bwin, bitmap, sx, sy, sw, sh, destx, desty, bd->width, bd->height, bd->depth);*/

	if (display->flags & _BRIGHTON_WINDOW)
		return(0);

	if (qrender == 0)
	{
		qrender = (brightonQRender *)
			brightonX11malloc(BRIGHTON_QR_COLORS * sizeof(brightonQRender));
		bd->qrender = qrender;
		ncolors = bd->ocount = BRIGHTON_QR_COLORS;
	}

	/*
	 * See if we need an image or to resize the image
	 */
	if (display->image == 0)
	{
		bd->width = ((brightonWindow *) display->bwin)->width;
		bd->height = ((brightonWindow *) display->bwin)->height;

		display->image = (void *) XCreatePixmap(bd->display, (Window) bwin->win,
			bd->width, bd->height, bd->depth);
	} else {
		if ((bd->width != ((brightonWindow *) display->bwin)->width)
			|| (bd->height != ((brightonWindow *) display->bwin)->height))
		{
			XFreePixmap(bd->display, (Pixmap) display->image);

			bd->width = ((brightonWindow *) display->bwin)->width;
			bd->height = ((brightonWindow *) display->bwin)->height;

			display->image = (void *) XCreatePixmap(bd->display,
				(Window) bwin->win, bd->width, bd->height, bd->depth);
		}
	}

	/*
	 * We now go through each pixel in the bitmap, check we have its color, and
	 * then render it onto the pixmap. After that we copy the relevant area
	 * of the pixmap over to the screen.
	 *
	 * This is currently the main performance hog since the color cache was 
	 * implemented in the libbrighton color management code. The only real
	 * resolution that I can see is to use on screen bitmap management.
	 */
	for (y = sy; y < (sy + sh); y++)
	{
		if (y >= bitmap->height)
			break;

		dx = destx;

		for (x = sx; x < (sx + sw); x++)
		{
			if (x >= bitmap->width)
				break;

			pindex = y * bitmap->width + x;

			/*
			 * Do not render blue
			 */
			if ((palette[pixels[pindex]].red == 0) 
				&& (palette[pixels[pindex]].green == 0)
				&& (palette[pixels[pindex]].blue == 65535))
			{
				++dx;
				continue;
			}

			if (palette[pixels[pindex]].gc == 0)
			{
				/*
				 * Get a GC
				 */
				BAllocGC(display, &palette[pixels[pindex]],
					palette[pixels[pindex]].red,
					palette[pixels[pindex]].green,
					palette[pixels[pindex]].blue);
			}

			tgc = palette[pixels[pindex]].gc;
			for (cindex = 0; cindex < ncolors; cindex++)
			{
				if (qrender[cindex].gc == tgc)
					break;

				if (qrender[cindex].gc == 0)
				{
					/* 
					 * We have not filled this queue yet.
					 */
					qrender[cindex].gc = tgc;
					qrender[cindex].index = cindex;
					break;
				}
			}

			if (cindex == ncolors) {
				missed++;
				XDrawPoint(bd->display, (Pixmap) display->image,
					(GC) palette[pixels[pindex]].gc, dx, dy);
			} else {
				qrender[cindex].queue[qrender[cindex].count].x = dx;
				qrender[cindex].queue[qrender[cindex].count].y = dy;
				if (++qrender[cindex].count == BRIGHTON_QR_QSIZE)
				{
					XDrawPoints(bd->display, (Pixmap) display->image,
						tgc, qrender[cindex].queue, BRIGHTON_QR_QSIZE,
						CoordModeOrigin);
					qrender[cindex].count = 0;
				}
			}

			++dx;
		}

		++dy;
	}

	for (cindex = 0; cindex < ncolors; cindex++)
	{
		if (qrender[cindex].count == 0)
			continue;

		XDrawPoints(bd->display, (Pixmap) display->image,
			qrender[cindex].gc, qrender[cindex].queue,
			qrender[cindex].count, CoordModeOrigin);

		qrender[cindex].count = 0;
	}

	if (missed)
	{
		brightonX11free(bd->qrender);
		bd->ocount += BRIGHTON_QR_COLORS;

		bd->qrender = (brightonQRender *)
			brightonX11malloc(bd->ocount * sizeof(brightonQRender));
/*printf("Allocated %i colors (%i)\n", ncolors, bd->ocount);*/
	}
	BCopyArea(display, destx, desty, sw, sh, destx, desty);

	return(0);
}

int
BDrawArea(brightonDisplay *display, brightonBitmap *bitmap,
int sx, int sy, int sw, int sh, int destx, int desty)
{
	if (display->flags & _BRIGHTON_WINDOW)
		return(0);

#ifdef BRIGHTON_XIMAGE
	/* 0.10.7 code for XImage accelerators */
	if (display->flags & BRIGHTON_BIMAGE)
		return(BDrawImage(display, bitmap, sx, sy, sw, sh, destx, desty));
#endif

	return(BDrawPixmap(display, bitmap, sx, sy, sw, sh, destx, desty));
}
