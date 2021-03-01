
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

#ifndef B_X11_INTERNALS_H
#define B_X11_INTERNALS_H

#define B_NAMESIZE 64
#define B_ALLOCATED 0x010000

#define BRIGHTON_X 0
#define BRIGHTON_Y 1

#include "brightoninternals.h"

#ifdef BRIGHTON_HAS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#ifdef BRIGHTON_SHMIMAGE
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

typedef struct BrightonQRender {
	int count;
	int index;
	GC gc;
	XPoint queue[BRIGHTON_QR_QSIZE];
} brightonQRender;

typedef struct Bdisplay {
	unsigned int flags;
	struct Bdisplay *next, *last;
	char name[B_NAMESIZE];
	Display *display;
	int uses;
	unsigned int x, y, width, height, depth, border;
	int screen_num;
	Screen *screen_ptr;
	Pixmap icon_pixmap;
	Window root, win;
	XSizeHints size_hints;
	XWMHints wm_hints;
	XClassHint class_hints;
	XEvent event;
	XTextProperty windowName, iconName;
	char *icon_name;
	Colormap cm;
	XVisualInfo dvi;
/*	Pixmap image; */
	int iw, ih;
	GC gc;
	brightonQRender *qrender;
	int ccount, ocount;
#ifdef BRIGHTON_SHMIMAGE
	XShmSegmentInfo shminfo;
#endif
} bdisplay;
#else
typedef struct BrightonQRender {
	int count;
	int index;
//	GC gc;
//	XPoint queue[BRIGHTON_QR_QSIZE];
} brightonQRender;

typedef struct Bdisplay {
	unsigned int flags;
	struct Bdisplay *next, *last;
	char name[B_NAMESIZE];
	int uses;
	unsigned int x, y, width, height, depth, border;
	int screen_num;
	char *icon_name;
/*	Pixmap image; */
	int iw, ih;
	brightonQRender *qrender;
	int ccount, ocount;
} bdisplay;
#endif


/* Depends on struct definitions above */
#include "brightonX11.h"


#endif /* B_X11_INTERNALS_H */

