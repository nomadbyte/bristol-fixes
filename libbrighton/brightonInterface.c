
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

#include "brightoninternals.h"

extern void cleanout(brightonWindow *);

brightonDisplay *dlist = 0;

brightonWindow *
brightonInterface(brightonApp *app, int quality, int library, int aa, float aad,
int gs, int x, int y)
{
	brightonDisplay *display;

	/*
	 * Connect to the display.
	 */
#ifdef BRIGHTON_HAS_X11
	if (app->flags & BRIGHTON_WINDOW)
	{
		printf("brightonInterface(cli)\n");
		display = brightonOpenDisplay("cli");
		display->flags |= _BRIGHTON_WINDOW;
	} else if ((display = brightonOpenDisplay(NULL)) == 0) {
		printf("brightonInterface() failed\n");
		return(0);
	}
#else
	app->flags |= BRIGHTON_WINDOW;
	printf("brightonInterface(cli)\n");
	display = brightonOpenDisplay("cli");
	display->flags |= _BRIGHTON_WINDOW;
#endif
	/*
	 * Link it into our list.
	 */
	display->next = dlist;
	if (dlist)
		dlist->last = display;
	dlist = display;

	/* This should be made into an option and needs to be stuffed early */
	if ((library != 0) && (display->flags & BRIGHTON_BIMAGE))
		display->flags |= BRIGHTON_BIMAGE;
	else
		display->flags &= ~BRIGHTON_BIMAGE;

	if (aa & BRIGHTON_LIB_DEBUG)
	{
		printf("libbrighton debuging enabled\n");
		display->flags |= BRIGHTON_LIB_DEBUG;
	}
	aa &= ~BRIGHTON_LIB_DEBUG;

	switch (aa) {
		case 1:
			display->flags |= BRIGHTON_ANTIALIAS_1;
			break;
		case 2:
			display->flags |= BRIGHTON_ANTIALIAS_2;
			break;
		case 3:
			display->flags |= BRIGHTON_ANTIALIAS_3;
			break;
		case 4:
			display->flags |= BRIGHTON_ANTIALIAS_4;
			break;
		case 5:
			display->flags |= BRIGHTON_ANTIALIAS_5;
			break;
	};

printf("brighton %p %i %i\n", app, app->width, app->height);
	/*
	 * Request a new toplevel window for this app.
	 */
	if ((display->bwin = brightonCreateWindow(display,
		app,
		BRIGHTON_CMAP_SIZE,
		BRIGHTON_DEFAULT_ICON,
		quality, gs, x, y)))
	{
		if (display->flags & BRIGHTON_LIB_DEBUG)
			display->bwin->flags |= BRIGHTON_DEBUG;

		if ((display->bwin->quality = quality) < 2)
			display->bwin->quality = 2;
		else if (display->bwin->quality > 8)
			display->bwin->quality = 8;

		if (app->init)
			app->init(display->bwin);

		((brightonWindow *) display->bwin)->display = display;

		/*
		 * Map its panels and devices. Devices can be menus as well.
		 */
		brightonCreateInterface((brightonWindow *) display->bwin, app);

		display->bwin->antialias = aad;

		/*brightonEventLoop(&dlist); */

		return(display->bwin);
	}

	return(0);
}

int
brightonEventMgr()
{
	return(brightonEventLoop(&dlist));
}

int
brightonRemoveInterface(brightonWindow *bwin)
{
	brightonDisplay *d;

printf("brightonRemoveInterface(%p)\n", bwin);

	bwin->flags |= BRIGHTON_EXITING;

	if ((d = brightonFindDisplay(dlist, (brightonDisplay *) bwin->display))
		== 0)
		return(0);

	if (bwin->template->destroy != 0)
		bwin->template->destroy(bwin);

	BAutoRepeat(bwin->display, 1);
	brightonDestroyInterface(bwin);

	/*
	 * Unlink the display
	 */
	if (d->next)
		d->next->last = d->last;

	if (d->last)
		d->last->next = d->next;
	else
		dlist = d->next;

	if (dlist == 0)
	{
		brightonDestroyWindow(bwin);
		cleanout(bwin);
	}

	/*
	 * Remove window and its contents.
	 */
	brightonDestroyWindow(bwin);

	return(0);
}

/*
 * This will cover a bit of code will cover the timing for double click. It
 * will be tested here and should eventually go into the brighton library.
 *
 * First open a 'double click handle', send events that need to be checked,
 * and close the handle when no longer required. Closing is actually not 
 * trivial here if we do not have a destroy routine.
 */
#include <sys/time.h>

#define DC_TIMERS 128
struct {
	struct timeval time;
	int timeout;
} dc_timers[DC_TIMERS];

int
brightonGetDCTimer(int timeout)
{
	int dcth;

	for (dcth = 0; dcth < DC_TIMERS; dcth++)
		if (dc_timers[dcth].timeout == 0)
		{
			dc_timers[dcth].time.tv_sec = timeout / 1000000;
			dc_timers[dcth].time.tv_usec = timeout % 1000000;
//			dc_timers[dcth].time.tv_usec = 0;
			dc_timers[dcth].timeout = timeout;
			return(dcth);
		}
	return(0);
}

void
brightonFreeDCTimerHandle(int dcth)
{
	dc_timers[dcth].time.tv_sec = 0;
}

int
brightonDoubleClick(int dcth)
{
	struct timeval current;
	int delta_uS;

	if ((dcth < 0) || (dcth >= DC_TIMERS))
		return(0);

	gettimeofday(&current, NULL);

	if ((current.tv_sec - dc_timers[dcth].time.tv_sec) > 1)
	{
		/* Not a double click */
		dc_timers[dcth].time.tv_sec = current.tv_sec;
		dc_timers[dcth].time.tv_usec = current.tv_usec;

		return(0);
	}

	/*
	 * So now build a delta of the time of the two clicks and see if it is
	 * within the limit of the timer.
	 */
	if (current.tv_sec == dc_timers[dcth].time.tv_sec)
		delta_uS = current.tv_usec - dc_timers[dcth].time.tv_usec;
	else
		delta_uS = current.tv_usec + (1000000 - dc_timers[dcth].time.tv_usec);

	if (delta_uS < dc_timers[dcth].timeout)
		return(1);

	dc_timers[dcth].time.tv_sec = current.tv_sec;
	dc_timers[dcth].time.tv_usec = current.tv_usec;

	return(0);
}

/*
 * End of double click timer code.
 */

