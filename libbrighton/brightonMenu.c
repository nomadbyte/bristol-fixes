
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
#include "brightoninternals.h"

/*
 * This will eventually go into a header file and be placed inside the bwin
 * structure.
 */
#define BRIGHTON_MENUPOSTED 0x0001
typedef struct BrightonMenuCtl {
	unsigned int flags;
	int origx, origy;
	int postx, posty;
	int postw, posth;
	void *deviceMenu; /* menu entries and source data, etc */
	int panel, device;
} brightonMenuCtl;

brightonMenuCtl menu;

int
brightonMenu(brightonWindow *bwin, int x, int y, int w, int h)
{
	int dx, dy, cw, ch = h, color;
	brightonBitmap *mlayer = bwin->mlayer;

	if (menu.flags && BRIGHTON_MENUPOSTED)
	{
		brightonInitBitmap(bwin->mlayer, -1);

		brightonDoFinalRender(bwin, menu.postx, menu.posty,
			menu.postw, menu.posth);

		menu.flags &= ~BRIGHTON_MENUPOSTED;

		return(-1);
	}

	menu.origx = x;
	menu.origy = y;
	menu.postw = w;
	menu.posth = h;

	/*
	 * Do some bounds checking to make sure the menubox even fits the window.
	 * The rhodesbass is small and all can be resized.
	 */
	if (((y + h) >= bwin->height) &&
		((y = bwin->height - h - 1) < 0))
		return(-1);

	if (((x + w) >= bwin->width) &&
		((x = bwin->width - w - 1) < 0))
		return(-1);

	/*
	 * Put a box in to represent a menu, just to test the results.
	 */
	dy = y * mlayer->width;
	dx = x;

	/*
	 * Color of the border should be a parameter and we should consider finding
	 * if not inserting it.
	 */
	if ((color = brightonGetGC(bwin, 0xff00, 0xff00, 0x0000)) < 0)
	{
		printf("missed color\n");
		color = 1;
	}

	for (; h > 0; h--)
	{
		for (cw = 0; cw < w; cw++)
			mlayer->pixels[dy + dx + cw] = color;

		dy += mlayer->width;
	}

	dy = (y + 1) * mlayer->width;
	dx = x + 1;

	/*
	 * Color of the background should be a parameter and we should consider
	 * finding if not inserting it.
	 */
	if ((color = brightonGetGC(bwin, 0, 0, 0)) < 0)
	{
		printf("missed color\n");
		color = 1;
	}

	for (h = ch; h > 2; h--)
	{
		for (cw = 0; cw < (w - 2); cw++)
			mlayer->pixels[dy + dx + cw] = color;

		dy += mlayer->width;
	}

	brightonDoFinalRender(bwin, x, y, w, ch);

	menu.postx = x;
	menu.posty = y;

	menu.flags |= BRIGHTON_MENUPOSTED;

	return(-1);
}

