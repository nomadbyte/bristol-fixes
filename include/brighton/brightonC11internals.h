
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

#ifndef B_C11_INTERNALS_H
#define B_C11_INTERNALS_H

#include "brightoninternals.h"

#define B_NAMESIZE 64

#define B_ALLOCATED 0x010000

extern int brighton_cli_dummy;

#define BRIGHTON_X 0
#define BRIGHTON_Y 1
typedef struct BrightonQRender {
	int count;
	int index;
	void *gc;
	void *queue[BRIGHTON_QR_QSIZE];
} brightonQRender;

typedef struct Bdisplay {
	unsigned int flags;
	struct Bdisplay *next, *last;
	char name[B_NAMESIZE];
	void *display;
	int uses;
	unsigned int x, y, width, height, depth, border;
	int screen_num;
	void *screen_ptr;
	void *icon_pixmap;
	void *root, *win;
	void *size_hints;
	void *wm_hints;
	void *class_hints;
	void *event;
	void *windowName, *iconName;
	char *icon_name;
	void *cm;
	void *dvi;
/*	Pixmap image; */
	int iw, ih;
	void *gc;
	brightonQRender *qrender;
	int ccount, ocount;
} bdisplay;


/* Depends on struct definitions above */
#include "brightonC11.h"


#endif /* B_X11_INTERNALS_H */

