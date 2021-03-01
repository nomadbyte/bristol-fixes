
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

#include <stdio.h>
#include <math.h>

#include "brightonX11.h"

/*
 * This is for some basic profiling of some of the routines, needed to optimise
 * the codebase.
 */
#define STATS
//#define STATS2
#ifdef STATS
#include <sys/time.h>
struct timeval tstart, tend;
#endif

#include "brightoninternals.h"

#define SQRT_TAB_SIZE 128
double sqrttab[SQRT_TAB_SIZE][SQRT_TAB_SIZE];
extern int cacheInsertColor(unsigned short, unsigned short, unsigned short, int);

void
initSqrtTab()
{
	int i, j;

	for (j = 0 ; j < SQRT_TAB_SIZE; j++)
	{
		for (i = 0 ; i < SQRT_TAB_SIZE; i++)
		{
			if ((sqrttab[i][j] = sqrt((double) (i * i + j * j))) == 0)
				sqrttab[i][j] = 0.0001;
		}
	}
}

int
brightonInitBitmap(brightonBitmap *bitmap, int value)
{
	register int pindex, total = bitmap->width * bitmap->height;

	for (pindex = 0; pindex < total; pindex++)
		bitmap->pixels[pindex] = value;

	return(0);
}

/*
 * This is dead slow.
 */
int
brightonWorldChanged(brightonWindow *bwin, int width, int height)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonWorldChanged(%i %i %i %i) %i %i\n",
			width, height,
			bwin->width, bwin->height,
			bwin->template->width, bwin->template->height);

	/*
	 * We need to maintain aspect ratio. First take the new width, and build
	 * a corresponding height. If the height is outside the realms of the
	 * display then do the reverse. If, however, the width and height are
	 * within a reasonable amount of the desired aspect ratio, just use
	 * them.
	 *
	 * 110908 - FR 2243291 - Add an option to override the checks for aspect
	 * ratio. Required by some window managers to prevent permanant redrawing 
	 * of the window however it results in some pretty gruesome images if the
	 * ratio is not a reasonable approximation of the original.
	 *
	 * There is some pretty ugly resizing stuff here. Going to propose the app
	 * will lose aspect ratio as resizing takes place, then on Focus change
	 * events it will see if aspect should be maintained and do a single resize
	 * at that point.
	 */
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("%i %i, %i %i\n", width, height, bwin->width, bwin->height);

	if (bwin->height != height)
		bwin->flags |= _BRIGHTON_SET_HEIGHT;

	/* This is the KDE Postage Stamp fix */
	if ((height < 30) || (width < 50))
	{
		brightonRequestResize(bwin,
			bwin->template->width, bwin->template->height);
		return(0);
	}

	bwin->width = width;
	bwin->height = height;

	brightonFreeBitmap(bwin, bwin->canvas);
	brightonFreeBitmap(bwin, bwin->render);
	brightonFreeBitmap(bwin, bwin->dlayer);
	brightonFreeBitmap(bwin, bwin->slayer);
	brightonFreeBitmap(bwin, bwin->tlayer);
	brightonFreeBitmap(bwin, bwin->mlayer);

	bwin->canvas = brightonCreateNamedBitmap(bwin, bwin->width, bwin->height,
		"canvas");
	bwin->dlayer = brightonCreateNamedBitmap(bwin, bwin->width, bwin->height,
		"dlayer");
	bwin->slayer = brightonCreateNamedBitmap(bwin, bwin->width, bwin->height,
		"slayer");
	bwin->tlayer = brightonCreateNamedBitmap(bwin, bwin->width, bwin->height,
		"tlayer");
	bwin->mlayer = brightonCreateNamedBitmap(bwin, bwin->width, bwin->height,
		"mlayer");
	bwin->render = brightonCreateNamedBitmap(bwin, bwin->width, bwin->height,
		"render");

	if (bwin->display->flags & BRIGHTON_ANTIALIAS_3)
	{
		brightonFreeBitmap(bwin, bwin->renderalias);
		bwin->renderalias
			= brightonCreateBitmap(bwin, bwin->width, bwin->height);
	}

	if (bwin->opacity == 0)
		bwin->opacity = 0.5f;

	/*
	 * Our device and shadow layer need to be initialised with -1
	 */
	brightonInitBitmap(bwin->dlayer, -1);
	brightonInitBitmap(bwin->slayer, -1);
	brightonInitBitmap(bwin->tlayer, -1);
	brightonInitBitmap(bwin->mlayer, -1);

	//placebox(bwin, bwin->mlayer, 100, 100, 100, 200);

	bwin->lightX = -1000;
	bwin->lightY = -1000;
	bwin->lightH = 3000;
	bwin->lightI = 0.95;

	/*
	 * We now have a configured size. We need to render the surface onto the
	 * window background, and render the image if we have any configured 
	 * blueprints. These are rendered onto our canvas.
	 */
	if (bwin->app->flags & BRIGHTON_STRETCH)
		brightonStretch(bwin, bwin->surface, bwin->canvas, 0, 0,
			bwin->width, bwin->height, bwin->app->flags);
	else
		brightonTesselate(bwin, bwin->surface, bwin->canvas, 0, 0,
			bwin->width, bwin->height, bwin->app->flags);

	if ((bwin->display->flags & BRIGHTON_ANTIALIAS_5)
		|| (bwin->display->flags & BRIGHTON_ANTIALIAS_3))
		brightonStretchAlias(bwin, bwin->image, bwin->canvas, 0, 0,
			bwin->width, bwin->height, 0.8);
	else
		brightonStretch(bwin, bwin->image, bwin->canvas, 0, 0,
			bwin->width, bwin->height, 0);

	brightonRender(bwin, bwin->canvas, bwin->render, 0, 0,
		bwin->width, bwin->height, 0);

	bwin->flags |= BRIGHTON_NO_DRAW;

	if  (bwin->app)
	{
		int panel;
		brightonEvent event;

		for (panel = 0; panel < bwin->app->nresources; panel++)
		{
			/*
			 * We need to configure the device to resize itself
			 */
			event.command = BRIGHTON_RESIZE;
			event.x = (int) bwin->app->resources[panel].x
				* bwin->width / 1000;
			event.y = (int) bwin->app->resources[panel].y
				* bwin->height / 1000;
			event.w = (int) bwin->app->resources[panel].width
				* bwin->width / 1000;
			event.h = (int) bwin->app->resources[panel].height
				* bwin->height / 1000;

			bwin->app->resources[panel].configure(bwin,
				&bwin->app->resources[panel], &event);
		}
	}

	bwin->flags &= ~BRIGHTON_NO_DRAW;

	brightonRePlace(bwin);

	brightonFinalRender(bwin, 0, 0, bwin->width, bwin->height);

	/*
	 * Call the application to see if it wants to alter anything after the
	 * world changed.
	 */
	if (bwin->template->configure)
		bwin->template->configure(bwin);

	return(0);
}

int
brightonColorQuality(brightonWindow *bwin, int quality)
{
	if ((bwin->quality = quality) < 2)
		bwin->quality = 2;
	else if (bwin->quality > 8)
		bwin->quality = 8;

	return(0);
}

int
brightonOpacity(brightonWindow *bwin, float opacity)
{
	if ((bwin->opacity = opacity) < 0.01f)
		bwin->opacity = 0.01f;
	else if (bwin->opacity > 1.0f)
		bwin->opacity = 1.0f;

	return(0);
}

void
brightonLogo(brightonWindow *bwin)
{
	bwin->flags |= BRIGHTON_NO_LOGO;
}

static int
antialias(brightonWindow *bwin, brightonBitmap *b, int x, int y, float primary)
{
	register int nr, ng, nb, dy, color = 0, cpre, cnext, pindex, cpy, cny;
	register brightonPalette *palette = bwin->display->palette;
	float secondary;

	if ((primary <= 0) || (primary > 1.0))
		return(color);

	if ((x == 0) || (x >= bwin->width - 1)
		|| (y == 0) || (y >= bwin->height - 1))
		return(0);

	dy = x + y * bwin->width;

	cpy = b->pixels[x + (y - 1) * bwin->width];
	cny = b->pixels[x + (y + 1) * bwin->width];
	cpre = b->pixels[dy - 1];
	color = b->pixels[dy];
	cnext = b->pixels[dy + 1];


	/*
	 * Generally the palette for any color C will be specified as
	 *
	 *	C * aa + (4C * (1 - n)/4)
	 *
	 * This would work if aa was a floating point from 0 to 1.0. To do this
	 * requires two options, one of which is a flag to state that we are doing
	 * either texture or full image antialiasing and the value we want to use.
	 *
	 * For now we have a couple of values for each and they use predefined
	 * levels. As the value tends to one the antialiasing lessens, ie, we take
	 * more of the target pixel and less smoothing from the surrounding ones.
	 * This is counter intuitive in my opinion, I think the smoothing should 
	 * increase as this value does, hence we will make it reciprocal.
	 *
	 * Values greater that 0.5 are actually counter productive, reducing the
	 * definition of the target pixel to the benefit of the surrounding ones.
	 */
	secondary = (1.0 - primary) / 4.0;

	nr = (int) ( ((float) palette[color].red) * primary
		+ ((float) (palette[cpy].red + palette[cny].red
		+ palette[cpre].red + palette[cnext].red)) * secondary);
	ng = (int) ( ((float) palette[color].green) * primary
		+ ((float) (palette[cpy].green + palette[cny].green
		+ palette[cpre].green + palette[cnext].green)) * secondary);
	nb = (int) ( ((float) palette[color].blue) * primary
		+ ((float) (palette[cpy].blue + palette[cny].blue
		+ palette[cpre].blue + palette[cnext].blue)) * secondary);

	/*
	 * We want to be flexible on matches otherwise we use excessive colors and
	 * slow the GUI down
	 */
	if ((pindex = brightonFindColor(palette, bwin->cmap_size,
		nr, ng, nb, bwin->quality)) >= 0)
	{
		palette[pindex].uses++;
		return(pindex);
	}

	/*
	 * If we have no free colors, then palette[0] is the default
	 */
	if ((pindex = brightonFindFreeColor(palette, bwin->cmap_size)) < 0)
		return(0);

	palette[pindex].red =  nr;
	palette[pindex].green = ng;
	palette[pindex].blue = nb;
	palette[pindex].uses++;

	palette[pindex].flags &= ~BRIGHTON_INACTIVE_COLOR;
	palette[pindex].uses++;

	/*
	 * Set up the cache.
	 */
	cacheInsertColor(nr, ng, nb, pindex);

	return(pindex);
}

static void
brightonAliasArea(brightonWindow *bwin, int x, int y, int width, int height)
{
	register int i, j, dy, aa = 1;
	register int *dest;

	if (bwin->renderalias == NULL)
		bwin->renderalias
			= brightonCreateBitmap(bwin, bwin->width, bwin->height);

	dest = bwin->renderalias->pixels;

	aa = bwin->display->flags & BRIGHTON_ANTIALIAS_3? 1:2;

	if (y == 0)
	{
		y += 1;
		height -= 1;
	}
	if (x == 0)
	{
		x += 1;
		width -= 1;
	}

	for (j = y; j < (y + height); j++)
	{
		if (j >= (bwin->height - 1))
			break;

		dy = j * bwin->width;

		for (i = x; i < (x + width); i++)
		{
			if (i >= (bwin->width - 1))
				break;

			dest[i + dy] = antialias(bwin, bwin->render, i, j, bwin->antialias);
		}
	}
}

static int
makeMonoChrome(brightonWindow *bwin, int dest)
{
	register int r, g, b;
	register int pindex;
	register brightonPalette *palette = bwin->display->palette;

	r = palette[dest].red;
	g = palette[dest].green;
	b = palette[dest].blue;

	/*
	 * Greyscale tests.
	 *	1 is average,
	 *	2 is min
	 *	3 is max
	 */
	switch (bwin->grayscale) {
		case 1:
			r = (r + g + b) / 3;
			g = b = r;
			break;
		case 2:
			r = (r + g + b) / 6;
			g = b = r;
			break;
		case 3:
			r = (r + g + b) / 12;
			g = b = r;
			break;
		case 4:
		    if (r < g) {
   				r = r < b? r:b;
			} else {
				r = g < b? g:b;
			}
			g = b = r;
			break;
		case 5:
		    if (r > g) {
   				r = r > b? r:b;
			} else {
				r = g > b? g:b;
			}
			g = b = r;
			break;
	}

	/*
	 * We want to be flexible on matches otherwise we use excessive colors and
	 * slow the GUI down
	 */
	if ((pindex = brightonFindColor(palette, bwin->cmap_size,
		r, g, b, bwin->quality - 2)) >= 0)
	{
		palette[pindex].uses++;
		return(pindex);
	}

	/*
	 * If we have no free colors, then palette[0] is the default
	 */
	if ((pindex = brightonFindFreeColor(palette, bwin->cmap_size)) < 0)
		return(0);

	palette[pindex].red =  r;
	palette[pindex].green = g;
	palette[pindex].blue = b;
	palette[pindex].uses++;

	palette[pindex].flags &= ~BRIGHTON_INACTIVE_COLOR;
	palette[pindex].uses++;

	cacheInsertColor(r, g, b, pindex);

	return(pindex);
}

static int
makeTransparent(brightonWindow *bwin, int dest, int tlayer, float opacity)
{
	register int nr, ng, nb;
	register int pindex;
	register brightonPalette *palette = bwin->display->palette;

	if (opacity >= 0.99)
		return(tlayer);

	/*
	 * Take the color selections between the two values, the more opaque the
	 * greater the more of the selected value will be taken from the top patch
	 * layer.
	 */
	nr = (int) (
		(((float) (palette[dest].red)) * (1.0f - opacity))
		+ (((float) (palette[tlayer].red)) * opacity));
	ng = (int) (
		(((float) (palette[dest].green)) * (1.0f - opacity))
		+ (((float) (palette[tlayer].green)) * opacity));
	nb = (int) (
		(((float) (palette[dest].blue)) * (1.0f - opacity))
		+ (((float) (palette[tlayer].blue)) * opacity));

	/*
	 * We want to be flexible on matches otherwise we use excessive colors and
	 * slow the GUI down
	 */
	if ((pindex = brightonFindColor(palette, bwin->cmap_size,
		nr, ng, nb, bwin->quality - 2)) >= 0)
	{
		palette[pindex].uses++;
		return(pindex);
	}

	/*
	 * If we have no free colors, then palette[0] is the default
	 */
	if ((pindex = brightonFindFreeColor(palette, bwin->cmap_size)) < 0)
		return(0);

	palette[pindex].red =  nr;
	palette[pindex].green = ng;
	palette[pindex].blue = nb;
	palette[pindex].uses++;

	palette[pindex].flags &= ~BRIGHTON_INACTIVE_COLOR;
	palette[pindex].uses++;

	cacheInsertColor(nr, ng, nb, pindex);

	return(pindex);
}

static int
makeMenuPixel(brightonWindow *bwin, int x, int y, int dy)
{
	int nr, ng, nb;
	int cpy = bwin->render->pixels[x + (y - 1) * bwin->width];
	int cny = bwin->render->pixels[x + (y + 1) * bwin->width];
	int cpre = bwin->render->pixels[dy - 1];
	int color = bwin->render->pixels[dy];
	int cnext = bwin->render->pixels[dy + 1];
	brightonPalette *palette = bwin->display->palette;

	/*
	 * This takes a pixel and does the following options as a single call:
	 *
	 *	dest[i + dy] = antialias(bwin, bwin->render, i, j, 0.05);
	 *	dest[i + dy] = makeMonoChrome(bwin, dest[i + dy]);
	 *	dest[i + dy] = makeTransparent(bwin, dest[i + dy], menu[index],
	 *		bwin->opacity);
	 *
	 * These routines were called separately however that had two negative
	 * side	effects:
	 *
	 *	1. Creates excess colours as each routine will request new brighton
	 *		colours that may never actually be rendered but are cached.
	 *	2. Has the overhead of 3 subroutine calls per pixel.
	 *
	 * The code may need to be put into brightonMenu.c
	 */
	if ((x == 0) || (x >= bwin->width - 1)
		|| (y == 0) || (y == bwin->height - 1))
		return(0);

	/*
	 * Antialias the color:
	 */
	nr = (int) (((float) palette[color].red) * 0.08
		+ ((float) (palette[cpy].red + palette[cny].red
		+ palette[cpre].red + palette[cnext].red)) * 0.23);
	ng = (int) (((float) palette[color].green) * 0.08
		+ ((float) (palette[cpy].green + palette[cny].green
		+ palette[cpre].green + palette[cnext].green)) * 0.23);
	nb = (int) (((float) palette[color].blue) * 0.08
		+ ((float) (palette[cpy].blue + palette[cny].blue
		+ palette[cpre].blue + palette[cnext].blue)) * 0.23);

	/*
	 * Make monochrome based on selected algorithm, failing any match it will
	 * not be grayscaled:
	 */
	switch (bwin->grayscale) {
		case 1:
			nr = (nr + ng + nb) / 3;
			ng = nb = nr;
			break;
		case 2:
			nr = (nr + ng + nb) / 6;
			ng = nb = nr;
			break;
		case 3:
			nr = (nr + ng + nb) / 12;
			ng = nb = nr;
			break;
		case 4:
		    if (nr < ng)
   				nr = nr < nb? nr:nb;
			else
				nr = ng < nb? ng:nb;
			ng = nb = nr;
			break;
		case 5:
		    if (nr > ng)
   				nr = nr > nb? nr:nb;
			else
				nr = ng > nb? ng:nb;
			ng = nb = nr;
			break;
	}

	/*
	 * Make it transparent to the menu layer
	 */
	nr = (int) (
		(((float) nr) * (1.0f - bwin->opacity))
		+ (((float) (palette[bwin->mlayer->pixels[dy]].red)) * bwin->opacity));
	ng = (int) (
		(((float) ng) * (1.0f - bwin->opacity))
		+ (((float) (palette[bwin->mlayer->pixels[dy]].green)) * bwin->opacity));
	nb = (int) (
		(((float) nb) * (1.0f - bwin->opacity))
		+ (((float) (palette[bwin->mlayer->pixels[dy]].blue)) * bwin->opacity));

	/*
	 * We want to be flexible on matches otherwise we use excessive colors and
	 * slow the GUI down
	 */
	if ((color = brightonFindColor(palette, bwin->cmap_size,
		nr, ng, nb, bwin->quality - 2)) >= 0)
	{
		palette[color].uses++;
		return(color);
	}

	/*
	 * If we have no free colors, then palette[0] is the default
	 */
	if ((color = brightonFindFreeColor(palette, bwin->cmap_size)) < 0)
		return(0);

	palette[color].red =  nr;
	palette[color].green = ng;
	palette[color].blue = nb;
	palette[color].uses++;

	palette[color].flags &= ~BRIGHTON_INACTIVE_COLOR;
	palette[color].uses++;

	cacheInsertColor(nr, ng, nb, color);

	return(color);
}

/*
 * This is the final render algorithm.
 * It will take the window canvas, device layer and shadow layer at a given
 * offset, put them all onto the rendering plane, and update our screen 
 * interface library.
 */

#ifdef STATS
int
statFunction(int op, char *operation)
{
	if (op == 0) {
		gettimeofday(&tstart, 0);
	} else {
		time_t delta;

		gettimeofday(&tend, 0);
		/*
		 * I hate this calculation.
		 *
		 * If seconds are the same then take the difference of the micros.
		 * If seconds are not the same then diff secs - 1 blah blah blah
		 */
		delta = (tend.tv_sec - tstart.tv_sec) * 1000000
			- tstart.tv_usec + tend.tv_usec;

		printf("%16s: %7i us\n", operation, (int) delta);
	}
	return(0);
}
#endif

static int lock = 0;

int
brightonDoFinalRender(brightonWindow *bwin,
register int x, register int y, register int width, register int height)
{
	register int i, j, dy, pindex, aa;
	register int *src;
	register int *dest;
	register int *device;
	register int *shadow;
	register int *top;
	register int *menu;
	int count = 40;

#ifdef STATS2
	statFunction(0, "Brighton");
#endif

	while (lock)
	{
		usleep(25000);
		if (--count == 0)
			lock = 0;
	}

	lock = 1;

	if (bwin == NULL)
		return(lock = 0);

	if (bwin->canvas)
		src = bwin->canvas->pixels;
	else	
		return(lock = 0);

	if (bwin->render)
		dest = bwin->render->pixels;
	else 
		return(lock = 0);

	device = bwin->dlayer->pixels;
	shadow = bwin->slayer->pixels;
	top = bwin->tlayer->pixels;
	menu = bwin->mlayer->pixels;

	if ((width < 0) || (height < 0))
		return(lock = 0);

	aa = bwin->display->flags & (BRIGHTON_ANTIALIAS_1 | BRIGHTON_ANTIALIAS_2);

	for (j = y; j < (y + height); j++)
	{
		if (j >= bwin->height)
			break;

		dy = j * bwin->width;

		for (i = x; i < (x + width); i++)
		{
			if (i >= bwin->width)
				break;

			/*pindex = i + j * bwin->width; */
			pindex = i + dy;

			/*
			 * We have 3 layers, the topmost device layer, the middle shadow
			 * layer, and the lower canvas. The colors from each layer are
			 * moved into the render layer in order of priority. Where any
			 * layer has a negative brightonColor reference it is passed over
			 * to a lower plane. Other planes may be introduced at a later date
			 * as other features are incorporated.
			 *
			 * Adding another layer initially for patch cabling. We should
			 * consider adding the top layer last and allow it to act as a 
			 * transparancy, but that is non trivial since we are not dealing
			 * with colors, but with pixel identifiers. FFS. Done.
			 *
			 * FS: We should paint in the pixel as if the transparency were
			 * not there, then, if we have a color in the transparency then we
			 * will paint it in with opacity.
			 *
			 * Going to try and add in some 'smoothing', or antialiasing of the
			 * rendering. Start with some trivial stuff - horizontal. Finally
			 * implemented a trivial LR/TB antialiasing. Will extend it to use
			 * a sandwiched bitmap since otherwise there are issues with how
			 * devices are rendered - they are not antialiased as they appear
			 * in a separate layer. The results are actualy quite nice, the
			 * textures and blueprints get smoothed and the devices stand out
			 * as more than real. Optional total smoothing is also possible.
			 */
			if (device[pindex] >= 0)
				dest[i + dy] = device[pindex];
			else if (shadow[pindex] >= 0)
				dest[i + dy] = shadow[pindex];
			else if (aa &&
					((i > 0) && (i < bwin->width - 1)) &&
					((j > 0) && (j < bwin->height - 1)))
				dest[i + dy] = antialias(bwin, bwin->canvas, i, j,
					bwin->antialias);
			else
				dest[i + dy] = src[pindex];

			if (top[pindex] >= 0)
				dest[i + dy] =
					makeTransparent(bwin, dest[i + dy], top[pindex],
						bwin->opacity);

			/*
			 * Now look for floating or other menus. These should generally be
			 * on top as not to be obscured by devices or patch cables.
			 */
			if (menu[pindex] >= 0) {
				/*
				 * So, we have to do something. I would initially suggest that
				 * the existing image be greyscaled, blurred, made transparent
				 * to the color in the menu layer.
				 *
				 * This should be put into a separate subroutine since calling
				 * each operation individually means our internal colormap gets
				 * filled with entries that are not used. This does not put any
				 * greater load on the windowing system but will increase our
				 * color cache lookup depth and hence lower lookup rate.
				dest[i + dy] = antialias(bwin, bwin->render, i, j, 0.05);
				dest[i + dy] = makeMonoChrome(bwin, dest[i + dy]);
				dest[i + dy] = makeTransparent(bwin, dest[i + dy], menu[pindex],
						bwin->opacity);
				 */
				dest[i + dy] = makeMenuPixel(bwin, i, j, i + dy);
			}
		}
	}

#ifdef STATS2
	statFunction(1, "Brighton");
	statFunction(0, "Library");
#endif

	/*
	 * The third levels of antialias is to re-alias the whole image rather
	 * than just the backgrounds. This only works if we have already been
	 * given the renderAlias image.
	 */
	if (bwin->display->flags & (BRIGHTON_ANTIALIAS_3|BRIGHTON_ANTIALIAS_4))
	{
		brightonAliasArea(bwin, x, y, width, height);
		BDrawArea(bwin->display, bwin->renderalias, x, y, width, height, x, y);
	} else
		/*
		 * Finally call the B library, of which currently only one version, the
		 * B11 interface to X11.
		 */
		BDrawArea(bwin->display, bwin->render, x, y, width, height, x, y);

#ifdef STATS2
	statFunction(1, "Library");
#endif
	return(lock = 0);
}

int
brightonFinalRender(brightonWindow *bwin,
register int x, register int y, register int width, register int height)
{
	/* 
	 * Speed up on resizing - prevent excess redraws since we will have to
	 * scan the whole lot again anyway
	 */
	if (bwin->flags & BRIGHTON_NO_DRAW)
		return(0);

/*	if (bwin->flags & BRIGHTON_BUSY) */
/*		return(0); */

/*	bwin->flags |= BRIGHTON_BUSY; */

	brightonDoFinalRender(bwin, x, y, width, height);

/*	bwin->flags &= ~BRIGHTON_BUSY; */

	return(0);
}

/*
 * A tesselation algorithm, takes a bitmap and tiles it onto the specified area.
 */
int
brightonTesselate(register brightonWindow *bwin, register brightonBitmap *src,
register brightonBitmap *dest, register int x, register int y,
register int width, register int height, int direction)
{
	register int i, j, dy, w, h;
	register int *pixels;

	if ((src == 0) || (dest == 0))
		return(0);

/*printf("brightonTesselate()\n"); */

	pixels = src->pixels;

	for (j = 0; j < height; j += src->height)
	{
		if (((j + y) >= dest->height) || (j >= height))
			break;

		dy = (j + y) * dest->width;

		/*
		 * Bounds check for reduction of last image to be rendered.
		 */
		if (j + src->height >= height)
			h = height - j;
		else
			h = src->height;

		for (i = 0; i < width; i += src->width)
		{
			if (((i + x) >= dest->width) || (i >= width))
				break;

			if (i + src->width >= width)
				w = width - i;
			else
				w = src->width;

			brightonRender(bwin, src, dest, i + x, j + y, w, h, direction);
		}
	}
	return(0);
}

/*
 * This is a render algorithm. Takes a bitmap and copies it to the
 * brightonBitmap area. We should do some bounds checking first?
 */
int
brightonRender(register brightonWindow *bwin, register brightonBitmap *src,
register brightonBitmap *dest, register int x, register int y,
register int width, register int height, int direction)
{
	register int i, j, dy;
	register int *pixels;

	if ((src == 0) || (dest == 0) || (dest == src))
		return(0);

/*printf("	render %i %i %i %i, %i %i, %i %i %x\n", x, y, width, height, */
/*src->width, src->height, dest->width, dest->height, src->pixels); */
/*printf("bitmap \"%s\" uses = %i\n", src->name, src->uses); */

	pixels = src->pixels;

	for (j = 0; j < height; j++)
	{
		if (((j + y) >= dest->height) || (j >= height))
			break;

		dy = (j + y) * dest->width + x;

		for (i = 0; i < width; i++)
		{
			if (((i + x) >= dest->width) || (i >= width))
				break;

			if (isblue(i + j * src->width, bwin->display->palette, pixels))
				continue;

			dest->pixels[i + dy] = src->pixels[i + j * src->width];
		}
	}
	return(0);
}

#define round(x) x - ((float) ((int) x)) >= 0.5? x, x + 1;

static int
brightonround(float x)
{
	int ix = x;
	float fx = ix;

	if (x - fx >= 0.5)
		return(ix + 1);

	return(ix);
}

/*
 * This is a scaling render algorithm. Takes a bitmap and speads it across
 * the brightonWindow canvas area with interpolative antialiasing. It is only
 * really intended for blueprints as they are the only ones that are not easily
 * interpolated.
 */
void
brightonStretchAlias(register brightonWindow *bwin, brightonBitmap *src,
register brightonBitmap *dest, register int x, register int y,
register int width, register int height, float primary)
{
	register float i, j, fx, fy, px, py;
	register int ix, iy, ng, nb, nr, pindex;
	register int c, n, s, e, w;
	register brightonPalette *palette = bwin->display->palette;
	register int *pixels;
	register int *scratch;

	if ((src == 0) || (dest == 0) || (src == dest))
		return;

/*
printf("	stretchAlias %i %i %i %i, %i %i: p %f\n", x, y, width, height,
src->width, src->height, primary);
*/

	pixels = src->pixels;

	/*
	 * Do not support rendering outside of the panel area, excepting where we
	 * go over outer edges.
	 */
	if ((x < 0) || (x >= bwin->width) || (y < 0) || (y >= bwin->height))
		return;

	/*
	 * Scratch should be allocated and only reallocated if it inceases in size
	 * which is a fairly easy enhancement.
	 */
	scratch = (int *) brightonmalloc(sizeof(int) * dest->width * dest->height);
	bcopy(dest->pixels, scratch, sizeof(int) * dest->width * dest->height);

	/*
	 * We now have the test bitmap, and it should have generated a palette.
	 * render the bits onto our background, and copy it over.
	 */
	for (j = y; j <= (y + height); j++)
	{
		fy = (j - y) * ((float) src->height) / ((float) height);
		iy = fy;

		if ((iy = fy) >= src->height)
			break;

		if ((fy -= iy) < 0.5)
			py = fy;
		else
			py = 1.0 - fy;

		for (i = x; i <= (x + width - 1); i++)
		{
			/*
			 * Take the x relative index into the source bitmap. These are
			 * relative since they are floating points that will eventually
			 * specify the depth of antialiasing.
			 * 
			 * We take a floating point index and an integer, one for the 
			 * bitmap index and the float for the eventual alias munging.
			 */
			fx = (i - x) * ((float) src->width) / ((float) width);
			if ((ix = fx) >= src->width)
				break;

			/* 
			 * Set pixel. Since we are using brightonBitmaps we do not have to
			 * go around using X library calls to set the pixel states.
			 *
			 * We may want to put in here isblue then destpixel;continue;
			 */
			if (isblue(ix + iy * src->width, palette, pixels))
				c = scratch[(int) (i + j * dest->width)];
			else
				c = pixels[ix + iy * src->width];
			if (c >= bwin->cmap_size) c = 1;

			if (c < 0)
				continue;

			if (iy <= 0) {
				n = c;
			} else {
				if (isblue(ix + (iy - 1) * src->width, palette, pixels))
					n = scratch[(int) (i + (j - 1) * dest->width)];
				else
					n = pixels[ix + (iy - 1) * src->width];
				if ((n < 0) || (n >= bwin->cmap_size)) continue;
			}

			if ((iy >= src->height - 1) || (j >= dest->height - 1))
				s = c;
			else {
				if (isblue(ix + (iy + 1) * src->width, palette, pixels))
					s = scratch[(int) (i + (j + 1) * dest->width)];
				else
					s = pixels[ix + (iy + 1) * src->width];
				if ((s < 0) || (s >= bwin->cmap_size)) continue;
			}

			if (ix >= src->width - 1)
				e = c;
			else {
				if (isblue(ix + 1 + iy * src->width, palette, pixels))
					e = scratch[(int) (i + j * dest->width)];
				else
					e = pixels[ix + 1 + iy * src->width];
				if ((e < 0) || (e >= bwin->cmap_size)) continue;
			}

			if (ix <= 0)
				w = c;
			else {
				if (isblue(ix - 1 + iy * src->width, palette, pixels))
					w = scratch[(int) (i - 1 + j * dest->width)];
				else
					w = pixels[ix - 1 + iy * src->width];
				if ((w < 0) || (w >= bwin->cmap_size)) continue;
			}

			/*
			 * We now have centre plus north/south/east/west. We could go for
			 * more compasspoints but that will do for now.
			 *
			 * We want to give most weight to the real current pixel and the
			 * remainder to the n/s/e/w components.
			 *
			 * Primary = 0.8
			 * Secondary = (0.999 - 0.8) / 4;
			 * 
			 * So, each secondary sums to 0.05 of itself but that is split
			 * between dx and dy respectively.
			 *
			 * primary: 0.5 + (1.0 - (fx - ix))
			 * secondary: (1.0 - primary) * 0.25;
			 * c: primary * c;
			 * n: (1.0 - (fy - iy)) * secondary * n
			 * s: (fy - iy) * secondary * s
			 * w: (1.0 - (fx - ix)) * secondary * w
			 * e: (fx - ix) * secondary * e
printf("(%f, %f) (%f, %f) (%i, %i): %i %i %i %i %i\n", i, j, fx, fy, ix, iy,
c, n, s, e, w);
printf("%f %i: %f\n", fy, iy, (fy - iy));
printf("%f %f: %f %i, %f %i\n",  px, py, fx, ix, fy, iy);
			 */
			if ((fx -= ix) < 0.5)
				px = fx;
			else
				px = 1.0 - fx;

			nr = (int) (((float) palette[c].red) * (py + px)
				+ ((float) palette[n].red) * (1.0 - fy) * (0.5 - py)
				+ ((float) palette[s].red) * fy * (0.5 - py)
				+ ((float) palette[w].red) * (1.0 - fx) * (0.5 - px)
				+ ((float) palette[e].red) * fx * (0.5 - px));

			ng = (int) (((float) palette[c].green) * (py + px)
				+ ((float) palette[n].green) * (1.0 - fy) * (0.5 - py)
				+ ((float) palette[s].green) * fy * (0.5 - py)
				+ ((float) palette[w].green) * (1.0 - fx) * (0.5 - px)
				+ ((float) palette[e].green) * fx * (0.5 - px));

			nb = (int) (((float) palette[c].blue) * (py + px)
				+ ((float) palette[n].blue) * (1.0 - fy) * (0.5 - py)
				+ ((float) palette[s].blue) * fy * (0.5 - py)
				+ ((float) palette[w].blue) * (1.0 - fx) * (0.5 - px)
				+ ((float) palette[e].blue) * fx * (0.5 - px));

			/*
			 * We want to be flexible on matches otherwise we use excessive
			 * colors and slow the GUI down
			 */
			if ((pindex = brightonFindColor(palette, bwin->cmap_size,
				nr, ng, nb, bwin->quality)) >= 0)
			{
				palette[pindex].uses++;
				dest->pixels[(int) (i + j * dest->width)] = pindex;
				continue;
			}

			/*
			 * If we have no free colors, then palette[0] is the default
			 */
			if ((pindex = brightonFindFreeColor(palette, bwin->cmap_size)) < 0)
				continue;

			palette[pindex].red =  nr;
			palette[pindex].green = ng;
			palette[pindex].blue = nb;
			palette[pindex].uses++;

			palette[pindex].flags &= ~BRIGHTON_INACTIVE_COLOR;
			palette[pindex].uses++;

			/*
			 * Set up the cache.
			 */
			cacheInsertColor(nr, ng, nb, pindex);
			dest->pixels[(int) (i + j * dest->width)] = pindex;
		}
	}

	brightonfree(scratch);
}

/*
 * This is a scaling render algorithm. Takes a bitmap and speads it across
 * the brightonWindow canvas area. We should do some bounds checking first.
 */
void
brightonStretch(register brightonWindow *bwin, brightonBitmap *src,
register brightonBitmap *dest, register int x, register int y,
register int width, register int height, int flags)
{
	register float i, j, x1 = 0;
	register int y1 = 0, ix1;
	register brightonPalette *palette = bwin->display->palette;
	register int *pixels;

	if ((src == 0) || (dest == 0) || (src == dest))
		return;

/*printf("	stretch %i %i %i %i, %i %i: flags %x\n", x, y, width, height, */
/*src->width, src->height, flags); */

	if ((pixels = src->pixels) == NULL)
		return;

	/*
	 * Do not support rendering outside of the panel area, excepting where we
	 * go over outer edges.
	 */
	if ((x < 0) || (x >= bwin->width) || (y < 0) || (y >= bwin->height))
		return;

	/*
	 * We now have the test bitmap, and it should have generated a palette.
	 * render the bits onto our background, and copy it over.
	 */
	for (i = y; i < (y + height); i++)
	{
		switch (flags & BRIGHTON_DIRECTION_MASK) {
			default:
			/* Default case and reverse take same values here */
			case BRIGHTON_REVERSE:
				y1 = (int) ((i - y) * src->height / height) * src->width;
				break;
			case BRIGHTON_VERTICAL:
				y1 = (i - y) / height * src->width;
				break;
			case (BRIGHTON_VERTICAL|BRIGHTON_REVERSE):
				if (flags & BRIGHTON_HALF_REVERSE)
				{
					y1 = (i - y) / height * src->width;
					if (y1 < src->width / 2)
						y1 += src->width / 2;
					else
						y1 -= src->width / 2;
				} else {
					y1 = src->width - (i - y) / height * src->width;
				}
				break;
		}

		for (j = x; j < (x + width); j++)
		{
			switch (flags & BRIGHTON_DIRECTION_MASK) {
				default:
				/* Default case and reverse take same values here */
				case 0:
					x1 = y1 + (j - x) * src->width / width;
					break;
				case BRIGHTON_REVERSE:
					/*
					 * This is for button operations, and is only "half"
					 * reversed. We should make this a rendering option?
					 */
					if (flags & BRIGHTON_HALF_REVERSE)
					{
						x1 = (j - x) / width * src->width;
						if (x1 < src->width / 2)
							x1 += y1 + src->width / 2;
						else
							x1 += y1 - src->width / 2;

						if (x1 >= src->width * src->height)
							x1 = src->width * src->height - 1;
					} else {
						x1 = y1 + src->width - (j - x) * src->width / width;
					}
					break;
				case BRIGHTON_VERTICAL:
				case (BRIGHTON_VERTICAL|BRIGHTON_REVERSE):
					x1 = y1
						+ (int) ((j - x) * src->height / width) * src->width;
					break;
			}

			ix1 = x1;

			/* 
			 * Set pixel. Since we are using brightonBitmaps we do not have to
			 * go around using X library calls to set the pixel states.
			 */
			if (!isblue(ix1, palette, pixels))
				dest->pixels[(int) (i * dest->width + j)] = src->pixels[ix1];
				/*   
				dest->pixels[(int) (i * dest->width + j)] = src->pixels[ix1];
				 * What I want to do here is take my source pixel and give it
				 * some antialias. This should only be done if neighbouring
				 * pixels are blue - primarily device edges and blueprints.
				 *
				 * The cheapest way to do this is just paint my pixel, then
				 * temper it from 4 neighbors.

				if (i <= 0)
					n = src->pixels[ix1];
				else
					n = dest->pixels[(int) ((i - 1) * dest->width + j)];

				if (i >= dest->height - 1)
					s = src->pixels[ix1];
				else
					s = dest->pixels[(int) ((i + 1) * dest->width + j)];

				if (j <= 0)
					w = src->pixels[ix1];
				else
					w = dest->pixels[(int) (i * dest->width + j - 1)];

				if (j >= dest->width - 1)
					e = src->pixels[ix1];
				else
					e = dest->pixels[(int) (i * dest->width + j + 1)];
				 */
		}
	}
}

/*
 * This is not a true alpha layer in a bitmap. The function is similar, it 
 * allows for fading bitmaps in and out.
 */
void
brightonAlphaLayer(register brightonWindow *bwin, register brightonBitmap *src,
register brightonBitmap *dest, register int dx, register int dy,
register int width, register int height, register double rotation)
{
	register int py, px, dp, sp, red, green, blue;
	register int i, j, ty, pindex;
	register brightonPalette *palette = bwin->display->palette;
	register float factor;

/*printf("brightonAlphaLayer()\n"); */

	for (py = 0; py < height; py+=1)
	{
		j = py * src->height / height;

		if (py >= dest->height)
			break;

		ty = (dy + py) * dest->width + dx;

		for (px = 0; px < width; px++)
		{
			i = px * src->width / width;

			if ((dp = dest->pixels[ty + px]) < 0)
				continue;

			sp = src->pixels[i + j * src->width];

/*			if ((palette[sp].red == 0) */
/*				&& (palette[sp].green == 0) */
/*				&& (palette[sp].blue == 0xff)) */
			if (isblue(i + j * src->width, palette, src->pixels))
				continue;

			/*
			 * The coloring wants to be white, but as the alpha layer gets 
			 * dimmer, the value tends back to the original:
			 *
			 * factor = (255 - palette[sp].red) / 255;
			 * red = 255 * factor + palette[dp].red * (1.0 - factor);
			 *
			red = 255 - ((255 - palette[sp].red) * palette[dp].red / 255);
			green = 255 - ((255 - palette[sp].green) * palette[dp].green / 255);
			blue = 255 - ((255 - palette[sp].blue) * palette[dp].blue / 255);
			red = (palette[dp].red + palette[sp].red) / 2;
			green = (palette[dp].green + palette[sp].green) / 2;
			blue = (palette[dp].blue + palette[sp].blue) / 2;
			red = 65535 * factor + palette[dp].red * (1.0 - factor);
			green = 65535 * factor + palette[dp].green * (1.0 - factor);
			blue = 65535 * factor + palette[dp].blue * (1.0 - factor);
			 *
			 * This works as a pure alpha layer scaling to white, I would
			 * prefer the ability to add shadow.
			 */
			if ((factor = ((float) palette[sp].red) / 65535.0) > 0.5)
			{
				red = palette[dp].red
					+ (65535 - palette[dp].red) * (factor - 0.5) * 2;
				green = palette[dp].green
					+ (65535 - palette[dp].green) * (factor - 0.5) * 2;
				blue = palette[dp].blue
					+ (65535 - palette[dp].blue) * (factor - 0.5) * 2;
			} else {
				red = palette[dp].red * factor * 2;
				green = palette[dp].green * factor * 2;
				blue = palette[dp].blue * factor * 2;
			}

			if ((pindex = brightonFindColor(palette, bwin->cmap_size,
				red, green, blue, bwin->quality))
				>= 0)
			{
				palette[pindex].uses++;
				dest->pixels[ty + px] = pindex;
				continue;
			}

			/*
			 * If we have no free colors, then palette[0] is the default
			 */
			if ((pindex = brightonFindFreeColor(palette, bwin->cmap_size)) < 0)
			{
				pindex = 0;
				dest->pixels[ty + px] = pindex;
				continue;
			}

			palette[pindex].red = red;
			palette[pindex].green = green;
			palette[pindex].blue = blue;
			palette[pindex].uses++;

			palette[pindex].flags &= ~BRIGHTON_INACTIVE_COLOR;
			palette[pindex].uses++;

			cacheInsertColor(red, green, blue, pindex);

			dest->pixels[ty + px] = pindex;
		}
	}
}

double roll = 0;
double rinc = 0.04;

/*
 * This could do with some XDrawPoints() optimisation as well.
 */
int
brightonRotate(register brightonWindow *bwin, register brightonBitmap *src,
register brightonBitmap *dest, register int dx, register int dy,
register int width, register int height, register double rotation)
{
	register int py, px;
	register double i, j;
	register int adjx, adjy, natx, naty;
	register double r, ho2, angle;
	register int x, y;
	register brightonPalette *palette = bwin->display->palette;
	register int *pixels;

/*printf("brightonRotate(%i, %i, %i, %i)\n", */
/*dx, dy, src->width, src->height); */

	if ((src == 0) || (dest == 0))
		return(0);

	pixels = src->pixels;
	ho2 = src->height / 2;
	/*
	 * Do not support rendering outside of the panel area, excepting where we
	 * go over outer edges.
	 */
	if ((dx < 0) || (dx >= bwin->width) || (dy < 0) || (dy >= bwin->height))
	{
		printf("bounds fail\n");
		return(0);
	}

	if (width & 0x01)
		width--;
	if (height & 0x01)
		height--;

	/*
	 * This is for 'wobble' on the pot cap as it rotates.
	 */
	if ((roll += rinc) > 0.3)
		rinc = -rinc;
	else if (roll < 0)
	{
		rinc = -rinc;
		roll = 0;
	}

	/*
	 * We now have the test bitmap, and it should have generated a palette.
	 * render the bits onto our background, and copy it over.
	 */
	for (py = 0; py < height; py+=1)
	{
		j = py * src->height / height;
		naty = j - ho2;

		if (py >= dest->height)
			break;

		for (px = 0; px < width; px++)
		{
			i = px * src->width / width;
			natx = i - ho2;

			if ((r = sqrttab[natx < 0?-natx:natx][naty < 0?-naty: naty]) > ho2)
				continue;

			/*
			 * Rotate this point, and then put them back into native coords.
			 * We need our own rounding algorithm - otherwise we get truncation.
			 */
			if (r < src->istatic) {
				/*
				 * Want to do something here to make the centre 'wobble'
				x = brightonround(natx + ho2);
				y = brightonround(naty + ho2);
				 */
				/*
				 * Take the angle of this point to the origin.
				 */
				if (naty < 0.0) {
					/*angle = asin(natx / r) + rotation; */
					angle = asin(natx / r) + 2*M_PI - roll;
					adjx = r * sin(angle);
					adjy = -r * cos(angle);
				} else {
					/*angle = -asin(natx / r) + M_PI; */
					angle = -asin(natx / r) + 2*M_PI - roll;
					adjx = -r * sin(angle);
					adjy = r * cos(angle);
				}
				x = brightonround(adjx + ho2);
				y = brightonround(adjy + ho2);

				/*x = brightonround(natx + ho2); */
				/*y = brightonround(naty + ho2); */
			} else if (r < src->ostatic) {
				/*
				 * Take the angle of this point to the origin.
				 */
				if (naty < 0.0) {
					angle = asin(natx / r) + rotation;
					adjx = r * sin(angle);
					adjy = -r * cos(angle);
				} else {
					angle = -asin(natx / r) + rotation;
					adjx = -r * sin(angle);
					adjy = r * cos(angle);
				}
				x = brightonround(adjx + ho2);
				y = brightonround(adjy + ho2);
			} else {
				x = brightonround(natx + ho2);
				y = brightonround(naty + ho2);
			}

			if ((x < 0) || (x >= src->height) || (y < 0) || (y >= src->height))
				continue;

			if (!isblue(x + y * src->width, palette, src->pixels))
				dest->pixels[(int) (((dy + py) * dest->width) + dx + px)]
					= src->pixels[(int) (y * src->width + x)];
		}
	}
	return(0);
}

