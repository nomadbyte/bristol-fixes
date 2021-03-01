
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
#include "icon_bitmap.xbm"

/* We should bury this somewhere in the window */
Atom wmDeleteMessage;

char *args[2] = {"bristol gui", (char *) 0};

int
BOpenWindow(brightonDisplay *display, brightonWindow *bwin, char *programme)
{
	bdisplay *bd = (bdisplay *) display->display;

	if (display->flags & _BRIGHTON_WINDOW)
	{
		bd->flags |= _BRIGHTON_WINDOW;
		return(bwin->win = (Window) 0xdeadbeef);
	}

	bd->x = bwin->x; bd->y = bwin->y;

	/*
	printf("libbrightonB11: %p %i, %i, %i, %i\n",
		bd->display, bwin->x, bwin->y, bwin->width, bwin->height);
	 */

	bwin->win = XCreateSimpleWindow(bd->display,
		RootWindow(bd->display, bd->screen_num),
		bwin->x, bwin->y,
		bwin->width, bwin->height, bd->border,
		WhitePixel(bd->display, bd->screen_num),
		BlackPixel(bd->display, bd->screen_num));

	/*
	 * Notify that we are interested in window manager delete events, and see
	 * bEvent.c regarding how we find the events in Brighton.
	 * */
	wmDeleteMessage = XInternAtom(bd->display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(bd->display, bwin->win, &wmDeleteMessage, True);

	bd->icon_pixmap = XCreateBitmapFromData(bd->display, bwin->win,
		(char *) icon_bitmap_bits,
		(unsigned int) icon_bitmap_width,
		(unsigned int) icon_bitmap_height);

	bd->size_hints.flags = PPosition|PSize|PMinSize;
	bd->size_hints.min_width = 100;
	bd->size_hints.min_height = 10;

	bd->icon_name = programme;
	bwin->window_name = programme;

	if (XStringListToTextProperty(&bd->icon_name, 1, &bd->iconName) == 0)
	{
		printf("%s: allocation error for icon failed\n", programme);
		display->bwin = 0;
		return(0);
	}
	if (XStringListToTextProperty(&bwin->window_name, 1, &bd->windowName)
		== 0)
	{
		printf("%s: allocation error for window failed\n", programme);
		display->bwin = 0;
		return(0);
	}

	bd->wm_hints.initial_state = NormalState;
	bd->wm_hints.input = True;
	bd->wm_hints.icon_pixmap = bd->icon_pixmap;
	bd->wm_hints.flags = StateHint|IconPixmapHint|InputHint;

	bd->class_hints.res_name = programme;
	bd->class_hints.res_class = "BasicWin";

	XSetWMProperties(bd->display, bwin->win,
		&bd->windowName, &bd->iconName, args, 1,
		&bd->size_hints, &bd->wm_hints, &bd->class_hints);

	XSelectInput(bd->display, bwin->win,
		KeyPressMask|KeyReleaseMask|
		ButtonPressMask|ButtonReleaseMask|Button1MotionMask|
		Button2MotionMask|Button3MotionMask|Button4MotionMask|
		Button5MotionMask|ButtonMotionMask|
		EnterWindowMask|LeaveWindowMask|
		KeymapStateMask|ExposureMask|
		VisibilityChangeMask|StructureNotifyMask|
		SubstructureNotifyMask|SubstructureRedirectMask|
		FocusChangeMask|FocusChangeMask|
		PropertyChangeMask|ColormapChangeMask|OwnerGrabButtonMask);

	/*
	 * keep a reference to this parent for later use.
	 */
	bwin->parentwin = RootWindow(bd->display, bd->screen_num);
	/*
	 * We have to register for the destroy event with our parent.
	 * (Panned).
	XSelectInput(bd->display, bwin->parentwin, SubstructureNotifyMask);
	 */

	XMapWindow(bd->display, bwin->win);

	if (~bwin->flags & _BRIGHTON_POST)
		XIconifyWindow(bd->display, bwin->win, bd->screen_num);

	/* Incorrect location? Should maybe be in win structure? */
	bwin->gc = DefaultGC(bd->display, bd->screen_num);

	return(bwin->win);
}

int
BCloseWindow(brightonDisplay *display, brightonWindow *bwin)
{
	bdisplay *bd = display->display;

	if (~display->flags & _BRIGHTON_WINDOW)
		XDestroyWindow(bd->display, bwin->win);

	return(0);
}

void
BRaiseWindow(brightonDisplay *display, brightonWindow *bwin)
{
	bdisplay *bd = display->display;

	if (~display->flags & _BRIGHTON_WINDOW)
		XRaiseWindow(bd->display, bwin->win);
}

void
BLowerWindow(brightonDisplay *display, brightonWindow *bwin)
{
	bdisplay *bd = display->display;

	if (~display->flags & _BRIGHTON_WINDOW)
		XLowerWindow(bd->display, bwin->win);
}

int
BGetGeometry(brightonDisplay *display, brightonWindow *bwin)
{
	bdisplay *bd = display->display;

	if (~bd->flags & _BRIGHTON_WINDOW)
	{
		if (XGetGeometry(bd->display, RootWindow(bd->display, bd->screen_num),
			&bd->root, (int *) (&bd->x), (int *) (&bd->y), &bd->width,
			&bd->height, &bd->border, &bd->depth) < 0)
			printf("cannot get root window geometry\n");

		/*
		 * I would prefer for these to come from the bitmap itself.
		 */
		bwin->width = bd->width;
		bwin->height = bd->height;
		bwin->depth = bd->depth;
	} else {
		bwin->width = 1024;
		bwin->height = 768;
		bwin->depth = 24;
	}

	return(0);
}

int
BFlush(brightonDisplay *display)
{
	bdisplay *bd = (bdisplay *) display->display;

	if (~display->flags & _BRIGHTON_WINDOW)
		XFlush(bd->display);

	return(0);
}

int
BAutoRepeat(brightonDisplay *display, int onoff)
{
	bdisplay *bd = display->display;

	if (display->flags & _BRIGHTON_WINDOW)
		return(0);

	if (onoff) {
		XAutoRepeatOn(bd->display);
	} else {
		XAutoRepeatOff(bd->display);
	}

	return(0);
}

