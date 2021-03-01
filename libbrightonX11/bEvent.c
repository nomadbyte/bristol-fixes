
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
#include <signal.h>

int command[BLASTEvent] = {
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_KEYPRESS,
    BRIGHTON_KEYRELEASE,
    BRIGHTON_BUTTONPRESS,
    BRIGHTON_BUTTONRELEASE,
    BRIGHTON_MOTION,
    BRIGHTON_ENTER,
    BRIGHTON_LEAVE,
    BRIGHTON_FOCUS_IN, /* FocusIn */
    BRIGHTON_FOCUS_OUT, /* FocusOut */
    BRIGHTON_NONE,
    BRIGHTON_EXPOSE,
    BRIGHTON_GEXPOSE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_DESTROY,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_CONFIGURE,
    BRIGHTON_CONFIGURE_REQ,
    BRIGHTON_NONE,
    BRIGHTON_RESIZE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_NONE,
    BRIGHTON_CLIENT,
    BRIGHTON_NONE
};

int
BSendEvent(brightonDisplay *display, brightonWindow *bwin, brightonEvent *event)
{
	bdisplay *bd = (bdisplay *) display->display;
	XEvent xevent;

	if ((bwin->flags & BRIGHTON_BUSY) || (display->flags & _BRIGHTON_WINDOW))
		return(0);

	xevent.xany.type = KeyPress;
	xevent.xany.window = (Window) bwin->win;
	xevent.xkey.x = event->x;
	xevent.xkey.y = event->y;
	xevent.xkey.keycode = 'u';

	if (XSendEvent(bd->display,
		(Window) bwin->win, False, KeyPressMask, &xevent) == 0)
		printf("send failed\n");

	XFlush(bd->display);

	return(0);
}

int
OldBNextEvent(brightonDisplay *display, brightonEvent *event)
{
	bdisplay *bd = (bdisplay *) display->display; 
	XEvent xevent;

	XNextEvent(bd->display, &xevent);

	event->type = xevent.xany.type;
	event->wid = xevent.xany.window;
	event->command = command[xevent.xany.type];

printf("event %i: %x\n", xevent.xany.type, event->wid);

	switch (xevent.xany.type) {
		case KeyPress:
		case KeyRelease:
			event->x = xevent.xkey.x;
			event->y = xevent.xkey.y;
			event->key = XLookupKeysym(&xevent.xkey, 0);
			/*
			 * Left shift for right shift
			 */
			if (event->key == 65506)
				event->key = 65505;
			break;
		case ButtonPress:
		case ButtonRelease:
			event->x = xevent.xbutton.x;
			event->y = xevent.xbutton.y;
			event->key = xevent.xbutton.button;
			break;
		case MotionNotify:
			event->x = xevent.xmotion.x;
			event->y = xevent.xmotion.y;
			break;
		case ConfigureNotify:
			event->x = xevent.xconfigure.x;
			event->y = xevent.xconfigure.y;
			event->w = xevent.xconfigure.width;
			event->h = xevent.xconfigure.height;
			break;
		case ResizeRequest:
			event->w = xevent.xresizerequest.width;
			event->h = xevent.xresizerequest.height;
			break;
		case Expose:
			event->x = xevent.xexpose.x;
			event->y = xevent.xexpose.y;
			event->w = xevent.xexpose.width;
			event->h = xevent.xexpose.height;
			break;
		default:
			break;
	}

	return(0);
}

extern Atom wmDeleteMessage;

int
BNextEvent(brightonDisplay *display, brightonEvent *event)
{
	bdisplay *bd = (bdisplay *) display->display; 
	XEvent xevent;
	long int l, n;

	if (display->flags & _BRIGHTON_WINDOW)
		return(0);

	l = LastKnownRequestProcessed(bd->display);
	n = XNextRequest(bd->display);

	if ((l - n) >= 0)
	{
		printf("request window out of sync %i - %i = %i\n",
			(int) l, (int) n, (int) (l - n));
		usleep(100000);
	}

	/*
	 * This is a bit of overkill, I want to just use NextEvent however I need
	 * to also get some notifications from the midi control channel into this
	 * thread since they may update the screen. Only have a callback active 
	 * means this is not easy. It would probably have been better to reconsider
	 * the thread separation but anyway, we check for masked and typed events,
	 * the masked are generic mouse motion, keyboard, etc, the typed events
	 * are specifically for WM notifications.
	 */
	if ((XCheckMaskEvent(bd->display, 0xffffffff, &xevent) == True) ||
		(XCheckTypedEvent(bd->display, ClientMessage, &xevent) == True))
	{
		event->type = xevent.xany.type;
		event->wid = xevent.xany.window;
		event->command = command[xevent.xany.type];

		switch (xevent.xany.type) {
			case KeyPress:
			case KeyRelease:
				event->x = xevent.xkey.x;
				event->y = xevent.xkey.y;
				event->key = XLookupKeysym(&xevent.xkey, 0);
				event->flags = xevent.xkey.state;
				/*
				 * Left shift for right shift
				 */
				if (event->key == 65506)
					event->key = 65505;
				break;
			case ButtonPress:
			case ButtonRelease:
				/*
				 * We need to do some event translation for button4 and button5
				 * as these are used for a mousewheel up/down. We need to make
				 * a translation into J/K for example.
				 */
				event->x = xevent.xbutton.x;
				event->y = xevent.xbutton.y;
				if (xevent.xbutton.button == 4)
				{
					event->type = KeyPress;
					event->command = command[2];
					event->key = 0x6b;
				} else if (xevent.xbutton.button == 5) {
					event->type = KeyPress;
					event->command = command[2];
					event->key = 0x6a;
				} else
					event->key = xevent.xbutton.button;
				break;
			case MotionNotify:
				event->x = xevent.xmotion.x;
				event->y = xevent.xmotion.y;
				break;
			case FocusIn:
			case FocusOut:
				break;
			case ConfigureNotify:
				event->x = xevent.xconfigure.x;
				event->y = xevent.xconfigure.y;
				event->w = xevent.xconfigure.width;
				event->h = xevent.xconfigure.height;
				break;
			case ResizeRequest:
				event->w = xevent.xresizerequest.width;
				event->h = xevent.xresizerequest.height;
				break;
			case Expose:
				event->x = xevent.xexpose.x;
				event->y = xevent.xexpose.y;
				event->w = xevent.xexpose.width;
				event->h = xevent.xexpose.height;
				break;
			case ClientMessage:
				/*
				 * This is a bit rough at the moment.
				 */
				if (xevent.xclient.data.l[0] == wmDeleteMessage)
				{
					BAutoRepeat(display, 1);
					/* This was originally PIPE */
					kill(getpid(), SIGHUP);
				}
				break;
			default:
				break;
		}

		return(1);
	}

	return(0);
}

