
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

extern void *brightonX11malloc(size_t);
extern int brightonX11free(void *);

extern bdisplay *BOpenDisplay(brightonDisplay *, char *);
extern int BCloseDisplay(bdisplay *);

extern int BOpenWindow(brightonDisplay *, brightonWindow *, char *);
extern int BCloseWindow(brightonDisplay *, brightonWindow *);
extern int BCloseWindow(brightonDisplay *, brightonWindow *);
extern void BRaiseWindow(brightonDisplay *, brightonWindow *);
extern void BLowerWindow(brightonDisplay *, brightonWindow *);

extern int BDrawArea(brightonDisplay *, brightonBitmap *, int, int, int, int, int, int);
extern int BCopyArea(brightonDisplay *, int, int, int, int, int, int);

extern int BFlush(brightonDisplay *);

extern brightonPalette *BInitColorMap(brightonDisplay *);
extern int BFreeColor(brightonDisplay *, brightonPalette *);
extern int BAllocGC(brightonDisplay *, brightonPalette *, unsigned short, unsigned short, unsigned short);
extern int BAllocColor(brightonDisplay *, brightonPalette *, unsigned short, unsigned short, unsigned short);
extern int BAllocColorByName(brightonDisplay *, brightonPalette *, char *);

extern int BAutoRepeat(brightonDisplay *, int);
extern int BResizeWindow(brightonDisplay *, brightonWindow *, int, int);

extern int BNextEvent(brightonDisplay *, brightonEvent *);


