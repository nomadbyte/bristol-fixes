
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
#include <sys/types.h>

#include "brightoninternals.h"
#include "brightonX11.h"

static float opacity = 1.0;

/*
 * Much of this could do with a rework to make it more generic, at the moment
 * a lot of the key handlers are very bespoke.
 */

extern void brightonKeyInput(brightonWindow *, int, int);
extern void brightonControlKeyInput(brightonWindow *, int, int);
extern void brightonShiftKeyInput(brightonWindow *, int, int, int);
extern void brightonControlShiftKeyInput(brightonWindow *, int, int, int);
extern void printColorCacheStats(brightonWindow *);

/*
 * TOOK THIS OUT BY POPULAR DEMAND
 */
//extern int brightonMenu();

int
brightonKeyPress(brightonWindow *bwin, brightonEvent *event)
{
	brightonIResource *panel;

	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonKeyPress(%i)\n", event->key);

	if ((event->key == 'p') && (event->flags & BRIGHTON_MOD_CONTROL))
	{
		brightonXpmWrite(bwin, "/tmp/brighton.xpm");
		printColorCacheStats(bwin);
	}

	/*
	 * All of the following up to the dispatch is for the top layer opacity, it
	 * could go into a separate file?
	 *
	 * T should just be a toggle between totally opaque and the current 
	 * setting. There are a few other keys that are natively interpreted for
	 * rather trivial reasons before being passed through to the GUI. These
	 * implicit mappings should be dropped.
	 */
	if ((event->key == 't') && (event->flags & BRIGHTON_MOD_CONTROL))
	{
		float hold;

		hold = bwin->opacity;
		bwin->opacity = opacity;
		opacity = hold;
		brightonFinalRender(bwin, 0, 0, bwin->width, bwin->height);
	} else if ((event->key == 'o') && (event->flags & BRIGHTON_MOD_CONTROL)) {
		/*
		 * 'O' should be to make more opaque, 'o' more transparent. This is not
		 * as * trivial as it seems since the shift key is always a separate
		 * event.
		 */
		if (event->flags & BRIGHTON_MOD_SHIFT)
		{
			if (bwin->opacity == 1.0f)
				bwin->opacity = 0.2f;
			else if ((bwin->opacity += 0.1f) > 1.0f)
				bwin->opacity = 1.0f;
			brightonFinalRender(bwin, 0, 0, bwin->width, bwin->height);
		} else {
			if (bwin->opacity <= 0.21f)
				bwin->opacity = 1.0f;
			else if ((bwin->opacity -= 0.2f) < 0.2f)
				bwin->opacity = 0.2f;
			brightonFinalRender(bwin, 0, 0, bwin->width, bwin->height);
		}
	}

	/*
	 * We now want to look to see if the request needs to be dispatched to any
	 * devices. This means 'if the mouse is within any device then pass the key
	 * event to that device.
	 */
	if ((bwin->flags & BRIGHTON_DEV_ACTIVE) && (bwin->activepanel != 0))
	{
		bwin->activepanel->configure(bwin, bwin->activepanel, event);
	} else if ((panel = brightonPanelLocator(bwin, event->x, event->y))
		> (brightonIResource *) NULL)
	{
		if (panel->configure)
			panel->configure(bwin, panel, event);
	}

	/*
	 * Finally, as this is a key press event I want to pass it through to the
	 * midi library. This is a rather ugly hack to have a more useful keyboard
	 * mapping. We should only really do this from the keyboard panel, but that
	 * if for later study.
	 */
	if ((event->flags & BRIGHTON_MOD_CONTROL)
		&& (event->flags & BRIGHTON_MOD_SHIFT))
		brightonControlShiftKeyInput(bwin, event->key, 1, event->flags);
	else if (event->flags & BRIGHTON_MOD_CONTROL)
		brightonControlKeyInput(bwin, event->key, 1);
	else if (event->flags & BRIGHTON_MOD_SHIFT)
		brightonShiftKeyInput(bwin, event->key, 1, event->flags);
	else
		brightonKeyInput(bwin, event->key, 1);

	return(0);
}

int
brightonKeyRelease(brightonWindow *bwin, brightonEvent *event)
{
	brightonIResource *panel;

	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonKeyRelease(%i)\n", event->key);

	if ((bwin->flags & BRIGHTON_DEV_ACTIVE) && (bwin->activepanel != 0))
		bwin->activepanel->configure(bwin, bwin->activepanel, event);
	else if ((panel = brightonPanelLocator(bwin, event->x, event->y))
		> (brightonIResource *) NULL)
	{
		if (panel->configure)
			panel->configure(bwin, panel, event);
	}

	if (~event->flags & BRIGHTON_MOD_CONTROL)
		brightonKeyInput(bwin, event->key, 0);

	return(0);
}

int
brightonButtonPress(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonButtonPress(%i)\n", event->key);

	/*
	 * TOOK THIS OUT BY POPULAR DEMAND
	if (event->key == BRIGHTON_BUTTON3) {
			brightonMenu(bwin, event->x, event->y, 100, 200);
		return(0);
	}
	 */

	/*
	 * We need to determine which device is under the selection, and force
	 * statefull delivery of events to that device for further motion.
	 */
	bwin->activepanel = 0;

	if ((bwin->activepanel = brightonPanelLocator(bwin, event->x, event->y))
		> (brightonIResource *) NULL)
	{
		bwin->flags |= BRIGHTON_DEV_ACTIVE;

		event->command = BRIGHTON_BUTTONPRESS;

		if (bwin->activepanel->configure)
			bwin->activepanel->configure(bwin, bwin->activepanel, event);
	} else
		bwin->flags &= ~BRIGHTON_DEV_ACTIVE;
	return(0);
}

int
brightonButtonRelease(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
	{
		if ((bwin) && (bwin->activepanel))
			printf("brightonButtonRelease(%p, %p, %p)\n",
				bwin,
				bwin->activepanel,
				bwin->activepanel->configure); 
	}

	event->command = BRIGHTON_BUTTONRELEASE;

	/*
	 * TOOK THIS OUT BY POPULAR DEMAND
	if (event->key == BRIGHTON_BUTTON3) {
			brightonMenu(bwin, event->x, event->y, 100, 200);
		return(0);
	}
	 */

	/*if (bwin->activepanel && bwin->activepanel->configure) */
	if ((bwin->flags & BRIGHTON_DEV_ACTIVE) && (bwin->activepanel != 0))
		bwin->activepanel->configure(bwin, bwin->activepanel, event);

	bwin->flags &= ~BRIGHTON_DEV_ACTIVE;
	bwin->activepanel = 0;
	return(0);
}

int
brightonMotionNotify(brightonWindow *bwin, brightonEvent *event)
{
	if ((bwin->flags & BRIGHTON_DEV_ACTIVE) && (bwin->activepanel != 0))
	{
		if (bwin->activepanel->configure)
			bwin->activepanel->configure(bwin, bwin->activepanel, event);
	}
	return(0);
}

static void
brightonFillRatios(brightonWindow *win)
{
	float wfact, hfact;

	wfact = ((float) win->display->width) * 0.95
		/ ((float) win->template->width);
	hfact = ((float) win->display->height) * 0.95
		/ ((float) win->template->height);

	if (hfact > wfact) {
		win->maxw = win->display->width * 0.95;
		win->minw = win->template->width;
		win->minh = win->template->height;
		win->maxh = win->template->height * wfact;
	} else {
		win->maxh = win->display->height * 0.95;
		win->minw = win->template->width;
		win->minh = win->template->height;
		win->maxw = win->template->width * hfact;
	}
}

int
brightonEnterNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonEnterNotify()\n");

#if (BRIGHTON_HAS_ZOOM == 1)
	if (bwin->flags & BRIGHTON_AUTOZOOM)
	{
		if (bwin->flags & BRIGHTON_DEBUG)
			printf("AutoZoom\n");

		// Make sure we are initted 
		if ((bwin->minh == 0) || (bwin->maxh == 0))
			brightonFillRatios(bwin);

		bwin->display->flags |= BRIGHTON_ANTIALIAS_5;

		brightonRequestResize(bwin,
			bwin->template->width,
			bwin->template->height);

		if (bwin->flags & BRIGHTON_SET_RAISE)
			BRaiseWindow(bwin->display, bwin);
	} else
#endif
	if (~bwin->flags & BRIGHTON_NO_ASPECT) {
		float aspect = ((float) bwin->width) / bwin->height;

	    if ((aspect / bwin->aspect < 0.98) || ((aspect / bwin->aspect) > 1.02))
		{
			/* Ratio has changed */
			if (bwin->flags & _BRIGHTON_SET_HEIGHT) {
				if ((bwin->height * bwin->aspect)
					< ((brightonDisplay *) bwin->display)->width)
					bwin->width = bwin->height * bwin->aspect;
				else {
					bwin->width =
						((brightonDisplay *) bwin->display)->width - 10;
					bwin->height = bwin->width / bwin->aspect;
				}
			} else {
				if ((bwin->width / bwin->aspect)
					< ((brightonDisplay *) bwin->display)->height)
					bwin->height = bwin->width / bwin->aspect;
				else {
					bwin->height =
						((brightonDisplay *) bwin->display)->height - 10;
					bwin->width = bwin->height * bwin->aspect;
				}
			}

			if (bwin->flags & BRIGHTON_DEBUG)
				printf("changed aspect ratio: %f %i %i\n",
					aspect, bwin->width, bwin->height);

			BResizeWindow(bwin->display, bwin, bwin->width, bwin->height);
			brightonWorldChanged(bwin, bwin->width, bwin->height);
		}
	}

	bwin->flags &= ~_BRIGHTON_SET_HEIGHT;

	if (bwin->flags & BRIGHTON_EXITING)
	{
		BAutoRepeat(bwin->display, 1);
		return(0);
	}

	BAutoRepeat(bwin->display, 0);

	return(0);
}

int
brightonLeaveNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonLeaveNotify()\n");

#if (BRIGHTON_HAS_ZOOM == 1)
	if (bwin->flags & BRIGHTON_AUTOZOOM)
	{
		if (bwin->flags & BRIGHTON_DEBUG)
			printf("AutoZoom\n");

		// Make sure we are initted 
		if ((bwin->minh == 0) || (bwin->maxh == 0))
			brightonFillRatios(bwin);

		// Flip the window size 
		brightonRequestResize(bwin, bwin->minw, bwin->minh);

		if (bwin->flags & BRIGHTON_SET_LOWER)
			BLowerWindow(bwin->display, bwin);
	} else
#endif
	if (~bwin->flags & BRIGHTON_NO_ASPECT) {
		float aspect = ((float) bwin->width) / bwin->height;

	    if ((aspect / bwin->aspect < 0.98) || ((aspect / bwin->aspect) > 1.02))
		{
			/* Ratio has changed */
			if (bwin->flags & _BRIGHTON_SET_HEIGHT) {
				if ((bwin->height * bwin->aspect)
					< ((brightonDisplay *) bwin->display)->width)
					bwin->width = bwin->height * bwin->aspect;
				else {
					bwin->width =
						((brightonDisplay *) bwin->display)->width - 10;
					bwin->height = bwin->width / bwin->aspect;
				}
			} else {
				if ((bwin->width / bwin->aspect)
					< ((brightonDisplay *) bwin->display)->height)
					bwin->height = bwin->width / bwin->aspect;
				else {
					bwin->height =
						((brightonDisplay *) bwin->display)->height - 10;
					bwin->width = bwin->height * bwin->aspect;
				}
			}

			if (bwin->flags & BRIGHTON_DEBUG)
				printf("changed aspect ratio: %f %i %i\n",
					aspect, bwin->width, bwin->height);

			BResizeWindow(bwin->display, bwin, bwin->width, bwin->height);
			brightonWorldChanged(bwin, bwin->width, bwin->height);
		}
	}

	bwin->flags &= ~_BRIGHTON_SET_HEIGHT;

	BAutoRepeat(bwin->display, 1);
	return(0);
}

int
brightonFocusIn(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonFocusIn()\n");

	brightonEnterNotify(bwin, event);

	return(0);
}

int
brightonFocusOut(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonFocusOut()\n");

	brightonLeaveNotify(bwin, event);

	return(0);
}

int
brightonKeymapNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonKeymapNotify()\n");
	return(0);
}

int
brightonExpose(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonExpose(%i %i %i %i)\n",
			event->x, event->y, event->w, event->h);

	BCopyArea(bwin->display, 
		event->x, event->y, event->w, event->h, event->x, event->y);
	return(0);
}

int
brightonGraphicsExpose(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonGraphicsExpose()\n");
	return(0);
}

int
brightonNoExpose(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonNoExpose()\n");
	return(0);
}

int
brightonVisibilityNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonVisibilityNotify()\n");
	return(0);
}

int
brightonCreateNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonCreateNotify()\n");
	return(0);
}

int
brightonDestroyNotify(brightonWindow *bwin, brightonEvent *event)
{
	/*
	 * Counter intuitive, we do nothing here. The process gets a SIGPIPE and
	 * the GUI intercepts this to request the engine to terminate the algo
	 * represented by this GUI. Engine will terminate when all algos have
	 * finished.
	 */
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonDestroyNotify()\n");
	return(0);
}

int
brightonUnmapNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonUnmapNotify()\n");
	return(0);
}

int
brightonMapNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonMapNotify()\n");
	return(0);
}

int
brightonMapRequest(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonMapRequest()\n");
	return(0);
}

int
brightonReparentNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonReparentNotify()\n");
	return(0);
}

int
brightonConfigureNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonConfigureNotify(%i, %i, %i, %i)\n",
			event->x, event->y, event->w, event->h);

	if (bwin->flags & BRIGHTON_SET_SIZE)
	{
		bwin->flags &= ~BRIGHTON_SET_SIZE;
		return(0);
	}

	if ((bwin->width != event->w) || (bwin->height != event->h))
		brightonWorldChanged(bwin, event->w, event->h);

	/* This is wrong. */
//	if ((bwin->width != event->w) || (bwin->height != event->h))
//		BResizeWindow(bwin->display, bwin, bwin->width, bwin->height);

	return(0);
}

int
brightonConfigureRequest(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonConfigureRequest()\n");
	return(0);
}

int
brightonGravityNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonGravityNotify()\n");
	return(0);
}

int
brightonRequestResize(brightonWindow *bwin, int w, int h)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonResizeRequest(%i, %i)\n", w, h);

	if ((bwin->width != w) || (bwin->height != h))
	{
		if ((bwin->width < w) && (bwin->flags & BRIGHTON_SET_RAISE))
			BRaiseWindow(bwin->display, bwin);

		/*
		 * Our size has changed. We need to re-render the background, and then 
		 * repaint it.
		 */
		BResizeWindow(bwin->display, bwin, w, h);
		brightonWorldChanged(bwin, w, h);
	}
	return(0);
}

int
brightonResizeRequest(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonResizeRequest(%i, %i)\n", event->w, event->h);

	if ((bwin->width != event->w) || (bwin->height != event->h))
	{
		/*
		 * Our size has changed. We need to re-render the background, and then 
		 * repaint it.
		 */
		brightonWorldChanged(bwin, event->w, event->h);

/*
		if ((bwin->width != event->w) || (bwin->height != event->h))
			BResizeWindow(bwin->display, bwin->win, bwin->width, bwin->height);
*/
	}
	return(0);
}

int
brightonCirculateNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonCirculateNotify()\n");
	return(0);
}

int
brightonCirculateRequest(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonCirculateRequest()\n");
	return(0);
}

int
brightonPropertyNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonPropertyNotify()\n");
	return(0);
}

int
brightonSelectionClear(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonSelectionClear()\n");
	return(0);
}

int
brightonSelectionRequest(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonSelectionRequest()\n");
	return(0);
}

int
brightonSelectionNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonSelectionNotify()\n");
	return(0);
}

int
brightonColormapNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonColormapNotify()\n");
	return(0);
}

int
brightonClientMessage(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonClientMessage()\n");
	return(0);
}

int
brightonMappingNotify(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonMappingNotify()\n");
	return(0);
}

int
brightonNullHandler(brightonWindow *bwin, brightonEvent *event)
{
	if (bwin->flags & BRIGHTON_DEBUG)
		printf("brightonNullHandler()\n");
	return(0);
}

typedef int (*eventRoutine)(brightonWindow *, brightonEvent *);

/*
 * Check out the bEvent from the library, it can mask some events that are
 * considered "not important", but actually may be.
 */
eventRoutine defaultHandlers[BLASTEvent] = {
	brightonNullHandler,
	brightonNullHandler,
	brightonKeyPress,
	brightonKeyRelease,
	brightonButtonPress,
	brightonButtonRelease,
	brightonMotionNotify,
	brightonEnterNotify,
	brightonLeaveNotify,
	brightonFocusIn,
	brightonFocusOut,
	brightonKeymapNotify,
	brightonExpose,
	brightonGraphicsExpose,
	brightonNoExpose,
	brightonVisibilityNotify,
	brightonCreateNotify,
	brightonDestroyNotify,
	brightonUnmapNotify,
	brightonMapNotify,
	brightonMapRequest,
	brightonReparentNotify,
	brightonConfigureNotify,
	brightonConfigureRequest,
	brightonGravityNotify,
	brightonResizeRequest,
	brightonCirculateNotify,
	brightonCirculateRequest,
	brightonPropertyNotify,
	brightonSelectionClear,
	brightonSelectionRequest,
	brightonSelectionNotify,
	brightonColormapNotify,
	brightonClientMessage,
	brightonMappingNotify
};

void brightonInitDefHandlers(brightonWindow *bwin)
{
	int i;

	for (i = 0; i < BLASTEvent; i++)
		bwin->callbacks[i] = defaultHandlers[i];
}

void
brightonOldEventLoop(brightonDisplay **dlist)
{
	brightonDisplay *display;
	brightonEvent event;
	brightonWindow *bwin = (*dlist)->bwin;

	while (1) {
		/*
		 * BNextEvent will block due to it using XNextEvent. This makes the
		 * use of other functions rather difficult. Will rework this to use
		 * XCheckMaskEvent which will not block.
		 *
		 *	while (1) {
		 *		foreach (win) {
		 * 			BCheckMaskEvent(bwin, &event);
		 *		}
		 *	}
		 *
		 * We need this since the MIDI routines also need to be checked, so we
		 * will select on the ALSA SEQ port looking for controller changes so
		 * that they can modify the GUI display.
		 */
		BNextEvent(bwin->display, &event);

		if (event.command == BRIGHTON_NONE)
			continue;

		/*
		 * This may need to be changed to a semaphore, but I am hoping all GUI
		 * activity can occupy a single thread only.
		 */
		bwin->flags |= BRIGHTON_BUSY;

		/*
		 * Look for the right window.
		 */
		display = *dlist;

		while (display != 0)
		{
			if (event.wid == ((brightonWindow *) display->bwin)->win)
				break;

			if ((event.type == BRIGHTON_DESTROY)
				&& (((brightonWindow *) display->bwin)->parentwin == event.wid))
				break;

			display = display->next;
		}

		if ((display == 0 || (event.type < 0) || (event.type >= BLASTEvent)))
			continue;

		((brightonWindow *) display->bwin)->callbacks[event.type]
			(display->bwin, &event);

		bwin->flags &= ~BRIGHTON_BUSY;
	}
}

int
brightonEventLoop(brightonDisplay **dlist)
{
	brightonDisplay *display;
	brightonEvent event;
	brightonWindow *bwin = (*dlist)->bwin;

/*	while (1) { */
		/*
		 * BNextEvent will block due to it using XNextEvent. This makes the
		 * use of other functions rather difficult. Will rework this to use
		 * XCheckMaskEvent which will not block.
		 *
		 *	while (1) {
		 *		foreach (win) {
		 * 			BCheckMaskEvent(bwin, &event);
		 *		}
		 *	}
		 *
		 * We need this since the MIDI routines also need to be checked, so we
		 * will select on the ALSA SEQ port looking for controller changes so
		 * that they can modify the GUI display.
		 */
		while (BNextEvent(bwin->display, &event) > 0)
		{
			if (event.command == BRIGHTON_NONE)
				continue;

			/*
			 * This may need to be changed to a semaphore, but I am hoping all
			 * GUI activity can occupy a single thread only.
			 */
			bwin->flags |= BRIGHTON_BUSY;

			/*
			 * Look for the right window.
			 */
			display = *dlist;

			while (display != 0)
			{
				if (event.wid == ((brightonWindow *) display->bwin)->win)
					break;

				if ((event.type == BRIGHTON_DESTROY)
					&& (((brightonWindow *) display->bwin)->parentwin
						== event.wid))
					break;

				display = display->next;
			}

			if ((display == 0 || (event.type < 0)
				|| (event.type >= BLASTEvent)))
				continue;

			((brightonWindow *) display->bwin)->callbacks[event.type]
				(display->bwin, &event);

			bwin->flags &= ~BRIGHTON_BUSY;

			/*
			 * Configure events are typically big resizes which are long events.
			 * If we handle all of them (they come in buckets) at once it is
			 * possible the engine will detect GUI failure from active sense
			 */
			if (event.command == BRIGHTON_CONFIGURE)
				return(1);
		}

		/*
		 * This will now become a select on the ALSA SEQ socket looking for 
		 * MIDI events. Not certain how they will be linked into the GUI at
		 * the moment. For now this is just a sleep until the ALSA SEQ interface
		 * registration code has been finalised.
		 *
		 * What we will be looking for are events on a MIDI channel, we will
		 * look for that MIDI channel in our window lists. We want to respond
		 * to key events and reflect that in the GUI optionally, but not send
		 * the key events since the engine will handle all those. We also want 
		 * to look for controller values and have some configurable method to
		 * link those to our controls. Now this linkage will be done via the
		 * GUI, preferably, with the <Control><Middle Mouse><MIDI CC # motion>.
		 * Since the GUI handles this then we can dispatch events to another
		 * module that does the linkage. Need to be able to save and retrieve
		 * configurations - <Control><Middle> will go to this module, and all
		 * MIDI controller events as well, and it will make the linkage and
		 * dispatch the events.
		 * We should try and have a vkeydb type X event keymap.
		 */
/*		usleep(10000); */
/*	} */
	return(0);
}

