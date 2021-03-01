
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

#include "brightoninternals.h"

extern void bvgRenderInt(brightonWindow *, char *, brightonBitmap *);

#define MY_CALL 0x01
struct {
	unsigned int flags;
	int px, py;
	brightonBitmap frame;
	brightonPanel *panel;
} window;

void
brightonCreatePanel(int x, int y, char *image)
{
	window.px = x;
	window.py = y;

	window.flags |= MY_CALL;
}

int
brightonDevUndraw(brightonWindow *bwin, brightonBitmap *dest, int ix, int iy,
int w, int h)
{
	int x, y, z, dy, s = dest->width * dest->height;

/*printf("undraw %x %x %x, %i %i %i %i\n", */
/*bwin, dest, dest->pixels, ix, iy, w, h); */

	for (y = iy; y <= (iy + h - 1); y++)
	{
		dy = y * dest->width;

		for (x = ix; x <= (ix + w - 1); x++)
		{
			if (((z = dy + x) < 0) || (z > s))
				continue;
			dest->pixels[z] = -1;
		}
	}
	return(0);
}

void
brightonPanelLocation(brightonWindow *bwin, int panel, int index,
int x, int y, int width, int height)
{
	bwin->app->resources[panel].devlocn[index].ax = x;
	bwin->app->resources[panel].devlocn[index].ay = y;
	bwin->app->resources[panel].devlocn[index].aw = width;
	bwin->app->resources[panel].devlocn[index].ah = height;
}

static brightonILocations *ldid = 0;

static int
configurePanel(brightonWindow *bwin, brightonIResource *panel,
	brightonEvent *event)
{
	int dev;
	brightonILocations *device;

	/* printf("configure panel: %i\n", event->command); */

	if (event->command == BRIGHTON_PARAMCHANGE)
	{
		if (event->type == BRIGHTON_EXPOSE)
		{
			if (event->intvalue)
			{
//printf("	REQ EXPOSE %x %x\n", (size_t) event, (size_t) panel);
				panel->flags &= ~BRIGHTON_WITHDRAWN;

				event->command = BRIGHTON_RESIZE;

				event->x = panel->sx;
				event->y = panel->sy;
				event->w = panel->sw;
				event->h = panel->sh;

				brightonDevUndraw(bwin, bwin->dlayer,
					event->x, event->y, event->w, event->h);
				brightonDevUndraw(bwin, bwin->slayer,
					event->x, event->y, event->w, event->h);

				/*
				 * We should also go through all the tools in the panel and
				 * rerender their shadow.
				 */
				for (dev = 0; dev < panel->ndevices; dev++)
				{
					if (panel->devlocn[dev].type == 0)
						brightonRenderShadow(
							(brightonDevice *) panel->devlocn[dev].dev, 0);
				}
			} else {
//printf("	REQ UNEXPOSE %x %x\n", (size_t) event, (size_t) panel);
				panel->flags |= BRIGHTON_WITHDRAWN;
				/*
				 * On unexpose events we dont do any redraws? Conceptually we
				 * only have overlapping panels, and EXPOSE/UNEXPOSE go in
				 * pairs - the next event on another panel will draw over the
				 * top of me.
				 */
				return(0);
			}
		} else
			return(0);
	}

	if (event->command == BRIGHTON_RESIZE)
	{
		panel->sx = event->x;
		panel->sy = event->y;
		panel->sw = event->w;
		panel->sh = event->h;

		/*
		 * We need to configure our size and render any image and blueprints for
		 * this panel. Then we need to call all our devices and make sure they
		 * are also configured for their location within this panel.
		 */
		if (panel->canvas)
			brightonFreeBitmap(bwin, panel->canvas);

		panel->canvas = brightonCreateBitmap(bwin, event->w, event->h);

		if (panel->flags & BRIGHTON_WITHDRAWN)
		{
			/*
			 * NOTE:
			 * this was a fix for incorrect shadow rendering if panels are
			 * resized whilst withdrawn.
			 */
			for (dev = 0; dev < panel->ndevices; dev++)
			{
				if (panel->devlocn[dev].type == -1)
					continue;

				event->x = (int) panel->devlocn[dev].x * panel->sw / 1000;
				event->y = (int) panel->devlocn[dev].y * panel->sh / 1000;
				event->w = (int) panel->devlocn[dev].width * panel->sw / 1000;
				event->h = (int) panel->devlocn[dev].height * panel->sh / 1000;

				brightonPanelLocation(bwin,
					panel->devlocn[dev].panel, panel->devlocn[dev].index,
					event->x, event->y, event->w, event->h);

				((brightonDevice *) panel->devlocn[dev].dev)->configure(
					panel->devlocn[dev].dev, event);
			}
			return(0);
		}

		/*
		 * Render our panel canvas.
		 */
		if (panel->surface == NULL) {
			/*
			 * A few panels do not use a surface. This works, it gets filled as
			 * 'Blue' and never gets rendered. It caused problems with the
			 * prealiasing code since unless a surface is rendered then we 
			 * cannot do the antialiasing. To overcome this we paint the actual
			 * content of the parent into the panel surface.
			 * This is not as trivial as it seems as we actually want a window
			 * into the backing store, most of which can be resolved by correct
			 * placement of these elements in the image stacking order.
			 */
			brightonTesselate(bwin, bwin->canvas, panel->canvas, 0, 0,
				panel->sw, panel->sh, panel->flags);
		} else {
			if (panel->flags & BRIGHTON_STRETCH)
				brightonStretch(bwin, panel->surface, panel->canvas, 0, 0,
					panel->sw, panel->sh, panel->flags);
			else
				brightonTesselate(bwin, panel->surface, panel->canvas, 0, 0,
					panel->sw, panel->sh, panel->flags);
		}

		/*
		 * This should be done with antialiasing since when it is scaled it
		 * can look rather grizzly. Also, if we don't have a surface then we
		 * should create a dummy one to allow for antialiasing.
		 */
		if ((bwin->display->flags & BRIGHTON_ANTIALIAS_5)
			|| (bwin->display->flags & BRIGHTON_ANTIALIAS_2))
			brightonStretchAlias(bwin, panel->image, panel->canvas, 0, 0,
				panel->sw, panel->sh, 0.2);
		else
			brightonStretch(bwin, panel->image, panel->canvas, 0, 0,
				panel->sw, panel->sh, 0);

		if (panel->image)
			bvgRenderInt(bwin, rindex(panel->image->name, '/'), panel->canvas);

		/*
		 * And then render it onto the window cavas area
		 */
		brightonStretch(bwin, panel->canvas, bwin->canvas, panel->sx, panel->sy,
			panel->sw, panel->sh, BRIGHTON_ANTIALIAS);

		brightonFinalRender(bwin, panel->sx, panel->sy, panel->sw, panel->sh);

		for (dev = 0; dev < panel->ndevices; dev++)
		{
			if (panel->devlocn[dev].type == -1)
				continue;

			event->x = (int) panel->devlocn[dev].x * panel->sw / 1000;
			event->y = (int) panel->devlocn[dev].y * panel->sh / 1000;
			event->w = (int) panel->devlocn[dev].width * panel->sw / 1000;
			event->h = (int) panel->devlocn[dev].height * panel->sh / 1000;

			brightonPanelLocation(bwin,
				panel->devlocn[dev].panel, panel->devlocn[dev].index,
				event->x, event->y, event->w, event->h);

			((brightonDevice *) panel->devlocn[dev].dev)->configure(
				panel->devlocn[dev].dev, event);
		}

		return(0);
	}

	device = brightonDeviceLocator(panel,
		event->x - panel->sx, event->y - panel->sy);

	if ((panel->flags & BRIGHTON_KEY_PANEL) || ldid)
	{
		brightonEvent nEv;

//printf("keypanel\n");
		/*
		 * We may have to reinterpret some events. We want to have motion
		 * tracking to move from key to key which means if the device ID changes
		 * then send a BUTTONRELEASE on the previous ID and a BUTTONPRESS on
		 * the new one.
		 */
		if (device != ldid)
		{
//printf("release button\n");
			memcpy(&nEv, event, sizeof(brightonEvent));

			nEv.command = BRIGHTON_BUTTONRELEASE;

			nEv.x -= panel->sx;
			nEv.y -= panel->sy;

			if (bwin->activedev)
				((brightonDevice *) bwin->activedev->dev)->configure
					(bwin->activedev->dev, &nEv);
		}
		if ((panel->flags & BRIGHTON_KEY_PANEL) && device)
		{
			if (ldid != device)
			{
//printf("press button\n");
				memcpy(&nEv, event, sizeof(brightonEvent));

				nEv.command = BRIGHTON_BUTTONPRESS;

				nEv.x = event->x - panel->sx;
				nEv.y = event->y - panel->sy;

				ldid = bwin->activedev = device;

				((brightonDevice *) device->dev)->configure
					(device->dev, &nEv);

				return(0);
			}
		} else {
			ldid = 0;
			bwin->activedev = 0;
		}
	}

	if (event->command == BRIGHTON_BUTTONRELEASE)
	{
		/*printf("panel button release: %i\n", bwin->activedev); */

		if (bwin->activedev == 0)
			return(0);

		/* This should be simplified to Deliver the Event */
		if ((bwin->app->resources[bwin->activedev->panel].devlocn[
			bwin->activedev->index].flags & BRIGHTON_CENTER)
			|| (bwin->app->resources[bwin->activedev->panel].devlocn[
				bwin->activedev->index].type == 5))
		{
			/*
			 * Deliver the event.
			 */
			event->x -= panel->sx;
			event->y -= panel->sy;

			((brightonDevice *) bwin->activedev->dev)->configure
				(bwin->activedev->dev, event);
		} else if (device == bwin->activedev) {
			/*
			 * Deliver the event.
			 */
			event->x -= panel->sx;
			event->y -= panel->sy;

			((brightonDevice *) bwin->activedev->dev)->configure
				(bwin->activedev->dev, event);
		} else {
			/*
			 * Deliver the event.
			 */
			event->x -= panel->sx;
			event->y -= panel->sy;

			((brightonDevice *) bwin->activedev->dev)->configure
				(bwin->activedev->dev, event);
		}

		bwin->activedev = 0;

		return(0);
	}

	if ((event->command == BRIGHTON_BUTTONPRESS)
		|| (event->command == BRIGHTON_BUTTONRELEASE))
	{
		/*printf("panel button press\n"); */

		if ((bwin->activedev = device) == 0)
			return(0);

		/*
		 * Deliver the event.
		 */
		event->x -= panel->sx;
		event->y -= panel->sy;

		((brightonDevice *) bwin->activedev->dev)->configure
			(bwin->activedev->dev, event);

		return(0);
	}

	if (event->command == BRIGHTON_MOTION)
	{
		if (bwin->activedev)
		{
			/*
			 * Deliver the event.
			 */
			event->x -= panel->sx;
			event->y -= panel->sy;

			if (bwin->activedev->flags & BRIGHTON_CHECKBUTTON)
			{
				if (device != bwin->activedev)
				{
					event->command = BRIGHTON_LEAVE;
					((brightonDevice *) bwin->activedev->dev)->configure
						(bwin->activedev->dev, event);
				} else {
					event->command = BRIGHTON_ENTER;
					((brightonDevice *) bwin->activedev->dev)->configure
						(bwin->activedev->dev, event);
				}
			} else {
				((brightonDevice *) bwin->activedev->dev)->configure
					(bwin->activedev->dev, event);
			}
		}
/*
			else {
			if (device->flags & BRIGHTON_TRACKING)
			{
					event->command = BRIGHTON_ENTER;
					device->configure(device, event);
			}
		}
*/
		return(0);
	}

	if (event->command == BRIGHTON_KEYRELEASE)
	{
		/*
		 * Deliver the event.
		 */
		event->x -= panel->sx;
		event->y -= panel->sy;

		if (bwin->activedev)
		{
			((brightonDevice *) bwin->activedev->dev)->configure
				(bwin->activedev->dev, event);
			return(0);
		}

		if (device == 0)
			return(0);

		((brightonDevice *) device->dev)->configure(device->dev, event);
	}

	if (event->command == BRIGHTON_KEYPRESS)
	{
		/*
		 * Deliver the event.
		 */
		event->x -= panel->sx;
		event->y -= panel->sy;

		if (bwin->activedev)
		{
			((brightonDevice *) bwin->activedev->dev)->configure
				(bwin->activedev->dev, event);
			return(0);
		}

		if (device == 0)
			return(0);

		((brightonDevice *) device->dev)->configure(device->dev, event);
	}
	return(0);
}

static int
brightonCreateDevices(brightonWindow *bwin, brightonResource *res, int index)
{
	int i;
	brightonLocations *dev = res->devlocn;

/*printf("brightonCreateDevices(%x, %x, %i)\n", bwin, res, index); */

	bwin->app->resources[index].devlocn = (brightonILocations *)
		brightonmalloc(res->ndevices * sizeof(brightonILocations));

	for (i = 0; i < res->ndevices; i++)
	{
		/*printf("		%s %i (%f,%f)/(%f,%f)\n		%s, %s\n", */
		/*	dev[i].name, dev[i].device, */
		/*	dev[i].x, dev[i].y, dev[i].width, dev[i].height, */
		/*	dev[i].image, dev[i].image2); */

		bwin->app->resources[index].devlocn[i].type = dev[i].device;
		bwin->app->resources[index].devlocn[i].index = i;
		bwin->app->resources[index].devlocn[i].panel = index;
		bwin->app->resources[index].devlocn[i].x = dev[i].x;
		bwin->app->resources[index].devlocn[i].y = dev[i].y;
		bwin->app->resources[index].devlocn[i].width = dev[i].width;
		bwin->app->resources[index].devlocn[i].height = dev[i].height;

		bwin->app->resources[index].devlocn[i].from = dev[i].from;
		bwin->app->resources[index].devlocn[i].to = dev[i].to;

		if (dev[i].device == -1)
			continue;

		bwin->app->resources[index].devlocn[i].callback = dev[i].callback;
		bwin->app->resources[index].devlocn[i].flags = dev[i].flags;

		bwin->app->resources[index].devlocn[i].image =
			brightonReadImage(bwin, dev[i].image);

		if (dev[i].image2 != 0)
		{
			bwin->app->resources[index].devlocn[i].image2 =
				brightonReadImage(bwin, dev[i].image2);
		}

		bwin->app->resources[index].devlocn[i].dev =
			(struct brightonDevice *)
			brightonCreateDevice(bwin, dev[i].device, index, i, dev[i].image);
	}
	return(0);
}

int
brightonCreateInterface(brightonWindow *bwin, brightonApp *app)
{
	int i;
	brightonResource *res;

	if (app == 0)
		return(0);

/*printf("brightonCreateInterface(%x, %x)\n", bwin, app); */

	bwin->template = app;
	bwin->app = (brightonIApp *) brightonmalloc(sizeof(brightonIApp));

	bwin->app->nresources = app->nresources;
	bwin->app->flags = app->flags;
	bwin->app->init = app->init;

/*	if (bwin->app->init) */
/*		bwin->app->init(bwin); */
	bwin->app->resources = (brightonIResource *)
		brightonmalloc(bwin->app->nresources * sizeof(brightonIResource));

	/*
	 * Go through each panel on the app panels list, generate them, then do the
	 * same for each device on each panel.
	 */
	for (i = 0; i < app->nresources; i++)
	{
		res = &app->resources[i];

		/*printf("%s (%i,%i)/(%i,%i)\n	%s, %s\n", res->name, */
		/*	res->x, res->y, res->width, res->height, */
		/*	res->image, res->surface); */

		bwin->app->resources[i].x = res->x;
		bwin->app->resources[i].y = res->y;
		bwin->app->resources[i].width = res->width;
		bwin->app->resources[i].height = res->height;
		bwin->app->resources[i].flags = res->flags | BRIGHTON_ACTIVE;

		bwin->app->resources[i].image = brightonReadImage(bwin, res->image);
		bwin->app->resources[i].surface = brightonReadImage(bwin, res->surface);

		/*
		 * We need to calculate our size, and then create a backgorund canvas
		 * for panel rendering. This will only happen when the first configure
		 * notify arrives.
		 */
		bwin->app->resources[i].init = res->init;
		bwin->app->resources[i].configure = configurePanel;
		bwin->app->resources[i].callback = res->callback;

		bwin->app->resources[i].ndevices = res->ndevices;

		brightonCreateDevices(bwin, res, i);
	}
	return(0);
}

static int
brightonDestroyDevices(brightonWindow *bwin, brightonIResource *res)
{
	int i;
	brightonILocations *dev = res->devlocn;
	brightonDevice *device = (brightonDevice *) dev->dev;

	for (i = 0; i < res->ndevices; i++)
	{
		device = (brightonDevice *) dev[i].dev;

		if ((device == 0) || (device->device == -1))
			continue;

		brightonFreeBitmap(bwin, device->image);
		brightonFreeBitmap(bwin, device->image2);

		brightonFreeBitmap(bwin,
			bwin->app->resources[device->panel].devlocn[device->index].image);
		brightonFreeBitmap(bwin,
			bwin->app->resources[device->panel].devlocn[device->index].image2);

/*		brightoneDestroyDevice(bwin, dev->device); */
	}

	brightonfree(res->devlocn);

	return(0);
}

int
brightonDestroyInterface(brightonWindow *bwin)
{
	int i;

	printf("brightonDestroyInterface(%p): %i\n", bwin, bwin->app->nresources);

	for (i = 0; i < bwin->app->nresources; i++)
	{
		/*
		 * Since we are going to wipe out this interface, prevent any further
		 * rendering.
		 */
		bwin->app->resources[i].flags |= BRIGHTON_WITHDRAWN;

		brightonDestroyDevices(bwin, &bwin->app->resources[i]);

		brightonFreeBitmap(bwin, bwin->app->resources[i].image);
		brightonFreeBitmap(bwin, bwin->app->resources[i].surface);
	}

	brightonfree(bwin->app->resources);
	brightonfree(bwin->app);

	return(0);
}

