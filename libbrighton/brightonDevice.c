
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
 * This is the generic device creator. It will generate the device and give it
 * all the generic handlers and tables. The device init is then also called to
 * provide its specifics.
 */

#include "brightoninternals.h"
#include "brightonDevtable.h"

extern int BSendEvent(brightonDisplay *, brightonWindow *, brightonEvent *);

static int
brightonInitDevice(brightonWindow *bwin, brightonDevice *device, int index,
char *bitmap)
{
	if (device->device == -1)
		return(0);

	/*
	 * Then call the generic inheritances.
	 */ 
	if ((brightonDevTable[device->device].from >= 0)
		&& (brightonDevTable[device->device].from != index))
	{
		int orgdev = device->device;

		device->device = brightonDevTable[device->device].from;

		brightonInitDevice(bwin, device, index, bitmap);

		device->device = orgdev;
	}

	/*
	 * And finally the specific instances
	 */
	brightonDevTable[device->device].create(bwin, device, index, bitmap);

	return(0);
}

brightonDevice *
brightonCreateDevice(brightonWindow *bwin, int type, int panel,
	int index, char *bitmap)
{
	brightonDevice *device;

/*	printf("brightonCreateDevice(%x, %i, %i, %s)\n", */
/*		bwin, type, index, bitmap); */

	if ((type < 0) || (type >= BRIGHTON_DEVTABLE_SIZE))
		return(0);

	device = brightonmalloc(sizeof(brightonDevice));
	device->device = type;
	device->panel = panel;
	device->index = index;
	device->flags |= BRIGHTON_DEV_INITED;

	if (brightonInitDevice(bwin, device, index, bitmap) == 0)
		return(device);
	else {
		brightonDestroyDevice(device);
		return(0);
	}
}

static void
brightonDestroyDevices(brightonDevice *device)
{
#warning we return early here, that may have to be resolved
printf("destroying %p\n", device);
return;
	if ((device == 0) || (device->device == -1))
		return;

	if ((brightonDevTable[device->device].from >= 0)
		&& (brightonDevTable[device->device].from != device->device))
	{
		int index = device->device;

		device->device = brightonDevTable[device->device].from;
		brightonDestroyDevices(device);
		device->device = index;
	}
	brightonDevTable[device->device].destroy(device);
}

void
brightonDestroyDevice(brightonDevice *device)
{
	printf("destroyDevice()\n");

	brightonDestroyDevices(device);

	if (device->destroy)
		device->destroy(device);

	if (device->next)
		device->next->last = device->last;
	if (device->last)
		device->last->next = device->next;

	if (device->shadow.coords)
		brightonfree(device->shadow.coords);
	if (device->shadow.mask)
		brightonfree(device->shadow.mask);

	brightonfree(device);
}

void
brightonConfigureDevice(brightonDevice *dev, brightonEvent *event)
{
}

/*
 * Event can be motion, keypress, varchange, buttonpress, etc.
 * Alterations to world are handled in configure - ie, changes in position,
 * size, etc. We may, however, collapse these into a single call?
 */
void
brightonDeviceEvent(brightonDevice *dev, brightonEvent *event)
{
}

int
brightonSendEvent(brightonWindow* bwin, int panel, int index, brightonEvent *event)
{
	/*
	 * This needs to go to BSendEvent(), this will evaluate global x/y location
	 * so that the correct device receives the even.
	 */
	event->x = bwin->app->resources[panel].devlocn[index].x
		+ bwin->app->resources[panel].x + 1;

	event->y = bwin->app->resources[panel].devlocn[index].y
		+ bwin->app->resources[panel].y;

	return(BSendEvent(bwin->display, bwin, event));
}

/*
 * This will make the GUI reflect the new value being given.
 */
int
brightonParamChange(brightonWindow *bwin, int panel, int index,
brightonEvent *event)
{
	char *image = "bitmaps/images/cable.xpm";

	if (((index < 0) && (event->type != BRIGHTON_EXPOSE))
		|| (panel < 0)
		|| (bwin == NULL))
		return(-1);

	if (panel >= bwin->app->nresources)
	{
		if (bwin->flags & BRIGHTON_DEBUG)
			printf("panel count %i over %i\n", panel, bwin->app->nresources);
		return(-1);
	}

	if (index >= bwin->app->resources[panel].ndevices)
	{
		/*
		 * Drop this debug
		printf("index %i over panel count %i (transposed keyboard?)\n",
			index, bwin->app->resources[panel].ndevices);
		 */
		return(-1);
	}

/*	if (bwin->flags & BRIGHTON_BUSY) */
/*		return(-1); */

/*	printf("brightonParamChange(%x, %i): link\n", bwin, index); */

	if (event->type == BRIGHTON_LINK)
	{
		int i2, x1, y1, x2, y2, xh;

		/*
		 * The main reason to be here is to adjust the location of the place
		 * request. We do not really mind what the device is, only the two 
		 * indices, source and dest. These are taken from the resource list
		 * and scaled according to the window geometry, then placed on the
		 * device bitmaps.
		 *
		 * We have the brightonWindow which gives us all our resources, and
		 * we should know which devices we are linking together.
		 */
 		if ((i2 = event->intvalue) < 0)
			return(0);

 		x1 = bwin->app->resources[panel].devlocn[index].x
			* bwin->app->resources[panel].sw / 1000
			+ bwin->app->resources[panel].sx;
 		y1 = bwin->app->resources[panel].devlocn[index].y
			* bwin->app->resources[panel].sh / 1000
			+ bwin->app->resources[panel].sy;
 		x2 = bwin->app->resources[panel].devlocn[i2].x
			* bwin->app->resources[panel].sw / 1000
			+ bwin->app->resources[panel].sx;
 		y2 = bwin->app->resources[panel].devlocn[i2].y
			* bwin->app->resources[panel].sh / 1000
			+ bwin->app->resources[panel].sy;

		if (x2 < x1) {
			xh = x2;
			x2 = x1;
			x1 = xh;
			xh = y2;
			y2 = y1;
			y1 = xh;
			image = "bitmaps/images/cablered.xpm";
		} else
			image = "bitmaps/images/cableyellow.xpm";

		/*
		 * We should correct positioning now: the aspect - if height is greater
		 * then width then we are going to use a different bitmap and it will
		 * be slid rather than shifted, and the endpoints need to have some
		 * constants added. Plus we want to give organised coord (x1 < x2).
		 */
		if (x2 < x1) {
			int w, h, xh = x2, yh = y2;

			image = "bitmaps/images/cableyellow.xpm";

			x2 = x1;
			y2 = y1;
			x1 = xh;
			y1 = yh;

			w = x2 - x1;
			if ((h = y2 - y1) < 0)
				h = -h;

			if (w < h) {
				x1 += 1;
				y1 -= 0;
				x2 += 3;
				y2 += 8;
				image = "bitmaps/images/cableV.xpm";
			} else {
				x1 += 2;
				y1 += 0;
				x2 += 9;
				y2 -= 1;
			}
		} else {
			int w, h;

			w = x2 - x1;

			if ((h = y2 - y1) < 0)
				h = -h;

			if (w < h) {
 				if (bwin->app->resources[panel].devlocn[i2].x <
 					bwin->app->resources[panel].devlocn[index].x)
					image = "bitmaps/images/cableVred.xpm";
				else
					image = "bitmaps/images/cableVyellow.xpm";
				if (y1 < y2) {
					x1 += 2;
					y1 += 1;
					x2 += 3;
					y2 += 7;
				} else {
					x1 += 1;
					y1 += 7;
					x2 += 2;
					y2 += 1;
				}
			} else {
				if (y1 < y2) {
					x1 += 2;
					y1 += 0;
					x2 += 8;
					y2 += 2;
				} else {
					x1 += 2;
					y1 -= 0;
					x2 += 8;
					y2 -= 1;
				}
			}
		}

		return(brightonPlace(bwin, image, x1, y1, x2, y2));
	}

	if (event->type == BRIGHTON_UNLINK)
	{
		brightonRemove(bwin, event->intvalue);
		return(-1);
	}

	if ((event->command != BRIGHTON_SLOW_TIMER)
		&& (event->command != BRIGHTON_FAST_TIMER))
		event->command = BRIGHTON_PARAMCHANGE;

/*
	if (type == 0)
		param.args.f = value;
	else if (type == 1)
		param.args.m = (void *) &value;
*/

	if (panel >= bwin->app->nresources)
		return(-1);

	if (index >= bwin->app->resources[panel].ndevices)
		return(-1);

	if (bwin->app->resources[panel].devlocn[index].type == -1)
		return(-1);

	/*
	 * See if this is a panel configuration event.
	 */
	if (index == -1)
	{
		if (bwin->app->resources[panel].configure)
			bwin->app->resources[panel].configure
				(bwin, &bwin->app->resources[panel], event);

	} else
		((brightonDevice *) bwin->app->resources[panel].devlocn[index].dev)
			->configure(bwin->app->resources[panel].devlocn[index].dev, event);

	return(0);
}

