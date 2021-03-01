
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

#include "brightonC11internals.h"
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
	return(0);
}

int
BNextEvent(brightonDisplay *display, brightonEvent *event)
{
	return(0);
}

